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
error. These are emulator results; no Bounce World hardware acceptance is claimed.

Boot the stream ATR with your existing server, with U1MB/HSIO enabled. Check the
object gallery, live animation, brief I/Q key presses, settings persistence and
quit/rejoin. Increase object count while staying below the 83-object buffer limit.
Capture sio.status and any stalls or visual/audio issues. Compare responsiveness
with the default disk; keep it available if the stream update rate is too low.

## First-row timing diagnostic

Build `make disk-atari-stream-diag` and boot
`disk-images/atari-stream-diag/bwcn.atr`. This separate variant reserves row 0
for diagnostics; objects behind that row are obscured. The normal stream and
packet builds do not include instrumentation. No firmware update is needed.

Example:

```text
U08.8 F006 M074 W002 R004 N00 C01 P:R
```

- **U**: newly received world steps per second, averaged over windows of at least
  two seconds of Atari video time. Duplicate server steps do not count. This is
  an update rate, not the server's physics rate or a host wall-clock measurement.
- **F**: duration of the last whole world fetch, from before the x-w write to
  completion of the framed reply; excludes rendering and keyboard handling.
- **M**: maximum F since entering the world. It keeps a brief stall visible.
- **W / R**: total time spent inside stream write/read calls for that fetch.
  A call includes both S/T exchanges and any internal library carrier retries.
- **N**: Busy/NotReady replies during that fetch, across writes and reads.
- **C**: actual wire read calls during that fetch. Reads satisfied from the
  read-ahead buffer do not count; internal SIO retries are not separate C calls.
- **P**: live phase: W write, R read, F between calls in a fetch, ! just returned
  a nonzero status, or - completed fetch/render/keyboard. Timing/count fields
  describe the previous completed fetch while P describes the current phase.

F/M/W/R are **video ticks**: normally 20 ms PAL or about 16.7 ms NTSC. Values
saturate on display at 999 ticks; N/C at 99. Counters reset on entering the world.
The row does not run an interrupt handler: during a stuck SIO call P remains W
or R, and timing fields update when it returns. This avoids changing the very
VBI/DLI workload being investigated. Phase changes write only one character;
full formatting happens once per loop. There is still some foreground overhead,
so this disk is for diagnosis, not an uninstrumented throughput benchmark.

How to interpret a pause:

- Large R, N=0: time inside serial/read transactions, possibly internal carrier
  retries; collect FujiNet `sio.status` to distinguish those from service delays.
- Large N/C: repeated no-data/backpressure replies. F minus W/R includes the
  application's three-video-tick retry pauses and other foreground work.
- Small F but low U: fetching is completing; repeated server step IDs or work
  outside fetching (rendering/input) could account for the low update rate.

Capture the row during a pause (especially P), then immediately after recovery
(especially M/F/W/R/N/C), with a FujiNet sio.status snapshot if possible.
Host tests exercise rate math, clock wrap, per-call accounting and inactive
hooks. The headless test can inject a known delay with `--response-delay-ms 400`
to verify nonzero NotReady counters and increased fetch duration.
