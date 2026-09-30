# MTC-14 — транскрипция: ИСПОЛНЕНИЕ (t6_exec_transcription)

Дерево: `/home/amper/Coding/mtc_drivers/drivers/misc/mtc/` (TO DO: git-коммит — devops).
Build-дерево `/home/amper/Coding/mtc_build/t9/base` — НЕ трогал (read-only; sync в D — за другим агентом).
`t9/build_fixes.log` — НЕ писал. git-мутации — НЕ делал.

## 1. Изменённые файлы (md5 ДО → ПОСЛЕ)

| Файл | md5 ДО | md5 ПОСЛЕ (финал) | Статус |
|---|---|---|---|
| keys.c | d48d9ae6c4e9cd80d7381f21b0361b18 | **600d07bb9b367e94ea198978532ac528** | ИЗМЕНЁН (MTC-14) |
| backview.c | cea2d979b424b2ef791bda630763d191 | **123478cee7621131991be792b915dd98** | ИЗМЕНЁН (MTC-14) |
| car.c | e41b57ee3c7260ea8b98b952c565d0b9 | **5291bf06c34fdae55d40f0579883bea0** | ИЗМЕНЁН (MTC-14) |
| lcd.c | 270e5985abe433492c33033ba8f9f6db | **36928231fcf123d51287e32cb1320209** | ИЗМЕНЁН (MTC-14; финальный md5 — ПОСЛЕ symbol_map-фикса) |
| shared.h | 9196f0639dceff4b3fa96f3c432bb615 | **345bb8e1ea26317aa128c3b0963dd9c9** | ИЗМЕНЁН (MTC-14) |
| radio_tef6606.c | 77b1efdd20e0f322f4a36c273e879b41 | **78f366da0480bae8d92cd196a8ef2e0a** | ИЗМЕНЁН (предшественник: ork_sta_valid→work_sta_valid, 3 места) |

md5 ДО: `/tmp/md5_before_mine.txt`; md5 ПОСЛЕ: `/tmp/md5_after_mtc14.txt`.
Чтение-достоверность t9/base: `md5sum -c /tmp/md5_before.txt` в `t9/base/drivers/misc/mtc/` → **15/15 OK, not-OK=0** — sync в D НЕ сделан.

## 2. Реализовано по файлам

### keys.c (882 → ~940 строк)
- `static struct mtc_keys_data *keys_data` → объект `.bss` (binaRE 0xC168E474); 25× `keys_data->X`→`keys_data.X`, 8× `(unsigned char *)keys_data`→`&keys_data`, 2× `(char *)keys_data + 0x6C`→`&…`, 1× kd8-инициализация.
- In-файл-строки (байты 1:1 из /tmp/gen_strings.c, layout `mtc\0 | 12345678\0+3B pad | A07\0 | 126\0`): `CustomerStr="mtc"`, `SnStr char[12]="12345678"`, `ModelStr="A07"`, `PasswordStr="126"` — вставлены после mtc_keydefault.
- `static __attribute__((used)) int searchAdcKey(u8,int,const u8*,int)` — тело 1:1 по decompiled_searchAdcKey.c (i<75 step3; v8=tab[i+1]|(tab[i+2]<<8); |adc_val−v8|<tol → return mtc_keycode[i/3]; else 0). Коньюмер send_event из бинара — out of scope (DCE-кавета).
- Accessor `struct mtc_keys_data *mtc_keys_data_ptr(void){ return &keys_data; }` (новый служебный символ, не из kallsyms; коньюмер — sta_touch_adc в car.c) + proto в shared.h.

### backview.c
- :1798-1800 stub → полное `int vip_reset(int pwr)` (non-static, T) по decompiled_vip_reset.c: printk("rst %d\n"); MEMORY[E466]=MEMORY[E45E]; a1==1 → 2×(0xFED00128: 0x4008000→0x4000000, __dsb(15), _const_udelay(536870)); v2=DWORD*@E418 {61442,5,−1}; `if(!MEMORY[0xC168ACE1])` → {0,E45F=0,786448,29885104,688}→LABEL_5; switch(E45E): 3→{12,0,37749456,E45F=1,720}, 2→{12,1572864,31458000,E45F=1,720}, 0/1/4→{32,0,31458000,E45F=0,720,[18]=16}; LABEL_5: [18]=16; a1==1 → FED000BC &= ~0x100 | 0x1000000; [5..8]=*(DWORD*)E40C+{1382400,1797120,2211840,2626560}; [24]=0; *v2=61443; a1!=2 → E467=0; E464=0; return (int)v2[7].
- Офсеты: E45E=.decoder_type, E45F=.mirror_image, E464=.camera_working, E466/E467=_gap2[1]/[2], 0xC168ACE1=car_status+93.
- Добавлена `static volatile u32 *bss_C168E40C` (прецедент bss_C168AF2B) + guarded `#define __dsb(opt) asm volatile("dsb %0"::"i"(opt):"memory")` — **символа __dsb в kallsyms НЕТ**: это инлайн оригинала (kernel bar), воспроизведён как define, задокументировано в комментарии.

