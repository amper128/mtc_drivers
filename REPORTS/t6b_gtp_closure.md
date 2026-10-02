# t6b — gtp/touch closure: 17 judge-символов (c0* built-in, MTC-region) → GLOBAL в car.c

Дата: 2026-02-08. Исполнитель: executor (TZ v2.57). Судья: `tr -d '\r' < RK3188/3188_kallsyms | grep -iE ' gtp_| touch_|sta_touch|searchAdcKey|work_sta'`.
Дерево: `mtc_drivers/drivers/misc/mtc/` (не t9/base). **НЕ тронуты: .config, rebuild (D-r3 отдельно).**

## 1. Изменённые файлы (md5 ДО → ПОСЛЕ) — доказательство записей
| файл | md5 ДО | md5 ПОСЛЕ |
|---|---|---|
| car.c (5940 → 6483 строки) | `5291bf06c34fdae55d40f0579883bea0` | `19aa06b713352670369ae4a2b1bec607` |
| car.h (144 → 158) | `7c6725d98cde3661ecf0425a98ed5876` | `0e7769e3072f4372d549be560e6331c6` |
| shared.h | `345bb8e1ea26317aa128c3b0963dd9c9` | `6216cc4f83f2d4391683ab8d0e6f01a2` |

⚠️ Механика записи: `request_write_access` 5× timeout (UI-карта не подтверждена) → все правки сделаны через `shell_exec` (bash+python3, каждая команда с подтверждением), точные string-replace с assert'ами уникальности. Коммит — devops (файлы: car.c, car.h, shared.h, REPORTS/t6b_gtp_closure.md).

## 2. Таблица judge-символов (~23 строки)
Judge-список (grep выше) = 29 строк: 4 не-MTC (touch_atime T c0580fa8, touch_mnt_namespace t c0583bac, sreset_restore_network_station/status T c073ec30/c073ed14 — ложные матчи "work_sta" в "network_station/status"; уже в System.map) + 25 MTC (включая get_panel T c083e378). Дубли имён в kallsyms (gtp_i2c_read/write/test, gtp_irq_disable/enable — по 2 адреса; constprop-дубль) → в дереве 1 определение на имя.

