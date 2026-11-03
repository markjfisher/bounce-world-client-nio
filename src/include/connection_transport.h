#ifndef BWC_CONNECTION_TRANSPORT_H
#define BWC_CONNECTION_TRANSPORT_H
#include "fujinet-nio.h"
#ifdef BWC_SMALL_STREAM
#include "stream/network.h"
/* Only the TCP path uses this adapter. AppStore retains normal NIO. */
#define bwc_tcp_init() bwc_stream_init()
#define bwc_tcp_open(h,m,u,f) bwc_stream_open(u)
#define bwc_tcp_close(h) bwc_stream_close()
#define bwc_tcp_read(h,o,b,n,c,f) bwc_stream_read(b,n,c,f)
#define bwc_tcp_write(h,o,b,n,c) bwc_stream_write(b,n,c)
#else
#define bwc_tcp_init fn_init
#define bwc_tcp_open fn_open
#define bwc_tcp_close fn_close
#define bwc_tcp_read fn_read
#define bwc_tcp_write fn_write
#endif
#endif
