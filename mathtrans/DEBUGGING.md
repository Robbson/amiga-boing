# mathtrans.library replacement — build & debugging log

Status: **RESOLVED — the replacement loads on Kickstart 1.3 and the demo
renders correctly with it** (verified visually under FS-UAE). The load failure
was caused by the auto-init **InitStruct table not initializing the library-base
fields**; see [Resolution](#resolution) below. The rest of this document is the
investigation log (kept so the dead ends are known).

## Goal

`boing` needs `mathtrans.library` (`SPSin`/`SPCos` for the sphere geometry). It
is a **disk** library (not in the Kickstart ROM), so a self-contained boot ADF
must carry a copy. The original is copyrighted (Commodore/Hyperion) and not
redistributable, so we build a **free replacement** with the same v1 API and the
same LVO layout, so a built ADF can be shared publicly. See
[`README.md`](README.md) and [`LICENSE`](LICENSE) (AROS Public License).

## Resolution

**Root cause.** `OpenLibrary("mathtrans.library", 31)` returned NULL because the
loaded library base had **`lib_Version = 0`** (and `ln_Type`/`ln_Name` also 0),
so the version-31 check was refused — even though the file loaded fine
(`IoErr = 0`) and `MakeLibrary` had run (the init function was called and the
vector table was installed).

On Kickstart 1.3, for a disk **auto-init** library the base fields are **not**
copied from the RomTag — the auto-init **`InitStruct` table must initialize
them**. The vendor library's table does exactly this
(`INITBYTE LN_TYPE,NT_LIBRARY`, `INITLONG LN_NAME,…`,
`INITWORD LIB_VERSION,0x25`, `INITWORD LIB_REVISION,1`,
`INITBYTE LIB_FLAGS,6`, `INITLONG LIB_IDSTRING,…`, `INITWORD LIB_OPENCNT,0`).
My first table was a bare `dc.b 0` (and before that, NULL), so the base kept
zeroed fields. `mathtrans_lib.s` now emits the full table, and `lib_Version`,
`ln_Name`, `ln_Type`, `lib_IdString` are set; `Tl` then shows
`mathtrans.library 37.1` and the ball renders correctly.

**Key diagnostic that pinpointed it:** a test program (run from a `sys:test`
startup-sequence) opens the library and stores the result and `IoErr` into fixed
low RAM; the debugger reads it back (`m 1000 3`). `IoErr = 0` with a NULL
result proved "file loaded, resident accepted, but init/version rejected", and
dumping the loaded base (`m c0c44e 4`) showed `lib_Version = 0`.

## What exists in the tree

- `mathtrans/mathtrans.c` — the 17 FFP functions, transcribed from AROS
  `workbench/libs/mathtrans/*.c` (freestanding C; FFP arithmetic delegated to
  the ROM-resident `mathffp.library`).
- `mathtrans/mathtrans_lib.s` — AmigaOS shared-library shell: RomTag/auto-init,
  Open/Close/Expunge, the 17 vectors (LVO −30…−126), and the `mathffp`
  register-ABI trampolines.
- `mathtrans/build.sh` — builds `mathtrans/build/mathtrans.library`.
- `scripts/build.sh` — builds and stages the replacement into
  `uae/dh0/libs/mathtrans.library`, falling back to the copyrighted vendor copy
  (with a warning) only if the free build is unavailable.
- `compat/` — unrelated NDK 3.2 include shim (see `docs/RUNNING.md`).

## Toolchain (found on this machine)

- C compiler + assembler + converter: the VS Code **amiga-debug** extension
  (`bartmanabyss.amiga-debug`):
  `~/.vscode-server/extensions/bartmanabyss.amiga-debug-*/bin/linux/`
  - `opt/bin/m68k-amiga-elf-gcc` (GCC 15.1.0), `…/m68k-amiga-elf-as`, `…-ld`
  - `vasmm68k_mot` (vasm 1.9)
  - `elf2hunk`, `exe2adf`
- Linker: `vlink` V0.16h from the **amiga-assembly** extension
  (`prb28.amiga-assembly-*/resources/bin/linux/vlink`).

Gotchas hit while setting the toolchain up:

- The bundled binaries ship **without the execute bit**; `chmod +x` is needed
  (the `mathtrans/build.sh` does `chmod +x` defensively).
- `m68k-amiga-elf-gcc` looks up its assembler as plain **`as`**, which resolves
  to the host `as` and fails (`as: unrecognized option '-mcpu=68000'`). Fix:
  put a shim dir on `PATH` containing `as -> m68k-amiga-elf-as`
  (build.sh creates `mathtrans/build/toolchain-shim/as`).
- `vlink` handles `-Felf` objects from both `vasm` and `m68k-amiga-elf-gcc`
  and links them to `-bamigahunk`. Symbols have **no leading underscore** in
  this toolchain, so C and asm names match directly.

## Facts established (independent of the load bug)

### FFP / mathffp calling convention (verified)

The AmigaOS `mathffp.library` LVOs use the natural `D0 op D1` order, but the
AROS C sources use the *swapped* convention inherited from the original FFP
library. Confirmed against the demo's own compiled code
(`globe.s:411` "D0 = (sin*band)/D5", `main.s:1563` "D0 = 192.0 - _fy",
`main.s:1551` "compare _fy : +96.0"):

| AROS C call            | AmigaOS LVO (D0 op D1)  | trampoline args |
|------------------------|-------------------------|-----------------|
| `SPAdd(a,b) = a+b`     | `SPAdd(D0,D1)=D0+D1`    | `(a, b)`        |
| `SPMul(a,b) = a*b`     | `SPMul(D0,D1)=D0*D1`    | `(a, b)`        |
| `SPSub(a,b) = b-a`     | `SPSub(D0,D1)=D0-D1`    | `(b, a)` swapped|
| `SPDiv(a,b) = b/a`     | `SPDiv(D0,D1)=D0/D1`    | `(b, a)` swapped|
| `SPCmp(a,b) = sgn(a-b)`| `SPCmp(D0,D1)=sgn(D0-D1)`| `(a, b)`       |
| `SPFloor(a) = floor(a)`| `SPFloor(D0)`           | `(a)`           |

This mapping is implemented in `mathtrans.c`; `mffp_2`/`mffp_1` in
`mathtrans_lib.s` put the first C arg in D0 and the second in D1.

`SPAdd/SPSub/SPMul/SPDiv/SPCmp/SPFloor` are the only `mathffp` functions the
implementation needs.

### Amiga library structure (from AROS `MakeLibrary`/`InitResident`)

- Vector table order = LVO order: `[Open, Close, Expunge, reserved, …user funcs…, -1]`;
  `MakeFunctions` assigns `table[0]` to LVO −6 (verified in
  `rom/exec/makefunctions.c`: `n=1` then `table[0]`).
- Auto-init struct read by `InitResident`: `{ dSize, vectors, structure, initFunc }`
  (`rom/exec/initresident.c`), called as `initFunc(D0=libBase, A0=segList, A6=SysBase)`.
- `InitStruct` table: entries `INITBYTE 0xA0 off val 0`, `INITWORD 0x90 off (word)`,
  `INITLONG 0x80 off (long bank)`; the table is terminated by a **single 0 byte**
  (`rom/exec/initstruct.c`: `while(*it!=0)`). **On KS1.3 this table must set the
  library-base fields** (`ln_Type`, `ln_Name`, `lib_Version`, `lib_Revision`,
  `lib_Flags`, `lib_IdString`, `lib_OpenCnt`) — they are not copied from the
  RomTag for a disk auto-init library (AROS's `InitResident` *does* copy them,
  which is why this is easy to miss).
- Resident field offsets: `MatchWord@0(w)`, `MatchTag@2(l)`, `EndSkip@6(l)`,
  `Flags@10(b)`, `Version@11(b)`, `Type@12(b)`, `Pri@13(b)`, `Name@14(l)`,
  `IdString@18(l)`, `Init@22(l)`; size 26.

### Resident values (ours vs the original)

The original `vendor/libs/mathtrans.library` resident reports **`rt_Version = 0x25
= 37`**, `rt_Flags = 0x81` (AUTOINIT|COLDSTART), `rt_Type = 9` (NT_LIBRARY).
The demo opens with **version 31**, so a version‑1 library was rejected — fixed
by setting `rt_Version = 37`.

### Hunk header format observed

Both `vlink` and AROS `elf2hunk` (and the Commodore-built vendor file) emit the
same shape:
`HUNK_HEADER(0x3F3)`, `table_size=0`, `numHunks`, `firstHunk=0`, `lastHunk`,
`numHunks` size words, then the hunk stream (each data hunk = `type`,`size(longs)`,
data; followed by `HUNK_RELOC32` (0x3EC), `HUNK_END` (0x3F2)). So the two extra
longs before the size table are normal, not a bug.

## What has been tried, and the result

Testing method: FS-UAE's built-in debugger (the `Robbson/fs-uae` fork supports
`--console_debugger=1 --debugger_signal=1`; `SIGUSR1` enters the debugger). The
`Tl` command lists loaded libraries. With the **vendor** library, `Tl` shows
`… mathtrans.library 37.1` while the demo runs; with **our** library it is
absent and `boing` exits (boot shell becomes responsive again). Script:
`/tmp/kilo/dbg.py`. Control test with the vendor library **passes**, so the
method is sound.

