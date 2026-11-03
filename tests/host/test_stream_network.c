#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fujinet-nio.h"
#include "fn_stream.h"
#include "stream/network.h"
#include "connection.h"
uint32_t read_offset;
uint8_t err, app_data[256];
static uint8_t input[700], output[400];
static unsigned pos, end, sent, calls, pauses, fault, eof, fragment, errors;
void gotoxy(unsigned char x,unsigned char y) {(void)x;(void)y;}
void cputc(char c) {(void)c;}
void cputs(const char *s) {(void)s;}
char *itoa(int n,char *b,int base) {(void)base;sprintf(b,"%d",n);return b;}
void handle_err(char *reason) {(void)reason;++errors;}
void network_retry_pause(void) {++pauses;}
uint8_t fn_stream_init(fn_stream_io_t *io) {(void)io;return 0;}
uint8_t fn_stream_open(fn_stream_io_t *io) {assert(io->length==15);return 0;}
uint8_t fn_stream_close(fn_stream_io_t *io) {(void)io;return 0;}
uint8_t fn_stream_write(fn_stream_io_t *io) {
    unsigned n;
    ++calls;
    assert(io->length && io->length<=120);
    if(fault) return fault;
    if(calls==1) return 4;
    n=io->length>17?17:io->length;
    memcpy(output+sent,io->data,n);sent+=n;io->actual=n;return 0;
}
uint8_t fn_stream_read(fn_stream_io_t *io) {
    unsigned n;
    ++calls;
    assert(io->length && io->length<=120);
    if(fault) return fault;
    n=end-pos;if(n>io->length)n=io->length;
    if(fragment && n>fragment)n=fragment;
    memcpy(io->received,input+pos,n);pos+=n;io->actual=n;
    io->flags=eof && pos==end?1:0;
    return 0;
}
static void reset(void) {
    pos=end=sent=calls=pauses=fault=eof=fragment=errors=0;err=0;read_offset=0;
    assert(!bwc_stream_init());
}
static void packet(unsigned at,unsigned len) {
    unsigned i;input[at]=len;input[at+1]=len>>8;
    for(i=2;i<len;i++)input[at+i]=(uint8_t)(i*17);
}
int main(void) {
    uint8_t data[300],flags,guard[258];uint16_t n;unsigned i, before;
    for(i=0;i<sizeof(data);++i)data[i]=(uint8_t)i;
    reset();assert(!bwc_stream_open("tcp://host:123" "4"));
    assert(!bwc_stream_write(data,300,&n));assert(n==300 && sent==300 && pauses==1);
    assert(!memcmp(data,output,300));
    reset();fault=128;assert(bwc_stream_write(data,10,&n)==FN_ERR_TRANSPORT);
    before=calls;fault=0;assert(bwc_stream_read(data,10,&n,&flags)==FN_ERR_TRANSPORT);
    assert(calls==before); /* Never replace an uncertain transaction. */
    reset();fault=4;assert(bwc_stream_write(data,10,&n)==FN_ERR_TIMEOUT);assert(pauses==119);
    before=calls;fault=0;assert(bwc_stream_write(data,10,&n)==FN_ERR_TIMEOUT);assert(calls==before);
    /* Full frame >120, arbitrary splitting, and a second queued packet. */
    for(i=1;i<=121;i+=30) {
        reset();fragment=i;packet(0,245);packet(245,5);end=250;
        memset(guard,0xa5,sizeof(guard));
        assert(read_response_min(guard+1,1,254)==243);
        assert(pos==245 && guard[0]==0xa5 && guard[257]==0xa5);
        assert(!memcmp(guard+1,input,245));
        assert(read_response_min(guard+1,1,254)==3 && pos==250);
    }
    reset();packet(0,245);end=245;eof=1;assert(read_response_min(data,1,254)==243);
    reset();packet(0,300);end=300;assert(read_response_min(data,1,254)==-1 && errors==1 && pos==2);
    reset();packet(0,245);end=100;eof=1;assert(read_response_min(data,1,254)==-1 && errors==1);
    /* Prefix read must drain >120 discarded bytes, preserving next packet. */
    reset();packet(0,400);packet(400,5);end=405;fragment=13;
    assert(read_response_prefix(data,8)==8 && pos==400);
    assert(read_response_min(data,1,254)==3 && pos==405);
    puts("stream adapter: partial writes, retry poisoning, fragmented frames, bounds, EOF and drain passed");
    return 0;
}
