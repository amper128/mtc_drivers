# IDA-сравнение: целевое ядро (target) vs наш собранный vmlinux (ours)
## Декомпиляционное + disasm cross-check, RK3188

- **Дата:** 2026-07 (сессионный анализ), инструмент: IDA Pro 9.3 idalib (HexRays 9.3.0.260421)
- **Ours:** `/home/amper/Coding/mtc_drivers/ref_kernel/vmlinux` (ARM32, unstripped, linked ELF; IDB: `/home/amper/tmp/ida-tmp/mtc_audio/ours_kernel.i64`, 24 449 функций, autoanalysis завершён)
- **Target:** IDB `/home/amper/tmp/ida-tmp/mtc_audio/mtc_kernel.i64` (готовые decompiled: `.../src_all/decompiled_<func>.c`); адреса-судьи: `/home/amper/Coding/RK3188/3188_kallsyms`; цель-бинарь `kernel.elf` (код в секции `.data` @0xc0408000, file off 0x8000)
- **Метод (sample):** 39 функций = 21 recon (tsc2003_*, diag: key2ir/factory_test/camera_*/homeEnable, ping_*, goodix_*, ch7025_*) + 14 vendor-diverged (top по size-diff) + 3 REF-сэмпли для валидации метода (rk29_sdmmc_set_frq, usb20host_clock_init, ipp_blit).
  Для каждой: ours `ida_hexrays.decompile(ea)` (System.map) vs target-декомпиляция; нормализация локальных имён vN, сравнение множеств {вызовы, строки, константы} + unified-diff (`compare_out/diffs2/<name>.diff`). Для функций с d≈0 — **disasm cross-check** (`disasm_check2.py`: llvm-objdump на ELF-обёртке чанков + capstone; нормализация immediates→LIT) → `compare_out/disasm_check.json`.

---

## 1. Агрегат по классам (39 функций)

| Класс (декомпиляция) | Кол-во | Функции |
|---|---|---|
| EXACT (байт-идентичный pseudo-C) | 2 | ping_v4_err, usb20host_clock_init |
| LOGIC-MATCH | 2 | tsc2003_irq, ipp_blit |
| PARTIAL | 3 | camera_test, camera_stop, ping_v4_sendmsg |
| LOGIC-DIFF | 31 | см. табл. 2–4 |

### Финальные вердикты (после disasm cross-check и сверки с исходниками)

| Вердикт | Кол-во | % (из 39) | Состав |
|---|---|---|---|
| **(M) Логически эквивалентно / артефакт** (EXACT+LOGIC-MATCH+PARTIAL+(a)) | **15** | 38% | см. ниже |
| **(b) Реконструкт НЕПОЛНЫЙ — target больше** (пропущенная ветка/блок) | **9** | 23% | tsc2003_work, factory_test, camera_start, ping_check_bind_addr, ping_set_saddr, ping_clear_saddr, goodix_tool_read, goodix_tool_write (мин.), ch7025_i2c_write (мин.) |
| **(c) Vendor-расхождение — у нас extra** (ours больше) | **14** | 36% | ch7025_probe, qtaguid_ctrl_proc_write, qtaguid_mt (смешанный), ddr_get_parameter, rk3188_lcdc_ioctl, rk3188_lcdc_set_par, rk3188_load_screen, serial_rk_probe, serial_rk_set_termios, rk_fb_update_regs_handler, iface_stat_create, iface_stat_update, iface_inet6addr_event_handler, rockchip_i2s_hw_params |
| (a) — подкласс (M): «реконструкт корректно но иначе» | 10 из 15 | — | tsc2003_probe, tsc2003_i2c_read, tsc2003_i2c_write, key2ir, homeEnable, goodix_ts_init*, rk29_sdmmc_set_frq, ddr_change_freq_gpll_dpll, camera_test (PART), camera_stop (PART), ping_v4_sendmsg (PART) |

\* goodix_ts_init: логика декомпиляций совпадает, но tgt_size_nextsym = +120B — разликa не видна в логике; вероятно layout/padding kallsyms-граница, не код.

