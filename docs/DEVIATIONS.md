# DEVIATIONS.md — how `src/` differs from the original AMICUS disassembly

**Single source of truth for the changes this repo makes to the AMICUS Boing source.** Other docs link here instead of repeating it.

## Baseline: byte-for-byte faithful

An unmodified `src/` assembles **byte-for-byte identical** to `archive/boing_original.s` (Harry Sintonen's disassembly), and that in turn matches the rawer Amicus `archive/boing_s` modulo include-symbol spelling. The split into five files, the comments, and the relabeling change **zero instructions**.

The repo deviates from that original in **three deliberate places** — auto-start (Deviation #1), the Kickstart 2.0+ frame-rate fix (Deviation #2), and bringing the ball screen to the foreground (Deviation #3), all below. Deviation #1 is a single-value flip; Deviation #2 adds a small version-gated code block plus two library stubs; Deviation #3 swaps one intuition call-site and adds a one-line stub — so a `src/`-built binary is no longer strictly byte-identical to `archive/` (see each section and "Re-verifying" below). `archive/` keeps the unmodified originals; **never edit `archive/`**, and don't "tidy" the disassembly's quirks away elsewhere — the quirks (overlapping globals, dead flags, the single chipset poke) are part of what's being preserved.

## Deviation #3 — ball screen brought to the foreground (foreground start)

- **Where:** `src/main.s`, Phase 7 (`.window_opened`); plus a `_ScreenToFront` stub added in `src/runtime.s` (next to the existing `_ScreenToBack`).
- **Original:** the demo opens its own `CUSTOMSCREEN` (no title bar / gadgets) and calls `ScreenToBack` ("let Workbench cover us until ready"), so it sits **in the background** — the original "polite" design. On Kickstart 1.x you had to drag the front (CLI/Workbench) screen's title bar **down** to reveal it. (On KS2.0+ the boot shell is windowless, so the ball screen was already the visible one.)
- **Change:** call `ScreenToFront` instead of `ScreenToBack`, so the ball is shown immediately at launch. The rest of the screen handling — the custom screen and the direct `RastPort`/`ViewPort` pointer pokes — is unchanged.
- **Effect:** on Kickstart 1.3 the demo now appears in the foreground at startup. You briefly see the globe being built (`_init_globe`) before the ball starts bouncing — that construction was always there, it was just hidden behind the CLI before.
- **Byte-identity caveat:** like Deviation #2, this changes `src/` (one instruction + a stub), so a `src/`-built binary is not byte-identical to `archive/`. The now-unused `_ScreenToBack` wrapper is left in place (kept per the repo's no-tidying rule).

## Deviation #1 — auto-start (no pause-on-start)

- **Where:** `src/main.s`, the `.nomsg` main-loop block and `.first_frame_init`.
- **Original:** after the first frame it sets `_sstep=1` intending "running", but the only animation path runs a frame solely when `_sstep`'s companion flag `_sstept` is set — and **`_sstept` is never assigned anywhere in the binary** (verified in `archive/boing_original.s` and `archive/boing_s`; the per-frame trigger it expected isn't present). So `_sstep=1` froze the demo until a mouse click flipped it to `_sstep=0`, the run path. That run path animates with **no explicit `WaitTOF`** — its frame pacing comes entirely from the `WaitTOF` that `RethinkDisplay` performs internally (RKRM: *"RethinkDisplay … also does a WaitTOF()"*), i.e. **one vblank wait per frame → the video field rate** (50 Hz PAL / 60 Hz NTSC).
- **Change:** default `_sstep=0` and `.first_frame_init` no longer flips it, so the demo runs from launch without a click. The run path's *pacing is the original's, untouched*: the `_sstep==0` branch animates and is paced by `RethinkDisplay`'s internal `WaitTOF`; only the paused (`_sstep!=0`) branch calls `WaitTOF` explicitly, to avoid busy-spinning the IDCMP poll. A left/right mouse click still toggles pause/resume; the dead `_sstept` single-step flag is retired (its data def is left in place).
- **Forward guard:** do **not** add an explicit `WaitTOF` on the running path. `RethinkDisplay` (and `MakeScreen`) already carry a `WaitTOF`, so a second one would mean two vblank waits per frame and halve the update rate to ~25 Hz. The running path's only frame sync is `RethinkDisplay`'s internal `WaitTOF` — one step per field. See [ANIMATION-DETAILS.md](ANIMATION-DETAILS.md) §1.

## Deviation #2 — Kickstart 2.0+ frame-rate fix (50 Hz on later OS)

Keeps the demo's intended **one-step-per-video-field** tempo when run on Kickstart 2.0+ (it isn't the original target — KS1.x is — but the demo runs there, just at half speed without this).

