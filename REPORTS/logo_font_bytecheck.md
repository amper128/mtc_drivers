# Byte-сравнение logo + fonts: OUR vmlinux vs TARGET kernel.elf

**Дата:** 2025-01 (run: finalize-4). READ-ONLY: ни один из сравниваемых файлов не изменялся; создан только этот отчёт.
**OUR** = `/home/amper/Coding/mtc_drivers/result/vmlinux` (10 832 133 B, полный symtab)
**TARGET** = `/home/amper/Coding/RK3188/kernel.elf` (8 266 216 B, symtab = 6 записей; адреса — из judge `3188_kallsyms`, 42 433 строк)

## 1. MAPPING (подтверждено readelf -lW / -SW, arm-eabi, TC=/home/amper/toolchain/arm-eabi-google/bin)

| файл | segment/section | offset | vaddr | filesz/size |
|---|---|---|---|---|
| OUR | main LOAD / .init+… | 0x8000 | 0xc0408000 | 0x7ebf88 |
| TARGET | single .data (PROGBITS, только он alloc) | 0x8000 | 0xc0408000 | 0x7da024 |

→ **Одна и та же формула для ОБОИХ файлов: `fileoff(v) = v − 0xc0408000 + 0x8000`.**
(В исходной задаче target mapping был указан как `vaddr−0xc0400000` — это неверно; readelf -SW TARGET: `.data c0408000 8000 7da024`.)
Контентная валидация формулы: glyph `!` шрифта (7e 81 a5 81 81 bd 99) найден в TARGET ровно при foft(0xc0a08bd0)=0x608bd0 (+18); ASCII-префикс clut совпал по содержимому.

Корректировки задачe (по objdump -t OUR):
- размер `logo_linux_800x480_clut224_data` в OUR = **0x5dc18 = 384 024 B** (в задаче было 381 752 — неверно);
- `logo_linux_clut224_data` (small) = c04300a0, **0x1918 = 6 424 B**; `logo_linux_clut224_clut` = c042fc74, **0x42a = 1 066 B**; fb_logo-структуры по 16 B (c0490c90, c0490ca8).

## 2. Таблица byte-сравнения (reference length = our_size; md5 — обоих выделенных блоков)

| symbol | our_addr | our_size | tgt_addr (judge) | tgt_size* | our_md5 | tgt_md5 | verdict |
|---|---|---|---|---|---|---|---|
| logo_linux_800x480_clut224_data | c0431e24 (foft 0x31e24) | **384 024** | c042f754 (foft 0x2f754) | **384 024** (exact fit до next symbol) | b3b6a73c4ac0a96ed6906c0292e28e67 | 3d9f5f1815dca37006c725287e858a26 | **DIFFERENT @52**, 298 134 B (77.63%) |
| logo_linux_800x480_clut224_clut | c04319b8 (foft 0x319b8) | 1 130 | c042f2e8 (foft 0x2f2e8) | ~1 132 (gap до data) | 46676230044e3e287f79ae7b2a26e2d5 | 112b4d46e1f3584b7b145b0fb601ce89 | **DIFFERENT @19**, 665 B (58.85%) |
| font_8x16 | c0a06a58 (foft 0x606a58) | 1 280 (.text!) | c0a08bd0 (foft 0x608bd0) | ~2 048 (gap до next = 4 KB page) | 7b31fad7839930c37c8d4772f7c783da | 9d30b5475e91e0043a6cd8976f4927a5 (1280 B; полные 2048 B: c6249f4c094600c1ab7caee3d7f38326) | **DIFFERENT @0**, 902 B (70.47%) — см. аномалию |
| logo_linux_clut224_data (small) | c04300a0 (foft 0x300a0) | 6 424 | — **нет в kallsyms target** | 0 (отсутствует) | — (фрагмент 32 B не найден в файле target) | — | **N/A: absent in target** |

\* tgt_size: из kallsyms (gap до следующего T/t-символа), т.к. target symtab не даёт размеры.

