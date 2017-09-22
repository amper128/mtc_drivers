// CONFIG_HZ = 100
// xloops = 107374

#include <asm-generic/gpio.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <stdbool.h>

#include "mtc_shared.h"

struct mtc_car_comm {
	unsigned int mcu_din_gpio;
	struct workqueue_struct *mcc_rev_wq;
	struct work_struct work;
	struct mutex snd_lock;
};

struct mtc_car_struct {
	struct mtc_car_drv *car_dev;
	struct mtc_keys_drv *keys_dev;
	char _gap0[156];
	struct mtc_car_comm *car_comm;
	int arm_rev_status;
	unsigned int arm_rev_cmd;
	char _gap1[28];
};

static struct mtc_car_struct *mtc_car_struct;

static int arm_rev(void);

/* fully decompiled */
static inline long
GetCurTimer()
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return tv.tv_usec;
}

/* fully decompiled */
static bool
CheckTimeOut(long timeout)
{
	long usec;
	struct timeval tv;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;
	if (usec < timeout) {
		usec = tv.tv_usec + 1000000;
	}
	return (usec - timeout) > 249999;
}

/* антидребезг? */
/* fully decompiled */
static int
getPin(int gpio)
{
	int value;

	do {
		value = gpio_get_value(gpio);
		udelay(1);
	} while (value != gpio_get_value(gpio));

	return value;
}

/* антидребезг? */
/* fully decompiled */
static int
getPin2(int gpio)
{
	int value;

	do {
		value = gpio_get_value(gpio);
		udelay(5);
	} while (value != gpio_get_value(gpio));

	return value;
}

/*
 * ==================================
 *	miscdev file operations
 * ==================================
 */

/* fully decompiled */
static int
car_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return 0;
}

/* fully decompiled */
static ssize_t
car_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	(void)filp;
	(void)buf;
	(void)count;
	(void)offp;

	return 0;
}

/* fully decompiled */
static ssize_t
car_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
	(void)filp;
	(void)buf;
	(void)count;
	(void)offp;

	return 0;
}

static long
car_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	// too many code
	(void)filp;
	(void)cmd;
	(void)arg;

	return 0;
}

/*
 * ==================================
 *	MCU communications
 * ==================================
 */

/* все это очень сильно смахивает на SPI, почему не использовали хардверную
 * шину?? */
