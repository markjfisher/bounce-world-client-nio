#ifndef BWC_FEATURES_H
#define BWC_FEATURES_H

/* Optional features, by target.
 *
 * makefiles/build.mk defines BWC_8BIT_TARGET for the BBC and Atari. Their
 * 40x24 text display gains nothing from these features, and they have little
 * code space or CPU to spare, so they build without them (and without the
 * matching sources):
 *
 *   BWC_FEATURE_FETCH_PACING  a configurable minimum interval between
 *                             world-state fetches (fetch_pacing.c and the
 *                             Fetch setting on the welcome screen)
 *   BWC_FEATURE_CAPS          registration capabilities (wide coordinates,
 *                             rotation, body id) and decoding the shape
 *                             records they add (shape_decode.c,
 *                             add_client_csv.c)
 */
#ifdef BWC_8BIT_TARGET
#define BWC_FEATURE_FETCH_PACING 0
#define BWC_FEATURE_CAPS         0
#else
#define BWC_FEATURE_FETCH_PACING 1
#define BWC_FEATURE_CAPS         1
#endif

#endif /* BWC_FEATURES_H */
