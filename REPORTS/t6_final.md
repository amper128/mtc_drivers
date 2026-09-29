# T6-r2: финальная re-верификация сборки после T7-фиксов
Входы: ref_kernel/System.map (49474 стр., build 29.09 09:05) + .config; оригинал RK3188/3188_kallsyms (42433 стр., CRLF снят). Дисциплина: awk/grep/sort/comm.

## 1. T6-регрессия — PASS (1 новый дрейф)
- Ранги 1-в-1 (grep System.map/kallsyms __initcall_*): backview=5, car=5s, dvd/lcd/keys/tef6606/tm=6, audio_card=6+7 — идентичный набор в обоих.
- mtc_dvd_driver / mtc_lcd_driver / mtc_keys_driver present (3/3) — T7-фикс подтверждён.
- НОВЫЙ ДРЕЙФ: CONFIG_KEYS_RK29=y → ДВА keys_init (c041e1e0 MTC, c041f9c0 rk29) + ДВА __initcall_keys_init6; в оригинале один. Action: CONFIG_KEYS_RK29=n.

## 2. MTC-полнота — 100/125 (80%)
Набор 125 уникальных имён (regex: mtc_/mtcGet/mtcWipe/tef6606/keys_{probe,isr,init,...}/lcd_/radio_Tef/wm8731/rk29_wm8731/audio_card/dvd_/Tv_/car_/backview/avm_/tm*/__k{str,sym}tab_...). Missing 25, 4 класса root-cause:
 (a) implicit-int/недо-реконструированные — T7 НЕ закрыл: dvd_play_cmd, dvd_send_command_direct, dvd_stop_cmd, dvd_rev(.part.2), lcd_show_symbol, mtc_keycode, mtc_keydefault, mtcWipeCheck, mtc_clear_screen, car_avm, wm8731_trigger (11)
 (b) test/debug-блок: mtc_test_port/2/3, mtc_init_test_io, mtc_debug_put_string/putc, mtc_get_pin_map, mtc_get_screen_{w,h} (9)
 (c) touch/FB-буфер: mtc_touch_work_func, mtc_direct_fb_buf + его __kstrtab/__ksymtab (4)
 (d) артефакт GCC: dvd_poweroff.constprop.10 — базовая dvd_poweroff В сборке есть; naming-артефакт, не дефект (1)
Классификация: (a)-(c) = не-реконструировано (implicit-int root-cause активен); (d) = артефакт; stub/placeholders — нет.

## 3. Общий % по именам
Уникальные имена: orig 40816, build 48756; common 38963 → **95.5% оригинала в сборке**; added 9793 (drift; 20.1% сборки); missing 1853 (4.5%).

## 4. CONFIG-drift ADDED (есть в сборке, нет в оригинале) — топ-15
| семья | символов | our .config | action |
| ftrace/kprobe/tracepoint/event_* | 872 | CONFIG_FTRACE=y | →n (orig off) |
| rk-platform (rk/rk29/rk30/rk1000) | 255 | новая платформа-кода/конфиг-дрейф | аудит |
| cifs | 224 | CONFIG_CIFS=y | →n (orig off) |
| clk_* (API) | 207 | — | version/API-дрейф, не CONFIG |
| udf+isofs | 187 | CONFIG_UDF_FS=y | →n (orig off) |
| sensor* | 131 | SENSORS_*=n (владелец hub-кода не найден) | →n/аудит |
| ddr* | 128 | CONFIG_DDR_FREQ=y, DDR_TYPE_DDR3_DEFAULT=y | →n/аудит |
| videobuf* | 99 | CONFIG_V4L_USB_DRIVERS=y | →n (orig off) |
| rtw/wifi* | 74 | RTL8723AU/BU/AS/BS = not set, явных 8192/8188 нет — драйвер билдится впрямую | найти владельца →n |
| soc* / uvc* / usb_* / scsi* / tcp* / ext4* | 76/55/50/47/47/38 | EXT4_FS=y (ext4) | →n/аудит |
Другое: act8846 36 (ACT8846_SUPPORT_RESET=y→n), audit 29, fiq 25, nf_tables 2.

## 5. REVERSE (есть в оригинале, нет в сборке) — объяснимо config-OFF у нас
| семья | символов | our .config | action |
| mali* | 566 (в сборке 24) | CONFIG_MALI отсутствует | **→=y** (GPU был built-in в orig) |
| wacom 96 + touch(goodix/gtp/tsc2003/aiptek) 46 + ump 41 | 183 | INPUT_TOUCHSCREEN not set; WACOM/GOODIX/TSC2003 отсутствуют | →y (периферия; по решению) |
| backlight* | 41 (в сборке 3) | FB_BACKLIGHT/BACKLIGHT_LCD_SUPPORT not set | →y |
| SDC/SDPAM/SDOAM* | 43 | — | naming-дрейф (Rockchip SDC-API), не config |
| rknand* | 33 missing / 26 в сборке | CONFIG_MTD_RKNAND=y | частичный: имена/API отличаются |
| hdmi* | 23 missing / 122 в сборке | RK_HDMI=y; HDMI_RK3028A not set | вариант-дрейф — сверить включённый HDMI orig |
| hiddev* | 17 | CONFIG_USB_HIDDEV not set | →y |

## 6. ИТОГОВЫЙ ВЕРДИКТ «1-в-1»
- **НЕ 100% «1-в-1»**: по именам 95.5%; MTC-ядро 80% (100/125); drift added = 9793 имени.
- T7: ранги+драйверы mtc_* — 1-в-1, подтверждено; KEYS_RK29 — новый дрейф (dual keys_init).
- Оставшиеся отличии: (1) implicit-int MTC, 24 имени, классы (a)-(c); (2) config-OFF у нас: mali/wacom/touch/backlight/hiddev; (3) naming/вариант-дрейф: SDC/rknand/hdmi/clk; (4) added-drift: ftrace/cifs/udf/ext4/wifi/videobuf/ddr/sensor.
- Пригодность утверждения: «ядро 1-в-1» — НЕТ без оговорок; «MTC-ядро 1-в-1» — НЕТ, пока не закрыты (a)-(c). Допустимое утверждение: **95.5% по именам + таблицы §4/§5 как register of drift**.
