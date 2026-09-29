# T7 — фиксы T6 + пересборка: DONE

## Фиксы (/home/amper/Coding/mtc_drivers)
- dvd.c:1198, lcd.c:461, keys.c:829: `module_platform_driver(x)` → явная регистрация по SDK-конвенции:
  `static int __init <dvd|lcd|keys>_init(void){ platform_driver_register(&mtc_<x>_driver); return 0; }` +
  `<x>_exit` + `module_init(<x>_init); module_exit(<x>_exit);` (имена dvd_init/lcd_init/keys_init = как в оригинальном бинаре).
- car.c:5594: `device_initcall` → `fs_initcall_sync(car_init)` (ранг "5s"); backview.c:1695: → `fs_initcall(backview_init)` (ранг "5"). device_initcall(6) с обоих снят.
- Порядок кластера tm→dvd→vs→tef6606→keys сохранён; python-replace, count==1 на якорь.

## SDK-конвенция (референсы)
- include/linux/platform_device.h (3.0.36): module_platform_driver/__platform_driver отсутствуют → явная platform_driver_register.
- Референс класса: drivers/input/keyboard/rk29_keys.c:582-593 (static int __init keys_init + module_init).
- include/linux/init.h:204-212: fs_initcall="5", fs_initcall_sync="5s", device_initcall="6"; в бинарной сборке module_init(fn)→6.

## Сборка
- scripts/apply_overlay.sh — OK; scripts/build.sh (smoke) — green: 11 .o + board-mtc.o.
- FULL=1 scripts/build.sh — завершён: tail build1.log = `SYSMAP .tmp_System.map`, grep Error/error:/undefined = 0.

## Recheck (ref_kernel/System.map)
- __initcall_backview_init5 / __initcall_car_init5s / __initcall_dvd_init6 / __initcall_lcd_init6 / __initcall_keys_init6 — ранги 1-в-1 с оригиналом.
- Присутствуют: mtc_dvd_driver, mtc_lcd_driver, mtc_keys_driver.

## Для коммита (devops)
drivers/misc/mtc/{dvd,lcd,keys,car,backview}.c (+ REPORTS/t7_status.md)