### Детали расхождений
- **big logo data** (384 024 B): common prefix = 52 B; first_diff @52: our `…21 20 21 20 21…` vs tgt `…20 20 20…` (palette-индексы, т.е. **разное изображение/палитра**, а не мусор). post16 после 384 024 B в target = `6c d3 48 c0 …` (32-битные указатели — следующий объект `early_platform_driver_list @ c048d36c`) → граница target **точно** на 384 024 B, «наезжания» нет. Размер одинаков: **delta = 0 B**.
- **clut 800x480**: первые 19 B идентичны — ASCII-тег вендора `"ppllogo_RKlogo_clut"` (оба файла!), палитра начинается с оффсета 19: our `db 00 00 00 01 01…` vs tgt `de 04 04 04 04…` → разные палитры.
- **small logo**: в target нет ни символа (`logo_linux_clut224*` отсутствуют в kallsyms), ни байтов (32-байтовый фрагмент our, начинающийся `00 50 00 50 "logo_RKlogo_data"`, не найден в kernel.elf).

## 3. ФОНТ-АНОМАЛИЯ (ключевое finding)

Наш символ `font_8x16` (c0a06a58, **section .text**, local object, size 0x500=1280 B) **НЕ является bitmap-шрифтом**:
- содержимое — таблица u32-записей с повторяющимся паттерном `ac 05 00 00 | XX 02/03 00 00 | N | F` (объект-данные, положенные в .text — артефакт вендор-сборки);
- сигнатура глифа `!` (`7e 81 a5 81 81 bd 99`): **0 hits по всему OUR vmlinux** (10 832 133 B);
- стандартный glyph `A` (`a5 ad bd a5 a5 a5 81 81`): **0 hits в OUR**;
- 96/64-байтовые фрагменты bitmap из target **не найдены в OUR** (в 4 KB-регионе target шрифта только тривиальные all-zero 16-байтовые чанки совпадают: оффсеты 0, 512, 720, …).

В TARGET `font_8x16 @ c0a08bd0` — **настоящий bitmap**: с оффсета 18 идут узнаваемые глифы (7e 81 a5 81 81 bd 99 = `!`; далее ff-глифы), стандартный объект ~2048 B.
**Вывод:** в нашем vmlinux нет стандартной console 8x16 bitmap-таблицы; символ `font_8x16` — мислейбл/вендорский объект-таблица. Наш font и target font **несравнимы по смыслу** (это разные объекты, просто с одинаковым именем в kallsyms/symtab).

## 4. Вердикт

- **logo byte-identical? НЕТ.** Размеры big-logo идентичны (384 024 B оба), но контент различается на 77.63% (298 134 B, first diff @52) — это **другое изображение** (другая pалитра/картинка RK-логотипа), а не сбой экстракции. Clut тоже отличается (58.85%, тоже разные палитры). Small logo (40x17 Linux) в target отсутствует вовсе.
- **fonts byte-identical? НЕТ.** Наш `font_8x16` — не шрифт (u32-таблица в .text, 1280 B), target — реальный 8x16 bitmap (~2048 B). Битмап-шрифта 8x16 в нашем vmlinux нет ни под одним адресом.

## 5. Значение для overshoot (+149 KB)

| объект | our | target | delta (our − target) |
|---|---|---|---|
| big logo data | 384 024 B | 384 024 B | **0** |
| big logo clut | 1 130 B | ~1 130 B | **≈0** |
| small logo (data+clut+struct) | 7 506 B | 0 B | **+7 506** |
| font_8x16 объект | 1 280 B (не bitmap) | ~2 048 B (bitmap) | **−768** |

**Чистый вклад logo+font в overshoot ≈ +6.7 KB** — это ~4.5% от +149 KB. Logo/font **НЕ являются источником overshoot**: основной 384-KB blob того же размера; остаток ~142 KB ищется в других секциях/объектах (см. REPORTS/overshoot_breakdown.md).

## 6. Изменённые файлы
- Создан: `/home/amper/Coding/mtc_drivers/REPORTS/logo_font_bytecheck.md` (этот файл).
- OUR/TARGET/judge — не изменялись (READ-ONLY).
