# mtc_defconfig — сигнатурный анализ RK3188 3.0.36+ ("Trk3188") — RE-BASELINE (SDK)

## Входы
- Бинар: /home/amper/Coding/RK3188/kernel.elf (stripped, CONFIG_ в strings = 0). Banner: "Linux version 3.0.36+ (root@chun) ... #315 SMP PREEMPT Sat Feb 20 14:44:56 CST 2016" → SMP=y, PREEMPT=y, MODULES=y (mod_unload).
- kallsyms: /home/amper/Coding/RK3188/3188_kallsyms (42433 строк, CRLF; tr -d '\r') → 40816 уникальных символов (/tmp/mkc/syms.txt).
- **SDK (новая база)**: /server/FTP/[tmp]/android/3188/mnt/rk3188_rk3066_r-box_android4.4.2_sdk/kernel (3.0.36, arm-eabi-4.6).
- Старая база: ref_kernel_rk3188/arch/arm/configs/xdauto_defconfig — **ЗАСТАРЕЛА**: board-rk3188-mtc.c есть ТОЛЬКО в SDK → бинар собран из SDK-линейки.

## Выбранный base: rk3188_box_pancake_kitkat_defconfig (455 строк)
- 8 SDK defconfigs (rk30_box_costdown/pizza/pizza2_kikat, rk30_hotdog_ti_kikat, rk3188_box_costdown/box_pancake/costdown_dcdc/magicwand_kitkat): ни один НЕ включает MACH_RK3188_MTC (board-mtc не в Makefile).
- По 19 сигнатурным точкам: 3 rk3188-кандидата (box_costdown, box_pancake, costdown_dcdc) вровень — IKCONFIG=0, RKWIFI/RT5631=0, vendor-govs=0, SMP/PREEMPT совпадают; везде BT=8(12) и нет RTL8192CU/WM8731.
- Тейкбрейкер: mtc = кастомный авто-борд box-семейства; costdown/DCDC-опции без сигнатур в бинаре → исключаем; box_pancake = чистый box-base (1 MACH-строка). Зафиксировано: **никакой defconfig mtc-board не покрывает — MACH строка добавлена (T4)**.

## Таблица изменений (mtc_defconfig = pancake + diff, 478 строк)
| Опция | Действие | Сигнатура (kallsyms) |
|---|---|---|
| CONFIG_MACH_RK3188_BOX=y | → CONFIG_MACH_RK3188_MTC=y (T4; choice, entry — T4-patches) | "Trk3188" x2 в ELF-строках; board-rk3188-mtc.c (только в SDK) |
| BT + 11 подопций | → not set (12 шт., в base было =y) | hci_*=0, l2cap_*=0, rfcomm*=0; един. hit "bluetooth" = rtl8192c_ReadBluetoothCoexistInfo |
| CPU_FREQ_* (6 стандартных govs + DEFAULT_GOV_INTERACTIVE) | без изменений (vendor-govs в SDK-базе НЕТ — xdauto-пункт неактуален) | cpufreq=61, interactive=36, ondemand/dbs=3; "rk3188 cpufreq version 2.2" |
| CONFIG_RTL8192CU | добавить =y (нет в base) | ReadEFuse_RTL8192C (rtl8192c_*=76); опция в SDK Kconfig (drivers/net/wireless rkusbwifi) |
| CONFIG_SND_SOC_WM8731 | добавить =y (нет в base) | wm8731_i2c_probe, rk29_wm8731_init (wm8731*=20); SDK Kconfig sound/soc/codecs:315 |
| CONFIG_IKCONFIG(_PROC) | явный not set (в base отсутствует) | ikconfig*=0 |
| RKWIFI, RK_CFG80211, RKWIFI_26M | явный not set (нет в base) | rt2860*=0 (wifi бинара = rtl8192cu) |
| CONFIG_SND_SOC_RT5631 | явный not set (нет в base) | rt5631*=0 |
| CONFIG_MTC_DRIVERS=y (T2) | сохранён (хвост файла) | mtc_*=18 built-in (mtc_getTouchKey, mtc_direct_fb_buf, mtc_get_pin_map) |

## Ключевые отличия новой базы от xdauto-базы
- xdauto "грязные" блоки (IKCONFIG=y, 6 vendor-governors, RKWIFI, RT5631) в pancake **уже чистые** → diff сузился до: +RTL8192CU, +WM8731, −BT(12), MACH-строка.
- WM8731: в SDK Kconfig ЕСТЬ (L315) — опция легитимна без mtc-патча (в xdauto-ref её не было).
- SMP/PREEMPT/MODULES, CFG80211-стек, EARLYSUSPEND, EXT4 (ext4_*=329), rknand, RGA (RGA_*=40) — в базе и бинаре, без изменений.

## НЕОПРЕДЕЛЁННЫЕ (TBD — честно)
1. TEF6606 FM — tef6606_*=8; драйвера нет в SDK; имя опции TBD (std: TEF6606).
2. Mali-400 — mali_*=529 (_mali_osk_*), drm_*=0 (mali БЕЗ DRM); имя опции TBD.
3. goodix — 11 + keytouch 3, ДВА инстанса (init6/init7); имя опции TBD.
4. ch7025 GPS — ch7025_*=8; драйвера нет в SDK; имя опции TBD (**НОВОЕ** относительно xdauto-отчёта).
5. vendor cpufreq "rk3188 cpufreq version 2.2" — имя драйвера/опции TBD (CPU_FREQ_DT в базе нет).
6. MACH_TYPE — TBD: machine_id/ATAG-id символов нет (match по atag-name, T4).

## Ограничения
- kallsyms = только built-in: часть "добавленных =y" в чужой сборке могла быть =m; у нас символы в core-образе → в данной сборке built-in.
- Нулевые блоки (BT, RKWIFI, RT5631, IKCONFIG) — высокая уверенность.
- Старые копии: /home/amper/Coding/mtc_overlay/config/mtc_defconfig + REPORTS/config_diff.md — ЗАСТАРЕЛИ (xdauto-база), не использовать.