/* fully decompiled */
static int
arm_send_cmd(unsigned int cmd)
{
	long clk_timeout;
	long timeout_word;
	unsigned int bit;
	int bit_pos;

	while (1) {
		if (!getPin2(gpio_MCU_CLK)) {
			printk("~ SND EXIT 0\n");
			goto send_err1;
		}
		if (getPin2(gpio_MCU_DIN)) {
			break;
		}
		printk("~ REV RESTART 0\n");
		arm_rev();
	}

	gpio_direction_output(gpio_MCU_DOUT, 0);
	clk_timeout = GetCurTimer();
	while (getPin2(gpio_MCU_CLK)) {
		if (CheckTimeOut(clk_timeout)) {
			printk("~ arm_send err0 %04x\n", cmd);
			goto send_err1;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 1);
	clk_timeout = GetCurTimer();
	while (!getPin2(gpio_MCU_CLK)) {
		if (CheckTimeOut(clk_timeout)) {
			printk("~ arm_send err1\n");
			goto send_err1;
		}
	}

	bit_pos = 0;
	mtc_car_struct->arm_rev_status = 0x10000;
	mtc_car_struct->arm_rev_cmd = cmd;

	udelay(10);

LABEL_10:
	timeout_word = GetCurTimer();
	do {
		if (getPin(gpio_MCU_DIN)) {
			long timeout;

			if ((cmd & 0x8000) == 0) {
				bit = 0;
			} else {
				bit = 1;
			}

			gpio_set_value(gpio_MCU_DOUT, bit);
			gpio_direction_output(gpio_MCU_CLK, 0);

			timeout = GetCurTimer();
			while (getPin(gpio_MCU_DIN)) {
				if (CheckTimeOut(timeout)) {
					printk("~ arm_send_16bits err1 %d\n",
					       bit_pos);
					goto send_err0;
				}
			}

			if (bit_pos != 15) {
				gpio_direction_output(gpio_MCU_CLK, 1);
				bit_pos++;
				goto LABEL_10;
			}

			gpio_set_value(gpio_MCU_DOUT, 1);
			gpio_direction_output(gpio_MCU_CLK, 1);
			return 1;
		}
	} while (!CheckTimeOut(timeout_word));

	printk("~ arm_send_16bits err0 %d\n", bit_pos);

send_err0:
	gpio_set_value(gpio_MCU_DOUT, 1);
	gpio_direction_input(gpio_MCU_CLK);
	udelay(10);

send_err1:
	gpio_set_value(gpio_MCU_DOUT, 1);
	return 0;
}

/* fully decompiled */
static int
arm_send_ack()
{
	long timeout;

	gpio_direction_input(gpio_MCU_CLK);
	timeout = GetCurTimer();
	while (!getPin(gpio_MCU_DIN)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_send_ack err0\n"); // а тут китайцы забыли
			// перенос строки
			goto send_error;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 0);
	timeout = GetCurTimer();
	while (getPin(gpio_MCU_CLK)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_send_ack err1\n");
			goto send_error;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 1);
	timeout = GetCurTimer();
	do {
		if (getPin(gpio_MCU_CLK)) {
			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_send_ack err2\n");

send_error:
	gpio_set_value(gpio_MCU_DOUT, 1);

	return 0;
}

/* fully decompiled */
int
arm_rev_8bits(unsigned char *byteval)
{
	unsigned char byte;
	int bit_n;
	long timeout;
	long timeout_bit;

	byte = 0;
	gpio_direction_input(gpio_MCU_CLK);
	bit_n = 0;

LABEL_2:
	timeout = GetCurTimer();
	do {
		if (!getPin(gpio_MCU_CLK)) {
			byte = (unsigned char)(byte << 1u);
			if (getPin(gpio_MCU_DIN)) {
				byte |= 1u;
			}
			gpio_set_value(gpio_MCU_DOUT, 0);

			timeout_bit = GetCurTimer();
			while (!getPin(gpio_MCU_CLK)) {
				if (CheckTimeOut(timeout_bit)) {
					printk("~ arm_rev_8bits err1 %d\n",
					       bit_n);
					goto err_rev;
				}
			}

			bit_n = (bit_n + 1);
			gpio_set_value(gpio_MCU_DOUT, 1);
			if (bit_n != 8) {
				goto LABEL_2;
			}
			*byteval = byte;

			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_rev_8bits err0 %x %x %d\n",
	       mtc_car_struct->arm_rev_status, mtc_car_struct->arm_rev_cmd,
	       bit_n);

err_rev:
	gpio_set_value(gpio_MCU_DOUT, 1);

	return 0;
}

/* fully decompiled */
int
arm_rev_bytes(unsigned char *buf, int count)
{
	int pos;
	int result;

	if (count) {
		pos = 0;
		while (1) {
			mtc_car_struct->arm_rev_status++;
			result = arm_rev_8bits(&buf[pos++]);

			if (!result) {
				break;
			}
			if (pos == count) {
				return 1;
			}
		}
	} else {
		result = 1;
	}

	return result;
}

/* fully decompiled */
static int
arm_rev_ack()
{
	long timeout;

	timeout = GetCurTimer();
	while (getPin(gpio_MCU_DIN)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_rev_ack err0\n"); // а тут была копипаста,
			// снова без переноса
			goto send_ack_err;
		}
	}

	gpio_direction_output(gpio_MCU_CLK, 0);
	timeout = GetCurTimer();
	do {
		if (getPin(gpio_MCU_DIN)) {
			gpio_direction_input(gpio_MCU_CLK);
			udelay(10);
			getPin(gpio_MCU_DIN);
			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_rev_ack err1\n"); // и тут копипаста

send_ack_err:
	gpio_direction_input(gpio_MCU_CLK);
	udelay(10);

	return 0;
}

/* fully decompiled */
static int
arm_rev()
{
	int mcu_clk_val;
	int mcu_din_val;
	unsigned int arm_rev_cmd;
	int bit_pos;
	long timeout;

	mcu_clk_val = getPin2(gpio_MCU_CLK);
	if (mcu_clk_val) {
		if (getPin2(gpio_MCU_DIN)) {
			mcu_clk_val = 0;
		} else {
			gpio_direction_output(gpio_MCU_CLK, 0);
			udelay(1);

			timeout = GetCurTimer();
			while (1) {
				mcu_din_val = getPin2(gpio_MCU_DIN);
				if (mcu_din_val) {
					break;
				}

				if (CheckTimeOut(timeout)) {
					printk("~ arm_rev err0\n");
					gpio_direction_input(gpio_MCU_CLK);
					udelay(10);

					return mcu_din_val;
				}
			}

			gpio_direction_output(gpio_MCU_CLK, 1);
			udelay(1);
			mtc_car_struct->arm_rev_status = 0x20000;
			gpio_direction_input(gpio_MCU_CLK);
			arm_rev_cmd = 0;
			bit_pos = 0;

		LABEL_8:
			timeout = GetCurTimer();
			do {
				if (!getPin(gpio_MCU_CLK)) {
					arm_rev_cmd =
					    (unsigned char)(arm_rev_cmd << 1);
					if (getPin(gpio_MCU_DIN)) {
						arm_rev_cmd |= 1u;
					}

					gpio_set_value(gpio_MCU_DOUT, 0);
					timeout = GetCurTimer();
					while (!getPin(gpio_MCU_CLK)) {
						if (CheckTimeOut(timeout)) {
							printk("~ "
							       "arm_rev_16bits "
							       "err1 %d\n",
							       bit_pos);
							goto LABEL_19;
						}
					}

					bit_pos++;
					gpio_set_value(gpio_MCU_DOUT, 1);
					if (bit_pos != 16) {
						goto LABEL_8;
					}

					mtc_car_struct->arm_rev_cmd =
					    arm_rev_cmd;

					return process_mcu_command(arm_rev_cmd);
				}
			} while (!CheckTimeOut(timeout));

			printk("~ arm_rev_16bits err0 %d\n", bit_pos);

		LABEL_19:
			gpio_set_value(gpio_MCU_DOUT, 1);
			mcu_clk_val = 0;
		}
	}

	return mcu_clk_val;
}

/* fully decompiled */
void
arm_send(unsigned int cmd)
{
	int hi_byte;
	unsigned char byteval = 0;

	disable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_lock(&mtc_car_struct->car_comm->snd_lock);

	if (arm_send_cmd(cmd)) {
		hi_byte = cmd & 0xFF00;

		if ((hi_byte == 0xF00) || (cmd == 0x201)) {
			if (arm_rev_8bits(&byteval)) {
				arm_rev_ack();
			}
		} else {
			arm_send_ack();
		}
	}

	enable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_unlock(&mtc_car_struct->car_comm->snd_lock);
}
EXPORT_SYMBOL_GPL(arm_send);

/* fully decompiled */
static int
arm_send_multi(unsigned int cmd, int count, unsigned char *buf)
{
	int recv;
	int pos;
	unsigned char byte;
	char bitval;
	int bit;
	long timeout;
	int res;
	int bit_pos;

	disable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_lock(&mtc_car_struct->car_comm->snd_lock);
	recv = arm_send_cmd(cmd);

	if (!recv) {
	LABEL_18:
		res = recv;
		goto LABEL_19;
	}

	if (cmd != 0xA000) {
		if (cmd & 0x8000) {
			if (count) {
				for (pos = 0; pos < count; pos++) {
					mtc_car_struct->arm_rev_status += 0x100;
					byte = buf[pos];

					for (bit_pos = 0; bit_pos < 7;
					     bit_pos++) {
						timeout = GetCurTimer();
						while (!getPin(gpio_MCU_DIN)) {
							if (CheckTimeOut(
								timeout)) {
								printk(
								    "~ "
								    "arm_send_"
								    "8bits "
								    "err0 %d\n",
								    bit_pos);
							LABEL_23:
								gpio_set_value(
								    gpio_MCU_DOUT,
								    1);
								gpio_direction_input(
								    gpio_MCU_CLK);
								res = 0;
								udelay(10);

								goto LABEL_19;
							}
						}

						if ((byte & 0x80) == 0) {
							bit = 0;
						} else {
							bit = 1;
						}

						byte =
						    (unsigned char)(byte << 1);

						gpio_set_value(gpio_MCU_DOUT,
							       bit);
						gpio_direction_output(
						    gpio_MCU_CLK, 0);
						timeout = GetCurTimer();
						while (getPin(gpio_MCU_DIN)) {
							if (CheckTimeOut(
								timeout)) {
								printk(
								    "~ "
								    "arm_send_"
								    "8bits "
								    "err1 %d\n",
								    bit_pos);
								goto LABEL_23;
							}
						}

						if (bit_pos != 7) {
							gpio_direction_output(
							    gpio_MCU_CLK, 1);
						}
					}
					gpio_set_value(gpio_MCU_DOUT, 1);
					gpio_direction_output(gpio_MCU_CLK, 1);
				}
			}

			goto LABEL_21;
		}
		recv = arm_rev_bytes(buf, count);
		if (recv) {
			res = arm_rev_ack();
			goto LABEL_19;
		}
		goto LABEL_18;
	}

	if (!count) {
	LABEL_21:
		res = arm_send_ack();
		goto LABEL_19;
	}

	for (pos = 0; pos < count; pos++) {
		byte = buf[pos];
		bit_pos = 8;

		while (1) {
			while (!getPin(gpio_MCU_DIN)) {
				;
			}

			bitval = (byte & 0x80) == 0;
			byte = (unsigned char)(byte << 1);
			bit = bitval ? 0 : 1;
			gpio_set_value(gpio_MCU_DOUT, bit);
			gpio_direction_output(gpio_MCU_CLK, 0);

			while (getPin(gpio_MCU_DIN)) {
				;
			}

			if (bit_pos == 1) {
				break;
			}

			bit_pos--;
			gpio_direction_output(gpio_MCU_CLK, 1);

			if (!bit_pos) {
				goto LABEL_36;
			}
		}

		gpio_set_value(gpio_MCU_DOUT, 1);
		gpio_direction_output(gpio_MCU_CLK, 1);

	LABEL_36:;
	}

	res = arm_send_ack();

LABEL_19:
	enable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_unlock(&mtc_car_struct->car_comm->snd_lock);

	return res;
}

/* fully decompiled */
int
car_comm_init()
{
	struct mtc_car_comm *car_comm;

	car_comm = kmalloc(sizeof(struct mtc_car_comm), GFP_ATOMIC | GFP_NOIO);

	mutex_init(&car_comm->snd_lock);

	mtc_car_struct->car_comm = car_comm;

	gpio_request(gpio_MCU_CLK, "mcu_clk");
	gpio_pull_updown(gpio_MCU_CLK, 0);
	gpio_direction_input(gpio_MCU_CLK);
	gpio_request(gpio_MCU_DIN, "mcu_din");
	gpio_pull_updown(gpio_MCU_DIN, 0);
	gpio_direction_input(gpio_MCU_DIN);
	gpio_request(gpio_MCU_DOUT, "mcu_dout");
	gpio_pull_updown(gpio_MCU_DOUT, 0);
	gpio_direction_output(gpio_MCU_DOUT, 1);
	gpio_request(gpio_PARROT_RESET, "parrot_reset");
	gpio_pull_updown(gpio_PARROT_RESET, 0);
	gpio_direction_output(gpio_PARROT_RESET, 0);
	gpio_request(gpio_PARROT_BOOT, "parrot_boot");
	gpio_pull_updown(gpio_PARROT_BOOT, 0);
	gpio_direction_output(gpio_PARROT_BOOT, 0);

	car_comm->mcu_din_gpio = gpio_MCU_DIN;
	irq_set_irq_wake(gpio_MCU_DIN, 1);
	request_threaded_irq(gpio_MCU_DIN, mcu_isr_cb, 0, 2u, "keys", car_comm);

	car_comm->mcc_rev_wq = create_singlethread_workqueue("mcu_rev_wq");

	INIT_WORK(&car_comm->work, mtc_car_work);

	return 0;
}

/* fully decompiled */
static int
car_suspend(struct device *dev)
{
	(void)dev;

	printk("car_suspend\n");

	return 0;
}

/* fully decompiled */
static int
car_resume(struct device *dev)
{
	(void)dev;

	printk("car_resume\n");

	return 0;
}

static int
car_probe(struct platform_device *pdev)
{
	(void)pdev;
	// too many code

	return 0;
}

/* fully decompiled */
static int __devexit
car_remove(struct platform_device *pdev)
{
	(void)pdev;

	return 0;
}

/* fully decompiled */
static irqreturn_t
mcu_isr(unsigned int irq)
{
	disable_irq_nosync(irq);
	queue_work(mtc_car_struct->car_comm->mcc_rev_wq,
		   &mtc_car_struct->car_comm->work);

	return IRQ_HANDLED;
}

/* recovered structures */

static struct file_operations mtc_car_fops = {
    .read = car_read,
    .write = car_write,
    .unlocked_ioctl = car_ioctl,
    .open = car_open,
};

static struct miscdevice mtc_car_miscdev = {
    .minor = 255, .name = "mtc-car", .fops = &mtc_car_fops,
};

static struct dev_pm_ops car_pm_ops = {
    .suspend = car_suspend, .resume = car_resume,
};

static struct platform_driver mtc_car_driver = {
    .probe = car_probe,
    .remove = __devexit_p(car_remove),
    .driver =
	{
	    .name = "mtc-car", .pm = &car_pm_ops,
	},
};

module_platform_driver(mtc_car_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC CAR driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-car");
