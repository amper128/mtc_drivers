/*
 * drivers/input/touchscreen/tsc2003.c
 *
 * TSC2003 I2C-тачскрин драйвер (MTC-платформа, rk-эра 2.6/3.0).
 *
 * Реконструкция: decompiled (src_all/decompiled_tsc2003_*.c) — AUTHORITY на
 * топологию/логику; openembedded 2.6 tsc2003 base — рег. логика (0xC0/0xD0/
 * 0xE0/0xF0); i2c-touch-стиль дерева (drivers/input/ts/ts-i2c.c, tsc2005/7).
 *
 * Оффсеты drvdata (decompiled): input@+0, name[32]@+4, delayed_work@+36
 * (work_struct@+36, timer_list@+52), wq@+80, client@+84, gpio=217@+88,
 * debounce@+92, pen_down@+93, calib_x_avg[2]@+104, calib_y_avg[2]@+116,
 * calib_idx@+128, sample_cnt@+129, touch_x@+132, touch_y@+136; sizeof=140.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/kobject.h>
#include "calibration_ts.h"	/* MTC core (calibration_ts.c): TouchPanelCalibrateAPoint */

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/* MTC core-зависимости (global, T в vmlinux — определены в дереве): */
extern int isTouchDisable(void);	/* MTC core: car.c (judge-символ) */
extern void backlight_on(void);	/* MTC core: car.c (дерево: void; арг. в binaRE — stale R0) */

#define TSC2003_GPIO_INT	217	/* decompiled: gpio = 217 */

struct tsc2003 {
	struct input_dev	*input;		/* +0   */
	char			name[32];	/* +4   */
	struct delayed_work	work;		/* +36  (work@+36, timer@+52) */
	struct workqueue_struct *wq;	/* +80  */
	struct i2c_client	*client;		/* +84  */
	int			gpio;		/* +88  */
	u8			debounce;	/* +92  */
	u8			pen_down;	/* +93  */
	u8			reserved[10];	/* +94  */
	int			calib_x_avg[2]; /* +104 calib-точка X (средн. пары) */
	int			calib_y_avg[2]; /* +116 calib-точка Y (средн. пары) */
	u8			calib_idx;	/* +128 */
	u8			sample_cnt;	/* +129 */
	u8			reserved2[2];	/* +130 */
	int			touch_x;	/* +132 */
	int			touch_y;	/* +136 */
};					/* sizeof(struct tsc2003) = 140 */

/* ---- file-scope globals (decompiled: MEMORY[0xC168E5xx] / [0xC168ACxx]) ---- */

static struct tsc2003		*tsc2003_data;	/* 0xC168E5B4: текущий drvdata */
static int			tsc2003_probe_done;	/* 0xC168AC8B: probe однократен */
static u8			tsc2003_suspend_flag; /* 0xC168E5B0: suspend-флаг */
static int			tsc2003_calib_x;	/* 0xC168E5B8: калиб X (init = 0) */
static int			tsc2003_calib_y;	/* 0xC168E5BC: калиб Y (init = 0) */
static unsigned char	tsc2003_err_count;	/* 0xC168E5C0: счётчик i2c-ошибок (u8: ldrb/strb) */
static u8			tsc2003_pen_down;	/* 0xC168E5C1: pen-down флаг */

/* MTC core-флаги (MEMORY[0xC168ACxx]) — передекларированы как file-static. */
static u8			tsc2003_touch_enable = 1; /* 0xC168AC85: touch включён */
static int			tsc2003_mtc_status;	/* 0xC168ACA4: используется & 0xFFFF00 */
static u8			tsc2003_key_state;	/* 0xC168ACA5 */
static u8			tsc2003_bl_state;		/* 0xC168ACA6 */

/* tp_calibration kobject (decompiled: статичный в .data, имя "tp_calibration") */
static void tsc2003_kobj_release(struct kobject *kobj)
{
}

static struct kobj_type tsc2003_ktype = {
	.release = tsc2003_kobj_release,
};

static struct kobject tsc2003_kobj;

/* ---- i2c (Rockchip-хелперы; дефолт 150000 = SCL rate, как в decompiled) ---- */

