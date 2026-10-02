#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C # детерминированная сортировка (kallsyms.txt: sort) и сборка
TC=/home/amper/toolchain/arm-eabi-google/bin/arm-eabi-
ROOT=$(cd "$(dirname "$0")/.." && pwd); BASE=$ROOT/ref_kernel; RES=$ROOT/result
# предпос: ref_kernel submodule @d2440f70 (clone --recurse-submodules)
cd "$BASE"
for p in "$ROOT"/patches/vendor/*.patch; do patch -p1 -N -f < "$p"; done
for p in "$ROOT"/patches/local/*.patch;  do patch -p1 -N -f < "$p"; done
cp -a "$ROOT"/src/. .                       # overlay MTC-платформы (авторитетен, поверх patches)
cp config/.config .config
# kbuild ≤3.6: цели olddefconfig НЕТ (scripts/kconfig/Makefile) → silentoldconfig (non-interactive, метод r5/t9)
make ARCH=arm CROSS_COMPILE=$TC silentoldconfig
if ! diff -q .config "$ROOT"/src/config/.config >/dev/null; then echo "CONFIG DRIFT"; diff .config "$ROOT"/src/config/.config | head; exit 1; fi
# r5-parity: только vmlinux (без modules) — kernel.elf = vmlinux-blob byte-1:1 (R5/T9: 1508 CC, без .ko)
make ARCH=arm CROSS_COMPILE=$TC -j$(nproc) vmlinux 2> build.log || { tail -40 build.log; exit 1; }
mkdir -p "$RES/modules"; cp vmlinux System.map build.log "$RES"/ 2>/dev/null
${TC}strip --strip-all vmlinux -o "$RES/vmlinux.stripped"
nm vmlinux | awk '{print $3, $2}' | sort > "$RES/kallsyms.txt"
# r5-parity: только vmlinux; .ko-модули (mali/ump/rtw) — отдельно, не входят в byte-1:1 vmlinux-blob.
# Секция остаётся (будет пустая — .ko не строятся), чтобы layout $RES/modules был стабильным.
find . -name '*.ko' -exec cp {} "$RES/modules/" \; 2>/dev/null || true
bash "$ROOT"/scripts/verify_symbols.sh "$RES"
echo "BUILD OK: $RES"
