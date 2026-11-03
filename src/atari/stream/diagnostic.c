/* Foreground-only diagnostic: no extra interrupt work or serial logging.
 * All durations use the OS video clock, not host wall-clock time. */
#include <stdint.h>
#include <string.h>
#include "diagnostic.h"
#include "double_buffer.h"
extern uint8_t display_buffer[];
#ifdef BWC_DIAG_TEST
extern uint16_t bwc_diag_clock(void);
extern void bwc_diag_row(const char *text);
#define ticks bwc_diag_clock
#else
static uint16_t ticks(void) {
    volatile uint8_t *p=(volatile uint8_t *)0x13;
    uint8_t hi,lo;
    do {hi=p[0];lo=p[1];} while(hi!=p[0]);
    return ((uint16_t)hi<<8)|lo;
}
#endif
/* Public labels allow the headless bridge to check the observations. */
uint16_t bwc_diag_rate, bwc_diag_fetch, bwc_diag_max;
uint16_t bwc_diag_write, bwc_diag_read;
uint8_t bwc_diag_notready, bwc_diag_calls;
static uint16_t start,io_start,window,updates;
static uint16_t write_ticks,read_ticks;
static uint8_t notready,calls,phase,hz,active;
static char row[41];
static void number(uint8_t at,uint16_t n,uint8_t width) {
    if(n>999) n=999;
    while(width) {row[at+--width]=(char)('0'+n%10);n/=10;}
}
static void draw(void) {
#ifndef BWC_DIAG_TEST
    uint8_t i,ch;
    uint8_t *front=*(uint8_t **)0x58;
#endif
    memcpy(row,"U00.0 F000 M000 W000 R000 N00 C00 P:-    ",40);
    number(1,bwc_diag_rate/10,2);number(4,bwc_diag_rate%10,1);
    number(7,bwc_diag_fetch,3);number(12,bwc_diag_max,3);
    number(17,bwc_diag_write,3);number(22,bwc_diag_read,3);
    number(27,bwc_diag_notready>99?99:bwc_diag_notready,2);
    number(31,bwc_diag_calls>99?99:bwc_diag_calls,2);
    row[36]=phase;
#ifdef BWC_DIAG_TEST
    bwc_diag_row(row);
#else
    /* Both game buffers: survive page flips and leave the last metrics visible
     * while a blocking call is in flight. Only the diagnostic row is reserved. */
    for(i=0;i<40;++i) {
        ch=row[i];if(ch<96) ch-=32;
        front[i]=ch;
        display_buffer[i]=ch;
        ((uint8_t *)screen_mem_orig)[i]=ch;
    }
#endif
}
static void draw_phase(void) {
#ifdef BWC_DIAG_TEST
    row[36]=phase;bwc_diag_row(row);
#else
    uint8_t ch=(uint8_t)(phase-32);
    display_buffer[36]=ch;
    ((uint8_t *)screen_mem_orig)[36]=ch;
#endif
}
void bwc_diag_start(void) {
    active=1;
#ifdef BWC_DIAG_TEST
    hz=50;
#else
    hz=(*(uint8_t *)0x62)==0?60:50; /* OS PALNTS */
#endif
    window=ticks();updates=0;
    bwc_diag_rate=bwc_diag_fetch=bwc_diag_max=bwc_diag_write=bwc_diag_read=0;
    bwc_diag_notready=bwc_diag_calls=0;phase='-';draw();
}
void bwc_diag_begin(void) {
    start=ticks();write_ticks=read_ticks=0;notready=calls=0;phase='F';draw_phase();
}
void bwc_diag_io_begin(uint8_t p) {
    if(!active) return;
    phase=p;draw_phase();io_start=ticks();
    if(p=='R' && calls<255) ++calls;
}
void bwc_diag_io_end(uint8_t status) {
    uint16_t elapsed;
    if(!active) return;
    elapsed=(uint16_t)(ticks()-io_start);
    if(phase=='W') write_ticks+=elapsed;else read_ticks+=elapsed;
    if((status==3 || status==4) && notready<255) ++notready;
    phase=status?'!':'F';draw_phase();
}
void bwc_diag_end(uint8_t changed) {
    uint16_t now=ticks(), elapsed=(uint16_t)(now-window);
    bwc_diag_fetch=(uint16_t)(now-start);
    if(bwc_diag_fetch>bwc_diag_max) bwc_diag_max=bwc_diag_fetch;
    bwc_diag_write=write_ticks;bwc_diag_read=read_ticks;
    bwc_diag_notready=notready;bwc_diag_calls=calls;
    if(changed) ++updates;
    if(elapsed>=hz*2) {
        bwc_diag_rate=(uint16_t)(((uint32_t)updates*hz*10)/elapsed);
        updates=0;window=now;
    }
    phase='-';
}

void bwc_diag_refresh(void) { draw(); }

void bwc_diag_stop(void) { active=0; }