int tsc2003_i2c_read(struct i2c_client *client, int *val)
{
	char buf[2];

	if (i2c_master_normal_recv(client, buf, 2, 150000) == 2) {
		/* IDA: *val = (16*b0) | (b1>>4); TSC2003 12-bit: MSB=b0[3:0],
		 * LSB=b1[7:4] (старшие 4 бита во втором байте). */
		*val = ((buf[0] & 0x0F) << 4) | (buf[1] >> 4);
		return 0;
	}
	printk("--mtc tsc2003 r err\n");
	tsc2003_err_count++;
	return -EIO;
}

int tsc2003_i2c_write(struct i2c_client *client, char reg)
{
	char buf[1];

	buf[0] = reg;
	if (i2c_master_normal_send(client, buf, 1, 150000) == 1)
		return 0;
	printk("--mtc tsc2003 w err\n");
	tsc2003_err_count++;
	return -EIO;
}

/* ---- IRQ: сброс state, disable, deferred work 10ms (decompiled) ---- */

irqreturn_t tsc2003_irq(int irq, void *data)
{
	struct tsc2003 *ts = data;

	ts->debounce = 0;
	ts->calib_idx = 0;
	ts->sample_cnt = 0;
	disable_irq_nosync(irq);
	queue_delayed_work(ts->wq, &ts->work, msecs_to_jiffies(10));
	return IRQ_HANDLED; /* decompiled: return 1 */
}

/* ---- work: чтение 2 точек (0xC0/0xD0), calib-loop, input-события (decompiled) ---- */

int tsc2003_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct tsc2003 *ts = container_of(dwork, struct tsc2003, work);
	int raw[4]; /* x0 y0 x1 y1 — IDA-стек v43..v46 */
	int press_x, press_y; /* 0xE0 / 0xF0 */
	int avg_x, avg_y;
	int cal_x, cal_y;
	int sum_x, sum_y, j;
	int i;

	if (tsc2003_suspend_flag)
		return;

	for (i = 0; i < 2; i++) {
		tsc2003_i2c_write(ts->client, 0xC0); /* X-канал */
		udelay(10); /* IDA: _const_udelay */
		if (tsc2003_i2c_read(ts->client, &raw[i * 2]) != 0)
			goto pen_up;
		if (raw[i * 2] == 0)
			goto pen_up;
		tsc2003_i2c_write(ts->client, 0xD0); /* Y-канал */
		udelay(10); /* IDA: _const_udelay */
		if (tsc2003_i2c_read(ts->client, &raw[i * 2 + 1]) != 0)
			goto pen_up;
		if (raw[i * 2 + 1] != 4095)
			continue; /* IDA: оба ветвления -> следующая точка */
	}

	/* press-фаза: 0xE0/0xF0 */
	tsc2003_i2c_write(ts->client, 0xE0);
	udelay(10);
	if (tsc2003_i2c_read(ts->client, &press_x) != 0)
		goto pen_up;
	tsc2003_i2c_write(ts->client, 0xF0);
	udelay(10);
	if (tsc2003_i2c_read(ts->client, &press_y) != 0)
		goto pen_up;

	if (press_x && press_x < press_y &&
	    (press_y - press_x) * ((raw[0] + raw[1]) / 2) / press_x <= 5000) {
		int dx = abs(raw[1] - raw[0]);
		int dy = abs(raw[3] - raw[2]);

		if (dx <= 200 && dy <= 200) {
			/* calib: запомнить среднюю точку пары */
			ts->calib_x_avg[ts->calib_idx] = (raw[0] + raw[1]) / 2;
			ts->calib_y_avg[ts->calib_idx] = (raw[2] + raw[3]) / 2;
			if (ts->calib_idx <= 1)
				ts->calib_idx++;
			if (ts->sample_cnt <= 2)
				ts->sample_cnt++;

			/* усреднение запомненных calib-точек */
			sum_x = 0;
			sum_y = 0;
			for (j = 0; j < ts->sample_cnt; j++) {
				sum_x += ts->calib_x_avg[j];
				sum_y += ts->calib_y_avg[j];
			}
			avg_x = sum_x / ts->sample_cnt;
			avg_y = sum_y / ts->sample_cnt;

			/* MTC-деп: raw-середина -> calib-координаты (stub: identity) */
			TouchPanelCalibrateAPoint(avg_x, avg_y, &cal_x, &cal_y);
			tsc2003_calib_x = avg_x; /* 0xC168E5B8 (raw-среднее) */
			cal_x /= 4;
			cal_y /= 4;
			tsc2003_calib_y = avg_y; /* 0xC168E5BC (raw-среднее) */

			if (tsc2003_touch_enable) {
				if ((tsc2003_mtc_status & 0xFFFF00) == 0)
					backlight_on(); /* IDA: backlight_on(cal_y+3) — арг. stale; дерево: void */
				tsc2003_pen_down = 1;

				if (tsc2003_key_state && !tsc2003_bl_state) {
					/* сглаживание точки */
					if (ts->sample_cnt <= 1) {
						ts->touch_x = cal_x;
						ts->touch_y = cal_y;
					} else {
						int sdx = abs(cal_x - ts->touch_x);
						int sdy = abs(cal_y - ts->touch_y);

						if (sdx > 2 || sdy > 2) {
							ts->touch_x = cal_x;
							ts->touch_y = cal_y;
						}
					}

					if (!isTouchDisable()) {
						if (tsc2003_key_state) {
							if (!ts->pen_down) {
								input_event(ts->input, EV_KEY, BTN_TOUCH, 1);
								ts->pen_down = 1;
							}
							input_event(ts->input, EV_ABS, ABS_X, ts->touch_x);
							input_event(ts->input, EV_ABS, ABS_Y, ts->touch_y);
							input_event(ts->input, EV_SYN, SYN_REPORT, 0);
						} else {
							tsc2003_key_state = 1;
							backlight_on(); /* IDA: backlight_on(0) — арг. stale; дерево: void */
							tsc2003_pen_down = 1;
						}
					}
				}
			}
			ts->debounce = 0;
			return queue_delayed_work(ts->wq, &ts->work,
						  msecs_to_jiffies(20));
		}
	}

