# RUNNING.md — building and running Boing under FS-UAE

How to build `src/boing.s` and watch it run on an emulated Amiga. Companion to the build-pipeline notes in the header of [`src/boing.s`](../src/boing.s).

## Quick start

```sh
./scripts/build.sh             # assemble + build mathtrans.library + stage the uae/ run target
./scripts/run.sh               # launch FS-UAE (builds first if stale)
```

`run.sh` alone is usually enough afterwards — it rebuilds if the sources changed, then launches. Close the FS-UAE window to quit.

## `vendor/` — headers and the Kickstart ROM

The build reads the Amiga assembler headers from `vendor/include/` and the Kickstart ROMs from `vendor/rom/`:

```
vendor/include/   Amiga assembler headers (*.i)            use the public NDK 3.2 (Aminet); NDK 1.3 also works
vendor/rom/       kick13.rom   (Kickstart 1.3,  r34.5)     <- required (the demo's native OS)
vendor/rom/       kick204.rom  (Kickstart 2.04, r37.175)   <- optional (becomes the default boot ROM when present)
vendor/libs/      mathtrans.library                        <- not needed (free replacement built by default; see below)
```

The Amiga **NDK 3.2** includes are freely available on Aminet ([`dev/misc/NDK3.2`](https://aminet.net/package/dev/misc/NDK3.2)); NDK 1.3 headers also work (see the `compat/` note below). The Kickstart ROMs are © Commodore / Cloanto and **not redistributable** — supply them yourself (e.g. from Cloanto Amiga Forever): **1.3 required, 2.04 optional**. `build.sh` requires the 1.3 ROM, and uses 2.04 as the default boot ROM when it's present (otherwise it falls back to 1.3); it reports exactly what's missing. **The ROMs live here** — there's no dependency on FS-UAE's own `Kickstarts/` folder.

`mathtrans.library` is **not** taken from `vendor/` by default: the build compiles a free, redistributable replacement from [`../mathtrans/`](../mathtrans/) (AROS-derived, APL) and stages *that*, so the run target (and any disk image built from it) can be shared publicly. `vendor/libs/mathtrans.library` (Commodore/Hyperion, not redistributable) is used only as a fallback when the free build is unavailable or the C toolchain is missing — and `build.sh` then warns that the result is not redistributable. Force either side with `MATHTRANS_LIB=free|vendor`. Build/design notes: [`../mathtrans/DEBUGGING.md`](../mathtrans/DEBUGGING.md).

If you can only supply **NDK 3.2+** headers, that also builds: `build.sh` detects the newer layout (`lvo/` present, `graphics/graphics_lib.i` absent) and prepends the committed `compat/` include layer, which maps the NDK 1.3 include paths used by `src/boing.s` onto NDK 3.2's `lvo/*.i` (NDK 3.2's `*_lib.i` otherwise fail on an undefined `FUNCDEF` macro). The demo only needs the `_LVO*` equates, so the mapping is complete. A genuine NDK 1.3 `vendor/include/` is used as-is, with no compat layer.

## Toolchain

`scripts/build.sh` finds `vasmm68k_mot` + `vlink` on your `PATH`, otherwise falls back to the native binaries bundled with the VS Code **amiga-assembly** extension (`prb28.amiga-assembly`):

```
~/.vscode/extensions/prb28.amiga-assembly-*/resources/bin/<darwin|linux>/{vasmm68k_mot,vlink}
```

(The copies under the extension's `dist/wasm/` are WebAssembly and won't run from a normal shell — the script uses the `resources/bin/<platform>/` native ones.) Override with `VASM=/path VLINK=/path ./scripts/build.sh`.

The free `mathtrans.library` build (`mathtrans/build.sh`) additionally needs the **m68k Amiga C toolchain** bundled with the VS Code **amiga-debug** extension (`bartmanabyss.amiga-debug`): `m68k-amiga-elf-gcc` + `vasmm68k_mot` under `bin/<platform>/opt/bin/`. Override with `M68K_GCC=/path` (and `VASM=`). If it is missing, `build.sh` falls back to `vendor/libs/mathtrans.library` and warns that the result is not redistributable.

## Build (what the script does)

```sh
# run from the repo root; INCDIR (vendor/include) + INCLUDE paths are repo-root-relative
vasmm68k_mot -m68000 -Fhunk -linedebug src/boing.s -o build/boing.o
vlink -bamigahunk -Bstatic build/boing.o -o uae/dh0/boing
```

`build.sh` then **stages the generated `uae/` run target** (git-ignored) from `src/assets/uae/` (skeleton: the `boing.fs-uae` and `boing.uae` configs, which are templates — `build.sh` substitutes the chosen default ROM filename for the `@@KICKSTART@@` token) + `vendor/` (ROMs → `uae/rom/` — `kick13.rom` always, `kick204.rom` if present) + `src/assets/{boing.samples,Boing.info}` + the ~63 KB binary at `uae/dh0/boing`. `mathtrans.library` in `uae/dh0/libs/` comes from `mathtrans/build.sh` (free/redistributable; see above). (The assemble/link recipe is mirrored in `.vscode/tasks.json`, entrypoint `src/boing.s` — note that path expects `vendor/include` to exist.)

The build produces an FS-UAE **directory hard drive** (FS-UAE mounts `uae/dh0/` as `DH0:` and `uae/dh0/libs` as `LIBS:`). To get a self-contained, bootable **ADF** floppy image, run `scripts/make-adf.sh` (default output `build/boing.adf`). It uses [amitools](https://pypi.org/project/amitools/)' `xdftool`, which it self-bootstraps into `build/pylibs` on first run (`pip --target`, so the system Python is untouched; set `ADF_NO_BOOTSTRAP=1` to disable). The ADF embeds `boing`, `boing.samples`, and the free `mathtrans.library`, and boots on Kickstart 1.3. Because KS1.3 has no `Assign` command on a bare disk, the ADF's volume is named **`LIBS`** and the library sits in its root, so `OpenLibrary("mathtrans.library")` resolves `LIBS:mathtrans.library`; the startup-sequence is then just `boing`. (The Kickstart ROM is not part of the ADF — it lives in the machine/emulator.)

## Run (FS-UAE)

`scripts/run.sh` launches FS-UAE with `uae/boing.fs-uae` — a fully self-contained config (ROM, disk, and `LIBS:` all repo-relative via `$CONFIG`). It boots an **A500 / 68000 / OCS** machine. The default Kickstart is **2.04 when that ROM is available, otherwise the required 1.3** (chosen by `build.sh` at stage time and baked into the config). The only external requirement is **FS-UAE itself**. Close the window to quit.

Why 2.04 is preferred as the default: on Kickstart 2.0+ the boot shell is windowless, so there is no separate Workbench/CLI screen at all, and the version-gated fix in [DEVIATIONS.md](DEVIATIONS.md) Deviation #2 keeps it at the full field rate. (The demo now brings its own screen to the **front** on both OSes — Deviation #3 — so Kickstart 1.3 also shows the ball immediately.) The original 1.3 environment is one flag away (next section).

Run by hand per OS:

```sh
# macOS
/Applications/FS-UAE.app/Contents/MacOS/fs-uae uae/boing.fs-uae      # or: fs-uae uae/boing.fs-uae
# Linux  (package: fs-uae)
fs-uae uae/boing.fs-uae
# Windows  (cmd / PowerShell; or use the FS-UAE Launcher GUI)
"C:\Program Files\FS-UAE\Windows\x86-64\fs-uae.exe" uae\boing.fs-uae
```

On Windows, `scripts/build.sh` / `run.sh` are bash — run them from **Git Bash** or **WSL** (or invoke `fs-uae.exe` on the staged `uae\boing.fs-uae` directly after building).

### Running on Kickstart 1.3 (the demo's native OS)

```sh
./scripts/run.sh 13     # boots uae/rom/kick13.rom instead of 2.04
```

`13` (also `1.3` / `--ks13`) is shorthand `run.sh` translates into `--kickstart_file=…/uae/rom/kick13.rom`; any other args still pass through to FS-UAE. `kick13.rom` is the required ROM, so it's always staged into `uae/rom/`. (If 2.04 isn't present it's already the default, and `13` is a no-op.)

## Run (WinUAE, Windows)

For Windows users who prefer **WinUAE**, the build also stages `uae/boing.uae` — a WinUAE counterpart to the FS-UAE config (same A500 / OCS machine, same default-ROM selection, and the same two directory hard drives). Load it via **File → Load configuration…**, or:

```bat
winuae.exe -f uae\boing.uae
```

> **Untested here.** This repo is developed on macOS (FS-UAE), so `boing.uae` is provided best-effort and hasn't been run under WinUAE. The likely sticking point is **paths**: WinUAE resolves relative paths against *its own* working directory, not the config file. If it can't find the ROM or drives, either start WinUAE with the `uae\` folder as the working directory, or edit the `kickstart_rom_file` and `uaehf0`/`uaehf1` lines in `boing.uae` to **absolute** paths (`uae\rom\kick204.rom`, `uae\dh0`, `uae\dh0\libs`). For Kickstart 1.3, point `kickstart_rom_file` at `rom\kick13.rom`. Reports of what works are welcome.

## What the bare boot drive needs (gotcha)

A bare directory hard drive only gets `SYS:` from Kickstart (1.3 or 2.x) — not `LIBS:`. Without `LIBS:`, `OpenLibrary("mathtrans.library")` fails and `boing` silently `_exit(1)`s. So `uae/boing.fs-uae` mounts `dh0/libs` **as a second drive labelled `LIBS`** (`hard_drive_1` / `hard_drive_1_label = LIBS`), which makes `LIBS:mathtrans.library` resolve — the startup-sequence is then just `sys:boing`, with no Amiga `Assign` command needed. (`mathffp.library` is **ROM-resident** on 1.3, so only `mathtrans.library` needs to be on disk.)

## Deviations from the original AMICUS source

`src/` stays close to the original disassembly with **three deliberate changes** — auto-start; a Kickstart 2.0+ frame-rate fix; and bringing the ball screen to the **foreground** (the original called `ScreenToBack`, so it sat behind the CLI/Workbench). Rationale, exact locations, and how to re-verify: **[DEVIATIONS.md](DEVIATIONS.md)**.

## Symptom → cause cheat-sheet

- **Boots to `AmigaDOS … Release 1.3` prompt, no ball** → `boing` exited during init (it no longer hides behind the CLI — Deviation #3 brings its screen to the front). Check the `LIBS` mount in `uae/boing.fs-uae` (so `mathtrans.library` loads). At startup you briefly see the globe being built before the ball starts bouncing; that is normal.
  - **Animation *speed* on Kickstart 2.0+:** without the version-gated fix in [DEVIATIONS.md](DEVIATIONS.md) Deviation #2 the demo would run at half rate; KS1.x is unaffected. (The PAL/NTSC tempo gap — PAL ≈ 0.83× NTSC — is unrelated.)
- **`Illegal instruction: 4e7b at 00FCxxxx` in the FS-UAE log** → harmless. That's the Kickstart ROM's CPU-detection probe (`MOVEC`), not your code.
- **Emulator quits instantly** → something ran `UAEquit` (`dh0/c/UAEquit`, a UAE helper that quits the emulator). The current `startup-sequence` is just `sys:boing` and does **not** call it; `boing` detaches from the CLI, so any command placed after `sys:boing` would execute immediately — don't put `UAEquit` there.
- **Want NTSC (60 Hz) instead of PAL?** FS-UAE **forces PAL** for this A500 / KS1.3 setup — its model region quickstart overrides `ntsc = 1` (whether set in the config or as `--ntsc=1`), so the emulated Agnus stays PAL (227×312, 50 Hz). Don't chase the `ntsc=1` dead end. A genuine NTSC run would need an NTSC-region model (e.g. an A1000, which needs a bootstrap/WCS ROM we don't have). For timing work, record at PAL: the demo runs at the PAL field rate (~50 Hz, one step per field); a true NTSC run would be ~17 % faster (60/50) with identical per-step constants — see [ANIMATION-DETAILS.md](ANIMATION-DETAILS.md) §1.
