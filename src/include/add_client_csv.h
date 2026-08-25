#ifndef BWC_ADD_CLIENT_CSV_H
#define BWC_ADD_CLIENT_CSV_H

#include <stdint.h>

/* Build the add-client CSV payload:
 *   name,<version>,<screenW>,<screenH>,<worldW>,<worldH>[,<caps>]
 *
 * A nonzero capabilities mask is appended as a 7th field formatted as
 * 0x-prefixed hex text with minimal digits (e.g. "0x1"); zero caps omits
 * the field entirely so legacy registrations stay byte-identical. The
 * mask has no fixed width: every bit of `unsigned` is representable.
 *
 * Writes at most dst_cap bytes including the terminating NUL.
 * Returns the payload length excluding the NUL, or 0 on truncation,
 * empty name, or invalid arguments. */
uint16_t bwc_build_add_client_csv(char *dst, uint16_t dst_cap,
                                  const char *name, unsigned version,
                                  unsigned screen_w, unsigned screen_h,
                                  unsigned world_w, unsigned world_h,
                                  unsigned caps);

#endif /* BWC_ADD_CLIENT_CSV_H */
