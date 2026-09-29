# arch/arm/mach-rk3188 — T4 CLOSED

- **board-mtc.c** (2048L) — FULL copy of SDK `board-rk3188-mtc.c`
  (rk3188_rk3066_r-box_android4.4.2, md5 e9e727eadd544de66576fc3f03c99fab) + ONE patch:
  `MACHINE_START(RK30, "RK30board")` → `MACHINE_START(RK30, "Trk3188")` (L2040)
  — binary strings x2 "Trk3188" (rodata machine table + kstrtab) verified against
  /home/amper/Coding/RK3188/kernel.elf.
- **Kconfig.patch** — +`config MACH_RK3188_MTC / bool "RK3188 MTC board (Trk3188)"`
  in the "RK3188 Board Type" choice (before endchoice). Apply: `patch -p1` from kernel root.
- **Makefile.patch** — +`board-$(CONFIG_MACH_RK3188_MTC) += board-mtc.o`.
- Both patches TESTED against scratch copies of the SDK files: applied OK.
- defconfig: `config/mtc_defconfig` L340 — TBD DS1006H replaced by `CONFIG_MACH_RK3188_MTC=y`.
- kallsyms cross-check: `.init_machine = machine_rk30_board_init` (binary c040e744 ✓) and
  `customize_machine` c040a398 ✓ = RK-patched `arch_initcall(customize_machine)` in
  arch/arm/kernel/setup.c calling machine_desc->init_machine — SDK tree matches.
- MACH_TYPE: **TBD (honest)** — no machine_id/MACH_TYPE symbol in 3188_kallsyms
  (machine table is read-only); board Kconfig entries carry no int MACH_TYPE
  (name-based atag match), so nothing to add.
- devices intact in copy: mtc_vs/car/lcd/dvd/keys (L1111-15), "mtc-backview" (L1486),
  "mtc_ch7025" (L1502), wifi rk29sdk_wifi_device, i2c0-4+gpio — as per binary symbol set.
