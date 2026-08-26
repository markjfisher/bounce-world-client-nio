# Snapshot interpolation for the Bouncy World client

Implemented design notes for the Bouncy World renderer. The Amiga client
keeps two past snapshots and renders their blend at the display rate while a
separate worker fetches the next snapshot. Legacy clients retain the existing
raw-packet path.

## Why the obvious approach seems impossible (and isn't)

The natural worry: "to smooth motion I must predict where shapes go between
packets, which means simulating collisions, which I can't do." That worry
applies to **extrapolation**, not interpolation. They are different
techniques:

- **Extrapolation** (dead reckoning) predicts the *future*: keep drawing a
  shape along its last known velocity until the next packet. It diverges at
  every collision — your predicted path leaves the wall while the real body
  bounced — and then snaps back when the truth arrives. Handling that well
  requires local collision knowledge or error-convergence machinery.
- **Interpolation** renders the *past*: keep the last two snapshots and draw
  each shape blended between them at a render clock lagged by roughly one
  packet period. You never guess, so you never diverge, and collisions need
  zero local physics — a bounce is just an ordinary new snapshot that the
  blend curves toward.

The entire cost of interpolation is one packet of added latency (typically
50–150 ms). For this application it is invisible.

## Server identity extension and cross-client sync

The server remains authoritative. It now offers an opt-in `BODY_ID`
capability and emits a stable uint32 body id after each visible-shape record
for capable clients. It also orders records deterministically by body id and
visible-image position. Older clients keep their existing record layout.

Each client buffers its own two snapshots and lags its own render clock.
Clients were already desynchronised by jitter and tick phase before
interpolation; afterwards they differ by the same order of magnitude.
There is no cross-client consistency contract — the server is the sole
authority and every client approximates it independently. Legacy targets
(atari/bbc/linux/msdos) simply keep rendering raw packets; the server
neither knows nor cares which clients interpolate.

Buffer cost: two snapshots × shapeCount × record size ≈ worst case
2 × 240 × 9 = ~4.3 KB on Amiga (`APP_DATA_SIZE` is already 2304). Typical
worlds are far smaller. Tight for 8-bit targets, which can just opt out.

## The genuinely tricky part: matching shapes across snapshots

Blending requires pairing each shape in snapshot N with "the same" shape in
snapshot N−1. `shapeId` alone is **not sufficient**:

- Live captures show multiple entries with equal ids (e.g. three entries
  with id 3) — some are wrap-seam copies of one body, some are distinct
  bodies sharing a type.

Implemented identity-aware matcher:

1. Filter previous-snapshot candidates by equal `bodyId`.
2. For every visible copy of that body, consider translations by ±screen
   width and height, and choose the nearest image to the current copy.
3. Blend the matched image in its local pixel space. This preserves separate
   copies at a wrap seam and never substitutes an opposite-edge image.
4. If the best match moved further than plausible (teleport / despawn /
   respawn), skip blending for that shape this cycle: draw snapshot N
   directly instead of interpolating garbage.

Because copies are matched image-to-image, position blending stays inside one
region's pixel space and the existing viewport clipping handles straddling
results unchanged.

## Rotation is the easy case

Angles arrive as uint16 with ω as int16 fixed point (rad/s = bits/256).
Two options:

- **Blend:** unwrap first, then interpolate:
  `d = ((a2 - a1 + 32768) mod 65536) - 32768`; `a = a1 + d·u`.
- **Advance by ω:** `angle(t) = a1 + ω·(t − t1)` — the wire hands you the
  exact derivative, so rotation interpolates essentially perfectly, and a
  collision shows up as a small ω change at the next resync rather than a
  visual pop.

Either way, always re-sync from the newest packet (the wire contract
already states this rule).

## Algorithm sketch

```
keep prev[] and curr[] snapshots + arrival timestamps
render_time = now - interp_delay          # ≈ 1–1.5× measured packet interval

on packet arrival:
    prev <- curr
    curr <- decode(new payload)           # existing bwc_decode_shapes

each frame:
    u = clamp((render_time - t_prev) / (t_curr - t_prev), 0, 1)
    for each shape s in curr:
        p = best wrap-aware image match in prev (body id, then nearest)
        if no plausible p: draw s directly; continue
        draw(s.id,
             lerp(p.x, s.x, u),
             lerp(p.y, s.y, u),
             advance_angle(p.angle, s.angle, p.omega, u))
    # if packets stall, u clamps at 1.0 → freeze on curr. Never extrapolate.
```

`interp_delay` starts fixed at one packet period and can later adapt to the
observed inter-arrival variance (a mini jitter buffer). Tune by eye before
adding cleverness.

## Where this fits the current codebase

- Decode: `bwc_decode_shapes()` already parses 5-byte (WIDE_COORDS) records;
  the 9-byte ROTATION layout including angle/omega is implemented and tested.
- Render entry: `gfx_show_shape_px(id, x, y)` in `src/amiga/gfx.c`; adding
  an angle parameter plus the rotate-about-centre transform is the deferred
  ROTATION goal recorded in the workspace `deferred-work.md`.
- `src/amiga/interpolation.c` supplies the snapshot worker, handoff, and
  blend path. `src/common/bwc_interpolation_math.c` keeps matching and
  blending host-testable.

## Prior art

- **Valve — "Source Multiplayer Networking"** (developer.valvesoftware.com):
  the canonical writeup of entity interpolation with a lagged render clock,
  and the explicit comparison against extrapolation. Start here.
- **Quake 3 snapshot interpolation**: the origin of the technique; Dave
  Marshall's "Quake 3 Networking" article series dissects it.
- **Glenn Fiedler — Gaffer on Games**: "Snapshot Compression" and the
  "Networked Physics" series; jitter buffers and why naive prediction
  diverges.
- **Gabriel Gambetta — "Fast-Paced Multiplayer"**: clearest diagrams of
  smoothing vs prediction/reconciliation.
- **IEEE 1278 (DIS) dead reckoning**: the formalised version of the
  *other* technique — error thresholds, convergence protocols — included
  as evidence of what extrapolation costs and why to avoid it here.
