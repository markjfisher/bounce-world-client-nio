# Optional Atari small-stream TCP backend

Build with a fujinet-nio-lib checkout containing `fn_stream.h` and firmware
supporting small-stream version 1 (s/S/T). Library release 0.1.2 does not include
this API. No new firmware changes are required beyond the small-stream firmware
used for STREAMTEST and SYN-ACK WIRE.

```sh
# From workspace root
source scripts/env.sh
make -C repos/bounce-world-client-nio disk-atari-stream
```

Outside the workspace, set `FUJINET_NIO_LIB` and provide cc65 plus dir2atr.
Executable: `build/atari-stream/bwcn.atari`.
Disk: **`disk-images/atari-stream/bwcn.atr`**.
`make disk-atari` still builds the packet backend in `disk-images/atari/`.
Objects, executables and splash assets are separate, so no clean is needed to
switch between these two targets.

Enter the same TCP server and player name as before. Quit returns to the title;
saved settings are still read/written through AppStore using the general NIO
transport, with title effects paused during those calls. The TCP path uses the
small stream and does not alter U1MB tables or FujiNet HSIO configuration.
TLS URLs are not supported by this backend. No renderer or server protocol change
is required. Other platform builds use their existing packet APIs.

## Framing and recovery

Responses have a two-byte little-endian total length including the header.
The world payload then has step, status and object count, followed by three bytes
per object. The TCP adapter reads the length first, validates the existing buffer
limit, then receives at most 120 bytes at a time. Short reads can split headers
or object records. Rendering only begins after assembling the complete frame;
the adapter never asks past its end or consumes the next response accidentally.

The Atari world buffer remains 256 bytes, including the two-byte length and
three-byte world header: at most 83 three-byte records fit. Chunking does not
increase that bound. Oversized world packets remain errors. Auxiliary client-list
responses can be larger: retain the visible prefix and drain the rest in chunks.

Writes are split and partial acceptance advances only the accepted prefix.
Busy/NotReady/zero-progress writes pause and retry, bounded to 120 consecutive
idle attempts (each pause is three video frames). Any terminal error latches the
session failed. In particular, an exhausted exact carrier retry cannot be
followed by a different request at the same uncertain sequence. Generic caller
retries return that error without resending a partly completed command. A new
init/open replaces the connection; it does not promise that an uncertain command
was never applied by the server. Reads retain the existing application's
Busy/NotReady polling policy. There is no automatic reconnect in the adapter.

## Costs and measurements

Measured with the same cc65 build and library checkout:

| Metric | Packet (default) | Small stream |
| --- | ---: | ---: |
| XEX bytes | 28,938 | 26,015 |
| CODE bytes | 25,282 | 22,360 |
| BSS bytes | 2,765 | 2,966 |
| BSS last address | $93CA | $8928 |
| 4 objects, fetches/s | 8.08 | 6.33 |
| 80 objects, fetches/s | 5.17 | 3.01 |

XEX saves 2,923 bytes; BSS grows by 201 bytes; the end of the application's
allocation moves down by 2,722 bytes. AppStore keeps normal packet infrastructure
linked, so this does not achieve the same saving as SYN-ACK WIRE. The small
adapter supplies its own 262-byte context. A C adapter retains the existing
application framing API; it is not a claim of the minimum possible ASM footprint.

Rates are headless AltirraSDL + real POSIX NetSIO firmware measurements against
the same deterministic local peer, with fragmented replies and 22 steady samples
per count. They measure fetch completion including rendering, not server physics
UPS or hardware performance. Stream is slower here: header-first reading adds
an exchange, and large responses require multiple S/T pairs at 19200. This is
an optional memory/interrupt-compatibility trade-off, not a performance upgrade.

Polled serial masks CPU IRQs but leaves VBI/DLI active, reserves POKEY channels
3/4 and serial clock control, and requires RTCLOK to tick. The existing keyboard
fallback remains. Refer to the library's `docs/atari-small-stream.md` for the
full register ownership and retry contract. Hardware timing still needs testing
with Bounce World's particular interrupt workload.

## Verification and hardware trial

Automated checks:

```sh
make test-host test-atari-keyboard
make -C "$FUJINET_NIO_LIB" test-atari-stream
```

The host suite mocks the stream API independently and covers partial writes,
Busy/NotReady, uncertain-result poisoning, split headers/records, packets above
120 bytes, queued consecutive packets, guard bytes, oversized frames, EOF and
prefix draining. soft65c02 tests cover the library carrier and Atari keyboard.
Both Atari disks and the Linux build compile.

Workspace headless integration (run sequentially; NetSIO uses a shared port):

```sh
python3 integration-tests/atari-sio/run_bounce_stream.py --backend stream --output /tmp/bwc-stream
python3 integration-tests/atari-sio/run_bounce_stream.py --backend packet --output /tmp/bwc-packet
# With a real server running and at least one moving object:
python3 integration-tests/atari-sio/run_bounce_stream.py --server 127.0.0.1:9003 --output /tmp/bwc-real
```

Both backends passed 4/80-object fragmented-peer tests, input, quit and rejoin.
The stream backend also passed against `server-3.0.0.jar` with a moving object,
12 distinct world steps before and after reconnect, keyboard input and no client
error. These are emulator results; no Bounce World hardware acceptance is claimed.

Boot the stream ATR with your existing server, with U1MB/HSIO enabled. Check the
object gallery, live animation, brief I/Q key presses, settings persistence and
quit/rejoin. Increase object count while staying below the 83-object buffer limit.
Capture sio.status and any stalls or visual/audio issues. Compare responsiveness
with the default disk; keep it available if the stream update rate is too low.
