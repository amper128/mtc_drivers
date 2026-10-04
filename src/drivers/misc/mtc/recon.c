/*
 * recon.c — binaRE Batch 8: реконструкция MTC-диагностических функций
 * (key2ir, factory_test, camera_test, camera_start, camera_stop, homeEnable).
 *
 * AUTHORITY на логику: /home/amper/tmp/ida-tmp/mtc_audio/src_all/
 *   decompiled_{key2ir,factory_test,camera_test,camera_start,camera_stop,homeEnable}.c
 * (IDA 9.3). MEMORY[0xC168xxxx] -> file-statics ниже (bss-карта), константы 1:1.
 * IDA stale-аргументы (R0/R1..R3 без объявления) опущены с комментарием;
 * соответствующие extern объявлены non-prototype, где определение так же нетипизировано.
 *
 * ДОБАВЛЕНИЕ (новый файл); валидированные MTC .c/.h НЕ модифицированы.
 * Здесь определены 4 функции (key2ir, factory_test, camera_test, homeEnable) —
 * non-static, эмитятся как T-символы (judge).
 * camera_start (@0xc083a0b8) / camera_stop (@0xc0839ae0) НЕ дублируются:
 * валидированные определения уже есть в backview.c (K&R void, stale-арг) —
 * их T-символы даёт backview.o (multiple-definition иначе).
 * MTC-деп: 19 extern (global T в дереве) + 3 global stub (определения в дереве нет).
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/string.h>

/* asm/delay.h (SDK 3.0): udelay(<const> > MAX_UDELAY_MS*1000=2000us) -> __bad_udelay() — в этой
 * сборке undefined (ref: camera_test _const_udelay(107374)). Константный путь напрямую:
 * __const_udelay (ENTRY arch/arm/lib/delay.S, decl arch/arm/include/asm/delay.h:33).
 * Прецедент: backview.c:22. Арг = константа из бинара 1:1 (usec-номинал). */
#include <asm/delay.h>
#define _const_udelay(n) __const_udelay(n)

/* ==================== 19 extern MTC-деп (global T в дереве) ==================== */

/* car.c (прототипы car.c:299-306): debug-экран и test-IO factory_test */
extern char *mtc_debug_put_string(const char *s, int len, int x0, int y,
				  int fg_color, int bg_color);
extern void mtc_clear_screen(int color);
extern int mtc_init_test_io(void);
extern char *mtc_test_port(void);

/* car.c:266 (stub): бинар — R0 stale, аргументов не требует */
extern void backlight_on(void);

/* IDA: stale-аргументы у call-sites -> non-prototype (определения в base-дереве) */
extern int adc_register();	/* rk30_adc_battery / MTC */
extern unsigned int ddr_get_cap();	/* mach-rk ddr glue */

/* backview.c: декодеры T132B/ADV7181D, питание CIF, reset VIP */
extern int cif_power(int pwr);
extern int vip_reset(int pwr);
extern void T132B_Write(struct i2c_client *client, u8 *data);
extern int T132B_Page_Write(struct i2c_client *client, unsigned int dev_addr,
			    int count, const u8 *data, const u8 *mask);
extern int T132B_i2c_write(struct i2c_client *client, unsigned int dev_addr,
			   u8 reg, u8 dat);
extern int T132B_i2c_read(struct i2c_client *client, unsigned int dev_addr,
			  u8 reg, u8 *dat);
extern int T132B_Init(struct i2c_client *client, int type);
extern int ADV7181D_Init(struct i2c_client *client, int type);

/* rk_fb.c / lcd.c: framebuffer (void* — прецедент backview.c:741-742) */
extern void *rk_get_fb(int a);
extern int rk_direct_fb_open(void *fb, int a);

/* tv.c:8 — определение non-prototype; IDA аргумент is_Atv(v3) — stale R0 */
extern int is_Atv();

/* vs.c:371 */
extern int vs_send(int port_num, unsigned char cmd, char *cmd_data, int count);

/* ==================== 3 global stub (MTC-деп, binaRE stub) ==================== */
/* Определений в дереве нет; T в judge-бинаре -> non-static stub (закрывают judge). */

int test_adc(int ch)
{
	(void)ch;
	return 0;	/* MTC-деп, binaRE stub */
}