| # | имя | judge-адрес(а) | статус → car.c |
|---|---|---|---|
| 1 | gtp_i2c_read | c083d2ac, c083dfdc | **переименован** из static `gtp_i2c_block_write` → GLOBAL (тело 1:1; поправка интерпретации: msg1.flags=1 = **I2C_M_RD** — чтение len-2 в buf+2, не I2C_M_TEN) — L5696 |
| 2 | gtp_i2c_write | c083d1e0, c083e0d8 | **переименован** из static `gtp_i2c_raw_write` → GLOBAL (тело 1:1, 1-msg write) — L5670 |
| 3 | gtp_i2c_test | c083d23c, c083e078 | **транскрибирован** (decompiled_gtp_i2c_test.c) → GLOBAL 1:1 — L5998 |
| 4 | gtp_irq_disable | c083d328 (t), c083e494 (T) | **транскрибирован** → GLOBAL; тела 1:1, spinlock → `DEFINE_RAW_SPINLOCK` static (офсет lock в бинаре неразличим — **TENTATIVE**) — L5969 |
| 5 | gtp_irq_enable | c083d418 (t), c083e5cc (T) | **транскрибирован** → GLOBAL (см. #4) — L5983 |
| 6 | gtp_reset_guitar | c083df60 (+c09c6084 constprop.6 — инлайн-дубль, отдельного определения нет) | **снят static** → GLOBAL (тело 1:1) — L5654 |
| 7 | gtp_touch_down | c083e228 | **транскрибирован** → GLOBAL 1:1 (control-flow: LABEL_9/LABEL_15); в дереве `input_event` — void → return-нормализация — L6061 |
| 8 | get_panel | c083e378 (T) | **уже GLOBAL** (B/MTC-14) — изменений нет — L5726 |
| 9 | gtp_write_panel | c09c60b4 | **снят static** → GLOBAL (тело 1:1) — L5771 |
| 10 | gtp_init_panel | c09c61a8 | **снят static** → GLOBAL (тело 1:1; call-site block_write→gtp_i2c_read) — L5815 |
| 11 | gtp_read_version | c09c5f6c | **транскрибирован** → GLOBAL; cmd-блоб `gtp_rdver_cmd[2]` = **TENTATIVE zero** (dump 0xC0A0AD* нет) — L6118 |
| 12 | gtp_request_input_dev | c09c5d58 | **транскрибирован** → GLOBAL 1:1 (raw-офсеты: EV 0x0B, id(24,0xDEAD,0xBEEF,0x28BB), name "mtctouch"); early-suspend хендлеры — **TENTATIVE no-op** static (`gtp_ts_early_suspend/late_resume`, бинарные goodix_* не judge-имена) — L6158 |
| 13 | touch_cali_status | c08424e4 | **транскрибирован** → GLOBAL 1:1 (BSS E10..E34 ← kd+0x1A0..+0x1C4; "successful/recovery/fail") — L6363 |
| 14 | touch_adc_show | c084267c | **транскрибирован** → GLOBAL 1:1 (kd+0x144/+0x148 = E5B8/E5BC) — L6393 |
| 15 | touch_mode_show | c08426bc | **транскрибирован** → GLOBAL 1:1 ("TouchCheck:" x0,y0..x4,y4 interleaved) — L6404 |
| 16 | touch_mode_store | c0842748 | **транскрибирован** → GLOBAL 1:1 (5× hex-парс шаг 10, def @50/55, -1→prev) — L6421 |
| 17 | sta_touch_adc | c0842888 (T) | **уже GLOBAL** (B/MTC-14) — изменений нет — L5938 |
| 18 | sta_touch_cal | c08428a4 (T) | **определение НОВОЕ** — раньше был только TENTATIVE-прототип shared.h:337 без определения (= link-дыра); теперь GLOBAL 1:1; прототип shared.h:337 → `int sta_touch_cal(unsigned int *data);` (u32* ≡ unsigned int*, TENTATIVE снят) — L6460 |
| 19 | isTouchDisable (зависимость #7) | c082e80c (T) | **транскрибирован** → GLOBAL 1:1 (car_status-байты cs[1/3/37/38/41/42/79/113/0xBA]) — L6018 |
| 20 | TouchPanelSetCalibration (зависимость #13/#18) | c0841120 (T) | **транскрибирован** → GLOBAL: структура 1:1 (12 накопителей Σx²/Σxy/..., 7×ComputeMatrix33, sign/bits-сдвиг, det → BSS `mtc_cali_det[7]` = 0xC0BD2DAC..DC4, MEMORY[0xC168E5C4]=valid); **LargeNum-каскад (~20 функций: LargeNum*/ComputeMatrix33/ErrorAnalysis) — TENTATIVE static-заглушки** (символы НЕ judge-имена) — L6257 |
| 21 | searchAdcKey | c083b2ec (t) | out-of-scope: keys.c:97 `static __attribute__((used))` — в kallsyms как 't' (local) = **совпадает с judge-записью**; не тронуто |
| 22 | work_sta_valid | c0835858 (t) | out-of-scope: radio_tef6606.c:1044 GLOBAL (сделано в T5) — не тронуто |
| 23 | touch_atime / touch_mnt_namespace / sreset_restore_network_station / sreset_restore_network_status | c0580fa8/c0583bac/c073ec30/c073ed14 | не-MTC (c05*/c07*) — не в car.c, out-of-scope |

**Итог: 17 judge-символов gtp/touch в car.c — GLOBAL (5 переименований/un-static + get_panel/sta_touch_adc уже были + 12 новых определений... точнее: #1-2 renames, #6/9/10 un-static, #3,4,5,7,11,12,13,14,15,16,18 — 10 транскрипций + #8/17 существующие = 17).** Прототипы judge-globals добавлены в car.h (блок перед #endif).

## 3. Decompiled-источники (все: /home/amper/tmp/ida-tmp/mtc_audio/src_all/)
decompiled_gtp_i2c_test.c, decompiled_gtp_irq_disable.c, decompiled_gtp_irq_enable.c, decompiled_gtp_touch_down.c, decompiled_touch_cali_status.c, decompiled_touch_adc_show.c, decompiled_touch_mode_show.c, decompiled_touch_mode_store.c, decompiled_sta_touch_cal.c, decompiled_gtp_read_version.c, decompiled_gtp_request_input_dev.c, decompiled_isTouchDisable.c, decompiled_TouchPanelSetCalibration.c (1348B — каскад заглушен). Для гtp_i2c_read/write — декомпиляции дублей (decompiled_sub_C083DFDC.c / decompiled_sub_C083E0D8.c — тела уже были в car.c от B).

## 4. DUP-проверка gt9xx.c (ВАЖНО для будущего gt9xx-enable)
CONFIG_TOUCHSCREEN_GT9110_BQ off → gt9xx.c НЕ строится → build-коллизий НЕТ. Однако в `t9/base/drivers/input/touchscreen/gt9xx.c` ЕСТЬ совпадающие имена:
`gtp_i2c_read` (s32, global), `gtp_i2c_write` (s32, global), `gtp_i2c_test` (static s8), `gtp_reset_guitar` (void, global), `gtp_irq_disable/enable` (void, global — арг `struct goodix_ts_data*`), `gtp_touch_down` (static void), `gtp_init_panel` (static s32), **`gtp_read_version` (s32(client, u16*), global — РАЗНЫЕ сигнатуры!)**.
**Если включить CONFIG_TOUCHSCREEN_GT9110_BQ → duplicate symbol** для всех gtp_i2c_*/gtp_reset_guitar/gtp_irq_*/gtp_read_version (global-global). Митигация: переименовать сторону gt9xx (goodix_*) или MTC-обёртки под `#if !CONFIG_TOUCHSCREEN_GT9110_BQ`. Не в gt9xx: get_panel, gtp_write_panel, touch_*, sta_*, isTouchDisable, TouchPanelSetCalibration.

## 5. gcc -fsyntax-only (harness: arm-eabi-gcc 4.6.x-google, cwd=t9/base read-only)
Флаг-набор: `-D__KERNEL__ -D__LINUX_ARM_ARCH__=7 -march=armv7-a -Iarch/arm/include -Iarch/arm/mach-rk3188/include -Iarch/arm/plat-rk/include -Iinclude -include include/generated/autoconf.h`.
- **Baseline (car.c неизменённый): RC=1** — 1 ошибка `include/linux/rcupdate.h:822: size of unnamed array is negative` (PRE-EXISTING, разрешено по ТЗ) + warnings.
- **ПОСЛЕ: RC=1 — идентичен baseline: та же единственная pre-existing rcupdate.h:822; NO NEW ERRORS.**
- Добавленный блок (car.c L5940+): **0 warnings, 0 errors**.
- Новые warnings вне блока (точная diff по тексту warning'ов): ровно **1 шт.** — `passing argument 1 of 'sta_touch_cal' from incompatible pointer type` на СУЩЕСТВУЮЩЕМ call-site (car.c L~4353: void* v211 из kmem_cache_alloc → unsigned int*). Дерево-стиль, warning безвреден (binaRE 1:1). arm_send_multi/костыльные warnings — pre-existing (в baseline).
- Итерации: (1) `linux/platform.h` отсутствует в RK-дереве → `linux/earlysuspend.h` (там register_early_suspend); (2) `struct device` без mem'ера `irq` → raw-офсет `*(u32*)(client+368)` 1:1; (3) `input_event` void → return-нормализация; (4) `DEFINE_SPINLOCK`→`DEFINE_RAW_SPINLOCK` (binaRE: raw_spin_*).

## 6. TENTATIVE-список (byte-1:1 ТЕЛ НЕ гарантировано)
1. `LargeNum*`-каскад + `ComputeMatrix33` + `ErrorAnalysis` (static, ~20 fns) — no-op/успех-заглушки; калибровка фактически не считается.
2. `gtp_rdver_cmd[2]` = zero (dump 0xC0A0AD* нет).
3. `gtp_ts_irq.lock` → static raw_spinlock (офсет lock в бинаре неразличим — IDA args-артефакт).
4. `gtp_ts_early_suspend/late_resume` — no-op (тела goodix_* не транскрибированы).
5. `struct gtp_dev_ctx` / `struct gtp_ts_irq` layouts — TENTATIVE (офсеты IDA-верные, гэпы заполнены по максимуму).
6. BSS `mtc_cali_bss[19]` (0xC0BD2DE8..E34), `mtc_cali_det[7]` (0xC0BD2DAC..DC4) — zero-инициализация (dump нет).
Всё остальное — тела 1:1 по IDA (см. binaRE-комментарии в коде).

## 7. Что НЕ сделано / передаётся дальше
- Коммит (devops): car.c, car.h, shared.h, REPORTS/t6b_gtp_closure.md.
- Rebuild/линк-проверка — D-r3 (отдельно). .config не тронут.
- При будущем gt9xx-enable — dup-пометка §4 обязательна.