- **Where:** `src/main.s`, the per-frame display-commit block (the `MakeScreen`/`RethinkDisplay` pair right after the gravity step); plus two small graphics.library stubs `_MrgCop` / `_LoadView` added in `src/runtime.s` (next to `_WaitTOF`).
- **Symptom:** on the target Kickstart 1.x (graphics.library v34) the demo advances one step per video field — the native field rate (50 Hz PAL / 60 Hz NTSC). On **Kickstart 2.0+** (gfx v36+) the *same* binary advances one step per **two** fields, i.e. **half speed** on any system (~25 Hz PAL / 30 Hz NTSC). Cause: the per-frame `MakeScreen` + `RethinkDisplay` pair costs **one** vblank wait on 1.x but **two** on 2.0+, because 2.0's reworked `RethinkDisplay` does an extra internal `WaitTOF`. (Same "two waits halve the rate" mechanism the Deviation #1 forward-guard warns about — except here 2.0+ adds the second wait *itself*.)
- **Change (version-gated; the 1.x path is logically the original):** read `graphics.library` `lib_Version` (word at offset 20 of `_GfxBase`).
  - **v < 36 (KS1.x):** unchanged — `MakeScreen` then `RethinkDisplay` (its single internal `WaitTOF` paces the frame).
  - **v ≥ 36 (KS2.0+):** keep `MakeScreen` — it propagates this frame's `RasInfo` scroll offsets *and* the bitplane-pointer rewrite into the screen's copperlist; **dropping it freezes the ball** (verified empirically). Then replace `RethinkDisplay` with its essentials at a *single* wait: `MrgCop(view)` + `LoadView(view)` + one explicit `WaitTOF`, with `view = GfxBase->ActiView` (offset 34). Net: exactly one vblank wait per frame on both OSes → one step per video field everywhere.
- **Rate-agnostic (PAL *and* NTSC):** the gate is on the gfx `lib_Version`, **not** on PAL/NTSC, and `WaitTOF` syncs to whatever the machine's field rate is. So the fix restores one-step-per-field on *both*: ~50 Hz on PAL and ~60 Hz on NTSC, each matching that system's 1.x tempo. The PAL/NTSC tempo gap itself (PAL ≈ 0.83× NTSC) is unchanged and unrelated — see [ANIMATION-DETAILS.md](ANIMATION-DETAILS.md) §1. (Only PAL is directly testable here: FS-UAE forces PAL for this A500/KS1.3 config, so the NTSC case is reasoned, not measured.)
- **Why not just drop a call:** `RethinkDisplay` alone (without `MakeScreen`) only merges *stale* per-viewport copperlists, so the ball doesn't move; and you can't "un-wait", so the only lever is to stop using the wrapper whose internal `WaitTOF` is redundant — hence the explicit `MrgCop`+`LoadView`+`WaitTOF` decomposition.
- **Result:** measured on A500 + KS2.04 (PAL) — full rate restored (~1.92 s bounce, vs ~3.8 s before), animation correct; KS1.3 runs identically to before (confirmed: both same speed). NTSC follows by the same one-wait-per-field logic.
- **Byte-identity caveat:** unlike Deviation #1 (a one-value flip), this adds instructions and two stubs, so a `src/`-built binary is **no longer byte-identical** to `archive/` even on the 1.x path — the new code is present though dormant on KS1.x. The block is self-contained and clearly commented, so a strictly-faithful build can be recovered by reverting Deviation #2 if ever needed.

## Re-verifying the split is otherwise faithful

Assemble the matching `archive/` original and `cmp` it against a `src/`-built binary. They now differ by **all three** deviations above (auto-start; the KS2.0+ commit block and the `_MrgCop`/`_LoadView` stubs; and the `_ScreenToFront` swap + its stub); to check the split alone, temporarily revert Deviations #2 and #3 and compare — what remains should differ only by the one-value auto-start flip. Note `archive/*.s` carry their own `INCDIR "include"`, so point them at the vendored NDK headers explicitly:

```sh
vasmm68k_mot -m68000 -Fhunk -I vendor/include archive/boing_original.s -o /tmp/orig.o
vlink -bamigahunk -Bstatic /tmp/orig.o -o /tmp/orig.bin
```

Don't edit the archived originals to "fix" that include path.
