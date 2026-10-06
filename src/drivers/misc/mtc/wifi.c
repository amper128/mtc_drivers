/*
 * wifi.c — binaRE G8 (группа 8/8): реконструкция 4 MTC wifi-функций
 * из target vmlinux (3188_kallsyms + IDA decomp, /home/amper/tmp/ida-tmp/mtc_audio/src_all/).
 *
 *   c07114c0 t wifi_pcba_write     52B
 *   c07114f8 t wifi_driver_write  144B
 *   c07115a0 t wifi_power_write    68B
 *   c07115ec t wifi_pcba_read      28B
 *
 * AUTHORITY на тела: decompiled_wifi_{driver_write,pcba_read,pcba_write,power_write}.c (1:1).
 * Все 4 — 't' (local) в target => static в этом .o (как G7 tool_i2c_* в recon.c);
 * без инициализаторов/колбэков (не дублируют другие объекты).
 *
 * MEMORY-карта original object:
 *   0xC158448C = wifi_pcba       (int) — состояние pcba
 *   0xC15844A4 = wifi_driver_on  (int) — 0 = rmmod / !=0 = insmod
 *   semaphore (down/up в wifi_driver_write) = wifi_wl_sem
 * IDA-константы: simple_strtol(base=0xAu=10); stale a1/a2 — опущены (void).
 *
 * ДЕП (extern, определения в дереве):
 *   rockchip_wifi_init_module / rockchip_wifi_exit_module:
 *     src/drivers/net/wireless/rkusbwifi/rtl8192cu/os_dep/linux/usb_intf.c:1622/1634
 *     (CONFIG_RTL8192CU=y — единственная активная реализация).
 *   rk29sdk_wifi_power: ref_kernel board-rk30-sdk-sdmmc.c (см. shared.h:334, car.c).
 */

#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/string.h>

/* --- MTC/wifi-деп (extern) --- */
int rockchip_wifi_init_module(void);
void rockchip_wifi_exit_module(void);
int rk29sdk_wifi_power(int on);

/* --- globals оригинального объекта (MEMORY-карта выше) --- */
static int wifi_pcba;      /* MEMORY[0xC158448C] */
static int wifi_driver_on; /* MEMORY[0xC15844A4] */
DEFINE_SEMAPHORE(wifi_wl_sem);

static int wifi_pcba_write(void *a1, void *a2, const char *buf, int count)
{
	int v;

	(void)a1; (void)a2;

	v = simple_strtol(buf, NULL, 10);
	wifi_pcba = v;
	if (v > 0)
		wifi_pcba = 1;
	return count;
}

static int wifi_driver_write(void *a1, void *a2, const char *buf, int count)
{
	int v;

	(void)a1; (void)a2;

	down(&wifi_wl_sem);
	v = simple_strtol(buf, NULL, 10);
	if (wifi_driver_on != v) {
		if (v <= 0) {
			rockchip_wifi_exit_module();
		} else if (rockchip_wifi_init_module() < 0) {
			up(&wifi_wl_sem);
			return count;
		} else {
			wifi_driver_on = v;
		}
		up(&wifi_wl_sem); /* LABEL_5 */
		return count;
	}

	printk("%s: wifi driver already %s\n", "wifi_driver_write",
	       wifi_driver_on ? "insmod" : "rmmod");
	up(&wifi_wl_sem);
	return count;
}

static int wifi_power_write(void *a1, void *a2, const char *buf, int count)
{
	int v;

	(void)a1; (void)a2;

	v = simple_strtol(buf, NULL, 10);
	printk("%s: poweren = %d\n", "wifi_power_write", v);
	rk29sdk_wifi_power(v > 0);
	return count;
}

static int wifi_pcba_read(void *a1, void *a2, char *buf)
{
	(void)a1; (void)a2;

	return sprintf(buf, "%d", wifi_pcba);
}

/* Анкер: в оригинальном объекте на 4 static-функции ссылались символы вне
 * набора G8 (fops-таблица объекта); без ссылки компилятор их выкинул бы.
 * __used => локальная data, в kallsyms T/t не попадает. */
static void *const wifi_g8_anchor[4] __used = {
	(void *)wifi_driver_write, (void *)wifi_pcba_read,
	(void *)wifi_pcba_write, (void *)wifi_power_write,
};
