#include <stdint.h>

#include "vector_outline.h"

#define VO_MAX_EDGES VO_MAX_PTS

uint8_t vo_cell_filled(const uint8_t *cells, uint8_t width, uint8_t x, uint8_t y)
{
    if (x >= width || y >= width) {
        return 0;
    }
    return cells[(uint16_t)y * width + x] != (uint8_t)' ';
}

/* Directed boundary edges: interior consistently on the right-hand side
 * (screen coords, y down). A single cell traces (0,0)->(1,0)->(1,1)->
 * (0,1), so outer contours accumulate positive shoelace area and holes
 * negative - that sign is the even-odd outer/hole classification. */
static void push_edge(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t *ex0, int16_t *ey0, int16_t *ex1, int16_t *ey1,
                      uint8_t *count, uint8_t *truncated)
{
    uint8_t n = *count;

    if (n >= VO_MAX_EDGES) {
        *truncated = 1;
        return;
    }
    ex0[n] = x0; ey0[n] = y0;
    ex1[n] = x1; ey1[n] = y1;
    *count = (uint8_t)(n + 1);
}

/* Deterministic saddle choice: when two outgoing edges meet at a corner
 * (diagonally touching cells), prefer the right turn relative to the
 * incoming direction, then straight, then left. Any consistent choice
 * preserves even-odd parity; this one just keeps loops tight. */
static uint8_t next_edge(uint8_t cur, int16_t cx, int16_t cy,
                         const int16_t *ex0, const int16_t *ey0,
                         const int16_t *ex1, const int16_t *ey1,
                         const uint8_t *used, uint8_t edge_count)
{
    int16_t dx_in  = (int16_t)(ex1[cur] - ex0[cur]);
    int16_t dy_in  = (int16_t)(ey1[cur] - ey0[cur]);
    /* right-turn vector of the incoming direction (y-down screen space) */
    int16_t rx = (int16_t)-dy_in;
    int16_t ry = (int16_t)dx_in;
    uint8_t j, best = VO_MAX_EDGES;
    int16_t best_rank = 3;

    for (j = 0; j < edge_count; j++) {
        int16_t dx_out, dy_out, rank;

        if (used[j] || ex0[j] != cx || ey0[j] != cy) {
            continue;
        }
        dx_out = (int16_t)(ex1[j] - ex0[j]);
        dy_out = (int16_t)(ey1[j] - ey0[j]);
        if (dx_out == rx && dy_out == ry) {
            rank = 0;
        } else if (dx_out == dx_in && dy_out == dy_in) {
            rank = 1;
        } else {
            rank = 2;
        }
        if (rank < best_rank) {
            best_rank = rank;
            best = j;
        }
    }
    return best;
}

int vo_trace(const uint8_t *cells, uint8_t width, vo_outline *out)
{
    int16_t ex0[VO_MAX_EDGES], ey0[VO_MAX_EDGES];
    int16_t ex1[VO_MAX_EDGES], ey1[VO_MAX_EDGES];
    uint8_t used[VO_MAX_EDGES];
    uint8_t edge_count = 0;
    uint16_t pt_count = 0;
    uint8_t x, y, i;

    out->contour_count = 0;
    out->truncated = 0;

    for (y = 0; y < width; y++) {
        for (x = 0; x < width; x++) {
            if (!vo_cell_filled(cells, width, x, y)) {
                continue;
            }
            if (!vo_cell_filled(cells, width, x, (uint8_t)(y - 1))) {
                push_edge(x, y, (int16_t)(x + 1), y, ex0, ey0, ex1, ey1, &edge_count,
                          &out->truncated);
            }
            if (!vo_cell_filled(cells, width, (uint8_t)(x + 1), y)) {
                push_edge((int16_t)(x + 1), y, (int16_t)(x + 1), (int16_t)(y + 1),
                          ex0, ey0, ex1, ey1, &edge_count,
                          &out->truncated);
            }
            if (!vo_cell_filled(cells, width, x, (uint8_t)(y + 1))) {
                push_edge((int16_t)(x + 1), (int16_t)(y + 1), x, (int16_t)(y + 1),
                          ex0, ey0, ex1, ey1, &edge_count,
                          &out->truncated);
            }
            if (!vo_cell_filled(cells, width, (uint8_t)(x - 1), y)) {
                push_edge(x, (int16_t)(y + 1), x, y, ex0, ey0, ex1, ey1, &edge_count,
                          &out->truncated);
            }
        }
    }

    for (i = 0; i < edge_count; i++) {
        used[i] = 0;
    }

    for (i = 0; i < edge_count; i++) {
        uint8_t start = i;
        uint8_t cur;
        uint8_t len = 0;
        int32_t area2 = 0;
        int16_t px, py;

        if (used[i]) {
            continue;
        }

        used[start] = 1;
        out->contour_start[out->contour_count] = (uint8_t)pt_count;
        px = ex0[start];
        py = ey0[start];
        cur = start;

        for (;;) {
            int16_t cx, cy;
            uint8_t nxt;

            if (pt_count >= VO_MAX_PTS || len >= 255U ||
                out->contour_count >= VO_MAX_CONTOURS) {
                out->truncated = 1;
                break;
            }
            out->pts[pt_count].x = px;
            out->pts[pt_count].y = py;
            pt_count++;
            len++;
            cx = ex1[cur];
            cy = ey1[cur];
            area2 += (int32_t)px * cy - (int32_t)cx * py;

            if (cx == ex0[start] && cy == ey0[start]) {
                break; /* closed */
            }

            nxt = next_edge(cur, cx, cy, ex0, ey0, ex1, ey1, used, edge_count);
            if (nxt == VO_MAX_EDGES) {
                out->truncated = 1;
                break; /* open chain: keep what was traced */
            }
            used[nxt] = 1;
            cur = nxt;
            px = ex0[cur];
            py = ey0[cur];
        }

        if (len > 0 && out->contour_count < VO_MAX_CONTOURS) {
            out->is_hole[out->contour_count] = (area2 < 0) ? 1U : 0U;
            out->contour_len[out->contour_count] = len;
            out->contour_count++;
        }
    }

    return out->truncated ? 1 : 0;
}
