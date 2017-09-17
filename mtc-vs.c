#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>

static int
vs_probe(struct platform_device *pdev)
{
	return 0;
}

static int __devexit
vs_remove(struct platform_device *pdev)
{
	int v1; // r5@1
	const void **v2; // r4@1
	const void *v3; // t1@2

	v1 = 0;
	v2 = dev_get_drvdata(&pdev->dev);
	mutex_lock(off_C09BDD9C);
	do
	{
		++v1;
		uart_remove_one_port(off_C09BDDA0, *v2);
		v3 = *v2;
		++v2;
		kfree(v3);
		*(v2 - 1) = 0;
	}
	while ( v1 != 4 );
	uart_unregister_driver(off_C09BDDA0);
	mutex_unlock(off_C09BDD9C);
	return 0;
}

/* recovered structures */

static struct platform_driver mtc_vs_driver = {
	.probe = vs_probe,
	.remove = __devexit_p(vs_remove),
	.suspend = vs_suspend,
	.resume = vs_resume,
	.driver = {
		.name = "mtc-vs",
	},
};

module_platform_driver(mtc_vs_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC VS driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-vs");
