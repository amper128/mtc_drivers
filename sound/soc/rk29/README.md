# sound/soc/rk29 — T3 PLACEHOLDER (not part of the build yet)
T3 will add here (rsync-ready FULL files, see PLAN §A):
- rk29_wm8731.c — new machine driver (binaRE from src_all/decompiled_{rk29_wm8731_init,audio_card_init,wm8731_*}.c).
  initcalls: __initcall_wm8731_modinit6 (c042ba74) and __initcall_audio_card_init6 (c042ba80) — both level 6,
  after goodix_ts_init6 in the binary; wm8731 codec itself is in ref sound/soc/codecs/ — DO NOT touch.
- Makefile, Kconfig — patch copies of sound/soc/{Makefile,Kconfig} (+rk29/, CONFIG_RK29_WM8731).
Note (PLAN R4): audio_card_init7 (c042bcf0) = mtc-audio (mtc-audio.c) — level 7, separate TU;
name collision resolved by T3 via static/weak.
