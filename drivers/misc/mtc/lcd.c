/*
 * mtc-lcd.c - reconstructed from binaRE (IDA 9.3 decompiled), kernel.elf RK3188.
 * Owns: Lcd_work, lcd_show_symbol, lcd_show_dig, lcd_flash, Lcd_Flash_work,
 *       lcd_show (call-site: mtc-car.c ctl_lcd), lcd_on, lcd_off (+ body of lcd_probe).
 * bss: page u8[32] @0xC168E330; enabled @0xC168E350; delayed_work @0xC168E354
 *      (func=Lcd_work, timer @0xC168E364); flash-mode @0xC168E380, flash-idx @0xC168E381;
 *      delayed_work @0xC168E384 (func=Lcd_Flash_work, timer @0xC168E394);
 *      mutex @0xC0BCA12C.  rodata: lcd_seg_tab @0xC0A09D28 (396 B).
 */
#include <linux/module.h>
#include <linux/time.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>

#include "shared.h"
#include "car.h"

/* cross-TU (mtc-car.c ctl_lcd и др. call-sites) */
void lcd_show(const char *str);
void lcd_flash(int mask);
void lcd_on(void);
void lcd_off(void);

static int
lcd_suspend(struct device * dev)
{
	(void)dev;

	return 0;
}

static int
lcd_resume(struct device * dev)
{
	(void)dev;

	return 0;
}

/* binaRE lcd_delay / lcd_send_cmd (skeleton, kept verbatim) */
static void
lcd_delay(void)
{
	long usec; // r4@1
	long check; // r3@2
	struct timeval tv;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;
	do
	{
		do_gettimeofday(&tv);
		check = tv.tv_usec - usec;
		if ( usec > tv.tv_usec )
		{
			check = tv.tv_usec + 1000000 - usec;
		}
	}
	while ( check <= 5 );
}

static void
lcd_send_cmd(int16_t cmd)
{
	int16_t bit_pos;

	bit_pos = (cmd & 0x1FF) | 0x800;
	gpio_direction_output(gpio_LCD_CS, 0);
	lcd_delay();

	unsigned int bits; /* C89: hoisted */
	for (bits = 12; bits > 0; bits--)
	{
		int bit = 1;

		gpio_direction_output(gpio_LCD_CLK, 0);

		if ( !(bit_pos & 0x800) )
		{
			bit = 0;
		}

		gpio_direction_output(gpio_LCD_DAT, bit);

		lcd_delay();
		bit_pos *= 2;
		gpio_direction_output(gpio_LCD_CLK, 1);
		lcd_delay();
	}

	gpio_direction_output(gpio_LCD_CS, 1);
	gpio_direction_output(gpio_LCD_DAT, 1);
	lcd_delay();
}

/* ---------- binaRE statics ---------- */
static u8 lcd_page[32];                    /* bss @0xC168E330 */
static int lcd_enabled;                    /* bss @0xC168E350 */
static struct delayed_work lcd_work;       /* bss @0xC168E354, func=Lcd_work, timer @0xC168E364 */
static int lcd_flash_mode;                 /* bss @0xC168E380 */
static int lcd_flash_idx;                  /* bss @0xC168E381 */
static struct delayed_work lcd_flash_work; /* bss @0xC168E384, func=Lcd_Flash_work, timer @0xC168E394 */
static struct mutex lcd_mutex;             /* bss @0xC0BCA12C (общий LCD mutex) */

/* rodata 0xC0A09D28..0xC0A09EB3 (396 B; kallsyms: symbol_map@+0, dig_seg@+32,
 *   dig_map@+56, flash_tab@+312/22*u32; конец — lcd_pm_ops@0xC0A09EB4):
 *   [u8 +0..29]    symbol_map: seg-коды (lcd_show_symbol / lcd_flash: page[seg&0x1F] bit(seg>>6))
 *   [u32 +32+4*v2] pattern-words glyph (lcd_show_dig, v2 = glyph idx)
 *   [u32 +56..]    dig_map (lcd_show_dig byte-walk: v6 = dig_map + 14*idx)
 *   [u32 +312..]   flash masks (Lcd_Flash_work: tab[idx+78], idx<=0x14)
 */
