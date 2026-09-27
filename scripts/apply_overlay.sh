#!/bin/bash
# Idempotent overlay application (T1, post-restructure: the mtc_drivers repo ITSELF is the overlay).
#  - overlay-source = /home/amper/Coding/mtc_drivers (in-repo tree; the former
#    /home/amper/Coding/mtc_overlay container is RETIRED) — only kernel-tree content is applied:
#    drivers/, sound/, arch/, config/ (rsync-ready: every overlay file is a FULL file, not a .patch);
#  - target = the WORKING COPY of the ref tree (created ONCE; original ref is NEVER touched);
#  - re-runnable — the final md5 aggregate is stable.
set -euo pipefail
REF="${REF:-/home/amper/tmp/ida-tmp/mtc_audio/ref_kernel_rk3188}"
WORK="${WORK:-/home/amper/Coding/mtc_build/ref_kernel}"
OVL="${OVL:-/home/amper/Coding/mtc_drivers}"

[ -d "$REF" ] || { echo "FATAL: ref tree not found: $REF" >&2; exit 1; }
[ -f "$REF/Makefile" ] || { echo "FATAL: $REF looks not like a kernel tree" >&2; exit 1; }

if [ ! -e "$WORK/Makefile" ]; then
	echo "[apply_overlay] creating working copy $WORK from $REF (excl .git) ..."
	mkdir -p "$(dirname "$WORK")"
	rsync -a --exclude=.git "$REF/" "$WORK/"
else
	echo "[apply_overlay] working copy exists: $WORK"
fi

# kernel-tree content only (docs/scripts stay in the repo, not in the kernel copy)
for d in drivers sound arch config; do
	[ -d "$OVL/$d" ] || continue
	mkdir -p "$WORK/$d"
	rsync -a --checksum "$OVL/$d/" "$WORK/$d/"
done

# idempotence aggregate (must be identical on every re-run)
echo -n "[apply_overlay] applied-files md5: "
(cd "$WORK" && find drivers/misc/mtc drivers/misc/Makefile drivers/misc/Kconfig \
	sound/soc/rk29 arch/arm/mach-rk3188 config \
	-type f ! -name README.md 2>/dev/null | LC_ALL=C sort | xargs md5sum) | md5sum
echo "[apply_overlay] OK: overlay ($OVL) applied to $WORK"
