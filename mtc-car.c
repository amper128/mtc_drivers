#include <asm-generic/gpio.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

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

/* все это очень сильно смахивает на SPI, почему не использовали хардверную шину?? */
static int
arm_send_cmd(unsigned int cmd)
{
	int clk_timeout;		       // r5@3 MAPDST
	int bit_pos;			       // r5@9
	int timeout_word;		       // r6@10
	unsigned int bit;		       // r1@13
	int timeout;			       // r6@15

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
		if (CheckTimeOut_constprop_4(clk_timeout)) {
			printk("~ arm_send err0 %04x\n", cmd);
			goto send_err1;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 1);
	clk_timeout = GetCurTimer();
	while (!getPin2(gpio_MCU_CLK)) {
		if (CheckTimeOut_constprop_4(clk_timeout)) {
			printk("~ arm_send err1\n");
			goto send_err1;
		}
	}

	bit_pos = 0;
	mtc_car_struct->arm_rev_status = 0x10000;
	mtc_car_struct->arm_rev_cmd = cmd;

	_const_udelay(0x10624Cu);

LABEL_10:
	timeout_word = GetCurTimer();
	do {
		if (getPin(gpio_MCU_DIN)) {
			if ((cmd & 0x8000) == 0) {
				bit = 0;
			} else {
				bit = 1;
			}

			gpio_set_value(gpio_MCU_DOUT, bit);
			gpio_direction_output(gpio_MCU_CLK, 0);

			timeout = GetCurTimer();
			while (getPin(gpio_MCU_DIN)) {
				if (CheckTimeOut_constprop_4(timeout)) {
					printk("~ arm_send_16bits err1 %d\n", bit_pos);
					goto send_err0;
				}
			}

			if (bit_pos != 15) {
				gpio_direction_output(gpio_MCU_CLK, 1);
				bit_pos++;
				goto LABEL_10;
			}

			_gpio_set_value(gpio_MCU_DOUT, 1);
			gpio_direction_output(gpio_MCU_CLK, 1);
			return 1;
		}
	} while (!CheckTimeOut_constprop_4(timeout_word));

	printk("~ arm_send_16bits err0 %d\n", bit_pos);

send_err0:
	_gpio_set_value(gpio_MCU_DOUT, 1);
	gpio_direction_input(gpio_MCU_CLK);
	_const_udelay(0x10624Cu);

send_err1:
	_gpio_set_value(gpio_MCU_DOUT, 1);
	return 0;
}

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

	INIT_WORK(car_comm->work, mtc_car_work);

	return 0;
}

static int
car_suspend(struct device *dev)
{
	printk("car_suspend\n");

	return 0;
}

static int
car_resume(struct device *dev)
{
	printk("car_resume\n");

	return 0;
}

static int
car_probe(struct platform_device *pdev)
{
	return 0;
}

static int __devexit
car_remove(struct platform_device *pdev)
{
	return 0;
}

/* recovered structures */

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
