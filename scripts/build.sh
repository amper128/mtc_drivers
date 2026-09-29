#!/bin/bash
# T5: build BASE+overlay with the EXACT binary toolchain:
#   arm-eabi-google GCC 4.6.x (aegcc) = /home/amper/toolchain/arm-eabi-google/bin/arm-eabi-
# Pipeline: mtc_defconfig -> olddefconfig -> SMOKE (drivers/misc/mtc/ + board-mtc.o, synchronous)
#           -> FULL build only when FULL=1 (setsid nohup make -j8 vmlinux, log $MTCBUILD/build1.log,
#              left running; poll: tail -f $MTCBUILD/build1.log).
# API errors in reconstructed sources are fixed on OUR side (mtc_drivers), never in the SDK tree.
set -uo pipefail
TC="${TC:-/home/amper/toolchain/arm-eabi-google/bin/arm-eabi-}"
BASE="${BASE:-/home/amper/Coding/mtc_build/ref_kernel}"
OVL="${OVL:-$(cd "$(dirname "$0")/.." && pwd)}"
MTCBUILD="$(dirname "$BASE")"
SMOKE_LOG="$MTCBUILD/smoke1.log"
BUILD_LOG="$MTCBUILD/build1.log"
JOBS="${JOBS:-8}"

[ -x "${TC}gcc" ] || { echo "FATAL: ${TC}gcc not executable" >&2; exit 2; }
[ -f "$BASE/Makefile" ] || { echo "FATAL: $BASE missing — run scripts/apply_overlay.sh first" >&2; exit 1; }
echo "[build] toolchain: $(${TC}gcc --version | head -1)"

# defconfig -> arch/arm/configs + .config
cp -f "$OVL/config/mtc_defconfig" "$BASE/arch/arm/configs/mtc_defconfig"
cp -f "$OVL/config/mtc_defconfig" "$BASE/.config"
echo "[build] config: mtc_defconfig -> .config; oldconfig (yes '' => NEW symbols get defaults; kernel 3.0 silentoldconfig prompts interactively) -> $SMOKE_LOG"
make -C "$BASE" ARCH=arm CROSS_COMPILE="$TC" oldconfig < <(yes "") >"$SMOKE_LOG" 2>&1 \
	|| { echo "FATAL: oldconfig failed — tail of $SMOKE_LOG:"; tail -25 "$SMOKE_LOG"; exit 1; }

# --- SMOKE (synchronous, fast): our side first ---
# NOTE: in kernel 3.0 "make <subdir>" is NOT a descend target ("Nothing to be done") -> build the
# objects listed in drivers/misc/mtc/Makefile explicitly (audio_card_glue.o + mtcGetSetVolume.o added T5-r5: between vs.o and radio_tef6606.o per binaRE c0833xxx order).
MTC_OBJS="$(grep -oE '[A-Za-z0-9_]+\.o' "$BASE/drivers/misc/mtc/Makefile" | sed "s|^|$BASE/drivers/misc/mtc/|")"
echo "[build] SMOKE 1/2: make $MTC_OBJS -> $SMOKE_LOG"
make -C "$BASE" ARCH=arm CROSS_COMPILE="$TC" $MTC_OBJS >>"$SMOKE_LOG" 2>&1
s1=$?
echo "[build] SMOKE 2/2: make arch/arm/mach-rk3188/board-mtc.o -> $SMOKE_LOG"
make -C "$BASE" ARCH=arm CROSS_COMPILE="$TC" arch/arm/mach-rk3188/board-mtc.o >>"$SMOKE_LOG" 2>&1
s2=$?
if [ $s1 -ne 0 ] || [ $s2 -ne 0 ]; then
	echo "SMOKE FAILED (mtc_dir=$s1 board_mtc=$s2) — see $SMOKE_LOG; fix OUR side (mtc_drivers), SDK tree stays untouched"
	exit 1
fi
echo "[build] SMOKE OK: $(ls "$BASE/drivers/misc/mtc/"*.o 2>/dev/null | wc -l) mtc objects:"
ls -la "$BASE/drivers/misc/mtc/"*.o "$BASE/arch/arm/mach-rk3188/board-mtc.o" 2>/dev/null | awk '{print "   ", $5, $9}'

[ "${FULL:-0}" = "1" ] || { echo "[build] stopped after green smoke (rerun with FULL=1 for the background full build)"; exit 0; }

# --- FULL build: detached background, log build1.log ---
: > "$BUILD_LOG"
setsid nohup make -C "$BASE" ARCH=arm CROSS_COMPILE="$TC" -j"$JOBS" vmlinux >>"$BUILD_LOG" 2>&1 < /dev/null &
echo $! > "$MTCBUILD/build1.pid"
echo "[build] FULL build started: pid $(cat "$MTCBUILD/build1.pid"), -j$JOBS vmlinux, log $BUILD_LOG (~30+ min; poll: tail -f $BUILD_LOG)"