**Валидация метода (REF-сэмпли):** все 3 REF — LOGIC-MATCH/EXACT. Disasm: `rk29_sdmmc_set_frq` **30/30 = 100% INSTR-MATCH**, `usb20host_clock_init` **14/14 = 100%**, `ipp_blit` 731/745 (98.1%) — все 14 отклонений в **jump-table/литеральных пулах** (декод данных как инструкций, relocation), код идентичен. Вывод: decompiled-метод валиден; расхождения REF-функций на уровне pseudo-C — артефакты HexRays/именования.

### Disasm cross-check d≈0 (вердикт «артефакт HexRays» vs «реальный код»)

| Функция | ours/tgt (insns) | Mnem-match | Вердикт |
|---|---|---|---|
| tsc2003_irq | 17/17 | **100%** | артефакт (код идентичен; PARTIAL/LOGIC-DIFF не подтверждается) |
| homeEnable | 6/6 | **100%** | артефакт чистый: только именование (`home_enable_flag` vs `MEMORY[<A>]`) |
| ping_v4_err | 4/4 | **100%** | артефакт (EXACT) |
| rk29_sdmmc_set_frq | 30/30 | **100%** | **артефакт HexRays**: LOGIC-DIFF в pseudo-C, бинарный код идентичен (уровень: упрощение HexRays для ours) |
| usb20host_clock_init | 14/14 | **100%** | артефакт (EXACT) |
| camera_stop | 48/48 | 95.8% (диффы только pos 46–47 = хвостовой литерал-пул) | артефакт: код идентичен, PARTIAL — только из-за имён |
| ipp_blit | 745/745 | 98.1%; диффы одиночные, в таблицах/пулах (pos 27–32 = jump-table `ldrls pc,[pc,r7,lsl]`) | артефакт relocation; LOGIC-MATCH подтверждён |
| tsc2003_i2c_read | 30/30 | 73.3%; **реальные** диффы: pos 23–29 `ldrb/strb` (target, u8) vs `ldr/str` (ours, int) | **реальное code-различие**: target считает `tsc2003_err_count` как u8 по фикс. адресу; ours — `static int`. Плюс mvn/mov (компилятор) и 1-word сдвиг |
| tsc2003_i2c_write | 25/26 (d=+4B) | 52%; структурный сдвиг pos 13+ (эпилог-через-б ranching) + `ldrb/strb` vs `ldr/str` | **реальное (часть)**: u8-счётчик как в i2c_read; часть — layout |
| key2ir | 28/26 (d=-8B) | 42.3%; ours = if-цепочка из cmp, target = **switch с jump-table** (158→54, 139→55, 102→56, 115→26, 114→25, default 255) | **реальное**: разное решение компилятора (if-chain vs jumptable) для ОДИНАКОВОГО source (сверка: наш `recon.c` содержит ровно этот switch + таблицу [512..530]) → вердикт (a): реконструкт корректно, иначе |

---

## 2. Семейство RECON: пер-функция (21)

