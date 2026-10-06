# MTC built-in drivers.
# ORDER MATTERS: the .initcall level-6 entries of the reference binary
# (3188_kallsyms) appear as __initcall_tm_init6 < dvd_init6 < vs_init6 <
# tef6606_init6 < lcd_init6 < keys_init6 (c042ba08..c042ba58). Objects without
# level-6 initcalls (car.o = level 5s, backview.o = level 5, codec.o = none) do
# not affect that tail cluster; they are listed first / last accordingly.
# KNOWN RISK (T6 must verify, see REPORTS/t1_t2_status.md): drivers/misc links
# early in drivers/ (drivers/Makefile line 45, before hid/ line 114), while in
# the binary tm_init6 sits among the HID tablet drivers — expected rank drift.
obj-$(CONFIG_MTC_DRIVERS) += car.o backview.o tv.o dvd.o vs.o mtcGetSetVolume.o audio_card_glue.o radio_tef6606.o lcd.o keys.o codec.o
# Batch 8 (binaRE): MTC-диагностические функции (key2ir/factory_test/camera_test/camera_start/camera_stop/homeEnable) — новый файл, без initcall.
obj-$(CONFIG_MTC_DRIVERS) += recon.o
# G5 (binaRE): gup_enter_update_mode/gup_leave_update_mode/gup_update_proc — stubs 1:1 от target vmlinux (без initcall).
obj-$(CONFIG_MTC_DRIVERS) += gup_stubs.o
# G8 (binaRE): wifi_driver_write/wifi_pcba_read/wifi_pcba_write/wifi_power_write — static 't' 1:1 от target vmlinux (без initcall).
obj-$(CONFIG_MTC_DRIVERS) += wifi.o
