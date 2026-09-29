# Разведка SDK RK3188 (R-Box, Android 4.4.2 Kitkat, kernel 3.0.36)

SDK: /server/FTP/[tmp]/android/3188/mnt/rk3188_rk3066_r-box_android4.4.2_sdk/
Бинар: /home/amper/Coding/RK3188/{kernel.elf, 3188_kallsyms}
Ref: /home/amper/tmp/ida-tmp/mtc_audio/ref_kernel_rk3188/

## P1) board-rk3188-mtc.c — ВЫВОД: PARTIAL (дерево = наша Т4-база, но имя НЕ патчено в SDK)
- Файл ЕСТЬ: kernel/arch/arm/mach-rk3188/board-rk3188-mtc.c (2048 строк).
- MACHINE_START(RK30, "RK30board") — строка "RK30board" (L2040), НЕ "Trk3188".
  MACH_TYPE: отдельной строки нет (MACHINE_START без явного machtype — авто по таблице).
- .init_machine=machine_rk30_board_init; i2c0..4 + gpio (rk30_i2c_register_board_info L1850/1857);
  wifi: rk-sdmmc-wifi.c L794, &rk29sdk_wifi_device L1091, rk29sdk_wifi_bt_gpio_control_init L1865,
  rk29sdk_wifi_combo_module_gpio_init L1867, wifi mmc0 status L831-832/916-917.
- MTC-устройства в файле: mtc_vs_device/mtc_car_device/mtc_lcd_device/mtc_dvd_device/mtc_keys_device,
  "mtc-backview" L1486, "mtc_ch7025" L1502.
- СВЕРКА С БИНАРОМ (strings kernel.elf / kallsyms | tr -d '\r'):
  * "Trk3188" — 2 вхождения в strings (есть в бинаре, нет в SDK-источнике → бинар собран из ПАТЧЕНОЙ копии);
  * customize_machine c040a398 t, machine_rk30_board_init c040e744 t — ЕСТЬ в bинаре (совпадает с .init_machine);
  * rk29sdk_wifi_reset c04a9af4 t, rk29sdk_wifi_mac_addr c04a9f70 T — совпадение с задачами (1:1).
- ВЫВОД: SDK-дерево = исходная Т4-база; бинар = это же дерево + патч MACHINE name "Trk3188". Не binaRE — YES.

## P2) SDK kernel vs Nu3001 ref — ВЫВОД: одно семейство (3.0.36), расхождение умеренное; к бинару ближе SDK (содержит mtc-board)
- diff -rq --brief kernel/drivers vs ref/drivers: 407 строк различий (top-level).
- diff -rq --brief kernel/arch/arm vs ref/arch/arm: 381 строка.
- Ref — тоже RK3188-дерево (COPYING/Makefile/...). Точный набор vendor-каталогов с diff не пересчитан
  (FTP-медленно); главное: SDK содержит board-rk3188-mtc.c — ref без него (ref — другой board).
- grep "Trk3188" по всему SDK kernel — НЕТ в источниках (только в бинаре) → патч имени не входит в SDK.
- ВЫВОД: для T4 берём дерево из SDK (есть mtc-board); Nu3001 ref — только как эталон аудиостека.

## P3) defconfig — ВЫВОД: ни один из 8 defconfigs НЕ включает MACH_RK3188_MTC
- configs (полный список): rk30_box_costdown_kikat, rk30_box_pizza2_kikat, rk30_box_pizza_kikat,
  rk30_hotdog_ti_kikat, rk3188_box_costdown_kitkat, rk3188_box_pancake_kitkat,
  rk3188_costdown_dcdc_kitkat, rk3188_magicwand_kitkat.
- MACH по defconfig'ам: rk30_* → CONFIG_MACH_RK30_BOX; rk3188_* → CONFIG_MACH_RK3188_BOX
  (+COSTDOWN в costdown). Grep MACH_RK3188_MTC по configs — пусто.
- kernel/arch/arm/mach-rk3188/Makefile: board-rk3188-mtc.c В Makefile НЕ ССЫЛАЕТСЯ (нет строки
  board-$(CONFIG_...) += board-rk3188-mtc.o; ближайший — board-rk3188-box.o).
- ВЫВОД: T4 = добавить Kconfig+Makefile-строчку под наш CONFIG_MACH_RK3188_MTC (или под existing BOARD)
  + переименовать "RK30board"→"Trk3188". База defconfig для сборки — rk3188_box_pancake_kitkat (или box_costdown).

## P4) Тулчейн — ВЫВОД: arm gcc 4.6 ЕСТЬ, путь:
- RKTools/linux/ = только Linux_Upgrade_Tool_v1.16 (не тулчейн).
- prebuilts/gcc/linux-x86/arm/: **arm-eabi-4.6**, arm-eabi-4.7, arm-linux-androideabi-4.6, arm-linux-androideabi-4.7.
  Компилер для kernel: prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc (и 4.7 рядом).
- NDK: ndk/toolchains/arm-linux-androideabi-{4.6,4.7,4.8,clang3.2,clang3.3} (для userland, не kernel).
- --version НЕ запущен (файл на FTP без +x → Permission denied); версия по каталогу = gcc 4.6.
- kernel/Makefile: VERSION=3 PATCHLEVEL=0 SUBLEVEL=36 ("Sneaky Weasel") — подтверждено.

## P5) mtc-драйверы в SDK kernel — ВЫВОД: наши vendor-драйверы ABSENT, wm8731-кодек ЕСТЬ
- grep -rl "tef6606|mtcGetSetVolume|RK29_WM" kernel/drivers kernel/sound → только
  kernel/sound/soc/rk29/{rk29_wm8900.c, rk29_wm8988.c, rk29_wm8994.c} (по RK29_WM); tef6606/mtcGetSetVolume — НЕТ.
- drivers/misc/ — mtc/tef/ch7025/vs_ — НЕТ (пусто).
- kernel/sound/soc/codecs/: wm8731.c/wm8731.h ЕСТЬ (стандартный WM-кодек) + полный набор wm*.
  sound/soc/rk29/ — machine-драйверы rk29_wm8900/8988/8994 (RK29-платформа, не RK3188-wm8731 machine).
- ВЫВОД: wm8731-кодек готов; machine-драйвер + tef6606 + mtcGetSetVolume (наш инвентарь) — НЕ В SDK,
  переносим из binaRE/нашего дерева.

## КЛЮЧЕВОЕ для плана
- T4 (board-патч): базовый файл board-rk3188-mtc.c готов (все mtc-устройства на месте); нужно
  (а) "RK30board"→"Trk3188", (б) Kconfig/Makefile entry CONFIG_MACH_RK3188_MTC, (в) defconfig на базе
  rk3188_box_pancake_kitkat. Символы бинара (rk29sdk_wifi_reset c04a9af4, rk29sdk_wifi_mac_addr c04a9f70,
  customize_machine c040a398, machine_rk30_board_init c040e744) подтверждают родство дерева.
- T5 (toolchain): CC=prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc (или 4.7).
  Kernel 3.0.36. Ничего из mtc-userland-тулчейна не требуется отдельно от prebuilts/NDK.
