#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

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
