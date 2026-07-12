# bounce-world-client-nio

`bounce-world-client-nio` is a Bouncy World client for the Bouncy World server:

- Server: https://github.com/markjfisher/bounce-world
- Transport/device project: https://github.com/markjfisher/fujinet-nio
- Client library: https://github.com/markjfisher/fujinet-nio-lib

This client uses the newer `fujinet-nio` stack rather than the older `fujinet-lib` stack. The shared client code is written as portable C99, and `fujinet-nio-lib` is POSIX/C99 compatible, which makes it possible to build and run a native Linux version of the client alongside the retro targets.

## What it does

The client connects to a running Bouncy World server over TCP, uses embedded shape data, registers itself as a world client, and then renders the live world state while allowing interactive commands from the keyboard.

Current build targets are:

- `atari`
- `bbc`
- `linux`
- `msdos`

## Build prerequisites

You will need:

- `make`
- `gcc` for the Linux target
- Open Watcom for the MS-DOS target
- `cl65` / cc65 for the 8-bit targets
- `FUJINET_NIO_LIB` pointing at a checkout of `fujinet-nio-lib`
- The `bbc` target fork of cc65 from https://github.com/markjfisher/cc65 to compile for the BBC

The Linux client links against the direct NIO build of `fujinet-nio-lib`. The
default MS-DOS build links against the `msdos-ioctl` backend, so it talks to the
resident NIO build of `FUJINET.SYS` instead of taking ownership of COM1.

## Building

Build all supported targets:

```sh
export FUJINET_NIO_LIB=$HOME/dev/nio/repos/fujinet-nio-lib
make
```

Build just the BBC Micro version:

```sh
make bbc
```

Build just the Linux version:

```sh
make linux
```

Build just the MS-DOS version:

```sh
source ~/.local/bin/add_watcom.sh
make msdos
```

Clean and rebuild the Linux version:

```sh
make clean linux
```

Output binaries are written to `build/`, named by target (e.g. `build/bwcn.bbc`,
`build/bwcn.linux`, `build/bwcn.msdos.exe`).

## BBC SSD disk image

After building the BBC target you can package the binary into a BBC SSD disk
image (`.ssd` file) suitable for use with emulators or a BBC Micro with a
FujiNet:

```sh
make disk
```

This builds the BBC binary (if needed), then runs
[`scripts/create_ssd.py`](scripts/create_ssd.py) to produce
`disk-images/bbc/bwcn.ssd`.  The binary is placed on the disk with a DFS leaf
name of `BWCN`, load address `&001900`, and execution address `&001900`.

The disk image path is target-scoped so the architecture can be extended for
future platform disk formats (e.g. Atari `.atr` images).

## Running on Linux

The Linux build talks to `fujinet-nio` through a POSIX serial or PTY endpoint exposed by the device/service. Set `FN_PORT` to the path of that port, then run the built client:

```sh
make clean linux
FN_PORT=/path/to/pts-port ./build/bwcn.linux
```

At startup the client prompts for, and saves via FujiNet NIO app-store:

- the Bouncy World server address
- your player/client name

Enter the server as a TCP endpoint such as:

```text
tcp://localhost:9003
```

This client uses the server's **framed TCP port** (default 9003). Every response is
prefixed with a 2-byte little-endian total packet size; the read helpers in
`connection.c` strip and validate that header automatically. Use `app_payload` (not
`app_data` directly) when interpreting response data stored in the shared buffer.

If you omit the `tcp://` prefix, the client adds it automatically.

## Running on MS-DOS

The MS-DOS build talks to `fujinet-nio` through the MS-DOS COM serial channel in
`fujinet-nio-lib`. Build it with Open Watcom available:

```sh
source ~/.local/bin/add_watcom.sh
export FUJINET_NIO_LIB=$HOME/dev/nio/repos/fujinet-nio-lib
make msdos
```

The resulting executable is:

```text
build/bwcn.msdos.exe
```

The default MS-DOS backend is `ioctl`, which requires the NIO `FUJINET.SYS`
block driver to be loaded from `CONFIG.SYS`. This is the correct mode when the
application is loaded from a FujiNet-hosted DOS drive because the driver remains
the single owner of COM1.

For direct serial testing without `FUJINET.SYS`, build with:

```sh
# ensure watcom is on your path
export FUJINET_NIO_LIB=$HOME/path/to/fujinet-nio-lib
make msdos MSDOS_NIO_BACKEND=serial
```

The serial backend uses COM1 at 115200 baud unless `fujinet-nio-lib` is rebuilt
with different `FN_MSDOS_COM` / `FN_MSDOS_BAUD_DIVISOR` flags.

To test the resident-driver `INT F5` path instead of DOS IOCTL, build with:

```sh
make msdos MSDOS_NIO_BACKEND=f5
```

## Notes on the Linux client

- The Linux target uses the same shared gameplay/network client logic as the retro builds.
- The MS-DOS target uses the same shared gameplay/network client logic and a small DOS console shim.
- The Linux platform layer implements the cc65-style console APIs needed by the client, using a terminal-friendly renderer.
- Neutral shape codes from the server are converted back into real box-drawing and block characters on Linux terminals.

## Related projects

- Bouncy World server: https://github.com/markjfisher/bounce-world
- FujiNet NIO device/service: https://github.com/markjfisher/fujinet-nio
- FujiNet NIO client library: https://github.com/markjfisher/fujinet-nio-lib