| Attempt | Result |
|---|---|
| Full library: `vasm -Felf` shell + `gcc` C → `vlink -bamigahunk -k` | not loaded |
| `rt_Version` 1 → 37 | still not loaded |
| Minimal asm-only stub library (resident + 4 std vectors + 17 stubs) | not loaded |
| `rt_Flags` 0x80 → 0x81 | not loaded |
| Provide a real `InitStruct` table (`dc.b 0`) instead of NULL | not loaded |
| **Full `InitStruct` table setting `ln_Type`/`ln_Name`/`lib_Version`/`lib_Revision`/`lib_Flags`/`lib_IdString`/`lib_OpenCnt`** | **loaded ✅** |
| Build via `elf2hunk` instead of `vlink` | not loaded |
| Assemble the shell with `vasm -Fhunk` instead of `-Felf` | not loaded |

Static inspection of our file found no defect vs the vendor:

- First hunk is `HUNK_CODE` and its data starts with `4AFC` (the RomTag at
  offset 0). ✔
- `rt_MatchTag` (offset 2) has a relocation against `.text+0` → resolves to the
  loaded resident address. ✔
- `rt_EndSkip` (offset 6) → `.text+0x1a` (end of the 26‑byte resident). ✔
- Vector table entries have relocations (21 entries at the FuncTab offsets). ✔
- `AutoInit = { dSize=34, vectors, structure, initFunc }` present. ✔