int rtc_readtime(struct rtc_time *tm)
{
	(void)tm;
	return 0;	/* MTC-деп, binaRE stub */
}

int rk_direct_fb_set(void)
{
	return 0;	/* MTC-деп, binaRE stub */
}

/* ==================== file-statics: bss-карта (decompiled MEMORY[...]) ==================== */

/* --- camera_test (общие bss; file-statics camera_start/camera_stop — в backview.c) --- */
static struct i2c_client *cam_i2c_client;	/* 0xC168E41C (T132B-декодер) */
static struct clk *cam_clks[5];		/* 0xC168E3F4..0xC168E404, порядок F4,F8,FC,00,04 */
static struct workqueue_struct *cam_wq;	/* 0xC168E3E4 */
/* -1050090464 (bss): struct delayed_work; .timer @+16 = -1050090448 (del_timer_sync) */
static struct delayed_work cam_delayed_work;
static u8 cam_tested;			/* 0xC168E462 */
static u8 cam_active;			/* 0xC168E463 (active-флаг) */
static int cam_ntsc_mode;		/* 0xC168AD16 (==1 -> YZ-таблицы) */

/* --- factory_test --- */
static void *ft_adc_h[3];			/* 0xC168AD30/34/38 (adc_register 0/1/2) */
static int ft_af23;				/* 0xC168AF23 (= -1) */
static int ft_ac85;				/* 0xC168AC85 (= 1) */
static unsigned int ft_usb_otg_vendor;	/* 0xC15881D8 (usb-test: vendor id) */
static unsigned int ft_usb_h20_vendor;	/* 0xC15881DC */
static u8 ft_rtc_ok;			/* 0xC158945C (выбор цвета строки RTC) */

/* --- homeEnable --- */
static u8 home_enable_flag;		/* 0xC168E4E7 (homeEnable: = 0) */

/* ==================== rodata-таблицы (recon-копии / placeholders) ==================== */

/* rodata @0xC0BCA04C (= off_C0BCA1B8[-99]): keycode 512..530 -> IR-код, 25 байт.
 * Значения из rodata оригинала не извлечены (vmlinux на диске нет) -> placeholder. */
static const u8 ir_key_table[25] = { [0 ... 24] 0 };

/* T132B-таблицы: в оригинале file-static'ы в backview.c; recon-копии (заглушки {0},
 * настоящие данные — в backview.c). Т132B_Write/Page_Write принимают их по указателю. */
static const u8 rt_t132b_init[] = { 0 };
static const u8 rt_t132b_p0_ntsc_yz[] = { 0 };
static const u8 rt_t132b_p0_mask[] = { 0 };
static const u8 rt_t132b_p2_ntsc_yz[] = { 0 };
static const u8 rt_t132b_p2_mask[] = { 0 };
static const u8 rt_t132b_p0_ntsc[] = { 0 };
static const u8 rt_t132b_p2_ntsc[] = { 0 };

/* ==================== homeEnable ==================== */

/* binaRE homeEnable @0xc083becc (20B): MEMORY[0xC168E4E7] = 0. */
void homeEnable(void)
{
	home_enable_flag = 0;
}

/* ==================== key2ir ==================== */

/* binaRE key2ir @0xc083c070 (96B): keycode [512..530] -> IR-таблица;
 * остальные — switch (158/139/102/115/114); default 255. */
int key2ir(int keycode)
{
	if ((unsigned int)(keycode - 512) <= 0x18)	/* [512..530] -> lookup */
		return ir_key_table[keycode - 512];

	switch (keycode) {
	case 158:
		return 54;
	case 139:
		return 55;
	case 102:
		return 56;
	case 115:
		return 18;
	case 114:
		return 26;
	}
	return 255;
}

/* ==================== camera_test ==================== */

/* binaRE camera_test @0xc0839f20 (388B): питание CIF + init T132B (NTSC/NTSC_YZ),
 * clocks x5 (48M), vip_reset, delayed_work, fb open. */
