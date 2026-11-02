# MTC Kernel Reconstruction — Restoration Plan & Status

## 1. Project Overview

- binaRE MTC: cleanroom reconstruction of an RK3188 (ARM, Linux 3.0.36+, Rockchip) MTC automotive kernel, from IDA Pro decompilation + a kernel-overlay build system.
- Goal: vmlinux byte-1:1 with the original vendor binary (kernel.elf blob, 8232996 B).
- Judge: 3188_kallsyms (42 433 lines, 28 973 unique T/t symbols).
- Toolchain: arm-eabi-google GCC 4.6.x-google 20120106 — confirmed IDENTICAL to the target compiler (embedded version string match).

## 2. Current State (verified via scripts/verify_symbols.sh)

- COMMON: **27 458 / 28 973** (94.8%) T/t symbol-name match.
- ADDED: **7 440** (our symbols absent from target; mostly scattered small + MIXED/MTC-core-linked).
- MISSING: ~3 063 (target symbols not present in ours).
- MTC-core verify: **PASS** (6/6 critical symbols).
- Size: our stripped vmlinux 8 242 396 B; target blob 8 232 996 B.
- **Byte-1:1: NOT MET** (see §3).

## 3. Byte-1:1 Verdict (honest)

- Target = the vendor's pre-built 2016 kernel (build **#315**, root@chun, Feb 2016), a flat raw image.
- Our from-source rebuild diverges from **byte 8** (a pc-relative LDR immediate differs → different downstream section layout).
- **88.3% of code+data image bytes differ**; only **65.1%** of sampled 16-byte code windows match (shared base source + toolchain, but layouts and a large share of code differ).
- The target image is **60 004 B larger** than our loadable image; the file-level delta is +9 400 B (our ELF bookkeeping vs the target's flat image).
- **Conclusion: byte-1:1 is NOT achievable** from our reconstruction — it would require the exact vendor source tree, config, patch set, and build metadata. The reconstruction achieves strong SYMBOL parity (94.8% kallsyms) but not byte-level code identity.

## 4. What Is Done (phases)

- Phase 1–2: vendor patches, MTC-14 transcription, defconfig-twin, FULL rebuild (gap_r5).
- Batches 1–10: closed per-family symbols (misc, mmc/usb/ipp, lcdfb tail, tsc2003 hybrid, MTC diag, ping/ch7025/goodix).
- RESTORE (b) + Vendor trim (c): CBN_SPI calibration cluster, ping v6, recon.c bundle; rk_fb / hdmi / ddr trims.
- Logo-fix + Font (Variant B): byte-exact big logo; std 8x16 font 4096 B (9-byte glyph-#42 diff AGREED).
- P2 ADDED-trim: config-trims (RFKILL/periph/DDR_FREQ), DDR byte-exact (7/14 EXACT), BLOCKS 2–11 (bCSCMtx, vt, serial_rk DMA, rk30_rmii, display-sys, vmac, ch7025/ypbpr, remotectl/dvfs-debug, sio-iosched, goodix/gtp parity).
- fef*/SRAM filter added to verify_symbols.sh ADDED metric (89 SRAM-region symbols excluded — absent from target kallsyms).

## 5. Key Constraints (documented decisions)

- Byte-1:1 target = NO (reconstruction from decompiled; practical maximum reached at the current source).
- kallsyms match = YES (94.8% COMMON, MTC-core PASS).
- No raw-address calls allowed (RULES.md); each such call is decompiled in IDB and replaced with a named call.
- CONFIG trims applied only when symbols are fully ADDED (0 in COMMON/judge); MIXED families require code-patches.
- fef*/SRAM symbols (0xFEFxxxxx): absent from target kallsyms → filtered from the ADDED metric (required SRAM code).

## 6. Pending / Next

- **.ko modules** (mali 652 + rk30xxnand 404 + ump 102 = 1 158 symbols) — separate deliverable, PENDING.
- gtp/goodix: functionally reconciled (25/25 target names present); strict name-parity for the remaining ADDED would require a full target-driver re-transcription from decompilation.
- Push (mtc_drivers ahead of origin; mtc-kernel at ffb6f23) — on explicit request only.

## 7. Artifacts & Paths

- Target: /home/amper/Coding/RK3188/kernel.elf (8 232 996 B blob).
- Ours: /home/amper/Coding/mtc_drivers/result/vmlinux.stripped (8 242 396 B).
- Judge: /home/amper/Coding/RK3188/3188_kallsyms (CRLF).
- Metrics: scripts/verify_symbols.sh; Logs: REPORTS/vendor_trim.log, build_fixes.log (fix 0–29).
- IDA: IDB mtc_kernel.i64 (313 MB); decompiled src_all/ (23 943 files).
