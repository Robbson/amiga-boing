# mathtrans.library — free, redistributable replacement

The Boing demo needs `mathtrans.library` at runtime (`SPSin`/`SPCos` for the
sphere geometry). On a classic Amiga that library is a **disk** library — it is
not in the Kickstart ROM — so a self-contained boot disk (the FS-UAE run target
in `uae/dh0/`, or an ADF made from it) has to carry a copy.

Commodore/Hyperion's `mathtrans.library` is copyrighted and **not
redistributable**, which means a disk embedding it cannot be shared publicly.
This directory builds a **free replacement** with the same v1 API and the same
LVO layout, so a Boing disk built from this repo *can* be shared.

## What this is

* `mathtrans.c` — the FFP transcendental algorithms (SPAtan, SPSin, SPCos,
  … , SPLog10), transcribed from the AROS project's `mathtrans.library`
  sources (`workbench/libs/mathtrans/*.c`). Freestanding C: all FFP arithmetic
  is delegated to the ROM-resident AmigaOS `mathffp.library`.
* `mathtrans_lib.s` — the AmigaOS shared-library shell: RomTag/auto-init,
  standard Open/Close/Expunge vectors, the 17 mathtrans vectors
  (LVO −30 … −126), and the `mathffp.library` call trampolines (register ABI).

The demo only ever calls `SPSin`/`SPCos`; the full vector set is implemented so
the library is a genuine drop-in, not a stub.

## License / attribution

The algorithms are derived from AROS (Copyright © 1995–2006, The AROS
Development Team), which is distributed under the **AROS Public License (APL)
v1.1** (see [`LICENSE`](LICENSE)). This directory is therefore a derivative
work under the same license; the APL permits redistribution of the compiled
library provided the license and notices accompany it. `mathtrans_lib.s`
(AmigaOS glue) is original to this repository, under the same license.

## Building

```sh
./mathtrans/build.sh          # -> mathtrans/build/mathtrans.library
```

The build uses the m68k Amiga C toolchain bundled with the VS Code
**amiga-debug** extension (`bartmanabyss.amiga-debug`: `m68k-amiga-elf-gcc`,
`vasmm68k_mot`) plus `vlink` from **amiga-assembly** (`prb28.amiga-assembly`).
Override with `M68K_GCC=/path VASM=/path VLINK=/path`.

`scripts/build.sh` builds this library automatically and stages it into the run
target as `uae/dh0/libs/mathtrans.library` (falling back to the copyrighted
`vendor/libs/mathtrans.library` only if this build is unavailable — in which
case the distributed disk is **not** redistributable). See
[docs/RUNNING.md](../docs/RUNNING.md).

## Notes / limitations

* **Status: working.** The library loads on Kickstart 1.3 and the demo renders
  correctly with it. A key subtlety: the auto-init **InitStruct table must
  initialize the library-base fields** (`ln_Type`, `ln_Name`, `lib_Version`, …)
  — on KS1.3 these are not copied from the RomTag for a disk auto-init library,
  so the earlier `dc.b 0` table left `lib_Version=0` and `OpenLibrary(name,31)`
  was refused. See [`DEBUGGING.md`](DEBUGGING.md) for the full investigation.
* `SetSR` (setting the caller's FFP condition codes) is a no-op: the Boing demo
  ignores the flags, and skipping the privileged SR write keeps the library
  safe to call from user mode.
* Targets 68000 (`-m68000`), matching the demo's A500/OCS environment.
* The numerator/denominator order of `mathffp.library`'s `SPDiv`/`SPSub`/`SPCmp`
  is quirky; the correct mapping was taken from the demo's own compiled code and
  is documented in the header of `mathtrans.c`.
