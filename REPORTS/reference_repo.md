# Reference kernel — omegamoon/rockchip-rk3188-generic (T9)

- Reference: https://github.com/omegamoon/rockchip-rk3188-generic — HEAD d2440f70 (2013-06-16).
- Версия: Linux 3.0.36+ (Makefile: VERSION=3 PATCHLEVEL=0 SUBLEVEL=36 EXTRAVERSION=+); линия Rikomagic RK3188;
  общий rk3x vendor-бейз с нашей линией.
- Наш драйвер-пак (kernel-overlay в mtc_drivers: drivers/misc/mtc/*, sound/soc/rk29/*,
  arch/arm/mach-rk3188/board-mtc.c, config/mtc_defconfig) накладывается ПОВЕРХ этого референса.
- Mali-400/UMP: исходники = это репо (drivers/gpu/mali/, ARM reference r3p2-01rel1, API_VERSION=20, CONFIG=rk30).

## ref_mali артефакты (OOT-сборка)
- Путь: /home/amper/Coding/mtc_build/modules/ref_mali/{mali.ko, ump.ko}; vermagic 3.0.36+ = наше ядро, unresolved=0.
- Загрузка: insmod ump.ko → mali.ko; userspace парность = ARM reference libmali r3p2 (API 20).
- md5 (md5sum /home/amper/Coding/mtc_build/modules/ref_mali/*.ko):
  - mali.ko: eb7d67b84fc067f194f63f84e188f4d9
  - ump.ko: 185f54773ff9580182c71dfe0f4fa1a0

## ОГОРОВКА
Действующая сборочная база vmlinux — SDK-ядро 3.0.36+ (/home/amper/Coding/mtc_build/ref_kernel,
scripts/apply_overlay.sh) — той же версии/линии; переключение сборки целиком на omegamoon-репо —
отдельная задача (не выполнена, не обещать).

## Ссылки
- REPORTS/mali_candidate_check.md — проверка кандидата (критерии a–d, инварианты vmlinux/System.map)
- PLAN.md §E (T9); REPORTS/README_mtc_drivers.md — блок "Reference kernel"
