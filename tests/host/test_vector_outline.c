/* Host tests for the rectilinear silhouette tracer (src/amiga/vector_outline).
 * The load-bearing assertion everywhere: rasterizing the traced contours
 * with even-odd parity reproduces the source cell bitmap exactly - not
 * merely that vertex lists were produced. */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "vector_outline.h"
#include "embedded_shapes.h"

static int failures = 0;

#define CHECK_EQ_INT(a, b, msg)                                            \
    do {                                                                   \
        int _a = (a), _b = (b);                                            \
        if (_a != _b) {                                                    \
            printf("FAIL %s:%d %s: %d != %d\n", __FILE__, __LINE__, msg,   \
                   _a, _b);                                                \
            failures++;                                                    \
        }                                                                  \
    } while (0)

/* Even-odd rasterization: cell centre inside an odd number of contour
 * crossings => filled. Only vertical edges can cross a horizontal ray. */
static int raster_cell_filled(const vo_outline *ol, int cx, int cy)
{
    int crossings = 0;
    uint8_t c, i;
    float sx = (float)cx + 0.5f;
    float sy = (float)cy + 0.5f;

    for (c = 0; c < ol->contour_count; c++) {
        uint8_t start = ol->contour_start[c];
        uint8_t len = ol->contour_len[c];

        for (i = 0; i < len; i++) {
            const vo_point *p0 = &ol->pts[start + i];
            const vo_point *p1 = &ol->pts[start + ((i + 1 == len) ? 0 : i + 1)];
            int16_t y_lo = p0->y < p1->y ? p0->y : p1->y;
            int16_t y_hi = p0->y < p1->y ? p1->y : p0->y;

            if (p0->x != p1->x) {
                continue; /* horizontal edge never crosses the ray */
            }
            /* half-open [y_lo, y_hi); half-integer sample never hits a vertex */
            if (sy <= (float)y_lo || sy >= (float)y_hi) {
                continue;
            }
            if (sx < (float)p0->x) {
                crossings++;
            }
        }
    }
    return crossings & 1;
}

static void check_silhouette(const char *name, const uint8_t *cells,
                             uint8_t width, const vo_outline *ol)
{
    uint8_t x, y;
    int mismatches = 0;

    for (y = 0; y < width; y++) {
        for (x = 0; x < width; x++) {
            int src = vo_cell_filled(cells, width, x, y);
            int dst = raster_cell_filled(ol, x, y);

            if (src != dst) {
                printf("FAIL %s: cell (%u,%u) src=%d raster=%d\n",
                       name, x, y, src, dst);
                mismatches++;
            }
        }
    }
    CHECK_EQ_INT(mismatches, 0, name);
}

static void trace_ok(const char *name, const uint8_t *cells, uint8_t width,
                     vo_outline *ol)
{
    memset(ol, 0xAA, sizeof(*ol));
    CHECK_EQ_INT(vo_trace(cells, width, ol), 0, name);
    CHECK_EQ_INT(ol->truncated, 0, name);
    check_silhouette(name, cells, width, ol);
}

/* Holes are pre-classified by the tracer (shoelace sign); just sum them. */
static uint8_t count_holes(const vo_outline *ol)
{
    uint8_t c, holes = 0;

    for (c = 0; c < ol->contour_count; c++) {
        holes = (uint8_t)(holes + ol->is_hole[c]);
    }
    return holes;
}

