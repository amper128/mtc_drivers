# font_8x16 anomaly — root cause + fix-recommendation (READ-ONLY)

Дата: 2026-07-15 · Метод: bounded grep по src/ + ref_kernel/, byte-сверка result/vmlinux vs car.c, decode u32-таблицы. Файлы дерева не менялись.

## 1. Что такое наш `font_8x16` (source, структура)

**Source: `src/drivers/misc/mtc/car.c:1350`** — наш (binaRE-реконструкция mtc-модуля) статический массив:

```c
/* binaRE 0xC0A06F48 (kernel.elf, file offset 0x606F48): шрифт 8x16, 80 глифов;
 * байты извлечены из kernel.elf. */
static const unsigned char font_8x16[1280] = { ... };
```

- **Сверка (проверено):** `result/vmlinux` @ `c0a04a58` (1280 B) == таблица car.c **byte-identical** (сравнено скриптом, `MATCH: True`). Символ в System.map: `c0a04a58 t font_8x16` — **local** (`t`).
- **Структура:** НЕ bitmap, НЕ conmakehash. Это **u32-дескрипторная таблица вендора**: 80 записей × 16 B (4× u32 LE). Первые записи:
  - `0: (1452, 583, 5, 3)` · `1: (1452, 588, 4, 3)` · `8: (1452, 595, 20, 3)` · `9: (1452, 596, 5, 5)` · `40: (0,0,0,3)` (пусто)
  - field0 = `1452 (0x5AC)` — константа для первых ~36 записей; величина > 1280, т.е. **ссылка за пределами нашего извлечённого окна** — вероятно, оффсет пиксельных данных глифов в БОЛЬШОМ вендорном font-blob (мы извлекли только первые 1280 B объекта).
  - field1 — малые варьирующиеся оффсеты (569…778), field2/field3 — счётчик/width-подобные и тип (3, 5, 0). Точный смысл полей без вендорного хедера не восстанавливается; утверждаем только «dескрипторы, а не пиксели».
- **Использование:** только `mtc_debug_putc` / `mtc_debug_put_string` (car.c:2246, `g = font_8x16 + 16*glyph`, побитовый вывод в FIQ-framebuffer) — debug-консоль mtc. ⚠ ВНИМАНИЕ: putc индексирует массив как **bitmap 16B/глиф**, а данные — дескрипторы; т.е. наша binaRE-копия семантически не согласована (либо в оригинале putc использует дескрипторы иначе, либо извлечено не то окно). Это уже есть в карме как «честный плейсхолдер».
- Стандартная VGA-сигнатура `7e 81 a5 81 81 bd 99` в нашем vmlinux **не найдена нигде** (проверено повторно: `data.find() == -1`).

## 2. Root cause (target vs our)

**Столкновение имён двух РАЗНЫХ объектов:**

| | TARGET | OUR (result/vmlinux) |
|---|---|---|
| Символ | `font_8x16` @ c0a08bd0 | `font_8x16` @ c0a04a58, **`t` (local)** |
| Природа | стандартный kernel console 8x16 **bitmap** (классический VGA-шрифт cpi2fnt: glyph0=blank, `'!' = 7e 81 a5 81 81 bd 99`) | **u32-дескрипторы вендорного mtc-консоли**, извлечены из vendor kernel.elf @foft 0x606F48 |
| Размер | ~4096 B (256×16) | 1280 B |
| Почему линкуется | `CONFIG_FONT_8x16=y` → `drivers/video/console/font_8x16.o` (`font-objs-$(CONFIG_FONT_8x16)`) | наш car.c (mtc-модуль); **в нашем .config НЕТ ни одного `CONFIG_FONT_*`**, консоль `console=ttyFIQ0` (не fbcon) → kernel-font не линкуется вовсе |

Иными словами: target — чистый kernel с включённым стандартным шрифтом консоли; вендор (и наша реконструкция) используют своё внутреннее поле `font_8x16` в mtc-драйвере — имя совпало случайно/по вендорской традиции, содержимое — разное. Это config-diff **и** другой объект одновременно.

## 3. Fix: как сделать byte-match; collision-вердикт

