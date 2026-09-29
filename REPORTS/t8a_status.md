# T8a status — DONE (2026-09-29 10:54 MSK)

Smoke: GREEN — 12 .o (11 mtc + board-mtc.o), 0 error. Финальный фикс: GCC4.6 не принял `void) __attribute__((noinline))` → `static noinline void car_avm(void);` (car.c л.234/5227).
build1: OK — vmlinux (10997108 B) + System.map регенерированы 10:53:45, 0 error; pid=649617 (build1.pid), лог build1.log (2560 строк).

Recheck 16 T6-символов — было→стало (.o | System.map):
| символ | .o | System.map |
|---|---|---|
| dvd_play_cmd | dvd.o T | c06cbb20 |
| dvd_stop_cmd | dvd.o T | c06cbb50 |
| dvd_send_command_direct | dvd.o (было только .part.5) | c06cbb80 |
| lcd_show_symbol | lcd.o T | c06d13d4 |
| mtc_keycode | keys.o R (data) | c09f863c |
| mtc_keydefault | keys.o R (data) | c09f86c8 |
| mtcWipeCheck | car.o T | c06c75f0 |
| mtc_clear_screen | car.o T | c06c7a40 |
| car_avm | car.o t (нлокал, noinline) | c06c4da0 |
| wm8731_trigger | НЕТ — 0-вызывающий static (N/A) | НЕТ (N/A) |
| mtc_test_port | car.o T | c06c7ca4 |
| mtc_test_port2 | car.o T | c06c7690 |
| mtc_test_port3 | car.o T | c06c77b8 |
| mtc_debug_putc | car.o T | c06c7ba8 |
| mtc_debug_put_string | car.o T | c06c7c38 |
| mtc_direct_fb_buf | backview.o T (реконстр.) | c06ca814 |

Root-cause: dvd×3+lcd×1+car×10 static-off (→T); car_avm inlining→noinline; mtc_keycode/keydefault — data-таблицы byte-exact из ELF; wm8731_trigger — 0-вызывающий static, вычищен cc (N/A, в оригинале 8B t LOCAL); mtc_get_screen_height (car.o T) + mtc_direct_fb_buf (backview.o) — реконструированы по бинарю; step6 N/A — caller'ы вне mtc-модуля (factory_test←host20, WipeCheck←patched request_suspend_state).
Devops (коммит, git не трогал): drivers/misc/mtc/{dvd.c, lcd.c, car.c, keys.c, codec.c, backview.c}
