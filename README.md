# amiga-boing

![The Boing Ball demo running under FS-UAE at PAL field rate — two bounces and a wall hit](docs/hero.gif)

> 🌐 **Try it now — no emulator needed:** **[Live demo ▶](https://mbackschat.github.io/amiga-boing-web/)** — a browser port of this demo, source at **[mbackschat/amiga-boing-web](https://github.com/mbackschat/amiga-boing-web)**.

The famous **Amiga Boing Ball** demo (Dale Luck / R. J. Mical, 1984 CES) — the red-and-white checkered sphere bouncing in a magenta wireframe room — as buildable, heavily-annotated 68000 assembly, plus deep documentation of how it works.

The source here is the **AMICUS Disk 9 "polite" Boing** (a Commodore-Amiga developer sample), reconstructed from Harry "Piru" Sintonen's disassembly, split into readable files and commented. It assembles with `vasm`/`vlink` into a Kickstart 1.x / OCS executable and runs under FS-UAE. See [docs/DEMO-BACKGROUND.md](docs/DEMO-BACKGROUND.md) for the full history and variant lineage.

**Fork additions.** This fork makes the demo **redistributable**: it builds a free `mathtrans.library` (AROS-derived, APL) so the staged run target — and the ADF built from it — no longer embeds Commodore/Hyperion's copyrighted library (see [`mathtrans/`](mathtrans/)), and it builds with the public **NDK 3.2** headers from [Aminet](https://aminet.net/package/dev/misc/NDK3.2) as well as NDK 1.3 (see `compat/`). The build stages an FS-UAE **directory-hard-drive run target** in `uae/` (the `dh0/` folder); [`scripts/make-adf.sh`](scripts/make-adf.sh) additionally writes a **bootable ADF** from it (it self-bootstraps [amitools](https://pypi.org/project/amitools/) into `build/pylibs` on first run via `pip --target` — system Python untouched; set `ADF_NO_BOOTSTRAP=1` to disable).

## Quick start

```sh
./scripts/build.sh               # assemble + build mathtrans.library + stage the uae/ run target
./scripts/run.sh                 # launch the demo in FS-UAE
```

Close the FS-UAE window to quit. Full details and per-OS notes: **[docs/RUNNING.md](docs/RUNNING.md)**.

**Requirements:** `vasmm68k_mot` + `vlink` (bundled with the VS Code [amiga-assembly](https://marketplace.visualstudio.com/items?itemName=prb28.amiga-assembly) extension, or on your `PATH`); **FS-UAE**; the Amiga assembler headers — the public **NDK 3.2** from [Aminet](https://aminet.net/package/dev/misc/NDK3.2) (NDK 1.3 also works); and a **Kickstart 1.3 ROM** (see the `vendor/` section below). Building the free `mathtrans.library` — the default — additionally needs the **`m68k-amiga-elf-gcc`** toolchain bundled with the VS Code [amiga-debug](https://marketplace.visualstudio.com/items?itemName=BartmanAbyss.amiga-debug) extension; without it the build falls back to a vendor `mathtrans.library`.

## `vendor/` — headers and the Kickstart ROM

`scripts/build.sh` reads the Amiga assembler headers from `vendor/include/` and the Kickstart ROM from `vendor/rom/`:

```
vendor/
  include/   Amiga assembler headers (*.i)          get the public NDK 3.2 (Aminet); NDK 1.3 also works
  rom/       kick13.rom  (Kickstart 1.3,  r34.5)    required — the demo's native OS          © Commodore / Cloanto
  rom/       kick204.rom (Kickstart 2.04, r37.175)  optional — default boot ROM when present © Commodore / Cloanto
  libs/      mathtrans.library                      not needed — a free replacement is built by default
```

- **Headers:** the Amiga **NDK 3.2** is freely available on Aminet ([`dev/misc/NDK3.2`](https://aminet.net/package/dev/misc/NDK3.2)); copy its includes into `vendor/include/`. NDK 1.3 headers also work — the committed `compat/` layer maps the NDK 1.3 include paths onto NDK 3.2's `lvo/` (see [docs/RUNNING.md](docs/RUNNING.md)).
- **Kickstart ROMs:** © Commodore / Cloanto, not redistributable — supply `vendor/rom/kick13.rom` (**required**) and optionally `kick204.rom` (preferred default boot ROM), e.g. from [Cloanto Amiga Forever](https://www.amigaforever.com/). `build.sh` reports what's missing.
- **`mathtrans.library`:** **not** needed from `vendor/`. The build compiles the free, redistributable replacement in [`mathtrans/`](mathtrans/) by default. `vendor/libs/mathtrans.library` (© Commodore-Amiga) is used only when the `m68k-amiga-elf-gcc` toolchain is unavailable — and then the disk image you ship is **not** redistributable (`MATHTRANS_LIB=vendor` also forces it).

## Layout

```
src/                 demo sources (assembled into one binary)
  boing.s              vasm entry point — INCLUDEs the five .s files below
  startup.s            Lattice C c.o startup boilerplate
  anim.s               _Boing audio per-frame + audio.device setup + sample DATA
  globe.s              _init_globe / _draw_globe — sphere math + polygon fill
  main.s               _main, the main loop, physics, palette cycling, globals
  runtime.s            Lattice C runtime + library glue stubs
  assets/
    boing.samples        the "BOING!" 8-bit PCM
    Boing.info           Workbench icon
    uae/                 FS-UAE run-target skeleton (config + startup-sequence + UAEquit)
vendor/              Amiga headers (public NDK 3.2) + Kickstart ROM (mathtrans not needed — free replacement built by default)
mathtrans/           free, redistributable mathtrans.library (AROS-derived) — source, build, license
compat/              NDK 3.2 include shim (used when vendor/include is NDK 3.2+)
scripts/             build.sh, run.sh, make-adf.sh
docs/                analysis & reference (see below)
specs/               handoff spec for a separate browser-port project
archive/             unmodified originals (Sintonen disasm + Maher's C reconstruction)
uae/                 generated FS-UAE run target (git-ignored; staged by build.sh)
build/               assembler output (git-ignored)
```

## Documentation

- **[docs/RUNNING.md](docs/RUNNING.md)** — build & run (macOS / Linux / Windows), toolchain, `vendor/`, NDK 3.2 compat, gotchas.
- **[mathtrans/README.md](mathtrans/README.md)** — the free, redistributable `mathtrans.library` (build, license, status).
- **[mathtrans/DEBUGGING.md](mathtrans/DEBUGGING.md)** — design/build notes and the investigation log for that replacement.
- **[docs/DEVIATIONS.md](docs/DEVIATIONS.md)** — the two deliberate changes this repo makes to the original AMICUS source (auto-start; KS2.0+ frame-rate fix), and how to re-verify.
- **[docs/ANIMATION-DETAILS.md](docs/ANIMATION-DETAILS.md)** — motion/timing/distances, measured from a UAE recording and reconciled with source (for recreating the animation).
- **[docs/BOING-ANALYSIS.md](docs/BOING-ANALYSIS.md)** — per-function analysis of the source.
- **[docs/AMIGA-KNOWHOW.md](docs/AMIGA-KNOWHOW.md)** — Amiga 1000 hardware/OS reference, scoped to what this demo uses.
- **[docs/DEMO-BACKGROUND.md](docs/DEMO-BACKGROUND.md)** — history, cultural context, variant lineage.
- **[docs/BOING-AMICUS(S)-VS-MAHER(C).md](docs/BOING-AMICUS%28S%29-VS-MAHER%28C%29.md)** — comparison with Jimmy Maher's C reconstruction.

## Fidelity note

`src/` stays very close to the original AMICUS disassembly (`archive/boing_original.s`), with **three deliberate deviations**: auto-start (the original starts paused); a Kickstart 2.0+ frame-rate fix (a version-gated commit block that keeps the demo at the native field rate — 50 Hz PAL / 60 Hz NTSC — on later OSes; KS1.x runs the original code path); and bringing the ball screen to the **foreground** at startup (the original called `ScreenToBack`, hiding it behind the CLI/Workbench — the authentic 1.3 "polite" behaviour, revealed by dragging the front screen down). Full details: **[docs/DEVIATIONS.md](docs/DEVIATIONS.md)**. The first is a one-value flip; the other two add code, so the build is no longer strictly byte-identical. Unmodified originals live in `archive/`. This fork's other additions — the free `mathtrans.library` ([`mathtrans/`](mathtrans/)) and the NDK 3.2 `compat/` layer — are **build-only** and do not modify `src/`.
