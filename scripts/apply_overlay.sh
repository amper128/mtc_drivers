#!/bin/bash
# T5: apply the mtc overlay onto the BASE working copy.
#   overlay = /home/amper/Coding/mtc_drivers (in-repo tree; the repo ITSELF is the overlay)
#   base    = /home/amper/Coding/mtc_build/ref_kernel — created ONCE by rsync of the exact SDK kernel:
#             rsync -a '/server/FTP/[tmp]/android/3188/mnt/rk3188_rk3066_r-box_android4.4.2_sdk/kernel/' ref_kernel/
#             (mtc_build/base_copy.log; 84686 files, apparent-bytes verified vs source).
#             The SDK lineage is never modified EXCEPT by the patches applied below.
# Idempotent: full-file rsync --checksum (content-stable) + patch --forward (already applied => skipped).
set -uo pipefail
OVL="${OVL:-/home/amper/Coding/mtc_drivers}"
BASE="${BASE:-/home/amper/Coding/mtc_build/ref_kernel}"

[ -f "$BASE/Makefile" ] || { echo "FATAL: base kernel tree missing: $BASE — run the T5 base rsync first" >&2; exit 1; }
[ -d "$OVL/drivers/misc/mtc" ] || { echo "FATAL: overlay missing: $OVL" >&2; exit 1; }
echo "[apply_overlay] overlay=$OVL base=$BASE"

# --- 1) full files (rsync-ready); README.md / *.patch never go into the kernel tree ---
mkdir -p "$BASE/drivers/misc/mtc" "$BASE/sound/soc/rk29" "$BASE/arch/arm/configs"
rsync -a --checksum --exclude='README.md' --exclude='*.patch' "$OVL/drivers/misc/mtc/" "$BASE/drivers/misc/mtc/"
echo "[apply_overlay] drivers/misc/mtc: $(find "$OVL/drivers/misc/mtc" -type f ! -name README.md | wc -l) files (9 .c + Makefile + Kconfig + headers)"

# sound/soc/rk29 — T3 PLACEHOLDER: only README.md yet => skip, no panic
if find "$OVL/sound/soc/rk29" -type f ! -name 'README.md' ! -name '*.patch' | grep -q .; then
	rsync -a --checksum --exclude='README.md' --exclude='*.patch' "$OVL/sound/soc/rk29/" "$BASE/sound/soc/rk29/"
	echo "[apply_overlay] sound/soc/rk29: files copied"
else
	echo "[apply_overlay] sound/soc/rk29: SKIPPED — T3 not delivered yet (README placeholder only, comment per T5 spec)"
fi

# arch/arm/mach-rk3188: board-mtc.c (+ future full files); patches applied below
rsync -a --checksum --exclude='README.md' --exclude='*.patch' "$OVL/arch/arm/mach-rk3188/" "$BASE/arch/arm/mach-rk3188/"
echo "[apply_overlay] arch/arm/mach-rk3188: board-mtc.c in place ($(stat -c%s "$BASE/arch/arm/mach-rk3188/board-mtc.c") bytes)"

# defconfig -> arch/arm/configs/
cp -f "$OVL/config/mtc_defconfig" "$BASE/arch/arm/configs/mtc_defconfig"
echo "[apply_overlay] arch/arm/configs/mtc_defconfig updated ($(wc -l < "$OVL/config/mtc_defconfig") lines)"

# --- 2) patches: patch -p1 from the base root; --forward => already applied skipped (idempotent) ---
apply_patch() { # $1 = overlay patch path, $2 = label
	local f="$1" label="$2" out="/tmp/mtc_patch_$(basename "$1").log"
	[ -f "$f" ] || { echo "[apply_overlay] PATCH SKIP ($label): $(basename "$f") not delivered yet"; return 0; }
	if (cd "$BASE" && patch -p1 --forward < "$f") >"$out" 2>&1; then
		echo "[apply_overlay] PATCH OK ($label)"
	elif (cd "$BASE" && patch -p1 -R --dry-run < "$f") >/dev/null 2>&1; then
		echo "[apply_overlay] PATCH SKIP ($label): already applied (idempotent)"
	else
		echo "FATAL: patch failed ($label): $f"; cat "$out" >&2; exit 1
	fi
}
# drivers/misc registration (NEW in T5: full Makefile/Kconfig from the retired mtc_overlay container did not survive the restructure; small end-appending patches instead)
apply_patch "$OVL/drivers/misc/Makefile.patch" "drivers/misc/Makefile (+obj mtc/)"
apply_patch "$OVL/drivers/misc/Kconfig.patch"  "drivers/misc/Kconfig (+source mtc/)"
# T4 (delivered): board-mtc
apply_patch "$OVL/arch/arm/mach-rk3188/Kconfig.patch"  "mach-rk3188/Kconfig (T4: MACH_RK3188_MTC)"
apply_patch "$OVL/arch/arm/mach-rk3188/Makefile.patch" "mach-rk3188/Makefile (T4: board-mtc.o)"
# T5 r22: kernel BUILD-INFRA script compat — host perl 5.44 vs kernel 3.0: timeconst.pl defined(@array) is
# fatal on perl >= 5.42 (full-file rsync does not cover in-tree scripts; patch channel instead, idempotent).
apply_patch "$OVL/patches/timeconst.pl.patch" "kernel/timeconst.pl (T5 r22: host perl 5.44 compat — build-infra)"
# T3 (PENDING): sound/soc/rk29/{Kconfig,Makefile}.patch — skipped with comment until delivered:
apply_patch "$OVL/sound/soc/rk29/Kconfig.patch"  "sound/soc/rk29/Kconfig (T3)"
apply_patch "$OVL/sound/soc/rk29/Makefile.patch" "sound/soc/rk29/Makefile (T3)"
# top-level sound/soc/{Makefile,Kconfig}: patched ONLY if T3's patches so require — none delivered yet:
[ -f "$OVL/sound/soc/Makefile.patch" ] && apply_patch "$OVL/sound/soc/Makefile.patch" "sound/soc/Makefile (T3)"
[ -f "$OVL/sound/soc/Kconfig.patch" ]  && apply_patch "$OVL/sound/soc/Kconfig.patch"  "sound/soc/Kconfig (T3)"

# remove patch artifacts
find "$BASE/drivers/misc/mtc" "$BASE/drivers/misc" "$BASE/sound/soc/rk29" "$BASE/arch/arm/mach-rk3188" \
	-maxdepth 2 \( -name '*.orig' -o -name '*.rej' \) 2>/dev/null | xargs -r rm -f
echo "[apply_overlay] OK: overlay ($OVL) applied to $BASE"
