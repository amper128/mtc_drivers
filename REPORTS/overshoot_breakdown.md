# Overshoot breakdown: +152 704 B (stripped vmlinux vs target verify)

## Header
- Our stripped: /home/amper/Coding/mtc_drivers/result/vmlinux.stripped = 8 385 700 B.
- Target verify = 8 232 996 B → overshoot = +152 704 B. Vs kernel.elf raw 8 266 216 → +119 484 B.
- our sizes: `nm -S result/vmlinux` (result/kallsyms.txt was CORRUPTED → rejected). target sizes: 3188_kallsyms (CRLF stripped), target_size = next_addr − addr, grouped by base name.

## Aggregates (JOIN by base name; COMMON = 27 318 funcs)
| bucket | #funcs | bytes |
|---|---|---|
| Δ>0 | 342 | +30 411 |
| Δ<0 | 1 427 | −52 351 |
| Δ=0 | 25 549 | 0 |
| only_our (997) | — | +131 796 |
| only_tgt (1 646) | — | −390 135 |
| T/t totals our 6 421 249 / tgt 6 701 528 | — | net −280 279 |

## Key insight
Function-code bloat is NOT the overshoot cause: common Δ net = −21 940 B; whole T/t net = −280 279 B (ours SMALLER, aided by target-only mali/NAND-FTL/ump modules missing from ours). The +152 704 B is carried by +432 983 B non-symbol image content (data/.rodata/.bss/layout/padding) minus the 280 279 symbol savings.

## TOP-15 Δ>0 (symbol / our / tgt / Δ / class)
1. T132B_P2_S_MASK 4657/160 +4497 [CODEGEN: data-label artifact]
2. wm8731_dapm_widgets 1620/0 +1620 [CODEGEN: tgt size=0 dup-addr]
3. keys_probe 2892/1728 +1164 [TRIMMABLE-vendor: mtc/keys.c]
4. qtaguid_ctrl_proc_write 5024/3900 +1124 [CODEGEN/base REF]
5. rk3188_lcdc_set_par 2320/1468 +852 [STRUCTURAL lcdfb b5/6]
6. qtaguid_mt 1960/1196 +764 [CODEGEN/base REF]
7. hdmitx_SetCSCScale 832/188 +644 [TRIMMABLE-vendor it66121]
8. rk3188_load_screen 1760/1164 +596 [STRUCTURAL lcdfb]
9. wm8731_snd_controls 480/48 +432 [CODEGEN artifact]
10. serial_rk_probe 1036/616 +420 [CODEGEN/base REF]
11. soc_codec_reg_show 856/440 +416 [TRIMMABLE-vendor mtc/codec.c]
12. rk30_map_io 732/356 +376 [CODEGEN/base]
13. touch_cali_status 772/408 +364 [TRIMMABLE-vendor mtc/car.c]
14. iface_stat_create 660/316 +344 [CODEGEN/base REF]
15. rockchip_i2s_hw_params 856/532 +324 [STRUCTURAL audio]
(остальные топ-40 в /tmp/final.json)

## Verdict (bytes)
- TRIMMABLE-vendor: top vendor subset ≈ +5 KB; realistic safe trim incl. long tail ≈ 10–15 KB; ceiling = sum Δ>0 = 30 411 B. (keys_probe, hdmitx_SetCSCScale, soc_codec_reg_show, touch_cali_status, wm8731_*×6, codec_*_file×3, act8846 suspend/resume, keys_remove, touch_mode_store — all in src/drivers/misc/mtc/ + it66121.)
- STRUCTURAL (batches 2–10, must stay): top-40 portion ≈ 2.4 KB + only_our features +131 796 B.
- CODEGEN/base: top-40 portion ≈ 21.6 KB (REF-base diffs + compiler artifacts) — untrimmable at source level.
- Realistic limit: trimming functions ≤ ~30 KB (realistically 10–15 KB) → overshoot floor ≈ +122–143 KB. Remaining gap = non-symbol content (fonts/logos/tables), needs section-level analysis.

## Caveats
- target_size = adjacent-addr approximation (padding; dup-addr → size 0 inflates wm8731_*/__aeabi_*).
- Base-name grouping folds $d/$a/.isra/.part.
- Non-symbol +432 983 B is a RESIDUAL, not a measured section sum.
- Evidence: /tmp/final.json, /tmp/our_nm.txt. decompiled targets in /home/amper/tmp/ida-tmp/mtc_audio/src_all/ (23 943 files).
