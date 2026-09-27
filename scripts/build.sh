#!/bin/bash
# Full vmlinux build of the OVERLAY working copy (T5 pipeline step 2).
# Baseline (step 1, clean ref without overlay) uses the same script with OVL empty.
# Prereqs: scripts/apply_overlay.sh run first; arm-eabi GCC 4.6 toolchain ready
# (T5 blocker while gcc is still building — binutils-only dir => early exit).
set -uo pipefail
TC="${TC:-/home/amper/toolchain/arm-eabi/bin/arm-eabi-}"
WORK="${WORK:-/home/amper/Coding/mtc_build/ref_kernel}"
OVL="${OVL:-$(cd "$(dirname "$0")/.." && pwd)}"
LOG="${LOG:-$(dirname "$WORK")/build_overlay.log}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

if [ ! -x "${TC}gcc" ]; then
	echo "FATAL: ${TC}gcc not executable — T5 toolchain blocker (GCC 4.6 build in progress, researcher)." >&2
	exit 2
fi
if [ ! -e "$WORK/Makefile" ]; then
	echo "FATAL: working copy $WORK missing — run scripts/apply_overlay.sh first." >&2
	exit 1
fi

# ccache if present (PLAN R6: обязательна для итераций)
CCACHE_BIN="$(command -v ccache 2>/dev/null || true)"
[ -n "$CCACHE_BIN" ] && { export PATH="$(dirname "$CCACHE_BIN"):$PATH"; echo "[build] ccache enabled"; }

if [ -f "$OVL/config/mtc_defconfig" ]; then
	cp "$OVL/config/mtc_defconfig" "$WORK/.config"
fi

echo "[build] olddefconfig (log $LOG)"
make -C "$WORK" ARCH=arm CROSS_COMPILE="$TC" olddefconfig >>"$LOG" 2>&1 \
	|| { echo "FATAL: olddefconfig failed, see $LOG" >&2; exit 1; }

echo "[build] vmlinux -j$JOBS (time-boxed: full build ~30+ min, log $LOG)"
if make -C "$WORK" ARCH=arm CROSS_COMPILE="$TC" -j"$JOBS" vmlinux >>"$LOG" 2>&1; then
	echo "[build] OK: $WORK/vmlinux — next: scripts/verify_symbols.sh $WORK/vmlinux"
else
	echo "FATAL: build failed, see $LOG" >&2
	exit 1
fi