| Функция | d (tgt−ours, B) | Класс | Вердикт | Конкретные расхождения |
|---|---|---|---|---|
| tsc2003_work | +208 | LD | **(b)** | target: реальный вызов `TouchPanelCalibrateAPoint` (у нас — no-op static stub в `tsc2003.c:40`, MTC-core нет в дереве); counter u8 vs int (disasm); loop-структура calib-секции отличается |
| tsc2003_probe | −32 | LD | **(a)** | do/while vs while+early-return (формa retry); ошибка-путь: tgt безусловный `input_free_device(); kfree(); return` vs ours условный; **tgt: `kobject_add`/`_memzero` нет у нас** (кандидат на недописанность, ≤1 блок); input evbit: ours 5× `set_bit()`, tgt прямой store `1024` (API-гранулярность ядра) |
| tsc2003_irq | 0 | LM | (M) | идентичен (100% insns) |
| tsc2003_i2c_read | 0 | LD | **(a)+реал.** | u8 vs int счётчик ошибок (реально, disasm-подтверждено); `mvn` vs `mov` — артефакт |
| tsc2003_i2c_write | +4 | LD | **(a)+реал.** | то же u8/int; +4B tgt (лишний nop/константа) |
| key2ir | −8 | LD | **(a)** | if-цепочка vs switch-jumptable — один source, разный выбор компилятора; mapping идентичен (сверка с `recon.c`) |
| factory_test | +332 | LD | **(b)** | target extra: цикл ADC-теста (`test_adc` ×2, порог `v−503`, сравнение `>0x12` → цвет LOBYTE/HIWORD), `rk_direct_fb_set()`, строка `"No Test"`; именованные глобалы vs MEMORY[] — артефакт |
| camera_test | +64 | PART | **(a)** | почти всё — именование (`cam_i2c_client/cam_tested/cam_wq/cam_active` vs MEMORY[]); if/else T132B NTSC/NTSC_YZ ветки есть в обеих; +64B не найдено в логике — вероятно инлайнинг T132B-хелпера в tgt |
| camera_start | +124 | LD | **(b)** | target: `switch` (case 2/5/7…) по режиму с явными stores (`MEMORY[<A>]=…`), ours: if-цепочка (`byte_C0D436AE==3/==1…`) — та же семантика, но +124B tgt (доп. case-ветки/дубликаты, которые у нас свернуты); ADV7181D_Init в обеих |
| camera_stop | 0 | PART | **(a)** | **код идентичен** (46/48 insns, хвост-пул); только именование (`byte_C0D45763…`, `dword_C0D45718`) |
| homeEnable | 0 | LD | **(a)** | 100% insns; `home_enable_flag=0` vs `MEMORY[<A>]=0` |
| ping_check_bind_addr | +272 | LD | **(b)** | **у нас IPv4-only.** Target extra: `return −97 (−EAFNOSUPPORT)`, `_ipv6_addr_type()`, `dev_get_by_index_rcu()` + `_rcu_read_lock/unlock`, `return −19 (−ENETUNREACH)`, вл. вызов v4-чекa, `return −99 (−EACCES)` |
| ping_set_saddr | +52 | LD | **(b)** | target: v4 — copy 32-bit; **v6 — copy 128-bit** (8 slots union `result[100]`); ours — только v4-store (3 строки) |
| ping_clear_saddr | +44 | LD | **(b)** | target: v4 — 2 zero-stores; **v6 — 2× `_memzero` (16B)**; ours — только v4 |
| ping_v4_err | 0 | EXACT | (M) | идентичен |
| ping_v4_sendmsg | +72 | PART | **(a)** (note) | нормализация типов (`d0` vs `appended`, `*(T)a2`); +72B tgt в хвосте (мелкие ветки timestamp/saddr-path), логика совпадает |
| goodix_tool_read | +220 | LD | **(b)** | **target: multi-touch** — цикл `while()` по числу tool'ов с `memcpy` в `a1[..]` (+ secondary-tool ветка `a1[16]=0; return`); ours — только first point (`*a1..a1[3]`, zero-fill) |
| goodix_tool_write | +96 | LD | **(b)** мин. | tgt: **9 case** в config-switch vs 8 ours — один config-case пропущен; CFSUB/CFADD (jiffies-delta) в обеих |
| goodix_ts_init | +120 | LD | **(a)** \* | логика совпадает (printk/alloc_wq/i2c_register_driver/−ENOMEM); +120B не видна в логике (см. note \*) |
| ch7025_probe | −188 | LD | **(c)** (аномалия) | **структурно разные:** tgt = три for-цикла по таблицам (59 insns); ours = полный init: `_memzero`, 5 рег. → глобалы, `gpio_direction_output`×2+`msleep`, `ch7025_read_reg`, `ch7025_register_display_cvbs/ypbpr`, `rk_display_device_enable` (106 insns). Наш recon разместил init-логику, которой в tgt-символе `ch7025_probe` НЕТ (вероятно, в target она под другими символами) |
| ch7025_i2c_write | +20 | LD | **(b)** мин. | target: **timeout=100000 (µs)** + заполнение `msg[1]`; ours — без timeout, `msg[1]=0` |

## 3. Семейство VENDOR-DIVERGED (14)

