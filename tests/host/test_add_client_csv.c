#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "add_client_csv.h"
#include "shape_decode.h"

static int failures;

static void check_str(const char *actual, const char *expected,
                      uint16_t len_actual, const char *what)
{
    if (len_actual == 0 || strcmp(actual, expected) != 0) {
        printf("FAIL %s: got \"%s\" (len %u), expected \"%s\"\n",
               what, actual, len_actual, expected);
        failures++;
    }
}

static void check_true(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL %s\n", what);
        failures++;
    }
}

static void test_legacy_six_field(void)
{
    char buf[64];

    memset(buf, 0x7F, sizeof(buf));
    check_str(buf, "myclient,2,80,24,40,24",
              bwc_build_add_client_csv(buf, sizeof(buf), "myclient", 2U,
                                       80U, 24U, 40U, 24U, 0U),
              "zero caps -> byte-identical legacy 6-field CSV");
}

static void test_caps_seven_field(void)
{
    char buf[64];

    check_str(buf, "myclient,3,320,256,40,24,0x1",
              bwc_build_add_client_csv(buf, sizeof(buf), "myclient", 3U,
                                       320U, 256U, 40U, 24U, BWC_CAP_WIDE_COORDS),
              "wide caps -> exact 7-field form ending in hex caps");
}

static void test_caps_wide_mask_no_truncation(void)
{
    char buf[64];

    /* Full-range mask must survive: no fixed width, no padding assumption */
    check_str(buf, "myclient,3,320,256,40,24,0xDEADBEEF",
              bwc_build_add_client_csv(buf, sizeof(buf), "myclient", 3U,
                                       320U, 256U, 40U, 24U, 0xDEADBEEFU),
              "full-width unsigned caps formatted completely");
}

static void test_truncation_rejected(void)
{
    char buf[8];
    uint16_t r = bwc_build_add_client_csv(buf, sizeof(buf), "myclient", 2U,
                                          80U, 24U, 40U, 24U, 0U);

    check_true(r == 0, "oversized destination rejected (returns 0)");
}

static void test_invalid_args_rejected(void)
{
    char buf[64];

    check_true(bwc_build_add_client_csv(NULL, sizeof(buf), "n", 2U,
                                        80U, 24U, 40U, 24U, 0U) == 0,
               "NULL destination rejected");
    check_true(bwc_build_add_client_csv(buf, 0U, "n", 2U,
                                        80U, 24U, 40U, 24U, 0U) == 0,
               "zero capacity rejected");
    check_true(bwc_build_add_client_csv(buf, sizeof(buf), NULL, 2U,
                                        80U, 24U, 40U, 24U, 0U) == 0,
               "NULL name rejected");
    check_true(bwc_build_add_client_csv(buf, sizeof(buf), "", 2U,
                                        80U, 24U, 40U, 24U, 0U) == 0,
               "empty name rejected");
}

int main(void)
{
    test_legacy_six_field();
    test_caps_seven_field();
    test_caps_wide_mask_no_truncation();
    test_truncation_rejected();
    test_invalid_args_rejected();

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all add-client CSV builder tests passed\n");
    return 0;
}
