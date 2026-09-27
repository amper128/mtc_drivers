# PLAN: оверлей mtc_drivers поверх Nu3001 RK3188 3.0.36 (цель — 1-в-1 бинарного ядра, gcc 4.6)

REF-дерево: /home/amper/tmp/ida-tmp/mtc_audio/ref_kernel_rk3188/ (3.0.36, полная сборка).
Бинар: /home/amper/Coding/RK3188/kernel.elf + 3188_kallsyms (42433 симв., CRLF — `tr -d '\r'`).
Факты: REPORTS/driver_inventory.md (ABSENT=14), config/mtc_defconfig + REPORTS/config_diff.md — ГОВОРЯТ.
Входные исходники binaRE: /home/amper/Coding/mtc_drivers/ (18 ф-ций, rounds 1-3);
декомпиляты машины: /home/amper/tmp/ida-tmp/mtc_audio/src_all/decompiled_{rk29_wm8731_init,audio_card_init,wm8731_*}.c
README mtc_drivers: board-база = board-rk3188-sdk.c; machine-name = "Trk3188" (strings elf ✓); MACH_TYPE — TBD (subtask 4).

## A. ДЕРЕВО оверлея (home = /home/amper/Coding/mtc_drivers, overlay-source для apply_overlay.sh = его drivers/, sound/, arch/, config/; каждый файл = ПОЛНАЯ замена файла ref; rsync → рабочая копия /home/amper/Coding/mtc_build/ref_kernel)
```
/home/amper/Coding/mtc_drivers/    # home = САМО дерево репо (overlay in-repo); контейнер mtc_overlay УПРАЗДНЁН
├── drivers/misc/mtc/                 # НОВОЕ (каталог в ref отсутствует — drivers/misc/Makefile patch-копия + `obj-y += mtc`)
│   ├── Makefile  Kconfig             # obj-y: car.o keys.o lcd.o tv.o dvd.o vs.o backview.o codec.o (все built-in)
│   ├── car.c  keys.c  lcd.c  tv.c  dvd.c  vs.c  backview.c   # из mtc-car/keys/lcd/tv/dvd/vs/backview.c
│   ├── radio_tef6606.c               # mtc-radio.c + tef6606 (tef6606_init + i2c probe, radio_cmd_work)
│   ├── codec.c                       # mtc-codec.c (mtc-common хелперы: mtc_direct_fb_buf/mtc_get_pin_map/...)
│   ├── car.h  vs.h  mtc_shared.h     # из входных; mtc_common-символы сверить с §1.1 инвентаря
├── drivers/media/radio/Makefile      # patch-копия: + tef6606, если radio-файл вынесен сюда (итог subtask 2)
├── sound/soc/rk29/rk29_wm8731.c      # НОВЫЙ machine-драйвер (binaRE; codec wm8731.c в ref есть — НЕ трогать)
├── sound/soc/rk29/{Makefile,Kconfig} # patch-копии: + rk29_wm8731 (obj-y, CONFIG_RK29_WM8731)
├── arch/arm/mach-rk3188/board-mtc.c  # НОВЫЙ (база board-rk3188-sdk.c + binaRE machine_desc; machine "Trk3188")
├── arch/arm/mach-rk3188/{Makefile.boot,Kconfig,include/mach/*.h}  # patch-копии: + MACH_TYPE_MTC, board entry
├── kernel/sys.c                      # patch-копия: kernel_restart → mtc-patch из mtc_drivers/kernel_patches.c
├── arch/arm/mach-rk3188/rk30_pm_*    # (см. kernel_patches.c: rk30_pm_power_off → arm_send(0x755)) — точный путь по ref
├── config/mtc_defconfig  REPORTS/{driver_inventory.md,config_diff.md}
└── scripts/{apply_overlay.sh, build.sh, verify_symbols.sh}
```
Init-порядок (ФАКТ, адреса записей .initcall в 3188_kallsyms = порядок вызова):
customize_machine(3, c042b3c8) → backview_init5 (c042b4e8) → car_init5s (c042b500) →
tm_init6 (c042ba08) → dvd_init6 (c042ba48) → vs_init6 (c042ba4c) → tef6606_init6 (c042ba50) →
lcd_init6 (c042ba54) → keys_init6 (c042ba58) → wm8731_modinit6 (c042ba74) → audio_card_init6 (c042ba80)
→ audio_card_init7 (c042bcf0). ⇒ В ovl: backview/car = `subsys_initcall_sync`, остальное = `device_initcall`;
audio_card — ДВЕ регистрации (ур.6: rk29_wm8731 machine; ур.7: mtc-audio c0421de8/c0421ec4 — два initcall'а в разных TUs).
Порядок в .initcall одного уровня = порядок линковки объектов ⇒ Makefile mtc/ обязан перечислять .o именно в
порядке tm,dvd,vs,tef6606,lcd,keys (subsys_sync car/backview — раньше), иначе сдвинется весь tail-кластер.

## B. СТРАТЕГИЯ БУИЛДА — ВЫБОР: полная сборка (прицельная НЕ даёт вердикт 1-в-1)
- Верификация 1-в-1 = diff kallsyms ВСЕЙ сборки vs 3188_kallsyms (имена+адресные кластеры).
  `make drivers/misc/mtc` по объектам не даёт .initcall/адресов — ОТКЛОНЯЕТСЯ как вердикт (допустима лишь
  smoke-проверка компиляции в subtask 2: `make drivers/misc/mtc/`).
- Пайплайн: (1) baseline — сборка ЧИСТОГО ref с mtc_defconfig без оверлея (30+ мин) = контрольная точка
  "toolchain+defconfig воспроизводят ~baseline"; (2) rsync оверлея → сборка; (3) verify_symbols.sh:
  нормализация CRLF, set-diff имён (цель: Δ = 0 mtc-семейств), кластерный анализ адресов: mtc-init-ф-ции
  в c0421xxx, .initcall c042bxxx, probes c09bbxxx, data c0a0xxxx — допуск ±1 страница внутри кластера;
  точное совпадение адресов НЕ гарантировано (gcc 4.6.4 vs 4.6.x-google, порядок сборки) ⇒ метрика:
  100% имён + сохранение порядка и кластерности, отчёт REPORTS/kallsyms_diff.md.
- build.sh: `make ARCH=arm CROSS_COMPILE=$TC/arm-eabi- olddefconfig по config/mtc_defconfig` (copy -s в
  WORKDIR-копии ref, НЕ в ref!), цели `vmlinux`; параллелизм -j$(nproc); ccache опц.

## C. SCOPE-ГРАНИЦЫ
ВХОДИТ: built-in mtc-драйверы (car/keys/lcd/tv/dvd/vs/backview/radio+tef6606/codec+common),
rk29_wm8731 machine, board-mtc.c (machine "Trk3188", i2c0-2, rk29sdk_wifi_* — сверить с §2 инвентаря:
rkwifi_sysif PRESENT → board только gpio/clk), mtc_defconfig, 5 скриптов/отчётов.
НЕ входит (плейсхолдеры + отметка в REPORTS): mali, ump, rk30xxnand-FTL (модули bf0* — bin-модули,
оверлей не нужен для vmlinux-kallsyms), gtp_9xx-вариант (инв. §1.13 — отдельный follow-up), haptic/thermal
(отсутствуют в бинаре).

## D. ПОДЗАДАЧИ (executor), порядок и критерии приёмки
1. T1 дерево+Kconfig+Makefile+scripts (нет завис.) →
   КРИТЕРИЙ: дерево = §A; apply_overlay.sh идемпотентен (2x rsync → одинаковый md5-суммарный); каждый
   patch-файл (drivers/misc/Makefile, sound/soc/rk29/{Makefile,Kconfig}, mach-rk3188/Makefile.boot/Kconfig,
   drivers/media/radio/Makefile) = полный файл из ref + ровно нужные строки (diff ≤10 строк к ref);
   `olddefconfig` по mtc_defconfig проходит на оверлей-копии без ошибок.
2. T2 адаптация 18 f-ций mtc_drivers под 3.0.36 (headers/EXPORTS) — зависит T1 →
   КРИТЕРИЙ: `make drivers/misc/mtc/` = 0 ошибок 0 предупреждений -Werror-экв.; все 27 EXPORT_SYMBOL
   (binaRE) присутствуют; T-символы kallsyms (car_ioctl, dvd_power, dvd_*, lcd_show*, mtc_key*, vs_*,
   tm_devices, backview_id, tef6606_id...) совпадают 1-в-1 по именам; initcall'ы с уровнями из §A.
3. T3 binaRE rk29_wm8731.c (+ audio_card_init мtc-audio ур.7) — зависит T1; входы: src_all/decompiled_* →
   КРИТЕРИЙ: `make sound/soc/rk29/` чисто; имена rk29_wm8731_init/audio_card_init/dai-ops совпадают с
   kallsyms; звук не проверяется — только символы (аппарат нет).
4. T4 board-mtc.c: machine_desc "Trk3188", MACH_TYPE, init i2c0-2, wifi-gpio — зависит T1 (+T2 для gpio-enum) →
   КРИТЕРИЙ: Kconfig/Makefile.boot обновлены; board init-ф-ции присутствуют в kallsyms сборки;
   MACH_TYPE зафиксирован (если не читаем из elf — IDA/strings dump, решение зафиксировать в REPORTS);
   machine_rk30_board_init/customize_machine/rk29sdk_wifi_* — имена совпадают с 3188_kallsyms.
5. T5 toolchain-интеграция + полная сборка (зависит T1-T4; toolchain-агент = researcher 304597…,
   GCC 4.6.4 build in progress — blocker) → КРИТЕРИЙ: baseline-сборка чистого ref УСПЕШНА; сборка с
   оверлеем доходит до vmlinux; log-и в REPORTS/build_{baseline,overlay}.log.
6. T6 верификация kallsyms (зависит T5) → КРИТЕРИЙ: verify_symbols.sh: Δ-имён по ABSENT-14 семействам = 0
   отсутствующих; общий set-diff ≤ 0.5% (доп = build-id/stamp); порядок .initcall из §A сохранён;
   отчёт REPORTS/kallsyms_diff.md с кластерной таблицей.

## РИСКИ
R1: gcc 4.6.4 ≠ 4.6.x-google (непубличный) — codegen-диффы ⇒ адресные сдвиги; метрика 1-в-1 = имена+кластеры
   (B), полный byte-for-byte НЕ обещать. R2: MACH_TYPE/board init нечитаемы из kallsyms → IDA/strings;
   fallback: board-rk3188-sdk.c как база (подтверждено README mtc_drivers). R3: порядок .initcall задан
   порядком линковки — ошибка в obj-порядке T2 ломает tail-кластер (сдвиг c042bxxx) → verify_symbols ловит.
R4: двойные init audio_card (ур.6+7, два c0421xxx) — два TU с разными инициалами; риск конфликта имён
   audio_card_init → переименование недопустимо (kallsyms!), решается static/weak в T3. R5: mali/ump не в
   vmlinux — их отсутствие НЕ портит kallsyms-diff vmlinux (проверяем только vmlinux-символы).
R6: time-box: полная сборка 30+ мин × итерации → T5/T6 закладывать ≥3 итерации; ccache обязателен.
