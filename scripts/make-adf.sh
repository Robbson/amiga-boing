#!/usr/bin/env bash
# Build a self-contained, bootable Boing ADF (880K DD floppy) from the staged
# uae/dh0 run target. Boots on Kickstart 1.3 (and later).
#
# Uses amitools' `xdftool`. If amitools is not available, it is installed
# locally into build/pylibs (via `pip --target`, which does NOT touch the system
# Python) unless ADF_NO_BOOTSTRAP=1 is set.
#
# Usage: scripts/make-adf.sh [output.adf]     (default: build/boing.adf)
#
# Notes:
#  - The demo opens "mathtrans.library" via LIBS:. Kickstart 1.3 has no `Assign`
#    command on a bare disk, so we name the disk volume **LIBS** and put the
#    library in the root dir -> exec resolves LIBS:mathtrans.library. The
#    startup-sequence then just runs `boing`.
#  - The Kickstart ROM is not part of the ADF (it lives in the machine/emulator).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO"

ADF="${1:-$REPO/build/boing.adf}"
LOCAL_LIBS="$REPO/build/pylibs"

have_xdftool_bin() { command -v xdftool >/dev/null 2>&1; }
have_module() { python3 -c "import amitools.tools.xdftool" >/dev/null 2>&1; }

# Make a previously bootstrapped copy importable, if present.
if [ -d "$LOCAL_LIBS" ]; then
  export PYTHONPATH="$LOCAL_LIBS${PYTHONPATH:+:$PYTHONPATH}"
fi

# Bootstrap amitools locally if we have neither the binary nor the module.
if ! have_xdftool_bin && ! have_module && [ "${ADF_NO_BOOTSTRAP:-0}" != 1 ]; then
  echo "==> amitools not found; installing into $LOCAL_LIBS (pip --target, system Python untouched)"
  python3 -m pip install --target "$LOCAL_LIBS" --break-system-packages --quiet amitools || true
  export PYTHONPATH="$LOCAL_LIBS${PYTHONPATH:+:$PYTHONPATH}"
fi

if ! have_xdftool_bin && ! have_module; then
  echo "error: amitools (xdftool) unavailable." >&2
  echo "  Install it, e.g.:" >&2
  echo "    python3 -m pip install --target build/pylibs --break-system-packages amitools" >&2
  echo "  (or pip-install amitools into your environment; or set ADF_NO_BOOTSTRAP=1 to skip this)" >&2
  exit 1
fi

xdf() {
  if have_xdftool_bin; then xdftool "$@"
  else python3 -m amitools.tools.xdftool "$@"; fi
}

# Ensure the run target is built (boing + the free mathtrans.library).
if [ ! -f uae/dh0/boing ] || [ ! -f uae/dh0/libs/mathtrans.library ]; then
  echo "==> run target missing; building first"
  ./scripts/build.sh
fi

mkdir -p "$(dirname "$ADF")"
rm -f "$ADF"

SS="$(mktemp)"
trap 'rm -f "$SS"' EXIT
printf 'boing\n' > "$SS"

echo "==> creating $ADF"
xdf "$ADF" create
xdf "$ADF" format "LIBS"
xdf "$ADF" boot install
xdf "$ADF" write   uae/dh0/boing                     boing
xdf "$ADF" protect boing rwed
xdf "$ADF" write   uae/dh0/libs/mathtrans.library    mathtrans.library
xdf "$ADF" write   uae/dh0/boing.samples             boing.samples
xdf "$ADF" makedir s
xdf "$ADF" write   "$SS"                             s/startup-sequence

echo "==> done: $ADF ($(wc -c < "$ADF") bytes)"
