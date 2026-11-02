#include <linux/module.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <asm/uaccess.h>
#include <linux/rk_fb.h>
#include "ch7025.h"

#ifdef CONFIG_ARCH_RK29
extern int FB_Switch_Screen( struct rk29fb_screen *screen, u32 enable );
#endif

struct ch7025 ch7025;

static int ch7025_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct rkdisplay_platform_data *tv_data;
	
	memset(&ch7025, 0, sizeof(struct ch7025));
	ch7025.client = client;
	ch7025.dev = &client->dev;
	ch7025.mode = CH7025_DEFAULT_MODE;

	if(client->dev.platform_data) {
		tv_data = client->dev.platform_data;
		ch7025.io_pwr_pin = tv_data->io_pwr_pin;
		ch7025.io_rst_pin = tv_data->io_reset_pin;
		ch7025.io_switch_pin = tv_data->io_switch_pin;
		ch7025.video_source = tv_data->video_source;
		ch7025.property = tv_data->property;
	}
	else {
		ch7025.video_source = DISPLAY_SOURCE_LCDC0;
		ch7025.property = DISPLAY_MAIN;
	}

	// Reset
	if(ch7025.io_rst_pin != INVALID_GPIO) {
		ret = gpio_request(ch7025.io_rst_pin, NULL);
		if(ret) {
			printk(KERN_ERR "ch7025/7026 request reset gpio error\n");
			return -1;
		}
		gpio_direction_output(ch7025.io_rst_pin, GPIO_LOW);
		msleep(1);
		gpio_direction_output(ch7025.io_rst_pin, GPIO_HIGH);
	}
	if(ch7025.io_switch_pin != INVALID_GPIO) {
		ret = gpio_request(ch7025.io_switch_pin, NULL);
		if(ret) {
			gpio_free(ch7025.io_switch_pin);
			printk(KERN_ERR "ch7025/7026 request video switch gpio error\n");
			return -1;
		}
	}
	
	#ifdef CONFIG_CH7025_7026_TVOUT_CVBS
	ch7025_register_display_cvbs(&client->dev);
	if( ch7025.cvbs && (ch7025.mode < TVOUT_YPbPr_720x480p_60) )
		rk_display_device_enable((ch7025.cvbs)->ddev);
	#endif
	
	#ifdef CONFIG_CH7025_7026_TVOUT_YPBPR
	ch7025_register_display_ypbpr(&client->dev);
	if( ch7025.ypbpr && (ch7025.mode > TVOUT_CVBS_PAL) ) 
		rk_display_device_enable((ch7025.ypbpr)->ddev);
	#endif
	
	
	printk(KERN_INFO "ch7025/7026 probe sucess.\n");
	return 0;
}

static int ch7025_remove(struct i2c_client *client)
{
	return 0;
}

#define DRV_NAME "ch7025/7026"

static const struct i2c_device_id ch7025_id[] = {
	{ DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ch7025_id);

static struct i2c_driver ch7025_driver = {
	.driver 	= {
		.name	= DRV_NAME,
	},
	.id_table = ch7025_id,
	.probe = ch7025_probe,
	.remove = ch7025_remove,
};

static int __init ch7025_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&ch7025_driver);
	if(ret < 0){
		printk("i2c_add_driver err, ret = %d\n", ret);
	}
	return ret;
}

static void __exit ch7025_exit(void)
{
    i2c_del_driver(&ch7025_driver);
}

module_init(ch7025_init);
module_exit(ch7025_exit);
