#include <asm-generic/gpio.h>
#include <asm/errno.h>
#include <linux/adc.h>
#include <linux/earlysuspend.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

static unsigned int mtc_keycodes[] = {
    KEY_NUMERIC_0,
    KEY_BACK,
    KEY_NUMERIC_2,
    KEY_NUMERIC_3,
    KEY_NUMERIC_4,
    KEY_NUMERIC_5,
    KEY_NUMERIC_6,
    KEY_NUMERIC_7,
    KEY_NUMERIC_8,
    KEY_NUMERIC_9,
    KEY_NUMERIC_STAR,
    KEY_NUMERIC_POUND,
    0x20E,
    0x20F,
    KEY_CAMERA_FOCUS,
    KEY_TOUCHPAD_ON,
    KEY_HOME,
    KEY_MENU,
    KEY_VOLUMEDOWN,
    KEY_VOLUMEUP,
    KEY_TOUCHPAD_OFF,
    KEY_CAMERA_ZOOMIN,
    KEY_CAMERA_ZOOMOUT,
    KEY_CAMERA_UP,
    KEY_CAMERA_DOWN,
};

struct mtc_keys_data {
	struct workqueue_struct *process_wq;
	char no_adc_ch1;
	struct input_dev *p_input_dev;
	int intval1;
	struct mtc_keys_work_struct keys_ws;
	struct mtc_keys_drv *keys_dev;
	char flag1;
	char flag2;
	char flag3;
	char home_disabled;
	int modemuteval;
	int ir_val;
};

static struct mtc_keys_data *keys_data;

static struct platform_driver mtc_keys_driver;
static struct mtc_keys_input_id *mtc_keys_devices;
static int input_dev_count = 7;
static int enable_key_repeat;

struct mtc_keys_input_dev {
	char _gap0[8];
	void *callback_param;
	char _gap1[12];
	struct timer_list ir_timer;
	struct timer_list mcu_timer;
	char _gap4[16];
	struct adc_client *wheel_adc_client;
	struct adc_client *wheel_adc_client_ch2;
	char _gap5[4];
	unsigned int irq;
	char _gap51[16];
	struct hrtimer adc_wheel_hrtimer;
	char _gap7[68];
	struct tasklet_struct ir_tasklet;
};

struct mtc_keys_input_id {
	int dev_type;
	unsigned int keycode1;
	unsigned int keycode2;
	int gpio1;
	int gpio2;
	int trig_type;
	int adc_ch1;
	int adc_ch2;
	const char *dev_name;
	int wakeup;
};

struct mtc_keys_drv {
	int input_dev_count;
	struct input_dev *keys_input0;
	struct mtc_keys_input_dev keys_input[7];
};

static struct mtc_keys_drv *keys_dev;

static int
keys_suspend(struct device *dev)
{
	return 0;
}

static int
keys_resume(struct device *dev)
{
	return 0;
}