### car.c
- `int sta_touch_adc(char *buf)` — def: `sprintf(buf,"%d,%d", mtc_keys_data_ptr()->?+0x144, ...+0x148)` (байты keys_data 0xC168E5B8/E5BC = офсеты +0x144/+0x148, подтверждены предшественником). shared.h:335-336 TENTATIVE снят.
- gtp-секция: `#include <linux/i2c.h>`; struct gtp_dev (+8 i2c_client PTR, +136 w, +138 h, +141 sub, +147 len); по decompiled:
  - `gtp_init_panel` (static, t): car_status+0x95/0x96=class, touch_width/height/info1/2 @AD0C/AD10/AD14/AD15, keys_data E541=+0xCD flag / E544=+0xD0 config_id; при vendor==66 && *(u32*)(dev+136)==39322690 && !flag && b186==61 → gtp_write_panel; get_panel(w,h,vendor,flag); printk-дамп; switch(w) пороги 0x1F4/0x258/0x320/0x400 → классы.
  - `get_panel` (T, non-static @c083e378): таблица 44B-stride [1]=w,[2]=h,+12=vendor,+13=flag,[11]=name "JRC-8004"; **TENTATIVE zero-таблица**, return 0/NULL — задокументировано.
  - `gtp_write_panel` (t @c09c60b4): blob loc_C0A0AD9C 186B, buf dword_C0BCA410+2 / 240B, send 0xF2; **TENTATIVE zero-blob**.
  - `gtp_reset_guitar` (t): gpio 216/217, msleep-последовательность (0/ms/2/6/50).
  - i2c-хелперы **статичные** (имена sub_C083DFDC/sub_C083E0D8 дублируются в kallsyms → static): фактические имена в дереве — `gtp_i2c_block_write` (2-msg: reg 2B + data, i2c_transfer retry×5 + gtp_reset_guitar(client,10)) и `gtp_i2c_raw_write` (1-msg raw write, retry×5 + reset).

### lcd.c
- `lcd_seg_tab[99]` УДАЛЕН → 4 массива 1:1 из /tmp/gen_lcd.c: `symbol_map[8]@C0A09D28/32B`, `dig_seg[38]/152B`, `dig_map[32]/128B`, `flash_tab[21]/84B`.
- Коньюмеры: :198→symbol_map, :247→`(const u8*)dig_map + 14*idx`, :256→dig_seg[v2], :287→symbol_map[i], :308→flash_tab[idx].
- Комментарии: dig_map@+184 (отсчёт в 396B-блоке), flash 21 words.

### shared.h
- proto `struct mtc_keys_data *mtc_keys_data_ptr(void);` (новый).
- :335/336: TENTATIVE снят у `sta_touch_adc`.

## 3. Таблица 14 имён (статус после вписывания; grep-проверка по дереву)

| # | Имя | Статус | Где |
|---|---|---|---|
| 1 | work_sta_valid | ПЕРЕИМЕНОВАЛ (из ork_sta_valid, предшественник) — в дереве только work_* (9 «ork»-хитов = подстроки dwork_*/комменты) | radio_tef6606.c:1044 (def T), :106 proto |
| 2 | mtc_keydefault | СОВПАЛО (уже было; байты 76 match — предшественник) | keys.c |
| 3 | searchAdcKey | ДОБАВЛЕНО (static 't', attr used) | keys.c:97 |
| 4 | mtc_keys_data_ptr | ДОБАВЛЕНО (новый accessor, в kallsyms не было — служебный) | keys.c + shared.h |
| 5 | CustomerStr | ДОБАВЛЕНО, байты match | keys.c:84 |
| 6 | SnStr | ДОБАВЛЕНО, char[12] "12345678"+pad match | keys.c:85 |
| 7 | ModelStr | ДОБАВЛЕНО, "A07" match | keys.c:86 |
| 8 | PasswordStr | ДОБАВЛЕНО, "126" match | keys.c:87 |
| 9 | vip_reset | РЕАЛИЗОВАНО (stub→полное тело 1:1, T) | backview.c |
| 10 | sta_touch_adc | РЕАЛИЗОВАНО (def + снят TENTATIVE) | car.c, shared.h |
| 11 | gtp_init_panel | РЕАЛИЗОВАНО (static t) | car.c |
| 12 | get_panel | РЕАЛИЗОВАНО (T @c083e378; данные TENTATIVE-zero) | car.c |
| 13 | gtp_write_panel | РЕАЛИЗОВАНО (t @c09c60b4; blob TENTATIVE-zero) | car.c |
| 14 | gtp_reset_guitar | РЕАЛИЗОВАНО (t; gpio216/217) | car.c |