## Known testing footguns (already hit)

- **`uae/dh0/s/startup-sequence` was overwritten with `sys:test` during
  debugging and not restored**, which silently invalidated several later tests
  (the demo wasn't running at all). Restore is
  `cp src/assets/uae/dh0/s/startup-sequence uae/dh0/s/` (or rerun
  `scripts/build.sh`). *Always confirm the demo actually runs (vendor control)
  before concluding anything.*
- FS-UAE directory hard-drive writes may not appear on the host after a hard
  kill; quit cleanly (debugger `q`) if you rely on host-visible files.
- The emulator boots in a couple of seconds; a ~4 s wait before `SIGUSR1` is
  enough.
- Use the **vendor** library as a positive control for any load test.

## Remaining / follow-ups

- ✅ **Load verified** — `mathtrans.library 37.1` appears in `Tl` and `boing`
  runs.
- ✅ **Runtime verified** — the ball renders correctly under FS-UAE with the
  replacement library (visual check).
- `SetSR` (FFP condition codes) is still a no-op; the demo ignores the flags.
  Revisit only if a client needs them.
- Targets 68000 (`-m68000`), matching the demo's A500/OCS environment.

### Diagnostics that worked (reusable)

- No-DOS test program run from `sys:test`: opens the library and stores the
  result + `IoErr` into fixed low RAM (`$1000`/`$1004`), then loops; read with
  the debugger `m 1000 3`. This distinguishes "file/resident" failures
  (`IoErr != 0`) from "init/version" failures (`IoErr == 0`).
- Dump the loaded library base with `m <base> 4` (e.g. `m c0c44e 4`) and read
  `ln_Type`(+8), `ln_Name`(+10), `lib_Version`(+20).
- `Tl` lists loaded libraries; use the vendor library as a positive control.

## Reproduce

```sh
./mathtrans/build.sh                      # -> mathtrans/build/mathtrans.library
./scripts/build.sh                        # default: builds + stages the free lib
MATHTRANS_LIB=vendor ./scripts/build.sh    # vendor control (copyrighted)
REPO="$PWD" python3 /tmp/kilo/dbg.py       # boot and dump the Tl library list
```
