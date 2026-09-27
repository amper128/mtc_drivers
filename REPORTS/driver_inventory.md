# Инвентаризация драйверов RK3188 (бинар 3.0.36 vendor): ЕСТЬ в каллсаймах — НЕТ в дереве Nu3001

Источники: `/home/amper/Coding/RK3188/3188_kallsyms` (42433 символа, CRLF снят) ×
ref-дерево `/home/amper/tmp/ida-tmp/mtc_audio/ref_kernel_rk3188/` (41482 файла, полный vendor 3.0.36).
Метод: семейства символов из kallsyms → grep дерева (путь + содержимое .c/.h/Kconfig/Makefile).

## 1. ABSENT — есть в бинаре, НЕТ в дереве (14 семейств)

| # | Драйвер / семейство | Доказательство в kallsyms (символ:адрес) | Статус в дереве | Путь для оверлея (предложенный) |
|---|---|---|---|---|
| 1 | **mtc-common** (вендорные хелперы mtc_*) | mtc_direct_fb_buf:c06a0a20, mtc_get_pin_map:c082d5c0, mtc_getTouchKey (ср. mtc_getTouchKey_tab), mtc_debug_putc, mtc_clear_screen | ABSENT (grep `mtc_` по .c/.h = 0 вендорных вхождений; только ложные s390/echoaudio) | `drivers/misc/mtc/mtc_common.c` |
| 2 | **mtc-car** (автомобильный AVM) | car_init:c04216e8, car_avm (T), car_probe, car_ioctl:c042b… (__initcall_car_init5s), car_pm_ops | ABSENT (`car_avm`, `car_probe` — 0) | `drivers/misc/mtc/car.c` |
| 3 | **mtc-keys** (кнопки) | mtc_keycode:c0a0ac84, mtc_keydefault:c0a09bd0, mtc_key_suspend:c083cfb8, mtc_key_resume:c083d064 | ABSENT (нет keys-модуля; mtc_key* — 0) | `drivers/misc/mtc/keys.c` |
| 4 | **mtc-lcd** (вендорный LCD-модуль) | lcd_probe:c09bc990, lcd_show_symbol:c083860c, lcd_show_dig:c0838684, lcd_send_cmd:c0838484, lcd_pm_ops:c0a09eb4 (__initcall_lcd_init6) | ABSENT (`lcd_show_symbol` — 0; драйверы `drivers/video/backlight/*` — другие) | `drivers/misc/mtc/lcd.c` |
| 5 | **mtc-tv** (TV-out, tm_*) | tm_init:c0420fe8, tm_probe:c0819374, tm_devices:c0a07efc (__initcall_tm_init6) | ABSENT (`tm_probe`/`tm_devices` — 0; drivers/hid/hid-tmff.c не совпадает) | `drivers/misc/mtc/tv.c` |
| 6 | **mtc-backview** | backview_init:c0421738, backview_probe:c09bcac4, backview_remove:c09bdde8, backview_id:c0a0ac54 (__initcall_backview_init5) | ABSENT (`backview_probe` — 0) | `drivers/misc/mtc/backview.c` |
| 7 | **mtc-dvd** (видео/DVD-плеер) | dvd_init:c04216c4, dvd_probe, dvd_isr:c082a56c, dvd_send_command, dvd_pm_ops (__initcall_dvd_init6) | ABSENT (`dvd_probe` — 0) | `drivers/misc/mtc/dvd.c` |
| 8 | **mtc-vs** (virtual serial/видео-порт) | vs_init:c04216dc, vs_probe, vs_set_mctrl:c082c9fc, vs_get_mctrl:c082c9e8, vs_pm_ops (__initcall_vs_init6) | ABSENT (`vs_set_mctrl`/`vs_probe` — 0) | `drivers/misc/mtc/vs.c` |
| 9 | **audio_card + rk29_wm8731** (snd-soc machine) | rk29_wm8731_init:c0862494, audio_card_init:c0421de8+c0421ec4 (двойной), __initcall_audio_card_init6/7 | ABSENT: `rk29_wm8731` в дереве = 0. В `sound/soc/rockchip/` есть rk29_wm8900/8988/8994 и др., но wm8731-machine нет. CODEC wm8731.c — present (`sound/soc/codecs/wm8731.c`) | `sound/soc/rockchip/rk29_wm8731.c` (+ Kconfig/Makefile) |
| 10 | **tef6606 + radio** (FM-приёмник) | tef6606_init:c0421700, tef6606_i2c_probe, __initcall_tef6606_init6, radio_cmd_work:c08376c8, Hit_radio_sta:c0836c5c | ABSENT (`tef6606`, `radio_cmd_work` — 0) | `drivers/media/radio/tef6606.c` (+ `radio` в `drivers/misc/mtc/`) |
| 11 | **Mali GPU** (модуль, bf-сегмент) | _mali_osk_* (584 символа), mali_open:bf04625c, __malidrv_build_info | ABSENT (`_mali_osk`/`malidrv` — 0; файлов *mali* в дереве нет) | `drivers/video/rockchip/mali/` (оверлей модуля malidrv.ko) |
| 12 | **UMP** (User Memory Protection, модуль) | ump_file_open:bf035ad4, ump_file_mmap:bf035bbc, ump_kernel_device_initialize:bf035eac, ump_memory_backend_create | ABSENT (каталога *ump* нет; упоминания только board-файлы) | `drivers/media/ump/` (оверлей модуля ump.ko) |
| 13 | **gtp_*** (2-й Goodix-вариант, GT9xx низкоуровневый) | gtp_init_panel, gtp_i2c_read, gtp_reset_guitar, gtp_touch_down, gtp_request_input_dev (25 символов) | Частично: goodix-ядро present (см. §2), но символы `gtp_*` в дереве = 0 → в бинаре 2-й custom GT9xx-вариант (goodix_ts_init6 + goodix_ts_init7) | `drivers/input/touchscreen/gt9xx_mtc.c` (оверлей) |
| 14 | **board-машинный тип (custom)** | machine_rk30_board_init:c040e744, customize_machine:c040a398, rk30_setgpio_suspend_board:c06958f8 | Фреймворк present (arch/arm/mach-rk30 + mach-rk3188, board-rk30-sdk/86v и др.), но конкретного MTC-board-файла нет; точное имя machine нечитаемо из kallsyms (машинная таблица — read-only) | `arch/arm/mach-rk30/board-mtc.c` (выяснить по dtb/machine-name из бинара) |

