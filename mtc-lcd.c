

static int
lcd_suspend(struct device *dev)
{
	return 0;
}

static int
lcd_resume(struct device *dev)
{
	return 0;
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