### Вариант A: `CONFIG_FONT_8x16=y` (недостаточно, не рекомендую)
- **Collision: НЕТ.** Наш — `static` (file-local), kernel-объект в ЭТОМ дереве ref_kernel (новое поколение) определяет **`static const unsigned char fontdata_8x16[4096+]`** + **`const struct font_desc font_vga_8x16`** (EXPORT) — имена не пересекаются вообще; даже если бы пересекались, static vs global в разных TU = link error, а тут и того нет.
- **Но byte-match НЕ достигается:**
  1. в этом дереве глобального символа с именем `font_8x16` **не создаётся** (он назван `font_vga_8x16`, данные — `fontdata_8x16`); наш local `t font_8x16` остаётся как есть;
  2. данные нового cpi2fnt ≠ target: `'!' = 00 01 … 0x18, 0x3c, 0x3c…` (проверено в `ref_kernel/drivers/video/console/font_8x16.c`), а не `7e 81 a5…`;
  3. vmlinux растёт на ~4.5 KB (fontdata + font_desc) — overshoot увеличивается, а не уменьшается.

### Вариант B: заменить содержимое car.c (рекомендуется для byte-parity)
- В `src/drivers/misc/mtc/car.c` заменить массив `font_8x16[1280]` на **классический VGA 8x16 bitmap 4096 B** (данные старого cpi2fnt-`font_8x16.c`; сигнатура `7e 81 a5…`). Имя оставить `font_8x16`.
- Collision: **НЕТ** (статический, и kernel-font при этом не включён).
- Побочный эффект: `mtc_debug_putc` начнёт рисовать нормальные VGA-глифы (его bitmap-индексация при этом перестанет быть ошибочной — см. ⚠ в п.1); вендорная u32-таблица из вmlinux уходит. Если нужна фиделность вендору — старую таблицу можно сохранить под отдельным именем (но тогда это лишний символ/байты).

### Вердикт
Enable `CONFIG_FONT_8x16=y` **не решает** задачу (чужое имя символа, чужие байты, +размер). Byte-match символа `font_8x16` = **Вариант B** (замена данных в car.c на 4096 B классического VGA) либо, если приоритет — фиделность вендорного mtc-драйвера, оставить как есть и принять диф.

## 4. Impact

- **Size:** сейчас our 1280 vs target ~4096 → **−2816 B** на этом символе (мы меньшие). Вариант B: our +2816 → Δ к target = **0**. (Примечание: REPORTS/logo_font_bytecheck.md оценивал target «~2048 (gap до next)» и давал −768; при установленном размере ~4096 — −2816.)
- **COMMON:** `font_8x16` уже есть в обоих наборах символов → остаётся COMMON; после Варианта B появляется возможность byte-1:1.
- **byte-1:1:** сейчас 70.47% байтов различаются (logo_font_bytecheck.md). После Варианта B — потенциально 0%, **при условии**, что target-таблица = ровно стандартный VGA-файл (подтверждено по сигнатурам glyph0='!'='"'; полная сверка 4096 B требует байтов target из IDB — kernel.idb.zip).

## 5. Рекомендация

1. **Для byte-parity с target:** Вариант B — заменить 1280 B u32-таблицу в car.c на 4096 B классического VGA bitmap; не трогать CONFIG_FONT_*. Это закрывает −2816 и делает символ byte-comparable.
2. **Если метрика не требует этого символа** (local `t`, входит в overshoot-баланс с другими блоками) — можно **leave as-is** и задокументировать как известный недифф (venдорный объект под ядровым именем).
3. Независимо: проверить корректность окна извлечения (0x606F48, 1280 B) — вероятно, извлечён только хвост/часть вендорного font-blob, и семантика `mtc_debug_putc` в нашей копии не согласована с данными (⚠ п.1).

## Доказательства (проверено в этой сессии)
- `src/drivers/misc/mtc/car.c:1350` — определение массива; `:2240-2263` — mtc_debug_putc.
- Byte-сверка: `result/vmlinux` @ c0a04a58 (ELF LOAD-seg off=0x8000+(c0a04a58−c0408000)) == car.c таблица → `True`.
- Decode u32×320: field0=1452 (15×), записи (1452,583,5,3)…(1452,778,260,3); оффсеты за пределами 1280 B.
- `result/System.map:27164: c0a04a58 t font_8x16`; `kallsyms.txt:65268` — local.
- `ref_kernel/drivers/video/console/font_8x16.c`: `fontdata_8x16[4096]` (static) + `font_vga_8x16` (font_desc, EXPORT); `'!' = 0x18, 0x3c…` — не классический VGA.
- `src/config/.config`: нет `CONFIG_FONT_*` (только `CONFIG_LOGO_*`); Makefile: `font-objs-$(CONFIG_FONT_8x16) += font_8x16.o`.
- Сигнатура `7e 81 a5 81 81 bd 99` не найдена ни в result/vmlinux, ни в console-шрифтах ref_kernel.