static const u32 lcd_seg_tab[99] =
{
	0xC0804000, 0x41810242, 0x0804C2C1, 0x9EDE100C,
	0x0B07035E, 0x1B19150F, 0x5F9FDF1F, 0x00001713,
	0x0000FC00, 0x00006000, 0x0000DB00, 0x0000F300,
	0x00006700, 0x0000B700, 0x0000BF00, 0x0000E000,
	0x0000FF00, 0x0000F700, 0x00000000, 0x00000300,
	0x0000EF00, 0x0000F148, 0x00009C00, 0x0000F048,
	0x00009F00, 0x00008F00, 0x0000BD00, 0x00006F00,
	0x00009048, 0x00007800, 0x00000E30, 0x00001C00,
	0x00006CA0, 0x00006C90, 0x0000FC00, 0x0000CF00,
	0x0000FC10, 0x0000CF10, 0x0000B700, 0x00008048,
	0x00007C00, 0x00000C24, 0x00006C54, 0x000000B4,
	0x00007700, 0x00009024, 0x0647C7C6, 0x8784C444,
	0x468685C5, 0xCBCA0545, 0xC8480A4B, 0x89C98B88,
	0x09494A8A, 0x0E4FCFCE, 0x8F8CCC4C, 0x4E8E8DCD,
	0xD3D20D4D, 0xD0501253, 0x91D19390, 0x11515292,
	0x1455D5D4, 0x95959454, 0x00000000, 0xD7D60000,
	0x96561657, 0x00009797, 0x00000000, 0x1859D9D8,
	0x99999858, 0x00000000, 0xDBDA0000, 0x9A5A1A5B,
	0x00009B9B, 0x00000000, 0x1C5DDDDC, 0x9D9D9C5C,
	0x00000000, 0x00000000, 0x00000071, 0x000000B3,
	0x00000137, 0x0000007F, 0x000000B8, 0x00000134,
	0x00000072, 0x000000B1, 0x00000133, 0x00000077,
	0x000000BF, 0x00000138, 0x00000074, 0x000000B2,
	0x00000131, 0x00000073, 0x000000B7, 0x0000013F,
	0x00000078, 0x000000B4, 0x00000132
};


/* binaRE Lcd_work @0xc0838510 (IDA 9.3 decompiled) */
static void
Lcd_work(struct work_struct *work)
{
	int i;
	int v;

	(void)work;

	mutex_lock(&lcd_mutex);
	gpio_direction_output(gpio_LCD_CS, 0);
	lcd_delay();

	/* 9-bit command 320 (0x140), MSB-first: test (v & 0x100), v <<= 1 */
	v = 320;
	for (i = 9; i > 0; i--)
	{
		gpio_direction_output(gpio_LCD_CLK, 0);
		gpio_direction_output(gpio_LCD_DAT, (v & 0x100) != 0);
		lcd_delay();
		v <<= 1;
		gpio_direction_output(gpio_LCD_CLK, 1);
		lcd_delay();
	}

	/* page[0..31] by 4 bits LSB-first: CLK 0, DAT=v&1, v >>= 1, CLK 1 */
	for (i = 0; i < 32; i++)
	{
		v = lcd_page[i];
		while (4)
		{
			gpio_direction_output(gpio_LCD_CLK, 0);
			gpio_direction_output(gpio_LCD_DAT, v & 1);
			lcd_delay();
			v >>= 1;
			gpio_direction_output(gpio_LCD_CLK, 1);
			lcd_delay();
		}
	}

	gpio_direction_output(gpio_LCD_CS, 1);
	gpio_direction_output(gpio_LCD_DAT, 1);
	lcd_delay();
	mutex_unlock(&lcd_mutex);
}

/* binaRE lcd_show_symbol @0xc083860c (IDA 9.3 decompiled)
 * mask: bits 9..29 (21-bit field) -> symbol_map bytes 1..29; result = mask >> 1
 * consumed LSB-first. seg = byte: page[seg & 0x1F] set/clear bit(seg >> 6). */
