# T5 build status (r22 — vs.c/keys.c green + timeconst.pl infra-patch)
2026-09-28 07:08; toolchain arm-eabi-gcc 4.6.x-google; base=/home/amper/Coding/mtc_build/ref_kernel
## Fixes (T5 total)
- backview.c 6 | vs.c 8 + int-return (container_of struct mtc_vs_port, pdev void*-cast, vs_dowork-flags, del-tentative, byte, tty_buffer-поля, car.h, kmalloc)
- keys.c 5 | shared.h:313 int | car.c:251 int | radio_tef6606.c:98 int | dvd.c 6
- r22 (this run): patches/timeconst.pl.patch — host perl 5.44 vs kernel 3.0 (defined(@array) FATAL, :373 -> `if (!@val)`; единственное место во всех base *.pl, dry-run+perl -c OK) + apply_overlay.sh: новый idempotent-step apply_patch
## SMOKE — GREEN (rc=0), 9 mtc .o + board-mtc.o:
38612  ers/misc/mtc/car.o
5608  ers/misc/mtc/dvd.o
2920  ers/misc/mtc/lcd.o
19328  isc/mtc/backview.o
86214  isc/mtc/built-in.o
53068  rk3188/board-mtc.o
5224  rs/misc/mtc/keys.o
3828  s/misc/mtc/codec.o
13520  tc/radio_tef6606.o
2920  vers/misc/mtc/tv.o
8424  vers/misc/mtc/vs.o
## FULL build1 — DEAD (pid 97339), stop в sound/soc/rk29 (driver-code, НЕ infra)
- infra-блокер снят: TIMEC kernel/timeconst.h пройден (log L237, без Error 255)
- стоп: rk29_wm8731.c:63,65 `snd_soc_dai_set_fmt(dai)` — 1 арг; source-API 2-арг (include/sound/soc-dai.h:115 `int snd_soc_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)`); + warn :81-82 (init/ops ptr)
- IDA: бинарный set_fmt (ea=0xc0855bb8, 52B) — 1 арг, dai->fmt НЕ пишет, вызывае ops set_fmt(dai) => fmt de-facto unused. Для T3: добавить 2-й аргумент (по бинару — любой, напр. 0). Driver-code fix — вне scope итераций (только infra), файл T3-сессии не трогал
- build1.log 2637 строк: scripts_basic->drivers/ зелёные; лог до фикса сохранён: build1.timeconst-fail.log. После фикса T3: FULL=1 bash scripts/build.sh (итерация 2/3)
## keys.o (T6-вопрос module_platform_driver)
- nm -g | ' T ' = 8: key_beep, key_enter_mode, mtc_getTouchKey, mtc_getTouchKey_tab, mtc_key_resume, mtc_key_suspend, send_event_key, send_ir_key
- keys_probe — static (глобального нет); keys.c:824 `module_platform_driver(mtc_keys_driver)` => ДА, авто-регистрация через макрос есть
## Notes/risks
- audio_card_glue.c не в smoke/obj (118 ошибок не блокируют) | DVD-офсет-дрейт (runtime-drift risk, без данных не трогали)
## Files for devops commit (mtc_drivers, git НЕ трогал)
- drivers/misc/mtc/{vs.c, keys.c, shared.h, car.c, backview.c, radio_tef6606.c, dvd.c}
- scripts/apply_overlay.sh | patches/timeconst.pl.patch | REPORTS/t5_build_status.md
- (sound/soc/rk29/* — файлы T3-сессии, не мои)
T5 r28-29 FINAL: rds_af_process@0xc0836748 + Af_Check@0xc08363ac transcribed; default_sdmmc2_data SDK-def; _const_udelay->__const_udelay; board-mtc.c +missing #endif CONFIG_SDMMC1_RK29. SMOKE green (11 .o + board-mtc.o, 0 error). FULL build SUCCESS (pid 605096): LD vmlinux + SYSMAP clean, vmlinux 10986530 bytes ELF ARM EABI5; mtc syms in System.map: c06cb670 T mtcGetSetVolume, c06ced54 T rds_af_process, c0bac604 D default_sdmmc2_data.