static int
keys_probe(struct platform_device *pdev)
{
	int v3;
	int v4;

	struct input_dev *gpio_keys_input0;
	struct mtc_keys_input_id *keys_ids;
	struct mtc_keys_input_id *cur_id;
	int v26;

	struct adc_client *wheel_adc_client;
	void (*adc_wheel_callback)(struct adc_client *, void *, int);
	struct adc_client *wheel_adc_client_ch2;
	struct adc_client *adc_client;
	const char *gpio_label;

	unsigned int irq;

	int trig_type;
	irq_handler_t isr_callback;
	unsigned int irq_flags;
	signed int error;
	int keycode2;

	int register_result;
	signed int wakeup = 0;

	printk("--mtc keys_probe\n");

	// unknown operations
	v3 = keys_dev->keys_input[0]._gap6[6];
	if (v3 == 12) {
		LOBYTE(keys_data->intval1) = 1;
	} else if (v3 == 8) {
		v4 = *(off_C09BD358 + 22); // unknown data
		if (v4 != 3) {
			if (v4 == 1) {
				BYTE1(keys_data->intval1) = 1;
			}
			keys_data->no_adc_ch1 = 1;
		}
	}

	keys_data->keys_ws.wheel_wq = create_workqueue("wheel_wq");
	keys_data->process_wq = create_workqueue("process_wq");

	INIT_WORK(keys_data->keys_ws.volkey_work, volkey_work);
	INIT_WORK(keys_data->keys_ws.modemute_work, modemute_work);

	keys_data->keys_dev = kzalloc(sizeof(struct mtc_keys_drv), GFP_KERNEL);
	if (!keys_data->keys_dev) {
		register_result = -ENOMEM;
		goto dev_create_failed;
	}

	gpio_keys_input0 = input_allocate_device();
	if (!gpio_keys_input0) {
		register_result = -ENOMEM;
		goto dev_create_failed;
	}

	dev_set_drvdata(&pdev->dev, keys_dev);

	gpio_keys_input0->id.vendor = 1;
	gpio_keys_input0->id.product = 1;
	gpio_keys_input0->name = pdev->name;
	gpio_keys_input0->id.version = 256;
	gpio_keys_input0->phys = "gpio-keys/input0";
	gpio_keys_input0->id.bustype = BUS_HOST;
	gpio_keys_input0->dev.power.suspend_timer.function = &pdev->dev; // ???
	keys_dev->keys_input0 = gpio_keys_input0;
	keys_dev->input_dev_count = input_dev_count;

	if (enable_key_repeat) {
		gpio_keys_input0->evbit[0] |= BIT_MASK(EV_REP);
	}

	for (int cur_dev = 0; cur_dev < input_dev_count; cur_dev++) {
		keys_ids = mtc_keys_devices;
		cur_id = &keys_ids[cur_dev];

		// unknown struct
		v26 = &keys_dev->keys_input[cur_dev].callback_param;
		*(v26 + 12) = gpio_keys_input0;
		*(v26 + 8) = &keys_ids[cur_dev];

		if (keys_ids[cur_dev].dev_type == 1) {
			goto skip_input_setup;
		}

		if (keys_ids[cur_dev].dev_type == 4) {
			*&keys_dev->keys_input[cur_dev]._gap4[0] = 1023;
			setup_timer(&keys_dev->keys_input[cur_dev].mcu_timer, adc_mcu_timer,
				    &keys_dev->keys_input[cur_dev].callback_param);
			mod_timer(&keys_dev->keys_input[cur_dev].mcu_timer,
				  msecs_to_jiffies(100u) - jiffies_64);

			goto skip_input_setup;
		}

		if (keys_ids[cur_dev].dev_type == 5) {
			if (keys_data->no_adc_ch1) {
				if (cur_id->adc_ch1) {
					goto skip_input_setup;
				}

				wheel_adc_client =
				    adc_register(0, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				keys_dev->keys_input[cur_dev].wheel_adc_client = wheel_adc_client;
				wheel_adc_client_ch2 =
				    adc_register(cur_id->adc_ch2, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				wheel_adc_client = keys_dev->keys_input[cur_dev].wheel_adc_client;
				keys_dev->keys_input[cur_dev].wheel_adc_client_ch2 =
				    wheel_adc_client_ch2;

				if (!wheel_adc_client) {
					register_result = -ENOMEM;
					goto gpio_keys_failed;
				}
			} else {
				adc_client =
				    adc_register(cur_id->adc_ch1, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				keys_dev->keys_input[cur_dev].wheel_adc_client = adc_client;

				if (!adc_client) {
					register_result = -ENOMEM;
					goto gpio_keys_failed;
				}
			}

			*&keys_dev->keys_input[cur_dev]._gap4[0] = 1023;
			hrtimer_init(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer,
				     HRTIMER_MODE_REL,
				     2000000); // ?
			keys_dev->keys_input[cur_dev].adc_wheel_hrtimer.function = adc_wheel_timer;
			hrtimer_start(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer, 10000000LL,
				      HRTIMER_MODE_REL);
		} else {
			if (keys_ids[cur_dev].dev_type == 2) {
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				gpio_request((unsigned int)cur_id->gpio1, gpio_label);
				gpio_pull_updown((unsigned int)cur_id->gpio1, 1u);
				gpio_direction_input((unsigned int)cur_id->gpio1);
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				gpio_request((unsigned int)cur_id->gpio2, gpio_label);
				gpio_pull_updown((unsigned int)cur_id->gpio2, 1u);
				gpio_direction_input((unsigned int)cur_id->gpio2);
				hrtimer_init(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer, 1,
					     2000000);
				keys_dev->keys_input[cur_dev].adc_wheel_hrtimer.function =
				    wheel_timer;
				hrtimer_start(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer,
					      10000000LL, HRTIMER_MODE_REL);
			} else if (cur_id->gpio1 != -1) {
				int res;

				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				res = gpio_request((unsigned int)cur_id->gpio1, gpio_label);
				if (res < 0) {
					register_result = res;
					pr_err("gpio-keys: failed to request GPIO %d, error %d\n",
					       cur_id->gpio1, res);

					goto gpio_keys_failed;
				}

				gpio_pull_updown((unsigned int)cur_id->gpio1,
						 cur_id->dev_type == 3);
				res = gpio_direction_input((unsigned int)cur_id->gpio1);
				if (res < 0) {
					register_result = res;
					pr_err("gpio-keys: failed to configure input direction for "
					       "GPIO %d, error %d\n",
					       cur_id->gpio1, res);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}

				keys_dev->keys_input[cur_dev].irq = (unsigned int)cur_id->gpio1;
				if (cur_id->gpio1 < 0) {
					register_result = cur_id->gpio1;
					pr_err("gpio-keys: Unable to get irq number for GPIO %d, "
					       "error %d\n",
					       cur_id->gpio1, cur_id->gpio1);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}

				if (cur_id->dev_type == 3) {
					tasklet_init(&keys_dev->keys_input[cur_dev].ir_tasklet,
						     ir_task,
						     &keys_dev->keys_input[cur_dev].callback_param);

					setup_timer(keys_dev->keys_input[cur_dev].ir_timer,
						    ir_timer,
						    &keys_dev->keys_input[cur_dev].callback_param);

					irq = keys_dev->keys_input[cur_dev].irq;
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					isr_callback = ir_isr;
				} else {
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					isr_callback = keys_isr;
				}

				if (trig_type) {
					irq_flags = IRQF_TRIGGER_FALLING;
				} else {
					irq_flags = IRQF_TRIGGER_RISING;
				}

				if (!gpio_label) {
					gpio_label = "keys";
				}

				error = request_threaded_irq(
				    irq, isr_callback, 0, irq_flags, gpio_label,
				    &keys_dev->keys_input[cur_dev].callback_param);

				if (error) {
					register_result = error;
					pr_err("gpio-keys: Unable to claim irq %d; error %d\n",
					       keys_dev->keys_input[cur_dev].irq, error);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}
			}
		}

	skip_input_setup:
		input_set_capability(gpio_keys_input0, 1u, cur_id->keycode1);
		if (keycode2) {
			input_set_capability(gpio_keys_input0, 1u, cur_id->keycode2);
		}

		if (cur_id->wakeup) {
			wakeup = 1;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(mtc_keycodes); i++) {
		input_set_capability(gpio_keys_input0, 1u, mtc_keycodes[i]);
	}

	input_set_capability(gpio_keys_input0, 1u, KEY_ENTER);
	input_set_capability(gpio_keys_input0, 1u, KEY_WAKEUP);
	input_set_capability(gpio_keys_input0, 1u, KEY_BACK);
	input_set_capability(gpio_keys_input0, 1u, KEY_WPS_BUTTON);
	input_set_capability(gpio_keys_input0, 1u, 0x20Cu); // unknown keycode

	register_result = input_register_device(gpio_keys_input0);
	if (!register_result) {
		device_init_wakeup(&pdev->dev, wakeup);
		register_early_suspend(mtc_keys_early_suspend);
		keys_data->p_input_dev = gpio_keys_input0;
		BYTE1(keys_dev->keys_input0) = 1; // ?

		return register_result;
	}

	pr_err("gpio-keys: Unable to register input device, error: %d\n", register_result);

gpio_keys_failed:
	dev_set_drvdata(&pdev->dev, 0);

dev_create_failed:
	input_free_device(gpio_keys_input0);
	kzfree(keys_dev);

	return register_result;
}

static int __devexit
keys_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_get_drvdata(dev);
	off_C09BDE20[2] = 0;
	device_init_wakeup(dev, 0);

	return 0;
}

/* recovered structures */

static struct dev_pm_ops keys_pm_ops = {
    .suspend = keys_suspend, .resume = keys_resume,
};

static struct platform_driver mtc_keys_driver = {
    .probe = keys_probe,
    .remove = __devexit_p(keys_remove),
    .driver =
	{
	    .name = "mtc-keys", .pm = &keys_pm_ops,
	},
};

module_platform_driver(mtc_keys_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC keys driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-keys");
