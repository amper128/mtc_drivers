# T4 status — board-mtc (CLOSED)

1. `arch/arm/mach-rk3188/board-mtc.c` — cp SDK board-rk3188-mtc.c (2048L, md5 e9e727eadd54…) + patch L2040:
   `MACHINE_START(RK30,"RK30board")` → `"Trk3188"` + comment `/* binaRE: binary = "Trk3188" */`.
   Binary check: strings /home/amper/Coding/RK3188/kernel.elf → "Trk3188" x2 (lines 7643/9647).
2. MACH_TYPE — **TBD honestly**: no machine_id/MACH_TYPE/ATAG-id symbol in 3188_kallsyms
   (machine table read-only); RK 3.0 board Kconfig has no int MACH_TYPE (atag name match) → nothing to add.
3. Patches created + APPLY-TESTED on scratch copies (`patch -p1`, both hunks OK):
   - `arch/arm/mach-rk3188/Kconfig.patch` — +`config MACH_RK3188_MTC / bool "RK3188 MTC board (Trk3188)"` in choice.
   - `arch/arm/mach-rk3188/Makefile.patch` — +`board-$(CONFIG_MACH_RK3188_MTC) += board-mtc.o`.
   defconfig: `config/mtc_defconfig` L340 TBD DS1006H → `CONFIG_MACH_RK3188_MTC=y`.
4. kallsyms cross-check: `.init_machine`=machine_rk30_board_init → c040e744 t ✓; customize_machine c040a398 t ✓
   (RK-patched arch_initcall in arch/arm/kernel/setup.c calling machine_desc->init_machine) — SDK matches binary.
5. README `arch/arm/mach-rk3188/README.md` rewritten: T4 CLOSED (placeholder removed).
6. Files for devops commit: board-mtc.c, Kconfig.patch, Makefile.patch, README.md (mach-rk3188/), config/mtc_defconfig.
   git untouched (no commits per task).
