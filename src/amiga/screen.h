#ifndef BWC_AMIGA_SCREEN_H
#define BWC_AMIGA_SCREEN_H

/* Text cell grid used by the shared UI code (8x8 topaz on a lores screen) */
#define SCREEN_WIDTH      (40)
#define SCREEN_HEIGHT     (32)

/* Pixel dimensions registered with the server (version 3 wire contract).
 * NTSC machines open a 200-pixel-tall screen at runtime; the shared code
 * keeps using the PAL cell grid and the excess bottom rows are unused. */
#define SCREEN_PIXEL_WIDTH  (320)
#define SCREEN_PIXEL_HEIGHT (256)

#define REG_SCREEN_WIDTH  (SCREEN_PIXEL_WIDTH)
#define REG_SCREEN_HEIGHT (SCREEN_PIXEL_HEIGHT)

/* Logical Bouncy World units, unchanged by pixel resolution */
#define REG_WORLD_WIDTH   (40)
#define REG_WORLD_HEIGHT  (24)

#endif /* BWC_AMIGA_SCREEN_H */
