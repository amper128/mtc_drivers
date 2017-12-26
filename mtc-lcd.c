#include <linux/module.h>
#include <linux/time.h>
#include <linux/device.h>

#include "mtc_shared.h"

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

/* decompiled */
static void
lcd_delay()
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

/* decompiled */
static void
lcd_send_cmd(int16_t cmd)
{
	int16_t bit_pos;

	bit_pos = (cmd & 0x1FF) | 0x800;
	gpio_direction_output(gpio_LCD_CS, 0);
	lcd_delay();

	for (unsigned int bits = 12; bits > 0; bits--)
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

static int
lcd_probe(struct platform_device *pdev)
{
	return 0;
}

static int __devexit
lcd_remove(struct platform_device *pdev)
{
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

module_platform_driver(mtc_lcd_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC LCD driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-lcd");
