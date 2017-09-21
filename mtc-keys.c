

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
	return 0;
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