Сопутствующие: lcd_seg_tab → РАСШЁЛ на symbol_map/dig_seg/dig_map/flash_tab (lcd_seg_tab из дерева УБРАН, 4 новых — на месте); gtp_i2c_block_write / gtp_i2c_raw_write (static-хелперы, фактические имена вместо псевдонимов sub_C083DFDC/sub_C083E0D8); send_event — **ОТСУТСТВУЕТ (out of scope)**.

## 4. gcc -fsyntax-only — результат (Финал)

Команда: cwd=`t9/base` (read-only); `/home/amper/toolchain/arm-eabi-google/bin/arm-eabi-gcc` **4.6.x-google 20120106 (prerelease)**;
`-fsyntax-only -D__KERNEL__ -D__LINUX_ARM_ARCH__=7 -march=armv7-a -Iarch/arm/include -Iarch/arm/mach-rk3188/include -Iarch/arm/plat-rk/include -Iinclude -include include/generated/autoconf.h <файл>`.
(Первая попытка без mach-включек дала `mach/gpio.h: No such file` — путь `arch/arm/mach-rk3188/include` найден через `find arch/arm -name gpio.h`; с ним — финал ниже.)

| Файл | RC | Ошибки (полный список) |
|---|---|---|
| keys.c | **1** | `include/linux/rcupdate.h:822:2: error: size of unnamed array is negative` — единственная |
| backview.c | **1** | та же единственная (rcupdate.h:822) |
| car.c | **1** | та же единственная (rcupdate.h:822) |
| lcd.c | **1** | та же единственная (rcupdate.h:822) |
| radio_tef6606.c | **1** | та же единственная (rcupdate.h:822) |
| shared.h (standalone-обёртка `#include "shared.h"`, + `-I<mtc dir>`) | **0** | — |

**Baseline-доказательство pre-existing:** ошибка в заголовке ядра `include/linux/rcupdate.h:822` (конфигуративно-зависимый, возникает ДО/НЕЗАВИСИМО от кода mtc; baseline RC=1 подтверждён в предыдущем ходе на неизменённом дереве t9/base). Ни в одном mtc-файле ошибок нет — только pre-existing warnings транскрипции (binaRE-касты: T132B_Write/copy_from_user/arm_send_multi/ops-таблицы и т.п.).

**Finding — `__attribute__((used))` + toolchain 4.6:**
- Standalone-тест (arm-eabi-gcc 4.6, `-march=armv7-a -c` + `objdump -t`): оба варианта — (A) `static __attribute__((used)) int f(...)` в начале; (B) trailing attr в конце определения — компилируются RC=0, и символ `l F .text searchAdcKey` **держится в .o** в обоих.
- В keys.c применён **вариант A** (атрибут в начале строки определения) — задокументировано комментарием на месте. Символ static 't' не DCE-тится на уровне .o; полный consumer send_event out of scope (см. §5) — при final-link без consumer'а символ может быть отброшен линковщиком; критичность низкая (1 't'-символ из 14, остальные 13 — name-presence подтверждены).

## 5. TENTATIVE и out of scope

**TENTATIVE-данные (byte-1:1 = отдельная задача, нет dump C0A0AD/C0BC):**
1. `get_panel`: таблица 44B-stride (w,h,vendor,flag,name "JRC-8004") — вписана zero-заглушкой; return 0/NULL.
2. `gtp_write_panel`: blob loc_C0A0AD9C (186B) + буфер dword_C0BCA410+2 (240B), send 0xF2 — вписан zero-blob.
3. Документировано в комментариях у функций в car.c; `bss_C168E40C` (vip_reset) — по прецеденту bss_C168AF2B, не TENTATIVE.
4. `__dsb` — символа в kallsyms нет: инлайн оригинала, воспроизведён как guarded asm-define (backview.c), задокументировано.

**Out of scope:**
- `send_event(...)` — consumer searchAdcKey из бинара (DCE-кавета; функция вписана static used, consumer не транскрибирован).
- Byte-1:1 gtp-данных (п. 5 выше) — нет соответствующих дампов адресов; вынесено отдельной задачей.

## 6. Границы выполнения (доказательства)
- Записи ТОЛЬКО в `mtc_drivers/drivers/misc/mtc/` (5 файлов) + REPORTS/t6_exec_transcription.md (этот).
- t9/base: `md5sum -c /tmp/md5_before.txt` → 15/15 OK — read-only подтверждено; sync в D НЕ делал.
- git: мутаций не было. t9/build_fixes.log: не писал.