pen_up:
	ts->debounce = 0;
	if (gpio_get_value(ts->gpio) != 1) {
		return queue_delayed_work(ts->wq, &ts->work,
					  msecs_to_jiffies(10));
	}
	ts->debounce++;
	if (ts->debounce <= 7) /* u8: debounce-окно ~8 x 10ms */
		return queue_delayed_work(ts->wq, &ts->work,
					  msecs_to_jiffies(10));

	/* debounce пройден — touch реально закончен */
	if (!isTouchDisable()) {
		if (!tsc2003_pen_down && ts->pen_down) {
			ts->pen_down = 0;
			input_event(ts->input, EV_KEY, BTN_TOUCH, 0);
			input_event(ts->input, EV_SYN, SYN_REPORT, 0);
		}
		tsc2003_pen_down = 0;
	}
	enable_irq(ts->gpio);
	return 0;
}

/* ---- early_suspend (decompiled: register_early_suspend(&static) в probe) ---- */

/* forward: PM-опсы определены ниже (SIMPLE_DEV_PM_OPS) */
int tsc2003_suspend(struct device *dev);
int tsc2003_resume(struct device *dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tsc2003_early_suspend_cb(struct early_suspend *h)
{
	tsc2003_suspend(NULL);
}

static void tsc2003_late_resume_cb(struct early_suspend *h)
{
	tsc2003_resume(NULL);
}

static struct early_suspend tsc2003_early_susp = {
	.suspend = tsc2003_early_suspend_cb,
	.resume  = tsc2003_late_resume_cb,
	.level   = 0x02, /* как ts-i2c.c */
};
#endif

/* ---- probe (decompiled: retry i2c_write(240) 5x msleep(30), input, workqueue,
 *      gpio 217, threaded-irq, kobject "tp_calibration", early_suspend) ---- */

int tsc2003_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct tsc2003 *ts;
	struct input_dev *input_dev;
	int ret, i;

	if (tsc2003_probe_done)
		return -EIO;

	printk("--mtc tsc2003_probe---\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -EIO;

	for (i = 5; i > 0; i--) {
		if (tsc2003_i2c_write(client, 240) == 0)
			break;
		if (!i) {
			printk("--mtc ts2003 not found---\n");
			return -EIO;
		}
		msleep(30);
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL); /* decompiled: 140 байт */
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		kfree(ts);
		if (input_dev)
			input_free_device(input_dev);
		return -ENOMEM;
	}

	ts->client = client;
	i2c_set_clientdata(client, ts); /* decompiled: dev_set_drvdata(&client->dev, ts) */
	ts->input = input_dev;

	gpio_request(TSC2003_GPIO_INT, NULL); /* decompiled: gpio_request(217, 0) */
	gpio_pull_updown(TSC2003_GPIO_INT, 0);
	gpio_direction_input(TSC2003_GPIO_INT);
	ts->gpio = TSC2003_GPIO_INT;

	snprintf(ts->name, sizeof(ts->name), "%s/input0", "mtctouch");
	input_dev->name = "mtctouch";
	input_dev->phys = ts->name;
	input_dev->id.bustype = BUS_I2C; /* decompiled: WORD@id = 24 = 0x18 */

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	input_set_abs_params(input_dev, ABS_X, 0, 800, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 479, 0, 0);

	tsc2003_data = ts;

	ret = input_register_device(input_dev);
	if (ret < 0)
		goto err_input;

	ts->wq = alloc_workqueue("tsc2003", WQ_MEM_RECLAIM, 1); /* IDA: flags=8, max=1 */
	if (!ts->wq) {
		ret = -ENOMEM;
		goto err_wq;
	}
	INIT_DELAYED_WORK(&ts->work, tsc2003_work);

	for (i = 0; i < 10; i++) /* decompiled: ~стабилизация, 10 x _const_udelay */
		udelay(15);

	ret = request_threaded_irq(ts->gpio, tsc2003_irq, NULL,
				   IRQF_TRIGGER_FALLING, "tsc2003", ts);
	if (ret < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->gpio);
		goto err_irq;
	}

	/* decompiled: kobject "tp_calibration" под input-dev */
	kobject_init(&tsc2003_kobj, &tsc2003_ktype);
	if (kobject_add(&tsc2003_kobj, &input_dev->dev.kobj, "tp_calibration") < 0)
		kobject_put(&tsc2003_kobj);

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&tsc2003_early_susp);
#endif

	printk("--mtc tsc2003 probe success\n");
	tsc2003_probe_done = 1;
	return 0;

