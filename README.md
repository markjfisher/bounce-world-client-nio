# bounce-world-client-nio

`bounce-world-client-nio` is a Bouncy World client for the Bouncy World server:

- Server: https://github.com/markjfisher/bounce-world
- Transport/device project: https://github.com/markjfisher/fujinet-nio
- Client library: https://github.com/markjfisher/fujinet-nio-lib

This client uses the newer `fujinet-nio` stack rather than the older `fujinet-lib` stack. The shared client code is written as portable C99, and `fujinet-nio-lib` is POSIX/C99 compatible, which makes it possible to build and run a native Linux version of the client alongside the retro targets.

## What it does

The client connects to a running Bouncy World server over TCP, downloads the available shape data, registers itself as a world client, and then renders the live world state while allowing interactive commands from the keyboard.

Current build targets are:

- `atari`
- `bbc`
- `linux`

## Build prerequisites

You will need:

- `make`
- `gcc` for the Linux target
- `cl65` / cc65 for the 8-bit targets
- A checkout of `fujinet-nio-lib` at `../fujinet-nio-lib`
- The `bbc` target fork of cc65 from https://github.com/markjfisher/cc65 to compile for the BBC

The Linux client links against the Linux build of `fujinet-nio-lib`, and the retro builds link against the corresponding platform libraries.

## Building

Build all supported targets:

```sh
make
```

Build just the Linux version:

```sh
make linux
```

Clean and rebuild the Linux version:

```sh
make clean linux
```

Output binaries are written to `build/`.

## Running on Linux

The Linux build talks to `fujinet-nio` through a POSIX serial or PTY endpoint exposed by the device/service. Set `FN_PORT` to the path of that port, then run the built client:

```sh
make clean linux
FN_PORT=/path/to/pts-port ./build/bwcn.linux
```

At startup the client prompts for:

- the Bouncy World server address
- your player/client name

Enter the server as a TCP endpoint such as:

```text
tcp://localhost:9002
```

If you omit the `tcp://` prefix, the client adds it automatically.

## Notes on the Linux client

- The Linux target uses the same shared gameplay/network client logic as the retro builds.
- The Linux platform layer implements the cc65-style console APIs needed by the client, using a terminal-friendly renderer.
- Neutral shape codes from the server are converted back into real box-drawing and block characters on Linux terminals.

## Related projects

- Bouncy World server: https://github.com/markjfisher/bounce-world
- FujiNet NIO device/service: https://github.com/markjfisher/fujinet-nio
- FujiNet NIO client library: https://github.com/markjfisher/fujinet-nio-lib
