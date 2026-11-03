#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stream/diagnostic.h"
static uint16_t clock_now;
static unsigned draws;
static char shown[41];
extern uint16_t bwc_diag_rate,bwc_diag_fetch,bwc_diag_max,bwc_diag_write,bwc_diag_read;
extern uint8_t bwc_diag_notready,bwc_diag_calls;
uint16_t bwc_diag_clock(void) {return clock_now;}
void bwc_diag_row(const char *s) {memcpy(shown,s,40);shown[40]=0;++draws;}
int main(void) {
    unsigned before;
    bwc_diag_io_begin('R');bwc_diag_io_end(4);assert(!draws);
    clock_now=65000;bwc_diag_start();bwc_diag_begin();
    bwc_diag_io_begin('W');clock_now+=2;bwc_diag_io_end(0);
    bwc_diag_io_begin('R');clock_now+=7;bwc_diag_io_end(4);
    clock_now=65015;bwc_diag_end(1);bwc_diag_refresh();
    assert(bwc_diag_fetch==15 && bwc_diag_max==15);
    assert(bwc_diag_write==2 && bwc_diag_read==7);
    assert(bwc_diag_notready==1 && bwc_diag_calls==1);
    assert(strstr(shown,"F015 M015 W002 R007 N01 C01 P:-"));
    clock_now=65100;bwc_diag_begin();clock_now=65150;bwc_diag_end(1);bwc_diag_refresh();
    assert(bwc_diag_rate==6 && bwc_diag_max==50); /* 2 new steps / 3 sec */
    assert(!strncmp(shown,"U00.6",5));
    clock_now=65530;bwc_diag_begin();bwc_diag_io_begin('R');
    assert(shown[36]=='R');
    clock_now=10;bwc_diag_io_end(0);bwc_diag_end(0);
    assert(bwc_diag_fetch==16 && bwc_diag_read==16 && bwc_diag_max==50);
    bwc_diag_stop();before=draws;bwc_diag_io_begin('W');bwc_diag_io_end(4);assert(draws==before);
    puts("stream diagnostic: phase, durations, wrap, rate, max and inactive hooks passed");
    return 0;
}