void
lcd_show_symbol(unsigned int mask)
{
	unsigned int result = mask >> 1;
	const unsigned char *p = (const unsigned char *)lcd_seg_tab;
	int i;

	for (i = 1; i < 30; i++)
	{
		if (i > 8)   /* active: bits 9..29 of mask -> symbol_map[9..29] */
		{
			unsigned int v4 = p[i];
			int v5 = v4 & 0x1F;
			int v6 = v4 >> 6;

			if (result & 1)
				lcd_page[v5] |= (u8)(1 << v6);
			else
				lcd_page[v5] &= ~((u8)(1 << v6));
		}
		result >>= 1;
	}
}

/* binaRE lcd_show_dig @0xc0838684 (IDA 9.3 decompiled)
 * glyph idx v2: '0'..'9' -> 0..9; '-' -> 11; other -> 10;
 *               'A'..'Z' -> c - 53 (0..25); 'a'..'z' -> c - 85 (12..37).
 * pattern word v4 = tab[(32 + 4*v2)/4] (i.e. u32 at byte offset 32+4*v2);
 * fallback (v2 > 0xB && idx > 3): v4 = 0, v5 = 8.
 * seg-walk: v6 = dig_map (= tab+56) + 14*idx, v5 = (idx > 3) ? 8 : 14 bytes;
 *   per byte: v9 = *v6++; if ((v4 & 0x8000) == 0) clear else set bit v9>>6 of page[v9&0x1F]; v4 <<= 1.
 */
static void
lcd_show_dig(int c, unsigned int idx)
{
	unsigned char v2;
	int v4, v5, i;
	const unsigned char *v6;

	v2 = (unsigned char)(c - 48);
	if (v2 > 9)
	{
		if ((unsigned char)(c - 97) > 0x19u)      /* not 'a'..'z' */
		{
			if ((unsigned char)(c - 65) > 0x19u)  /* not 'A'..'Z' */
				v2 = (c == '-') ? 11 : 10;
			else
				v2 = (unsigned char)(c - 53);
		}
		else
			v2 = (unsigned char)(c - 85);
	}

	v6 = (const unsigned char *)lcd_seg_tab + 14 + 14 * idx;   /* dig_map = tab+56, /4 -> +14 */

	if (v2 > 0xB && idx > 3)          /* unknown glyph beyond 4th digit: blank */
	{
		v4 = 0;
		v5 = 8;
	}
	else
	{
		v4 = ((const u32 *)((const unsigned char *)lcd_seg_tab + 32))[v2];  /* +32+4*v2 */
		v5 = (idx > 3) ? 8 : 14;
	}

	for (i = 0; i < v5; i++)
	{
		unsigned int v9 = v6[i];

		if ((v4 & 0x8000) == 0)
			lcd_page[v9 & 0x1F] &= ~((u8)(1 << (v9 >> 6)));
		else
			lcd_page[v9 & 0x1F] |= (u8)(1 << (v9 >> 6));
		v4 <<= 1;
	}
}

/* binaRE lcd_flash @0xc0838770 (IDA 9.3 decompiled) */
void
lcd_flash(int mask)
{
	int i;

	if (!lcd_enabled)
		return;

	del_timer_sync(&lcd_work.timer);
	clear_bit(0, &lcd_work);

	mutex_lock(&lcd_mutex);
	for (i = 0; i < 9; i++)
	{
		unsigned int v4 = ((const unsigned char *)lcd_seg_tab)[i];

		if ((mask & 1) == 0)
			lcd_page[v4 & 0x1F] &= ~((u8)(1 << (v4 >> 6)));
		else
			lcd_page[v4 & 0x1F] |= (u8)(1 << (v4 >> 6));
		mask >>= 1;
	}
	mutex_unlock(&lcd_mutex);

	schedule_delayed_work(&lcd_work, msecs_to_jiffies(0));
}

/* binaRE Lcd_Flash_work @0xc083881c (IDA 9.3 decompiled) */
static void
Lcd_Flash_work(struct work_struct *work)
{
	(void)work;

	if (lcd_enabled && lcd_flash_mode)
	{
		lcd_flash(lcd_seg_tab[lcd_flash_idx + 78]);
		lcd_flash_idx++;
		if ((unsigned int)lcd_flash_idx > 0x14u)
			lcd_flash_idx = 0;
		schedule_delayed_work(&lcd_flash_work, msecs_to_jiffies(120));
	}
}

