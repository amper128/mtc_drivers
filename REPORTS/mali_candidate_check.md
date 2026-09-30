# Проверка Mali/UMP кандидата (ARM reference r3p2-01rel1, omegamoon/rockchip-rk3188-generic)

**ВЕРДИКТ: КАНДИДАТ OK** — критерии (a)+(b) зелёные, OOT-сборка без трогания vmlinux.

## (a) vermagic — GREEN
mali.ko и ump.ko: `vermagic=3.0.36+ SMP preempt mod_unload ARMv7` (не 3.0.8+ как у SDK-prebuilt).

## (b) UNDEFINED-полнота — GREEN
- mali.ko: U=116 уникальных, **unresolved=0** (проверка по System.map ref_kernel, 47 736 симв. + ump.ko-экспорты, 104).
- ump.ko: U=62 уникальных, **unresolved=0** (по System.map).
- mali→ump cross-refs (модульная зависимость): `ump_dd_handle_create_from_secure_id, ump_dd_phys_block_count_get, ump_dd_phys_blocks_get, ump_dd_reference_release` — все EXPORT_SYMBOL в ump.ko.

## (c) Символы vs orig (3188_kallsyms, CRLF учтён)
- orig [mali]: 1192 строки / 652 уникальных; пересечение с mali.ko: **382** (~59%).
- orig [ump]: 217 строк / 102 уникальных; пересечение с ump.ko: **98** (~96%).
- cross: mali.ko∩orig[ump]=29, ump.ko∩orig[mali]=26.
- **Caveat (честно)**: orig [mali] — те же `_mali_osk_*` имена (т.е. kallsyms уже из reference-порта, не RK-закрытый `mali400_*`), высокий overlap ожидаем и НЕ есть критерий pass/fail.

## (d) Модули и порядок загрузки
- Имена: `mali`, `ump` (= имена .ko); mali.ko содержит `depends=ump`, ump.ko — без deps.
- **insmod order: ump.ko → mali.ko** (modprobe разберётся сам).

## Артефакты (создано)
`/home/amper/Coding/mtc_build/modules/ref_mali/{mali.ko=155777B, ump.ko=45013B, md5.txt, build_info.txt}`
md5: mali.ko `eb7d67b84fc067f194f63f84e188f4d9`, ump.ko `185f54773ff9580182c71dfe0f4fa1a0`.

## Сборка (OOT, логика SDK: Kbuild приоритетен над Makefile)
Копия `/home/amper/Coding/mtc_build/mali_oot/` (repos-клон НЕ тронут); TC arm-eabi GCC 4.6.x; KDIR=ref_kernel.
Точечные фиксы ТОЛЬКО в копии: 1) `ump/arch-rk3x` symlink→реальный каталог (config.h, OS-mem backend 64MB); 2) `mali/Kbuild: +ccflags-y -DCONFIG_MALI400_UMP=1` (эмуляция autoconf.h — иначе UMP-код в mali не компилируется). build-info в .ko: API_VERSION=20 BUILD=release TARGET_PLATFORM=rk30 USING_UMP=y.

## Инварианты
vmlinux (10 458 145 B) и System.map: md5 DO/POСЛЕ = OK; Module.symvers ref_kernel не изменён.

## Userspace caveat
Нужна **ARM reference libmali r3p2 (API 20)** в userspace — НЕ vendor-SDK libmali (другой ABI/ioctl). Драйвер регистрирует platform_driver "mali-utgard" без of_match: runtime нужен platform-устройство/глюк в mach-коде; UMP: /dev/ump, OS-memory backend.
