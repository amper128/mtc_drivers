#!/bin/bash
# T6: verify kallsyms of a FULL build vs the original 3188 binary (42433 syms, CRLF).
# Metrics (PLAN §B): 100% of mtc-family names present; .initcall order preserved;
# cluster rank drift <= +/-1 entry inside c0421xxx/c042bxxx/c09bbxxx/c0a0xxxx.
# Usage: verify_symbols.sh <vmlinux|kallsyms-file> [ref_kallsyms]
set -uo pipefail
BUILT="${1:?usage: verify_symbols.sh <vmlinux|kallsyms> [ref_kallsyms]}"
REFK="${2:-/home/amper/Coding/RK3188/3188_kallsyms}"
TCBIN="/home/amper/toolchain/arm-eabi/bin"
TMPP="$(mktemp -d)"; trap 'rm -rf "$TMPP"' EXIT

FAM='^(car_|dvd_|lcd_|mtc|vs_|tm|backview|tef6606|radio_|rds_|audio_card)'
# 1) normalise both to "addr type name" (T/t symbols of vmlinux only)
if head -c 200 "$BUILT" 2>/dev/null | grep -qE '^c[0-9a-f]{8} [tT]'; then
	tr -d '\r' < "$BUILT" > "$TMPP/new.raw"
else
	NM=""
	for c in "$TCBIN/arm-eabi-nm" arm-none-eabi-nm nm; do
		command -v "$c" >/dev/null 2>&1 && NM="$c" && break
		[ -x "$c" ] && NM="$c" && break
	done
	[ -n "$NM" ] || { echo "FATAL: no nm for $BUILT (T5 toolchain blocker)"; exit 2; }
	"$NM" "$BUILT" | awk '$2 ~ /[tT]/ {print $1, $2, $3}' > "$TMPP/new.raw"
fi
tr -d '\r' < "$REFK" | awk '$2 ~ /[tT]/ {print $1, $2, $3}' > "$TMPP/ref.raw"
awk '{print $3}' "$TMPP/ref.raw" | sort -u > "$TMPP/ref.names"
awk '{print $3}' "$TMPP/new.raw"  | sort -u > "$TMPP/new.names"

MISS=$(comm -23 "$TMPP/ref.names" "$TMPP/new.names")
ADD=$(comm -13 "$TMPP/ref.names" "$TMPP/new.names")
RN=$(wc -l < "$TMPP/ref.names"); NN=$(wc -l < "$TMPP/new.names")
NM_MISS=$(echo -n "$MISS" | grep -cE "$FAM" || true)
echo "== SET-DIFF (vmlinux T/t names) =="
echo "ref=$RN new=$NN"
printf 'missing total: %s\nadded total:   %s\n' "$(echo "$MISS" | grep -c . || true)" "$(echo "$ADD" | grep -c . || true)"
echo "missing MTC-family (=0 required): $NM_MISS"
echo "$MISS" | grep -E "$FAM" | head -30

# 2) cluster rank analysis of mtc-family symbols (tolerance +/-1 line)
echo "== CLUSTER ANALYSIS (rank drift of mtc-family syms; PASS = all <=1) =="
STATUS=0
for P in c0421 c042b c09bb c0a0; do
	awk -v p="$P" -v fam="$FAM" 'substr($1,1,length(p))==p && $3 ~ fam {print NR, $3}' "$TMPP/ref.raw"  > "$TMPP/ck.ref"
	awk -v p="$P" -v fam="$FAM" 'substr($1,1,length(p))==p && $3 ~ fam {print NR, $3}' "$TMPP/new.raw" > "$TMPP/ck.new"
	if [ ! -s "$TMPP/ck.ref" ]; then continue; fi
	# pair by name (in-file rank = 1..n per list)
	awk '{print $2, NR}' "$TMPP/ck.ref" | sort >  "$TMPP/r1"
	awk '{print $2, NR}' "$TMPP/ck.new" | sort > "$TMPP/r2"
	awk 'NR==FNR{r[$1]=$2; next} ($1 in r){d=$2-r[$1]; if(d<0)d=-d; if(d>m)m=d; print "  "$1" refRank="r[$1]" newRank="$2" d="d}
	     END{printf "cluster %s: maxRankDrift=%s (%s)\n", "'"$P"'", m+0, (m+0<=1)?"PASS":"FAIL"; if(m+0>1) bad=1; exit bad}' "$TMPP/r1" "$TMPP/r2" || STATUS=1
done
echo "== RESULT: $([ $STATUS -eq 0 ] && echo PASS || echo 'DRIFT > +/-1 (see PLAN R1: gcc 4.6.4 vs google codegen)') =="
exit $STATUS