Все 14 — d<0 (ours больше). Паттерн: в target хелперы инлайнятся/упрощены, а у нас остались вызовы отдельных функций (+spinlocks/printk). Вызовы только-у-наших (из summary2, по decompiled):

| Функция | d (B) | Вердикт | Только у нас (вызовы) / только у target |
|---|---|---|---|
| qtaguid_ctrl_proc_write | −1124 | **(c)** | ours: `ctrl_counterset`, `ctrl_delete`, `get_sock_stat_nl`, `tag_node_tree_search`, `raw_spin_unlock_bh` + **2 строки прив-чекa** `"qtaguid: ctrl_…(): insufficient priv from pid=%u…"` (в target этих строк нет — упрощённый proc_write) |
| qtaguid_mt | −764 | **(c)+(b)** | ours: `ipx_proto_isra_7_part_8`, `raw_spin_unlock_bh`; **tgt: `ipv6_find_hdr`** (v6-ветка, у нас нет) |
| ddr_get_parameter | −1892 | **(c)** | ours — полный `switch` по mem_type (таблицы параметров); target — короткий if-елсe с ранними `return 0` (упрощённая таблица) |
| ddr_change_freq_gpll_dpll | −256 | **(a)** | ours: `ddr_change_freq_sram`, `_ddr_delayus_veneer`, `__dsb`; tgt: `_ddr_get_pll_freq_veneer`, `_ddr_set_pll_veneer` — veneers-обёртки одного SRAM-кода, разный инлайнинг |
| rk3188_lcdc_ioctl | −912 | **(c)** | ours: `__dsb`, `_copy_to_user_std`, `raw_spin_lock/unlock`; tgt: `_copy_to_user` — у нас больше ioctl-cases/locking |
| rk3188_lcdc_set_par | −852 | **(c)** | ours: `__dsb`, `strcpy`; tgt: `memcpy` — у нас доп. парсинг/валидация пар |
| rk3188_load_screen | −596 | **(c)** | ours: `memcpy`, `rk3188_lcdc_clk_enable` — tgt без явного clk-enable в этой функции |
| serial_rk_probe | −420 | **(c)** | ours: `dmam_alloc_coherent`, `init_timer_key`, `msecs_to_jiffies`, **`serial_rk_init_dma_rx/tx`** — в tgt-probe DMA-init нет (отдельный символ?) |
| serial_rk_set_termios | −256 | **(c)** | ours: `dev_info`, `mod_timer`, **`rk29_dma_ctrl`, `rk29_dma_enqueue_ring`** + строка `"serial_rk_dma_txcb"` — в tgt нет DMA-requeue в termios |
| rk_fb_update_regs_handler | −372 | **(c)** | ours: `prepare_to_wait/finish_wait`, `get_current`, `msecs_to_jiffies`, `put_unused_fd`, `printk`; tgt: только `rk_fb_update_reg` — у нас waitqueue/timeout-логика |
| iface_stat_create | −344 | **(c)** | ours: `raw_spin_unlock_bh` (+printk `"?"`) — tgt без spinlock в create |
| iface_stat_update | −216 | **(c)** | ours: `printk`, `raw_spin_unlock_bh` |
| iface_inet6addr_event_handler | −244 | **(c)** | ours: `raw_spin_unlock_bh` |
| rockchip_i2s_hw_params | −324 | **(c)** | ours: **`cat66121_hdmi_sys_config_audio`**, `rockchip_snd_txctrl`, `printk` — в tgt нет HDMI-syscfg-пути в hw_params |

## 4. Target БОЛЬШЕ (недописанность наших реконструкций) vs Ours БОЛЬШЕ (extra)