## СТАТУС (2025-10-06) — CONFIG_FONT_8x16=y включён, НО std-шрифт НЕ попал в сборку (поправка к "СОГЛАСОВАНО")

1. **CONFIG включён:** Kconfig-патч `patches/local/0045-drivers-video-console-Kconfig.patch` — dep FONT_8x16 расширен `FRAMEBUFFER_CONSOLE || …` → `FB || …` (FB=y, FRAMEBUFFER_CONSOLE у нас не задан). `CONFIG_FONT_8x16=y` в `src/config/.config:2115` теперь ДЕРЖИТСЯ через silentoldconfig — CONFIG-DRIFT check проходит, BUILD RC=0.
2. **НО fontdata_8x16 отсутствует в vmlinux** (проверено: `arm-eabi-nm result/vmlinux | grep font` → только vendor `font_8x16` (car.c, static); kallsyms — только `con_font_op T`, `font_8x16 t`). Причина: в этом дереве `drivers/video/console/Makefile` ОПРЕДЕЛЯЕТ composite `font-objs := fonts.o` + `font-objs-$(CONFIG_FONT_8x16) += font_8x16.o`, но НИГДЕ нет строки `obj-… += fonts.o`/`font_8x16.o` → файл НЕ КОМПИЛИРУЕТСЯ никогда (после clean-build в каталоге нет ни одного font-*.o). Kconfig-only патч не способен его линковать.
3. **Доказательство zero-impact:** size result/vmlinux.stripped = 8377508 — ИДЕНТИЧЕН предыдущей сборке (logo-fix, до FONT_8x16); COMMON=27410 held; MTC-core PASS; ADDED=8335 (delta от пред. сборки = 0 — ни font_vga_8x16, ни fontdata_8x16 в kallsyms не появились).
4. **Collision-риск при линковке НЕТ:** font_8x16.c экспортирует `font_vga_8x16` (const font_desc), а vendor-массив в car.c — `static font_8x16[1280]`; имена не пересекаются.
5. **Что нужно для "std-шрифт в сборке" (требует одобрения, за рамками Kconfig-only):** минимальный Makefile-патч 0046 в `drivers/video/console/Makefile`: `obj-$(CONFIG_FONT_8x16) += font_8x16.o` (1 строка) → fontdata_8x16 (static, 4096 B) + font_vga_8x16 (global, kallsyms +1 'R') попадут в vmlinux, size ~+4.2 KB. Альтернатива без Makefile: Вариант B из п.5 (byte-замена 1280 B-таблицы car.c на 4096 B std bitmap).
6. **1-glyph diff СОГЛАСОВАНО** (осознанное отклонение, byte-parity по шрифту не требуется) остаётся в силе для vendor `font_8x16` vs std: разница ТОЛЬКО glyph #42 ('*'), 9 байт (vendor — заполненная звёздочка ff..; std = 66 3c ff 3c 66); остальные глифы ИДЕНТИЧНЫ. [metric: COMMON=27410, ADDED delta 0, size 8377508 B]

## РЕШЕНИЕ (СОГЛАСОВАНО) — Variant B: car.c font_8x16 = std-шрифт
car.c font_8x16[1280] (vendor u32-дескрипторы) → font_8x16[4096] std VGA 8x16 bitmap (md5 e4059ca64597883bfeff3dd6b22a6efc). mtc_debug_putc indexing: char-bitmap `const u8 *g = font_8x16 + 16 * glyph;` g[row], row=0..15, bit-цикл 0x80→LSB (совместимо со std 256-глиф bitmap, putc НЕ менялся).
judge's font_8x16 (c0a08bd0, md5 f689a99b) = std-шрифт; diff с vendor font_8x16 = ТОЛЬКО glyph #42 ('*', 9 байт: vendor=ff.., std=00 00 66 3c ff 3c 66 00). Остальные 255 глифов (4087/4096) идентичны.
Расхождение СОГЛАСОВАНО (осознанное отклонение, не баг). Метрики: COMMON=27410, ADDED=8335, size=8377508 B, font_8x16 @c0a04a58 size=4096 md5=e4059ca6. BUILD OK (RC=0).
