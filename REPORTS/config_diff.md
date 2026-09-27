# Конфигурация ядра RK3188 3.0.36+ (mtc): xdauto_defconfig → mtc_defconfig

## Метод
- banner (strings kernel.elf): "Linux version 3.0.36+ (root@chun) ... #315 SMP PREEMPT Sat Feb 20 14:44:56 CST 2016" → SMP=y, PREEMPT=y, MODULES=y (mod_unload).
- kallsyms: /home/amper/Coding/RK3188/3188_kallsyms (42433 строк, CRLF) → 40816 уникальных символов; ELF stripped, CONFIG_ в strings = 0.
- IKCONFIG: 0 символов ikconfig* в kallsyms → CONFIG_IKCONFIG выключен (в xdauto =y — убрано).
- ref-дерево: /home/amper/tmp/ida-tmp/mtc_audio/ref_kernel_rk3188 (RK vendor 3.0.36, 199 cfg); референт: arch/arm/configs/xdauto_defconfig (3237 строк); также xdauto-performance_defconfig.

## Machine — ключевая находка
- machine-строка бинаря: **Trk3188**; строки "--mtc rk3188_lcdc_open", "rk3188 cpufreq version 2.2" → mtc-борд, НЕ DS1006H (MACH_RK3188_DS1006H оставлен с пометкой TBD).

## Таблица изменений (mtc_defconfig = xdauto_defconfig + diff)
| Опция | Действие | Сигнатура (kallsyms) |
|---|---|---|
| CONFIG_IKCONFIG, CONFIG_IKCONFIG_PROC | убрать (→ not set) | 0 символов ikconfig* |
| CONFIG_RTL8192CU | добавить =y | ReadEFuse_RTL8192C, rtl8192c_ReadBluetoothCoexistInfo |
| CONFIG_RKWIFI, CONFIG_RK_CFG80211, CONFIG_RKWIFI_26M | убрать | 0 символов rt2860* (wifi в бинаре — rtl8192cu) |
| BT-стек: BT, L2CAP, SCO, RFCOMM(_TTY), BNEP, HIDP, HCIUART(+H4/LL), HCIBCM4325, AUTOSLEEP | убрать (все 12) | 0 символов l2cap*/rfcomm*/hci_*/bluetooth* — BT core не собран |
| CPU_FREQ_GOV_ADAPTIVE/HYPER/LIONHEART/LULZACTIVE/SMARTASS2/ONDEMANDX | убрать (vendor) | нет символов под этими именами; в бинаре: interactive (36), ondemand/dbs (6) |
| CONFIG_SND_SOC_RT5631 | убрать | 0 символов rt5631* |
| CONFIG_SND_SOC_WM8731 | добавить =y (имя std; драйвер — mtc-патч, нет в ref Kconfig) | wm8731_i2c_probe, rk29_wm8731_init (422 snd_soc_*) |
| MACH_RK3188_DS1006H=y | оставить + TBD-метка | machine "Trk3188" в ELF-строках |
| + TBD-комментарии в файле: TEF6606, Mali-400, goodix (2nd) + keytouch, vendor cpufreq/CPU_FREQ_DT | — | см. ниже |

## Совпадает с бинарем (без изменений)
- SMP=y, PREEMPT=y, MODULES=y; CFG80211/MAC80211/RFKILL_RK (cfg80211_*); EARLYSUSPEND/FB_EARLYSUSPEND (earlysuspend_*); EXT4_FS=y (ext4_*=329); F2FS отсутствует в обоих; CPU_FREQ=y + DEFAULT_GOV_INTERACTIVE=y; RGA_RK30=y (RGA_*); rknand (add_rknand_device, __initcall_rknand_init6).

## НЕОПРЕДЕЛЁННЫЕ (TBD)
1. TEF6606 — FM-радио в бинаре (tef6606_i2c_probe, __initcall_tef6606_init6, tef6606_id); драйвера нет в ref-дереве; имя опции TBD (std: TEF6606).
2. Mali-400 — 584 mali_* символа (_mali_osk_*); GPU-драйвера нет в ref-дереве (drivers/gpu: drm/ion/stub/vga); имя опции TBD.
3. goodix — 2 инстанса (__initcall_goodix_ts_init6 / _init7) + keytouch_init; в дереве drivers/input/touchscreen/rk29_i2c_goodix.c, имя опции TBD (2nd-инстанс — mtc).
4. vendor cpufreq "rk3188 cpufreq version 2.2" — имя драйвера/опции и CPU_FREQ_DT TBD.
5. MACH/board для "Trk3188" (mtc-борд) — TBD (board-файл, не в defconfig).
6. Точное имя SND_SOC_WM8731 (wm8731.c в tree есть, entry в Kconfig отсутствует) — mtc-патч.

## Ограничения
- kallsyms содержит символы загруженных модулей: часть добавленных =y в реальности может быть =m.
- Убранные блоки (BT, RKWIFI, RT5631, vendor-govs) подтверждены нулём символов — высокая уверенность.