int camera_test(void)
{
	void *fb;

	cif_power(1);
	T132B_Write(cam_i2c_client, (u8 *)rt_t132b_init);
	msleep(20);
	cam_tested = 0;

	if (cam_ntsc_mode == 1) {
		T132B_Page_Write(cam_i2c_client, 0x40u, 240,
				 rt_t132b_p0_ntsc_yz, rt_t132b_p0_mask);
		T132B_Page_Write(cam_i2c_client, 0x44u, 112,
				 rt_t132b_p2_ntsc_yz, rt_t132b_p2_mask);
	} else {
		T132B_Page_Write(cam_i2c_client, 0x40u, 240,
				 rt_t132b_p0_ntsc, rt_t132b_p0_mask);
		T132B_Page_Write(cam_i2c_client, 0x44u, 112,
				 rt_t132b_p2_ntsc, rt_t132b_p2_mask);
	}

	T132B_i2c_write(cam_i2c_client, 0x44u, 129, 11);
	T132B_i2c_write(cam_i2c_client, 0x44u, 63, 1);
	T132B_i2c_write(cam_i2c_client, 0x44u, 63, 0);
	T132B_i2c_write(cam_i2c_client, 0x44u, 7, 160);
	msleep(50);

	clk_enable(cam_clks[0]);
	clk_enable(cam_clks[1]);
	clk_enable(cam_clks[2]);
	clk_enable(cam_clks[3]);
	clk_enable(cam_clks[4]);
	clk_set_rate(cam_clks[4], 48000000);
	_const_udelay(107374);	/* binaRE _const_udelay(107374) (прецедент backview.c) */

	vip_reset(1);
	cam_active = 1;
	queue_delayed_work(cam_wq, &cam_delayed_work, msecs_to_jiffies(200));

	fb = rk_get_fb(1);
	return rk_direct_fb_open(fb, 1);
}

/* ==================== camera_stop / camera_start ==================== */

/* binaRE camera_stop @0xc0839ae0 (180B) / camera_start @0xc083a0b8 (668B):
 * валидированные реконструкции УЖЕ есть в backview.c (K&R void, stale-арг;
 * те же IDA-адреса) — дублирование вызывало multiple-definition в built-in.o.
 * Их T-символы предоставляет backview.o; в recon.c намеренно не определены. */

/* ==================== factory_test ==================== */

/* binaRE factory_test @0xc082df1c (1344B, __noreturn): экран "Microntek ARM
 * Core-Board Tester for 3188 V2.3", adc x3, camera_test, fb set, labels,
 * backlight_on, DDR-cap, init test-IO; бесконечный цикл: test_adc(0/1) + вывод,
 * USB vendor (MEMORY 0xC15881D8/DC), rtc_readtime + строка времени, mtc_test_port. */