/* binaRE lcd_show @0xc0838890 (IDA 9.3 decompiled)
 * call-site: mtc-car.c:3194 (ctl_lcd). */
void
lcd_show(const char *str)
{
	int v8;
	unsigned int v9, v11;

	if (!lcd_enabled)
		return;

	if (!strcmp(str, "0"))
	{
		lcd_flash_mode = 0;
		del_timer_sync(&lcd_flash_work.timer);
		clear_bit(0, &lcd_flash_work);
		lcd_flash(0);
		return;
	}

	if (!strcmp(str, "1"))
	{
		if (!lcd_flash_mode)
		{
			lcd_flash_idx = 0;
			lcd_flash_mode = 1;
			schedule_delayed_work(&lcd_flash_work, msecs_to_jiffies(0));
		}
		return;
	}

	if (strlen(str) != 21)
		return;

	del_timer_sync(&lcd_work.timer);
	clear_bit(0, &lcd_work);

	mutex_lock(&lcd_mutex);
	lcd_show_dig(str[0], 0);
	lcd_show_dig(str[1], 1);
	lcd_show_dig(str[2], 2);
	lcd_show_dig(str[3], 3);
	lcd_show_dig(str[5], 4);
	lcd_show_dig(str[6], 5);
	lcd_show_dig(str[8], 6);
	lcd_show_dig(str[9], 7);
	lcd_show_dig(str[10], 8);

	/* v7 = str+11: 9 it. str[12..20]: v9 = 10*v9 - 48 + ch */
	v8 = 9;
	v9 = 0;
	while (v8)
	{
		v8--;
		v9 = 10 * v9 - 48 + (unsigned char)str[12 + (9 - v8)];
	}

	v11 = v9 & 0xFFFFFFF;
	if (str[4] == '.')
		v11 |= 0x10000000u;
	if (str[7] == '.')
		v11 |= 0x20000000u;
	lcd_show_symbol(v11);
	mutex_unlock(&lcd_mutex);

	schedule_delayed_work(&lcd_work, msecs_to_jiffies(0));
}

/* binaRE lcd_on @0xc0838a4c (IDA 9.3 decompiled) */
void
lcd_on(void)
{
	if (lcd_enabled)
		lcd_send_cmd(6);
}

/* binaRE lcd_off @0xc0838a68 (IDA 9.3 decompiled) */
void
lcd_off(void)
{
	if (lcd_enabled)
		lcd_send_cmd(4);
}

/* binaRE lcd_probe @0xc09bc990 (IDA 9.3 decompiled) */
static int
lcd_probe(struct platform_device *pdev)
{
	(void)pdev;

	if (car_struct.car_status.wipe_flag & 0x20)
		lcd_enabled = 1;

	if (lcd_enabled)
	{
		printk(KERN_INFO "--mtc lcd probe\n");

		INIT_DELAYED_WORK(&lcd_work, Lcd_work);
		INIT_DELAYED_WORK(&lcd_flash_work, Lcd_Flash_work);

		gpio_request(gpio_LCD_CS, 0);
		gpio_request(gpio_LCD_DAT, 0);
		gpio_request(gpio_LCD_CLK, 0);
		gpio_direction_output(gpio_LCD_CS, 1);
		gpio_direction_output(gpio_LCD_DAT, 1);
		gpio_direction_output(gpio_LCD_CLK, 1);
		lcd_delay();

		memset(lcd_page, 0, 32);
		lcd_send_cmd(82);
		lcd_send_cmd(2);
		lcd_on();

		memset(lcd_page, 0, 32);
		lcd_show("INIT-__-___-000000000");
		lcd_show("1");
	}

	return 0;
}

static int __devexit
lcd_remove(struct platform_device *pdev)
{
	(void)pdev;

	return 0;
}

/* recovered structures */

static struct dev_pm_ops lcd_pm_ops = {
	.suspend = lcd_suspend,
	.resume = lcd_resume,
};

static struct platform_driver mtc_lcd_driver = {
	.probe = lcd_probe,
	.remove = __devexit_p(lcd_remove),
	.driver = {
		.name = "mtc-lcd",
		.pm = &lcd_pm_ops,
	},
};

