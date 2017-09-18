#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/irq.h>

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
	int arm_bytes_count;
	char _gap1[32];
};

static struct mtc_car_struct *mtc_car_struct;

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
	request_threaded_irq(gpio_MCU_DIN,
			     mcu_isr_cb, 0, 2u, "keys",
			     car_comm);

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