static void test_empty_bitmap(void)
{
    static const uint8_t cells[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    vo_outline ol;

    trace_ok("empty bitmap", cells, 3, &ol);
    CHECK_EQ_INT(ol.contour_count, 0, "empty bitmap contour count");
}

static void test_single_cell(void)
{
    static const uint8_t cells[1] = {'A'};
    vo_outline ol;

    trace_ok("single cell", cells, 1, &ol);
    CHECK_EQ_INT(ol.contour_count, 1, "single cell contours");
    CHECK_EQ_INT(ol.is_hole[0], 0, "single cell not hole");
    CHECK_EQ_INT(ol.contour_len[0], 4, "single cell square vertices");
}

static void test_plus_shape(void)
{
    /* plus: connected region, single outer contour, no hole */
    static const uint8_t cells[9] = {
        ' ', '*', ' ',
        '*', '*', '*',
        ' ', '*', ' '
    };
    vo_outline ol;

    trace_ok("plus shape", cells, 3, &ol);
    CHECK_EQ_INT(ol.contour_count, 1, "plus shape contours");
    CHECK_EQ_INT(count_holes(&ol), 0, "plus shape holes");
}

static void test_hollow_ring(void)
{
    /* ring: border filled, 2x2 enclosed hole */
    static const uint8_t cells[16] = {
        '#', '#', '#', '#',
        '#', ' ', ' ', '#',
        '#', ' ', ' ', '#',
        '#', '#', '#', '#'
    };
    vo_outline ol;

    trace_ok("hollow ring", cells, 4, &ol);
    CHECK_EQ_INT(ol.contour_count, 2, "ring has outer + hole contours");
    CHECK_EQ_INT(count_holes(&ol), 1, "ring has exactly one hole");
}

static void test_disconnected_components(void)
{
    /* two opposite corners: two separate components, no holes */
    static const uint8_t cells[9] = {
        'X', ' ', ' ',
        ' ', ' ', ' ',
        ' ', ' ', 'X'
    };
    vo_outline ol;

    trace_ok("disconnected", cells, 3, &ol);
    CHECK_EQ_INT(ol.contour_count, 2, "disconnected components");
    CHECK_EQ_INT(count_holes(&ol), 0, "disconnected has no holes");
}

static void test_diagonal_touch(void)
{
    /* saddle case: two cells touching only diagonally */
    static const uint8_t cells[4] = {
        'O', ' ',
        ' ', 'O'
    };
    vo_outline ol;

    trace_ok("diagonal touch", cells, 2, &ol);
    CHECK_EQ_INT(ol.contour_count, 2, "diagonal touch components");
    check_silhouette("diagonal touch recheck", cells, 2, &ol);
}

static void test_truncation_fallback(void)
{
    /* 8x8 checkerboard: 32 filled cells x 4 boundary edges = 128 edges,
     * exceeding VO_MAX_PTS (104): the tracer must report truncation so the
     * renderer can fall back to the block rectangle. */
    static uint8_t cells[64];
    vo_outline ol;
    uint8_t x, y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            cells[y * 8 + x] = ((x + y) % 2 == 0) ? '#' : ' ';
        }
    }
    CHECK_EQ_INT(vo_trace(cells, 8, &ol), 1, "checkerboard reports truncated");
    CHECK_EQ_INT(ol.truncated, 1, "truncated flag set");
}

static void test_embedded_shapes(void)
{
    uint16_t off = 0;
    uint8_t i;
    vo_outline ol;

    for (i = 0; i < embedded_shape_count; i++) {
        uint8_t id = embedded_shape_data[off];
        uint8_t w = embedded_shape_data[off + 1];
        const uint8_t *cells = &embedded_shape_data[off + 2];
        char label[64];

        snprintf(label, sizeof(label), "embedded shape %u", id);
        CHECK_EQ_INT((int)w * w <= 64, 1, label);
        if ((uint16_t)(off + 2U + (uint16_t)w * w) > embedded_shape_data_len) {
            printf("FAIL %s: record extends past blob end\n", label);
            failures++;
            break;
        }
        trace_ok(label, cells, w, &ol);
        off = (uint16_t)(off + 2U + (uint16_t)w * w);
    }
    CHECK_EQ_INT(off, embedded_shape_data_len, "embedded blob fully walked");
}

int main(void)
{
    test_empty_bitmap();
    test_single_cell();
    test_plus_shape();
    test_hollow_ring();
    test_disconnected_components();
    test_diagonal_touch();
    test_truncation_fallback();
    test_embedded_shapes();

    if (failures == 0) {
        printf("all vector outline tests passed\n");
        return 0;
    }
    printf("%d vector outline test(s) failed\n", failures);
    return 1;
}