err_irq:
	destroy_workqueue(ts->wq);
err_wq:
	input_unregister_device(input_dev);
err_input:
	kfree(ts);
	if (tsc2003_data == ts)
		tsc2003_data = NULL;
	return ret;
}

/* ---- remove (decompiled: free_irq, input_unregister, tp_calib_iface_exit, kfree) ---- */

int tsc2003_remove(struct i2c_client *client)
{
	struct tsc2003 *ts = i2c_get_clientdata(client);

	if (!ts)
		return 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&tsc2003_early_susp);
#endif
	cancel_delayed_work_sync(&ts->work);
	free_irq(ts->gpio, ts);
	gpio_free(ts->gpio);
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	tp_calib_iface_exit(); /* MTC core: calib_iface_ts.c (CBN_SPI) */
	kobject_put(&tsc2003_kobj);
	kfree(ts);
	if (tsc2003_data == ts)
		tsc2003_data = NULL;
	return 0;
}

/* ---- PM (SIMPLE_DEV_PM_OPS, как tsc2005.c) ---- */

int tsc2003_suspend(struct device *dev)
{
	if (!tsc2003_data)
		return 0;
	tsc2003_suspend_flag = 1;
	disable_irq_nosync(tsc2003_data->gpio);
	return 0;
}

int tsc2003_resume(struct device *dev)
{
	if (!tsc2003_data)
		return 0;
	tsc2003_suspend_flag = 0;
	tsc2003_i2c_write(tsc2003_data->client, 240);
	enable_irq(tsc2003_data->gpio);
	return 0;
}

static SIMPLE_DEV_PM_OPS(tsc2003_pm_ops, tsc2003_suspend, tsc2003_resume);

/* ---- i2c_driver + init (late_initcall => __initcall_tsc2003_init7) ---- */

static const struct i2c_device_id tsc2003_id[] = {
	{ "tsc2003", 0 },
	{ }
};

static struct i2c_driver tsc2003_driver = {
	.probe		= tsc2003_probe,
	.remove		= tsc2003_remove,
	.id_table	= tsc2003_id,
	.driver		= {
		.name	= "tsc2003",
		.owner	= THIS_MODULE,
		.pm	= &tsc2003_pm_ops,
	},
};

int tsc2003_init(void)
{
	tsc2003_calib_x = 0; /* MEMORY[0xC168E5B8] */
	tsc2003_calib_y = 0; /* MEMORY[0xC168E5BC] */
	return i2c_register_driver(THIS_MODULE, &tsc2003_driver);
}
late_initcall(tsc2003_init);

MODULE_LICENSE("GPL");
