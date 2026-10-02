#!/usr/bin/env bash; set -euo pipefail
# verify_symbols.sh <result_dir> — судья MTC-kernel vs 3188_kallsyms (r5 эталон)
export LC_ALL=C # детерминированная сортировка для comm/join
RES=${1:?usage: verify_symbols.sh <result_dir>}
REF=/home/amper/Coding/RK3188/3188_kallsyms
WORK=$(mktemp -d); trap 'rm -rf "$WORK"' EXIT

[ -f "$REF" ] || { echo "FATAL: нет референса $REF"; exit 2; }
[ -f "$RES/kallsyms.txt" ] || { echo "FATAL: нет $RES/kallsyms.txt"; exit 2; }

# --- reference: unique T/t (name<tab>addr) и все имена
tr -d '\r' < "$REF" | awk '$2=="T"||$2=="t"{print $3"\t"$1}' | sort -u > "$WORK/ref_tt.tsv"
tr -d '\r' < "$REF" | awk 'NF>=3{print $3}' | sort -u > "$WORK/ref_all.txt"

# --- result (kallsyms.txt: "name type", без адресов — из build.sh)
awk '$2=="T"||$2=="t"{print $1}' "$RES/kallsyms.txt" | sort -u > "$WORK/res_tt.txt"
awk '{print $1}' "$RES/kallsyms.txt" | sort -u > "$WORK/res_all.txt"

cut -f1 "$WORK/ref_tt.tsv" > "$WORK/ref_tt_names.txt"

COMMON=$(comm -12 "$WORK/ref_tt_names.txt" "$WORK/res_tt.txt" | wc -l)
MISSING=$(comm -23 "$WORK/ref_tt_names.txt" "$WORK/res_tt.txt" | wc -l)
comm -23 "$WORK/ref_tt_names.txt" "$WORK/res_tt.txt" > "$WORK/missing.txt"
ADDED=$(comm -13 "$WORK/ref_all.txt" "$WORK/res_all.txt" | wc -l)

# ADDED-by-type: добавленные имена (не в референсе) сгруппированы по типу из kallsyms.txt
awk 'NR==FNR{a[$0]; next} !($1 in a){print $2}' "$WORK/ref_all.txt" "$RES/kallsyms.txt" \
  | sort | uniq -c | sort -rn > "$WORK/added_by_type.txt"

# --- ADDR_MATCH % (T/t): адреса из nm -n result/vmlinux vs референс (по имени)
ADDR_MATCH="n/a"; ADDR_N=0; ADDR_EQ=0
if [ -f "$RES/vmlinux" ]; then
  if nm -n "$RES/vmlinux" 2>/dev/null | awk '$2=="T"||$2=="t"{print $3"\t"$1}' | sort -u > "$WORK/res_nm.tsv"; then
    # дубли имён (модульные $a/$d) → минимальный адрес на имя (join по уникальному map'у)
    sort -t $'\t' -k1,1 -k2,2 "$WORK/ref_tt.tsv"  | awk -F'\t' '!seen[$1]++' > "$WORK/ref_min.tsv"
    sort -t $'\t' -k1,1 -k2,2 "$WORK/res_nm.tsv" | awk -F'\t' '!seen[$1]++' > "$WORK/res_min.tsv"
    # точный подсчёт совпадений адресов (имя общее, min_addr_ref == min_addr_res)
    read -r ADDR_EQ ADDR_N < <(join -t $'\t' "$WORK/ref_min.tsv" "$WORK/res_min.tsv" \
      | awk -F'\t' 'BEGIN{eq=0;n=0} {n++; if($2==$3) eq++} END{print eq, n+0}')
    if [ "${ADDR_N:-0}" -gt 0 ]; then ADDR_MATCH=$(awk -v e="$ADDR_EQ" -v n="$ADDR_N" 'BEGIN{printf "%.2f", 100*e/n}'); else ADDR_MATCH="n/a"; fi
  fi
fi

# --- MTC-core check
MTC_CORE="car_init backview_init tef6606_init mtc_getTouchKey cat66121_hdmi_init board_clock_init"
MTC_OK=1; mtc_lines=""
for s in $MTC_CORE; do
  if awk -v S="$s" '$1==S{found=1} END{exit !found}' "$RES/kallsyms.txt"; then
    mtc_lines="${mtc_lines}- [OK] ${s}
"
  else
    mtc_lines="${mtc_lines}- [MISSING] ${s}
"; MTC_OK=0
  fi
done

# --- strings: Trk3188 в vmlinux
TRK=$(strings "$RES/vmlinux" 2>/dev/null | grep -c Trk3188 || true)

# --- size: vmlinux.stripped vs r5 (8303780) и target (8232996)
SIZE="n/a"; DIFF_R5="n/a"; DIFF_TARGET="n/a"
if [ -f "$RES/vmlinux.stripped" ]; then
  SIZE=$(stat -c%s "$RES/vmlinux.stripped")
  DIFF_R5=$((SIZE - 8303780))
  DIFF_TARGET=$((SIZE - 8232996))
fi

# --- отчёт
{
echo "# verify_report.md — MTC-kernel symbol verification (S2b/verify_symbols.sh)"
echo
echo "## Судья (reference)"
echo "- ref: \`$REF\` (через \`tr -d '\\r'\`), строк: $(wc -l < "$REF"), unique T/t: $(wc -l < "$WORK/ref_tt.tsv")"
echo
echo "## Метрики vs result/kallsyms.txt"
echo "| метрика | значение |"
echo "|---|---|"
echo "| COMMON (T/t) | $COMMON |"
echo "| MISSING (T/t ref → result) | $MISSING |"
echo "| ADDED (всего, по типу — см. ниже) | $ADDED |"
echo "| ADDR_MATCH % (T/t, min-адрес/имя, nm -n result/vmlinux vs ref) | ${ADDR_MATCH}% (${ADDR_EQ}/${ADDR_N}) |"
echo
echo "### ADDED-by-type"
cat "$WORK/added_by_type.txt"
echo
echo "## MTC-core check (должны присутствовать в kallsyms)"
printf '%s' "$mtc_lines"
echo "ИТОГО MTC-core: $([ $MTC_OK -eq 1 ] && echo PASS || echo FAIL)"
echo
echo "## Strings"
echo "- \`strings result/vmlinux | grep -c Trk3188\` = **$TRK** (треб. ≥1)"
echo
echo "## Size: result/vmlinux.stripped"
echo "| метрика | значение |"
echo "|---|---|"
echo "| size | $SIZE |"
echo "| vs r5 (8303780) | delta = $DIFF_R5 |"
echo "| vs target (8232996) | delta = $DIFF_TARGET |"
} > "$RES/verify_report.md"

echo "== verify_symbols: COMMON=$COMMON MISSING=$MISSING ADDED=$ADDED ADDR_MATCH=${ADDR_MATCH}% MTC-core=$([ $MTC_OK -eq 1 ] && echo PASS || echo FAIL) Trk3188=$TRK size=$SIZE"
cat "$RES/verify_report.md"
[ $MTC_OK -eq 1 ] && [ "${TRK:-0}" -ge 1 ]