## 2. PRESENT — есть и в бинаре, и в дереве

| Драйвер | Доказательство (kallsyms) | Где в дереве |
|---|---|---|
| goodix (ядро GT8xx/9xx) | goodix_ts_probe:c083d938, goodix_ts_irq_handler, __initcall_goodix_ts_init6/7 | `drivers/input/touchscreen/{rk29_i2c_goodix.c, goodix_touch.c, goodix_touch_82x.c}` (Makefile: CONFIG_TOUCHSCREEN_GT8XX / GT82X_IIC / D70_L3188A) |
| gtco (USB-таблет) | gtco_init:c041f124, gtco_urb_callback | `drivers/input/tablet/gtco.c` |
| kbtab (USB-таблет) | kbtab_init:c041f184, kbtab_irq | `drivers/input/tablet/kbtab.c` |
| keyreset (USB) | keyreset_init:c041f20c, keyreset_event | `drivers/input/keyreset.c` (+ include/linux/keyreset.h) |
| keytouch (HID) | keytouch_init:c0420b78, keytouch_report_fixup | `drivers/hid/hid-keytouch.c` |
| rknand (MTD) | rknand_init:c041dfe0, rknand_probe, rknand_dma_map_single (53 символа) | `drivers/mtd/rknand/rknand_base_ko.c` (+ Kconfig, source из `drivers/mtd/Kconfig:329`) |
| rk_fb (framebuffer) | rk_fb_probe:c09b9ac4, rk_fb_init, rk_fb_switch_screen (33 символа) | `drivers/video/rockchip/rk_fb.c` (+ rkfb_sysfs.c, lcdc/, transmitter/) |
| rga (2D-ускоритель) | rga_drv_probe:c09b9e90, rga_blit, rga_ioctl | `drivers/video/rockchip/rga/rga_drv.c` |
| cpufreq RK3188 | rk3188_cpufreq_driver_init:c040e644 | `arch/arm/mach-rk3188/cpufreq.c` (символ найден в файле) |
| rkwifi_sysif (WiFi-обвязка) | rkwifi_sysif_init:c0711668, rockchip_wifi_shutdown:c07118d8 | `drivers/net/wireless/wifi_sys/rkwifi_sys_iface.c` (Makefile: `obj-y += wifi_sys/rkwifi_sys_iface.o`) |
| rtl8723 (WiFi-чип) | rtw_* (589 символов), hal_EfuseGetCurrentSize_8723:c07422ec, ReadEFuse_RTL8723:c07426e0 | `drivers/net/wireless/{rtl8723as,rtl8723bs}/` (Kconfig:123-124, Makefile CONFIG_RTL8723BS) |
| snd-soc-rockchip (i2s/pcm) | rockchip_i2s_probe:c09bd550, rockchip_pcm_platform_probe:c09bd540, rockchip_snd_rxctrl | `sound/soc/rockchip/{rk29_i2s.c, rk30_i2s.c, rk29_pcm.c}` |
| rockchip_ion (DMA-буферы) | rockchip_ion_probe:c06c8824 | `drivers/gpu/ion/rockchip/rockchip_ion.c` |
| core rk30 (gpio/cpuidle/adc…) | rk30_gpio_init, rk30_cpuidle_init, rk30_adc_probe (123 символа) | `arch/arm/mach-rk30/` |

