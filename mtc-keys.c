#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

static int mtc_keycodes[] = {
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

static struct mtc_keys_data * keys_data;

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

// dirty code
// вообще почти разобрано, но приведем в порядок позже
static int
keys_probe(struct platform_device *pdev)
{
	int v3;						  // r3@1
	int v4;						  // r3@4
	struct workqueue_struct *wheel_wq;		  // r0@8
	struct workqueue_struct *process_wq;		  // r0@8
	int(__stdcall * volkey_work_cb)();		  // r3@8
	int (*modemute_work)();				  // r3@8
	struct mtc_keys_drv *keys_dev;			  // r5@8 MAPDST
	char alloc_check;				  // zf@8
	struct input_dev *gpio_keys_input0;		  // r4@10 MAPDST
	const char *dev_name;				  // r2@12
	struct mtc_keys_drv_data *p_drv;		  // r11@12 MAPDST
	int offset_40_n;				  // r8@12
	const char *gpio_keys_input0_s;			  // r2@12
	int cur_dev;					  // r9@12
	unsigned __int32 evbit;				  // r3@12
	char repeat_disabled;				  // zf@12
	struct mtc_keys_input_id *keys_ids;		  // r3@17
	struct mtc_keys_input_id *cur_id;		  // r6@17
	int v26;					  // r7@17
	int input_type;					  // r3@17
	void(__cdecl * mcu_timer_func)(unsigned __int32); // r3@19
	unsigned __int32 timeout;			  // r0@19
	int adc_ch1;					  // r0@21
	struct adc_client *wheel_adc_client;		  // r0@23 MAPDST
	void(__cdecl * adc_wheel_callback)(struct adc_client *, void *,
					   int);	    // r1@23
	struct adc_client *wheel_adc_client_ch2;	    // r0@23
	struct adc_client *adc_client;			    // r0@25
	unsigned int gpio1;				    // r0@27
	char *gpio_label;				    // r1@28 MAPDST
	int v40;					    // r0@36
	int v41;					    // r0@38
	signed int irq;					    // r0@40
	void(__cdecl * ir_timer_func_p)(unsigned __int32);  // r3@43
	int trig_type;					    // r3@43
	irqreturn_t(__cdecl * p_isr_callback)(int, void *); // r1@43
	unsigned __int32 irq_flags;			    // r3@46
	signed int error;				    // r0@50
	int keycode2;					    // r2@52
	int *keycodes_table;				    // r7@57
	int i;						    // r6@57
	unsigned int keycode;				    // r2@58
	int register_result;				    // r0@59 MAPDST
	signed int wakeup;  // [sp+8h] [bp-30h]@12 MAPDST
	struct device *dev; // [sp+Ch] [bp-2Ch]@12

	printk("--mtc keys_probe\n");

	// unknown operations
	v3 = pp_mtc_keys_drv_0->keys_input[0]._gap6[6];
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

	wheel_wq = _alloc_workqueue_key(str_wheel_wq, 8u, 1, 0, 0);
	keys_data = p_keys_data_12;
	p_drv = p_keys_drv_data_1;
	p_keys_data_12->keys_ws.wheel_wq = wheel_wq;
	process_wq = _alloc_workqueue_key(str_process_wq, 8u, 1, 0, 0);
	keys_data->keys_ws.volkey_work.work.entry.next =
	    &keys_data->keys_ws.volkey_work.work.entry;
	keys_data->keys_ws.volkey_work.work.entry.prev =
	    &keys_data->keys_ws.volkey_work.work.entry;
	volkey_work_cb = p_volkey_work_cb;
	keys_data->keys_ws.volkey_work.work.data.counter = 0x500;
	keys_data->keys_ws.volkey_work.work.func = volkey_work_cb;
	keys_data->process_wq = process_wq;
	init_timer_key(&keys_data->keys_ws.volkey_work.timer, 0, 0);
	keys_data->keys_ws.modemute_work.work.entry.next =
	    &keys_data->keys_ws.modemute_work.work.entry;
	keys_data->keys_ws.modemute_work.work.entry.prev =
	    &keys_data->keys_ws.modemute_work.work.entry;
	modemute_work = p_modemute_work_cb;
	keys_data->keys_ws.modemute_work.work.data.counter = 0x500;
	keys_data->keys_ws.modemute_work.work.func = modemute_work;
	init_timer_key(&keys_data->keys_ws.modemute_work.timer, 0, 0);
	keys_dev = _kmalloc(264 * p_drv->input_dev_count + 16,
			    __GFP_ZERO | __GFP_FS | __GFP_IO | GFP_NOIO);
	keys_data->keys_dev = keys_dev;
	gpio_keys_input0 = input_allocate_device();
	alloc_check = keys_dev == 0;
	if (keys_dev) {
		alloc_check = gpio_keys_input0 == 0;
	}
	if (alloc_check) {
		register_result = -12;
		goto adc_register_failed;
	}
	dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, keys_dev);
	dev_name = pdev->name;
	p_drv = p_keys_drv_data_1;
	gpio_keys_input0->id.vendor = 1;
	offset_40_n = 0;
	gpio_keys_input0->name = dev_name;
	gpio_keys_input0_s = str_gpio_keys_input0;
	cur_dev = 0;
	gpio_keys_input0->id.product = 1;
	evbit = 256;
	gpio_keys_input0->id.version = 256;
	gpio_keys_input0->phys = gpio_keys_input0_s;
	repeat_disabled = p_drv->enable_key_repeat == 0;
	wakeup = 0;
	gpio_keys_input0->id.bustype = 25;
	if (!repeat_disabled) {
		evbit = gpio_keys_input0->evbit[0];
	}
	gpio_keys_input0->dev.power.suspend_timer.function = dev; // ???
	if (!repeat_disabled) {
		gpio_keys_input0->evbit[0] = evbit | 0x100000; // EV_REP
	}
	keys_dev->input_dev_count = p_drv->input_dev_count;
	keys_dev->keys_input0 = gpio_keys_input0;
	while (cur_dev < p_drv->input_dev_count) {
		keys_ids = p_drv->mtc_keys_devices;
		cur_id = &keys_ids[offset_40_n];
		v26 = &keys_dev->keys_input[cur_dev].callback_param;
		*(v26 + 12) = gpio_keys_input0;
		*(v26 + 8) = &keys_ids[offset_40_n];
		input_type = keys_ids[offset_40_n].dev_type;
		if (input_type == 1) {
			goto skip_input_setup;
		}
		if (input_type == 4) {
			*&keys_dev->keys_input[cur_dev]._gap4[0] = 1023;
			mcu_timer_func = p_adc_mcu_timer_func;
			keys_dev->keys_input[cur_dev].mcu_timer.data =
			    &keys_dev->keys_input[cur_dev].callback_param;
			keys_dev->keys_input[cur_dev].mcu_timer.function =
			    mcu_timer_func;
			init_timer_key(&keys_dev->keys_input[cur_dev].mcu_timer,
				       0, 0); // init_timer(timer);
			timeout = msecs_to_jiffies(100u);
			mod_timer(&keys_dev->keys_input[cur_dev].mcu_timer,
				  timeout + *p_val_minus30000);
			goto skip_input_setup;
		}
		if (input_type == 5) {
			adc_ch1 = cur_id->adc_ch1;
			if (p_keys_data_12->no_adc_ch1) {
				if (adc_ch1) {
					goto skip_input_setup;
				}
				wheel_adc_client =
				    adc_register(0, p_adc_wheel_cb,
						 &keys_dev->keys_input[cur_dev]
						      .callback_param);
				adc_wheel_callback = p_adc_wheel_cb;
				keys_dev->keys_input[cur_dev].wheel_adc_client =
				    wheel_adc_client;
				wheel_adc_client_ch2 = adc_register(
				    cur_id->adc_ch2, adc_wheel_callback,
				    &keys_dev->keys_input[cur_dev]
					 .callback_param);
				wheel_adc_client = keys_dev->keys_input[cur_dev]
						       .wheel_adc_client;
				keys_dev->keys_input[cur_dev]
				    .wheel_adc_client_ch2 =
				    wheel_adc_client_ch2;
				if (!wheel_adc_client) {
					goto LABEL_62;
				}
			} else {
				adc_client =
				    adc_register(adc_ch1, p_adc_wheel_cb,
						 &keys_dev->keys_input[cur_dev]
						      .callback_param);
				keys_dev->keys_input[cur_dev].wheel_adc_client =
				    adc_client;
				if (!adc_client) {
				LABEL_62:
					register_result = -22;
					goto gpio_keys_failed;
				}
			}
			*&keys_dev->keys_input[cur_dev]._gap4[0] = 1023;
			hrtimer_init(&keys_dev->keys_input[cur_dev].hrtimer,
				     HRTIMER_MODE_REL, 2000000);
			keys_dev->keys_input[cur_dev].adc_wheel_timer_func =
			    p_adc_wheel_timer_func;
			hrtimer_start(&keys_dev->keys_input[cur_dev].hrtimer);
		} else {
			gpio1 = cur_id->gpio1;
			if (input_type == 2) {
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}
				gpio_request(gpio1, gpio_label);
				gpio_pull_updown(cur_id->gpio1, 1u);
				gpio_direction_input(cur_id->gpio1);
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}
				gpio_request(cur_id->gpio2, gpio_label);
				gpio_pull_updown(cur_id->gpio2, 1u);
				gpio_direction_input(cur_id->gpio2);
				hrtimer_init(
				    &keys_dev->keys_input[cur_dev].hrtimer, 1,
				    2000000);
				keys_dev->keys_input[cur_dev]
				    .adc_wheel_timer_func = p_wheel_timer_func;
				hrtimer_start(
				    &keys_dev->keys_input[cur_dev].hrtimer);
			} else if (gpio1 != -1) {
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}
				v40 = gpio_request(gpio1, gpio_label);
				if (v40 < 0) {
					register_result = v40;
					printk(log_gpio_keys_fail_gpio,
					       cur_id->gpio1, v40);
					goto gpio_keys_failed;
				}
				gpio_pull_updown(cur_id->gpio1,
						 cur_id->dev_type == 3);
				v41 = gpio_direction_input(cur_id->gpio1);
				if (v41 < 0) {
					register_result = v41;
					printk(log_gk_input_dir_fail,
					       cur_id->gpio1, v41);
					gpio_free(cur_id->gpio1);
					goto gpio_keys_failed;
				}
				irq = cur_id->gpio1;
				keys_dev->keys_input[cur_dev].irq = irq;
				if (irq < 0) {
					register_result = irq;
					printk(log_gpio_keys_fail_irq,
					       cur_id->gpio1, irq);
					gpio_free(cur_id->gpio1);
					goto gpio_keys_failed;
				}
				if (cur_id->dev_type == 3) {
					tasklet_init(
					    &keys_dev->keys_input[cur_dev]
						 .ir_tasklet,
					    p_ir_task_func,
					    &keys_dev->keys_input[cur_dev]
						 .callback_param);
					ir_timer_func_p = p_ir_timer_func;
					keys_dev->keys_input[cur_dev]
					    .ir_timer.data =
					    &keys_dev->keys_input[cur_dev]
						 .callback_param;
					keys_dev->keys_input[cur_dev]
					    .ir_timer.function =
					    ir_timer_func_p;
					init_timer_key(
					    &keys_dev->keys_input[cur_dev]
						 .ir_timer,
					    0, 0);
					irq = keys_dev->keys_input[cur_dev].irq;
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					p_isr_callback = p_ir_isr_func;
				} else {
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					p_isr_callback = p_keys_isr_func;
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
				    irq, p_isr_callback, 0, irq_flags,
				    gpio_label,
				    &keys_dev->keys_input[cur_dev]
					 .callback_param);
				if (error) {
					register_result = error;
					printk(
					    log_gk_fail_claim_irq,
					    keys_dev->keys_input[cur_dev].irq,
					    error);
					gpio_free(cur_id->gpio1);
					goto gpio_keys_failed;
				}
			}
		}
	skip_input_setup:
		input_set_capability(gpio_keys_input0, 1u, cur_id->keycode1);
		keycode2 = cur_id->keycode2;
		if (keycode2) {
			input_set_capability(gpio_keys_input0, 1u, keycode2);
		}
		++cur_dev;
		++offset_40_n;
		if (cur_id->wakeup) {
			wakeup = 1;
		}
	}
	keycodes_table = p_mtc_keycodes;
	i = 0;
	do {
		keycode = keycodes_table[i];
		++i;
		input_set_capability(gpio_keys_input0, 1u, keycode);
	} while (i != 25);
	input_set_capability(gpio_keys_input0, 1u, KEY_ENTER);
	input_set_capability(gpio_keys_input0, 1u, KEY_WAKEUP);
	input_set_capability(gpio_keys_input0, 1u, KEY_BACK);
	input_set_capability(gpio_keys_input0, 1u, KEY_WPS_BUTTON);
	input_set_capability(gpio_keys_input0, 1u, 0x20Cu); // unknown keycode
	register_result = input_register_device(gpio_keys_input0);
	if (!register_result) {
		device_init_wakeup(dev, wakeup);
		register_early_suspend(p_mtc_keys_early_suspend);
		p_keys_data_12->p_input_dev = gpio_keys_input0;
		BYTE1(pp_mtc_keys_drv_0->keys_input0) = 1;
		return register_result;
	}
	printk(log_gk_input_register_fail, register_result);
gpio_keys_failed:
	dev_set_drvdata(dev, 0);
adc_register_failed:
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