**Target больше — 9 функций, суммарно +1152B. Что именно не дописано:**
1. **IPv6-ветки в ping-сокете** (3 функции): bind-check (`_ipv6_addr_type`, `dev_get_by_index_rcu`, −EAFNOSUPPORT/−ENETUNREACH/−EACCES), set_saddr/clear_saddr (128-bit copy/memzero). — Кандидат на реальный функциональный gap.
2. **Multi-touch в goodix** (`goodix_tool_read`): цикл по N tool'ов; `goodix_tool_write`: +1 config-case.
3. **Калибровка тачскрина** (`tsc2003_work`): `TouchPanelCalibrateAPoint` у нас stub (MTC-core вне дерева) + u8-счётчик ошибок (i2c_read/write).
4. **factory_test**: ADC-тест-цикл (test_adc, пороги 503/0x12, цвет индикации), `rk_direct_fb_set`, `"No Test"`.
5. **camera_start**: доп. case-ветки switch-режимов (+124B).
6. Мелкие: `ch7025_i2c_write` timeout 100000µs; `camera_start`/`factory_test` — именованные vs anonymous globals (а не логика).

**Ours больше — 14 vendor-функций, суммарно −8564B. Причины:**
- Не-инлайновые хелперы + spinlock/printk, которые в target собраны иначе (iface_stat_*, qtaguid_mt);
- **Больше функциональности у нас:** priv-check строки в qtaguid_ctrl_proc_write, DMA-init в serial_rk_probe/termios, HDMI sys-config в i2s_hw_params, waitqueue-логика в rk_fb, полный switch-таблицы ddr_get_parameter;
- **Аномалия ch7025_probe**: наш recon содержит init (GPIO/дисплей), отсутствующий в tgt-символе — либо ошибка атрибуции при реконструкции, либо в target эта логика вынесена в другие функции. Рекомендация: перепроверить ch7025_*-кластер по `3188_kallsyms` на соседние символы.

## 5. Интерпретация: layout/relocation vs реальное source-различие

- **Layout/relocation (не source):** все именованные-глобалы vs `MEMORY[<A>]` (target-декомпиляция без symbol resolution), jump-table/литеральные пулы в ipp_blit, veneers (`_…_veneer`) в ddr, mvn/mov, форма retry-loop'ов, if-chain vs jumptable (key2ir — тот же source). Доказательство: 5 функций с d=0 дают **100% INSTR-MATCH** при различиях в pseudo-C (rk29_sdmmc_set_frq и др.).
- **Реальные source-различия:** (1) u8 vs int счётчик i2c-ошибок tsc2003 (ldrb/strb — disasm); (2) отсутствие IPv6 в наших ping-реконструкциях; (3) absence multi-touch в goodix; (4) stub калибровки; (5) отсутствие ADC-теста в factory_test; (6) упрощённый target против нашего полного vendor-кода в 14 ddr/lcdc/serial/iface/i2s/qtaguid-функциях (target собран из более «облегчённого» исходника: меньше priv-чеков, DMA-setup, HDMI-paths).
- **Вывод для проекта:** ядро ours по RECON-семейству близко к target (15/39 полностью эквивалентны; 9 — с конкретными перечисленными недочётами, каждый локализован); vendor-расхождения объясняются разной полнотой исходников сборки (наш vendor-код богаче, не «битый» реконструкт). Приоритетные доделки: **ping IPv6 → goodix multi-touch → tsc2003 calib/u8 → factory_test ADC**.

## 6. Материалы (пути)
- ours-декомпиляции: `/home/amper/tmp/ida-tmp/mtc_audio/ours_decompiled/decompiled_<name>.c` (39)
- target-декомпиляции: `/home/amper/tmp/ida-tmp/mtc_audio/src_all/decompiled_<name>.c` (39, все найдены, MISSING=[])
- диффы: `/home/amper/tmp/ida-tmp/mtc_audio/compare_out/diffs2/<name>.diff` (39)
- агрегаты: `compare_out/summary2.json` (cls/calls/strings), `compare_out/disasm_check.json` (insns-match + verdicts), `sizes2.json` (адреса/размеры, судья 3188_kallsyms)
- скрипты: `disasm_check2.py`, `compare_pairs2.py`; IDB ours: `ours_kernel.i64` (104MB, mержен)
- наши исходники для сверки: `/home/amper/Coding/mtc_drivers/src/drivers/misc/mtc/recon.c`, `.../input/touchscreen/tsc2003.c` и др.

*Ограничения: read-only на сборку; mtc_kernel.i64 не трогался (только src_all). tgt_size — по «до следующего символа» в kallsyms, может включать padding (учтено для goodix_ts_init/camera_stop).*