## 3. НЕ ЯСНО / ограничения

- **machine-name бинара**: машинная таблица (machine_desc:c042c8e8) — read-only; имя борта (board-mtc? board-rk30-sdk? 86v?) по kallsyms не определяется. Нужен strings/DWARF с бинара или dtb. Фреймворк board в дереве есть; конкретного MTC-борта — нет (ABSENT #14).
- **gtp_\* vs goodix_ts_***: в бинаре зарегистрированы ДВА goodix-инициала (init6+init7) — ядро совпадает с деревом, подмножество `gtp_*` (25 симв.) нет ни в одном драйвере дерева; вероятно второй custom-вариант GT9xx (ABSENT #13).
- **wm8731-codec**: код WM8731 в binaре и дереве один и тот же (`sound/soc/codecs/wm8731.c` present); ABSENT только MACHINE-драйвер rk29_wm8731 (#9).
- **haptic/thermal/tsadc**: в бинарном kallsyms отсутствуют (0 символов) — в ядро не собраны, для оверлея не нужны (tsadc.c в дереве есть, но не используется бинаром).
- **Ethernet**: вендорных eth-драйверов в бинаре нет (только generic phy_* и WiFi) — сетевой стек: только 8723-WiFi + phy-фреймворк.
- Mali/UMP — загружаемые модули (bf-сегмент, `bf0*`), в дереве отсутствуют целиком; оверлей нужен как .ko + исходники.
- `hid-tmff` — в бинаре НЕ найден (0 символов); в дереве есть `drivers/hid/hid-tmff.c` (лишний для данного бинара).

## Итог

- ABSENT-семейств: **14** (в т.ч. 8 mtc-модулей + mtc-common, machine rk29_wm8731, tef6606/radio, mali, ump, gtp-вариант, board-mtc).
- Ключевой блок: **18 вендорных mtc/mtc-смежных** (car, audio-card/rk29_wm8731, radio/tef6606, keys, lcd, backview, tv/tm_, dvd, vs + mtc-common) — ни одного в дереве.
- Все «стандартные» rockchip/инпутовые/MTD/WiFi-драйверы (goodix-ядро, gtco/kbtab/keyreset/keytouch, rknand, rk_fb, rga, cpufreq, rkwifi_sysif/rtl8723, snd-soc-rockchip, rockchip_ion) — present в дереве.
