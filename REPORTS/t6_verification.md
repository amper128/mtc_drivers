# T6. Верификация 1-в-1 сборки (READ-ONLY, 29.09)

## 1. Входы / баннер
- System.map: 49415 строк; vmlinux: 10986530 B ✓
- Banner: `Linux version 3.0.36+ (amper@localhost) (gcc 4.6.x-google 20120106) #3 SMP PREEMPT` — версия 3.0.36+ OK; host=amper@localhost (отличен от оригинального — зафиксировано, ожидаемо).

## 2. MTC-символы (134 имён реф-набора)
- Присутствуют **99/134 (74%)**, тип+адрес сверены (mtc.table).
- Ключевые задачи — ВСЕ на месте: car_init c041e178, audio_card_init c042267c, backview_init c06c7f24, keys_probe c09baec4, tm_init/tm_probe/tm_devices, tef6606_i2c_probe c06cd4d4, mtcGetSetVolume c06cb670, rds_af_process c06ced54, Af_Check c06cf9ec.
- Отсутствуют **35**: dvd_* 13, lcd_* 6 (init/probe/pm_ops/suspend/resume), mtc_* 15 (debug/test/io: mtc_test_port[2,3], mtc_keycode, mtc_keydefault, mtc_clear_screen…), car_avm, mtc_touch_work_func.

## 3. SET-DIFF (имена, sort -u + comm)
- ref 40816 / new 48705 / common **38932 → 95.4%** имён оригинала.
- missing **1884**: bf0*-модули (U) 1124 (mali/ump/rknand — ожидаемо, не в ядре); не-модулей 760: wacom_* 50, touch-семьи ~67 (hiddev 11, aiptek 9, tsc2003/hanwang/acecad/kbtab/ch7025/ipp), backlight/battery ~30, dvd_* 13 + lcd_* 6 + mtc_* 15 (root-cause п.6).
- added **9773** = CONFIG-drift вне scope (event_* 441, dev_*, cifs, udf, net*), не дефект реконструкции.

## 4. CLUSTER initcall (хвост)
- REF: tm_init < dvd_init < car_init < lcd_init < backview_init < audio_card_init×2 — один блок c0420fe8–c0421ec4.
- NEW: car_init c041e178 < tm_init c04217d0 < audio_card×2 c042267c/c042270c; backview_init ушёл в c06c7f24 (вне блока); dvd_init/lcd_init — отсутствуют.
- Расхождения: порядок **car↔tm** (ref: tm→car), **backview уровень 5/5s → 6**, dvd/lcd вылетели из initcall.

## 5. EXPORT-спотчек (T = export-кандидаты)
- MTC-T-набор: **36/53 (79%)**; все 17 отсутствующих — тот же root-cause (p5/p6). mtcGetSetVolume/rds_af_process/Af_Check — T на месте ✓.

## 6. Root-cause отсутствий
- dvd.c:1198 / lcd.c:461 / keys.c:829: `module_platform_driver(...)`; макрос **не определён в include/** (grep пуст) → implicit declaration, макрос НЕ раскрывается → драйвер не регистрируется → `--gc-sections` вырезает все ссылающиеся секции (исчезают и T, и static).

## 7. ВЕРДИКТ
- Совпадение имён с оригиналом: **95.4%**; MTC-ядро: 99/134, все расхождения — от ОДНОГО root-cause + ожидаемые U-модули.
- «1-в-1» по именам **утверждать нельзя** (прогалы dvd/lcd/keys + CONFIG-drift); по адресам — сдвиг ожидаем (не-модули, drift). Пригодно: ядро+MTc-core, после чинки.
- Чинить: (1) явная регистрация dvd/lcd/keys (определить/включить module_platform_driver); (2) уровни device_initcall: car/backview 5s→5, порядок car↔tm; (3) defconfig-drift (wacom/touch/backlight/battery).