static int __init
lcd_init()
{
	platform_driver_register(&mtc_lcd_driver);
	return 0;
}

static void
lcd_exit()
{
	platform_driver_unregister(&mtc_lcd_driver);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC LCD driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-lcd");

/* ====================== binaRE fb glue (rk_fb_open/close, rk_direct_fb_open, rk_fb_show_logo) ====================== */

static struct platform_device *lcd_platform_dev; /* binaRE 0xC0D1DE08, module BSS, no kallsyms name */

/* binaRE rk_fb_open @0xc069fa88 (IDA 9.3 decompiled) — 't' (local) в kallsyms → static */
static int
rk_fb_open(struct fb_info *info) /* a1 */
{
	int v1; /* r4 */
	int v2; /* r1 */

	v1 = *(int *)((char *)info + 604); /* binaRE *(a1+604) */
	v2 = ((int (*)(int, int))((char *)v1 + 472))(v1, (char *)info + 220); /* binaRE *(v1+472)(v1, a1+220) */
	if (!*(((char *)*(int *)((char *)v1 + 4 * (v2 + 4))) + 12)) /* binaRE !*( *(v1+4*(v2+4)) + 12 ) */
		((void (*)(int, int, int))((char *)v1 + 416))(v1, v2, 1); /* binaRE *(v1+416)(v1, v2, 1) — open=1 */
	return 0;
}

/* binaRE rk_fb_close @0xc069fad0 (IDA 9.3 decompiled) — 't' (local) в kallsyms → static */
static int
rk_fb_close(struct fb_info *info) /* a1 */
{
	int v1; /* r4 */
	int v2; /* r1 */

	v1 = *(int *)((char *)info + 604); /* binaRE *(a1+604) */
	v2 = ((int (*)(int, int))((char *)v1 + 472))(v1, (char *)info + 220); /* binaRE *(v1+472)(v1, a1+220) */
	if (*(((char *)*(int *)((char *)v1 + 4 * (v2 + 4))) + 12)) /* binaRE *( *(v1+4*(v2+4)) + 12 ) */
		((void (*)(int, int, int))((char *)v1 + 416))(v1, v2, 0); /* binaRE *(v1+416)(v1, v2, 0) — close=0 */
	return 0;
}

/* binaRE rk_direct_fb_open @0xc06a02dc (IDA 9.3 decompiled) — 'T' (EXPORT) в kallsyms */
int
rk_direct_fb_open(struct fb_info *fb, int a2) /* a1, a2 */
{
	if (a2)
		return rk_fb_open(fb); /* binaRE a2 → rk_fb_open(a1) */
	else
		return rk_fb_close(fb); /* binaRE → rk_fb_close(a1) */
}

/* binaRE rk_fb_show_logo @0xc06a26dc (IDA 9.3 decompiled) — 'T' (EXPORT) в kallsyms */
int
rk_fb_show_logo(void)
{
	int drvdata; /* r4 */
	struct fb_info *fb;
	void *ops;

	drvdata = dev_get_drvdata(&lcd_platform_dev->dev); /* binaRE dev_get_drvdata(MEMORY[0xC0D1DE08]+8) (&pdev->dev, struct platform_device: dev @+8) */
	fb = *(struct fb_info **)((char *)drvdata + 4); /* binaRE *(drvdata+4) */
	ops = *(void **)((char *)fb + 568); /* binaRE *(fb+568) — class_ops (IDA перечитывает в обеих ветках) */
	if (fb_prepare_logo(fb, 0)) { /* binaRE fb_prepare_logo(*(drvdata+4), 0) */
		fb_set_cmap((struct fb_cmap *)((char *)fb + 532), fb); /* binaRE fb_set_cmap(fb+532 [cmap], fb) */
		fb_show_logo(fb, 0); /* binaRE: IDA — арги stale; info/rotate=0 по аналогии с fb_prepare_logo(fb, 0) (SDK-патч fb.h: (fb_info, rotate)) */
		((void (*)(int))((char *)ops + 40))((char *)fb + 60); /* binaRE (*(fb+568)+40)(fb+60) */
	}
	return ((int (*)(void))((char *)ops + 68))(); /* binaRE return (*(fb+568)+68)() */
}
