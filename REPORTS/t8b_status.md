# T8b status — mtc_defconfig drift-подгонка под orig-бинар + rebuild (DONE)
Pipeline: apply_overlay.sh → build.sh mtc_defconfig (oldconfig) → smoke GREEN (12 .o) → FULL=1 build pid 698524
→ OK: LD vmlinux + SYSMAP, 0 errors (incremental). Build: ref_kernel/{vmlinux,System.map} 2026-09-29 11:43.
% common/orig: **95.50 → 95.95** | % added/our: **20.10 → 17.96** (orig=40816, our=47736, common=39162, added=8574, missing=1654)

| CONFIG-семья | orig-статус (kallsyms-sig) | наш action | новая System.map |
|---|---|---|---|
| CIFS | cifs_*=0 | =n | 0 ✓ (+224 убрано) |
| UDF_FS, UDF_NLS | udf_*=0 | =n×2 | 0 ✓ (+133) |
| ISO9660_FS, JOLIET, ZISOFS (НАЙДЕНО в rebuild-верификации) | isofs_*/zisofs=0 | =n×3 | 0 ✓ (+39; это отдельная fs/isofs, НЕ часть UDF_FS в kernel 3.0) |
| FIQ_DEBUGGER + NO_SLEEP/CONSOLE/_DEFAULT_ENABLE ×4 | fiq_debugger_*=0 (orig: только vector_fiq/do_unexp_fiq — core-векторы) | =n×4 | 0 ✓ (+25) |
| SOC_CAMERA, SOC_CAMERA_OV2659 | soc_camera_*=0, sensor_*=0 | =n×2 | 0 ✓ (+135; sensor_*=85 ВСЕ из ov2659.o — nm-проверено) |
| VIDEO_RK29, VIDEO_RK29_CAMMEM_ION | videobuf_*=0 (depends SOC_CAMERA) | =n×2 | 0 ✓ |
| INPUT_TABLET, TABLET_USB_WACOM | wacom_*=95 | =y×2 | 99 ✓ (появились) |
| INPUT_TOUCHSCREEN, TOUCHSCREEN_GT82X_IIC | goodix_*=11 (инстанс init6) | =y×2 | 13 ✓; init7-инстанс НЕ воспроизводим (только late_initcall goodix-драйверы на I2C2_RK29 — impossible на 3188) |
| BACKLIGHT_LCD_SUPPORT, CLASS_DEVICE, RK29_BL | backlight_*=44, rk29_backlight_*=12 | =y×3 | 12 rk + 22 generic ✓ |
| USB_HIDDEV | hiddev_*=17 | =y | 18 ✓ |
| KEYS_RK29 | orig: rk28_send_wakeup_key=1, rk29_keys_pdata=0 | ОСТАВЛЕНО =y (default) | tried =n → BUILD FAIL: rk29_keys.c = единственное определение rk28_send_wakeup_key (вызывается plat-rk/usb_detect.c + rk29_sdmmc.c; link-ошибка). Двойной keys_init (c041e1e0 mtc + c041f9c0 rk29) = source-revision drift orig |
| FTRACE, KPROBES, IP_NF_*/arptable/ip6t, EXT4, AUDIT, CFG80211, MAC80211, V4L_USB*, USB_VIDEO_CLASS, RTL8192CU, DDR_*, ACT8846 | символы ЕСТЬ в orig | ОСТАВЛЕНО (off-догадки t6 опровергнуты symbol-доказательствами) | без изменений |
| MALI400 / UMP | mali*=440 в orig | TBD — драйвера нет в SDK-дереве (drivers/mali* — No such file, GPU-Kconfig пуст) | mali_*=0: source-gap (нужен портированный mali400-драйвер), НЕ config-проблема; потолок common/orig ~95.95% до этого |

defconfig diff (HEAD→now): **26+/16-** (T8b-маркеры: 26 строк, конвенция `/* T8b ±orig-sig: <символы> */`)
Smoke: GREEN 12 .o (built-in.o 143481, board-mtc.o 30276, ...)
Build1: OK, pid 698524, /home/amper/Coding/mtc_build/build1.log (LD vmlinux + SYSMAP, без ошибок)
Остаток missing=1654: mali*(440) = главный блок; остальное — мелкие revision-drift (см. t6_final)
Для коммита devops: {config/mtc_defconfig} (git не трогал — read-only git diff только для проверки)
