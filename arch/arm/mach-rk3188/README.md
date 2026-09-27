# arch/arm/mach-rk3188 — T4 PLACEHOLDER (not part of the build yet)
T4 will add here (rsync-ready FULL files, see PLAN §A):
- board-mtc.c — base board-rk3188-sdk.c + binaRE machine_desc, machine "Trk3188" (strings elf ✓),
  i2c0-2 init, rkwifi gpio (rkwifi_sysif already PRESENT in ref — board only gpio/clk).
- Makefile.boot, Kconfig, include/mach/*.h — patch copies (+MACH_TYPE_MTC, board entry).
- MACH_TYPE: TBD — not readable from kallsyms; IDA/strings dump + decision goes to REPORTS (PLAN subtask 4).
NOTE for T4: config/mtc_defconfig currently has CONFIG_MACH_RK3188_DS1006H=y (marked TBD inline) —
must become CONFIG_MACH_RK3188_MTC=y with this overlay for the "Trk3188" machine.
