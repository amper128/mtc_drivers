

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