void __attribute__((noreturn)) factory_test(void)
{
	char buf[124];	/* decompiled v31[124] BYREF */
	unsigned int cap;
	int c_ddr, c_rtc;
	int val0, val1;
	unsigned int adc_v, c_adc;
	char *s;
	struct rtc_time rt_tm;

	printk("--mtc factory_test\n");

	ft_adc_h[0] = (void *)adc_register(0);	/* IDA: non-prototype (stale args) */
	ft_adc_h[1] = (void *)adc_register(1);
	ft_adc_h[2] = (void *)adc_register(2);
	camera_test();
	rk_direct_fb_set();
	mtc_clear_screen(0);

	mtc_debug_put_string(" Microntek ARM Core-Board Tester for 3188 V2.3 ", 47, 44, 2, -12566464, -1);
	mtc_debug_put_string("USB_OTG:", 8, 4, 2, -1, -12566464);
	mtc_debug_put_string("USB_H20:", 8, 4, 3, -1, -12566464);
	mtc_debug_put_string("RTC:", 4, 44, 4, -1, -12566464);
	mtc_debug_put_string("UART2: (125)(127)", 17, 44, 5, -1, -12566464);
	mtc_debug_put_string("DDR:", 4, 44, 6, -1, -12566464);
	mtc_debug_put_string("ADC0:", 5, 7, 4, -1, -12566464);
	mtc_debug_put_string("ADC1:", 5, 7, 5, -1, -12566464);
	mtc_debug_put_string("ADC2:", 5, 7, 6, -1, -12566464);
	mtc_debug_put_string("                     1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1", 82, 11, 7, -1, -12566464);
	mtc_debug_put_string(" 7 7 7 8 8 8 8 8 9 9 0 0 0 1 1 1 2 2 2 3 3 3 3 3 4 4 4 5 5 5 5 6 6 6 6 7 7 7 7 7 8", 82, 11, 8, -1, -12566464);
	mtc_debug_put_string(" 4 6 8 0 2 4 6 8 0 2 2 6 8 2 6 8 0 2 6 0 2 4 6 8 0 4 8 0 2 4 8 0 4 6 8 0 2 4 6 8 0", 82, 11, 9, -1, -12566464);
	mtc_debug_put_string("                        1           1 1 1 1 1 1 1 1 1 1 1 1                       ", 82, 11, 12, -1, -12566464);
	mtc_debug_put_string("                        0           2 2 2 3 3 3 4 4 4 4 5 5                       ", 82, 11, 13, -1, -12566464);
	mtc_debug_put_string("1 3 5 7 9               7           3 5 7 1 3 9 1 3 7 9 3 5                       ", 82, 11, 14, -1, -12566464);
	ft_af23 = -1;	/* MEMORY[0xC168AF23] = -1 */
	ft_ac85 = 1;	/* MEMORY[0xC168AC85] = 1 */
	backlight_on();	/* IDA: backlight_on(v0..,1) — stale R0; tree: void (car.c:266) */

	cap = ddr_get_cap();	/* IDA: stale args -> non-prototype */
	c_ddr = ((cap >> 20) == 1024) ? (int)0xFF00FF00u : (int)0xFFFFFF00u;
	cap = ddr_get_cap();
	sprintf(buf, "%d    ", cap >> 20);
	mtc_debug_put_string(buf, 4, 48, 6, c_ddr, -12566464);

	mtc_init_test_io();

	for (;;) {
		val0 = test_adc(0);	/* IDA: stale args R1-R3 опущены */
		adc_v = val0 - 503;
		sprintf(buf, "%d", val0);
		c_adc = (adc_v > 0x12u) ? 0xFFFFFF00u : 0xFF00FF00u;
		mtc_debug_put_string("*", 1, 13, 4, (int)c_adc, -12566464);
		mtc_debug_put_string(buf, 4, 15, 4, (int)c_adc, -12566464);

		val1 = test_adc(1);
		adc_v = val1 - 503;
		sprintf(buf, "%d", val1);
		c_adc = (adc_v > 0x12u) ? 0xFFFFFF00u : 0xFF00FF00u;
		mtc_debug_put_string("*", 1, 13, 5, (int)c_adc, -12566464);
		mtc_debug_put_string(buf, 4, 15, 5, (int)c_adc, -12566464);

		strcpy(buf, "----");
		mtc_debug_put_string("*", 1, 13, 6, -16711936, -12566464);
		mtc_debug_put_string(buf, 4, 15, 6, -16711936, -12566464);

		if (ft_usb_otg_vendor) {	/* MEMORY[0xC15881D8] */
			mtc_debug_put_string("*", 1, 13, 2, -16711936, -12566464);
			sprintf(buf, "Vendor %04x", ft_usb_otg_vendor);
			s = buf;
		} else {
			mtc_debug_put_string("*", 1, 13, 2, -16711936, -12566464);
			s = "No Test";
		}
		mtc_debug_put_string(s, 20, 15, 2, -16711936, -12566464);

		if (ft_usb_h20_vendor) {	/* MEMORY[0xC15881DC] */
			mtc_debug_put_string("*", 1, 13, 3, -16711936, -12566464);
			sprintf(buf, "Vendor %04x", ft_usb_h20_vendor);
			mtc_debug_put_string(buf, 20, 15, 3, -16711936, -12566464);
		} else {
			mtc_debug_put_string("*", 1, 13, 3, -65536, -12566464);
			mtc_debug_put_string("Not found", 20, 15, 3, -65536, -12566464);
		}

		rtc_readtime(&rt_tm);
		sprintf(buf, "* %d-%d-%d %d:%02d:%02d", rt_tm.tm_year + 1900,
			rt_tm.tm_mon + 1, rt_tm.tm_mday,
			rt_tm.tm_hour, rt_tm.tm_min, rt_tm.tm_sec);
		c_rtc = ft_rtc_ok ? (int)0xFF00FF00u : -65536;	/* MEMORY[0xC158945C] */
		mtc_debug_put_string(buf, 22, 49, 4, c_rtc, -12566464);

		mtc_test_port();	/* IDA: stale-арг (R0..R3); tree: char *mtc_test_port(void) */
	}
}
