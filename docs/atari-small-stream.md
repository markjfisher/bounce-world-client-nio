# Optional Atari small-stream TCP backend

Build with a fujinet-nio-lib checkout containing `fn_stream.h` and firmware
supporting small-stream version 1 (s/S/T). Library release 0.1.2 does not include
this API. Use current firmware with the Atari COMMAND capture recovery fixes (through
`ffaf8954`). Earlier small-stream firmware can exhibit roughly 300 ms retries
with this client. Upload firmware from its repository with `./build.sh -u`.

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
per object. The framing code reads the length first and validates the existing buffer limit.
Underneath it, the TCP adapter requests up to 120 available bytes whenever its
read-ahead buffer is empty. Header and payload usually arrive in one wire read
for small worlds. A short TCP read can still split headers or object records;
120 is a capacity, not a requirement to wait for a full block.

The adapter gives the framing code only the bytes it requested and retains the
surplus, even if it belongs to the next response. A separate 120-byte buffer
keeps that data safe across intervening writes. EOF is delayed until all buffered
bytes have been delivered. Init, successful open and successful close discard
buffered state. Rendering begins only after the complete frame is assembled.

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
| XEX bytes | 28,938 | 26,285 |
| CODE bytes | 25,282 | 22,630 |
| BSS bytes | 2,765 | 3,089 |
| BSS last address | $93CA | $8AB1 |
| 4 objects, fetches/s | 8.08 | 8.83 |
| 80 objects, fetches/s | 5.17 | 3.47 |

XEX saves 2,653 bytes; BSS grows by 324 bytes; the end of the application's
allocation moves down by 2,329 bytes. AppStore keeps normal packet infrastructure
linked, so this does not achieve the same saving as SYN-ACK WIRE. The small
adapter supplies its own 262-byte context plus 120 bytes of read-ahead data
and three bytes of read-ahead state. Read-ahead added 270 bytes of executable
and 123 bytes of BSS relative to the initial stream implementation. A C adapter retains the existing
application framing API; it is not a claim of the minimum possible ASM footprint.

Rates are headless AltirraSDL + real POSIX NetSIO firmware measurements against
the same deterministic local peer, with fragmented replies and 22 steady samples
per count. They measure fetch completion including rendering, not server physics
UPS or hardware performance. Initial header-first wire reads measured 6.33/s
and 3.01/s; the 120-byte read-ahead changed those to 8.83/s and 3.47/s. It removes
one S/T pair when the header and small payload arrive together. Large responses
still require multiple S/T pairs at 19200. Every read reply is padded to 131 wire
bytes even when fewer than 120 payload bytes are available; read-ahead does not
remove that protocol cost. These measurements do not guarantee a particular
hardware rate or a speed-up for every workload.

Polled serial masks CPU IRQs but leaves VBI/DLI active, reserves POKEY channels
3/4 and serial clock control, and requires RTCLOK to tick. The existing keyboard
fallback remains. Refer to the library's `docs/atari-small-stream.md` for the
full register ownership and retry contract. Hardware testing with Bounce World confirmed colour, sound and responsive
keyboard operation; the library register-ownership contract still applies.

## Verification and hardware trial

Automated checks:

```sh
make test-host test-atari-keyboard
make -C "$FUJINET_NIO_LIB" test-atari-stream
```

The host suite mocks the stream API independently and covers partial writes,
Busy/NotReady, uncertain-result poisoning, split headers/records, packets above
120 bytes, queued consecutive packets, guard bytes, oversized frames, EOF and
prefix draining, single-read small responses, cached data across writes,
deferred EOF and cache reset on reopen. soft65c02 tests cover the library carrier and Atari keyboard.
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
error. These are emulator results. Subsequent Atari hardware testing accepted normal
small-world operation after firmware capture fixes, with approximately 4.5 new
world steps/s. A six-minute undisturbed run kept maximum fetch duration at
25 PAL video ticks, and the final follow-up was accepted for normal use. This
does not constitute exhaustive testing of all RAM expansions or dense worlds.

Boot the stream ATR with your existing server, with U1MB/HSIO enabled. Check the
object gallery, live animation, brief I/Q key presses, settings persistence and
quit/rejoin. Increase object count while staying below the 83-object buffer limit.
Collect diagnostics after the observation period: console output itself can
block firmware polling and cause an application pause. Compare responsiveness
with the default disk; keep it available if the stream update rate is too low.

## Production build and diagnostics

The temporary U/F/M/W/R/N/C/P timing row, its hooks and diagnostic-only build
have been removed. Use `make disk-atari-stream` for the normal disk; stale
`atari-stream-diag` outputs from older builds are not current release assets.

The firmware retains `sio.trace flight` for fault investigation. Run `sio.reset`
once **after animation is running**, then leave the console idle while observing.
Dump `sio.trace flight` and `sio.status` after a problem or at the end. A frozen
recording can belong to startup if reset was earlier. Printing the dump can
itself pause SIO, so do not count pauses during console output as gameplay faults.
See the firmware's `docs/atari/sio-capture.md` for the recovery contract and
recorder limitations. The network adapter does not guarantee uninterrupted
animation during server or network outages.
