#ifndef BWC_SCREEN_H
#define BWC_SCREEN_H

#include <stdint.h>

/* BBC Micro MODE 7 (Teletext): 40 columns x 24 rows.
 * Column 0 on each playfield row holds the graphics colour control code
 * (145-151).  The server is told 39x24; drawable cells are columns 1-39. */
#define SCREEN_WIDTH         (40)
#define SCREEN_HEIGHT        (24)
#define PLAYFIELD_COLS       (39)
#define PLAYFIELD_COL_OFFSET (1)
#define REG_SCREEN_WIDTH     (PLAYFIELD_COLS)
#define REG_SCREEN_HEIGHT    (SCREEN_HEIGHT)
#define REG_WORLD_WIDTH      (PLAYFIELD_COLS)
#define REG_WORLD_HEIGHT     (SCREEN_HEIGHT)
#define SCREEN_ROW_BYTES     (SCREEN_WIDTH)
#define SCREEN_BUF_SIZE      (SCREEN_WIDTH * SCREEN_HEIGHT)

#define GFX_CHAR_EMPTY       (0x20)  /* blank mosaic background */

/* Graphics colour controls in MODE 7 RAM (edit.tf / mkglob.bas):
 * red=17 (0x11) .. white=23 (0x17).  Teletext references use +128 (145-151)
 * but bytes written to screen RAM are 17-23 — same as ISS DATA 18, 23, etc. */
#define GFX_COLOUR_FIRST     (17)
#define GFX_COLOUR_LAST      (23)
#define GFX_COLOUR_DEFAULT   (23)

extern uint8_t gfx_colour;

/* MOS screen base pointer (from &0350/&0351, Master-safe). */
extern uint8_t *screen_visible;

/* Off-screen compose buffer; blitted to screen_visible each frame. */
extern uint8_t screen_buf[SCREEN_BUF_SIZE];

void screen_init(void);
void screen_blit_rows(uint8_t num_rows);
void set_blit_rows(uint8_t rows);

void screen_playfield_clear(uint8_t max_row);
void screen_put_cell(uint8_t x, uint8_t y, uint8_t ch);

void gfx_show_shape(uint8_t shape_id, int8_t center_x, int8_t center_y,
                    uint8_t max_row);
void gfx_cycle_colour(void);
uint8_t gfx_get_colour(void);

#endif /* BWC_SCREEN_H */
