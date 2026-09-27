# T1+T2 status (executor, 2025-09) — deviations & TBD (honest log)

## Sделано
- Working copy ref-ядра: /home/amper/Coding/mtc_build/ref_kernel/ (rsync из ref, .git исключён, 685M). Ref НЕ тронут (read-only).
- Дерево оверлея (все файлы = ПОЛНЫЕ, rsync-ready): drivers/misc/{Makefile,Kconfig} (+1 строка к ref — diff проверен),
  drivers/misc/mtc/{Kconfig,Makefile,car.c,backview.c,tv.c,dvd.c,vs.c,radio_tef6606.c,lcd.c,keys.c,codec.c,car.h,vs.h,shared.h},
  sound/soc/rk29/README.md (T3-заглушка), arch/arm/mach-rk3188/README.md (T4-заглушка),
  scripts/{apply_overlay.sh,build.sh,verify_symbols.sh} (bash -n OK), config/mtc_defconfig += CONFIG_MTC_DRIVERS=y.
- Initcall-уровни 1-в-1 по ref include/linux/init.h (fs=5, device=6) + 3188_kallsyms:
  backview=fs_initcall(5) c042b4e8, car=fs_initcall_sync(5s) c042b500,
  tm/dvd/vs/tef6606/lcd/keys=device_initcall(6) c042ba08/c042ba48/c042ba4c/c042ba50/c042ba54/c042ba58.
- EXPORT_SYMBOL_GPL сохранены без изменений (T2: 11 шт в car/radio/vs) — ВСЕ сверены: T-символы в 3188_kallsyms
  (arm_parrot_boot c082e4cc, arm_send, arm_send_multi, car_add_work*, car_comm_init, radio_send_sta, rds_*, vs_send_raw).
  MODULE_DEVICE_TABLE(i2c, tef6606_id) сохранён (таблица — 't' c0a09cf8 в бинаре). U bf0* = модули — не экспортируются.
- 3.0.36 API: linux/{earlysuspend,wakelock,adc,miscdevice,input}.h в ref-дереве — есть; неподдерживаемого не найдено.

## ОТКЛОНЕНИЯ / TBD (ловит verify_symbols.sh на T6)
1. **Связочный порядок vs бинар**: drivers/Makefile (ref) строка 45 `base/ block/ misc/ mfd/` — misc линкуется РАНЬЕ hid/ (строка 114).
   В бинаре: tm_init6 сидит СРЕДИ HID-tablet драйверов (ga_init6 c042ba04 < tm_init6 c042ba08 < ts_init6 c042ba0c),
   dvd..keys между ch7025_init6/goodix_ts_init6, backview_init5 = ПОСЛЕДНИЙ в .initcall5 (перед net/ipv4), car_init5s — последний 5s.
   => rank-дрейф tail-кластера c042bxxx ОЖИДАЕТСЯ (допуск ±1, скорее всего FAIL по tm). Честная 1-в-1 реплика потребовала бы
   патча drivers/hid/Makefile (tmff.o между ga/ts) и переноса backview/car — ВНЕ scope T1/T2 (решение за T5/T6, PLAN R3).
2. **tm-стабы**: tm_probe/tmff_play = -ENODEV-стабы, tm_devices НЕ реконструирован: decompiled_tm_*.c ссылаются на
   невосстановленные статич. структуры (off_C0BC8AF4, сырые оффсеты). Имена + уровень initcall сохранены.
3. **defconfig MACH**: CONFIG_MACH_RK3188_DS1006H=y (inline-TBD в defconfig) — заменить на MACH_MTC в T4 (board "Trk3188").
4. **Smoke `make drivers/misc/mtc/`**: toolchain = ТОЛЬКО binutils (arm-eabi-gcc ОТСУТСТВУЕТ, GCC 4.6.4 собирается researcher-агентом)
   => полная smoke невозможна; выполнена артефакт-проверка: python-баланс {} () и #if/#endif по всем 9 .c — OK;
   bash -n по 3 скриптам — OK; idempotence apply_overlay.sh (2x rsync, md5) — см. отчёт run.
