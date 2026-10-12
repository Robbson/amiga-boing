#!/usr/bin/env bash
# Build a free, redistributable mathtrans.library (AROS-derived) for the Boing
# demo. Output: mathtrans/build/mathtrans.library  (Amiga hunk library).
#
# Toolchain: m68k-amiga-elf-gcc + vasmm68k_mot from the VS Code "amiga-debug"
# extension (bartmanabyss.amiga-debug) and vlink from "amiga-assembly"
# (prb28.amiga-assembly). Override with M68K_GCC=/path VASM=/path VLINK=/path.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO"
OUT="mathtrans/build"
mkdir -p "$OUT"

case "$(uname -s)" in
  Darwin) PLAT=darwin ;;
  Linux)  PLAT=linux  ;;
  *)      PLAT=linux  ;;
esac

newest() { ls -d "$@" 2>/dev/null | sort -V | tail -1 || true; }

# amiga-debug bundles the m68k-amiga-elf C toolchain + vasmm68k_mot
BAB="$(newest "$HOME"/.vscode*/extensions/bartmanabyss.amiga-debug-*/bin/"$PLAT")"
# amiga-assembly bundles vlink
P28="$(newest "$HOME"/.vscode*/extensions/prb28.amiga-assembly-*/resources/bin/"$PLAT")"

GCC="${M68K_GCC:-$(newest "${BAB:-/nonexistent}/opt/bin/m68k-amiga-elf-gcc")}"
GCC="${GCC:-$(command -v m68k-amiga-elf-gcc || true)}"
VASM="${VASM:-$(newest "${BAB:-/nonexistent}/vasmm68k_mot")}"
VASM="${VASM:-$(command -v vasmm68k_mot || true)}"
VLINK="${VLINK:-$(newest "${P28:-/nonexistent}/vlink")}"
VLINK="${VLINK:-$(command -v vlink || true)}"

for t in "$GCC" "$VASM" "$VLINK"; do
  if [ -z "$t" ] || [ ! -f "$t" ]; then
    echo "error: toolchain not found (${t:-<empty>})." >&2
    echo "  Install the VS Code 'amiga-debug' + 'amiga-assembly' extensions," >&2
    echo "  or set M68K_GCC= VASM= VLINK=." >&2
    exit 1
  fi
done

# The bundled binaries are shipped without the execute bit.
chmod +x "$GCC" "$VASM" "$VLINK" 2>/dev/null || true
GCC_DIR="$(cd "$(dirname "$GCC")" && pwd)"
GCC_OPT="$(cd "$GCC_DIR/.." && pwd)"
chmod +x "$GCC_DIR"/m68k-amiga-elf-* 2>/dev/null || true
chmod -R u+rwx "$GCC_OPT/libexec" 2>/dev/null || true

# gcc looks up its assembler as plain "as"; expose the target assembler on PATH.
SHIM="$OUT/toolchain-shim"
mkdir -p "$SHIM"
ln -sf "$GCC_DIR/m68k-amiga-elf-as" "$SHIM/as"
export PATH="$SHIM:$GCC_DIR:$PATH"

echo "==> compiling mathtrans.c"
"$GCC" -m68000 -O2 -ffreestanding -fno-builtin -nostdlib \
       -c mathtrans/mathtrans.c -o "$OUT/mathtrans_c.o"

echo "==> assembling mathtrans_lib.s"
"$VASM" -m68000 -Felf mathtrans/mathtrans_lib.s -o "$OUT/mathtrans_lib.o"

echo "==> linking $OUT/mathtrans.library"
"$VLINK" -bamigahunk -Bstatic -k \
         "$OUT/mathtrans_lib.o" "$OUT/mathtrans_c.o" -o "$OUT/mathtrans.library"

echo "==> done: $(wc -c < "$OUT/mathtrans.library") bytes at $OUT/mathtrans.library"
