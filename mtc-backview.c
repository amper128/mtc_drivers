#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>








int
backview_init()
{
	int result;

	result = i2c_register_driver(0, mtc_backview_i2c_driver);
	if (result) {
		pr_err("Unable to register Backview driver\n");
	}

	return result;
}

static const struct i2c_device_id backview_id[] = {
	{ "mtc-backview", 0 },
	{ }
};

static struct i2c_driver mtc_backview_i2c_driver = {
	.probe = backview_probe,
	.remove = backview_remove,
	.driver {
		.name = "mtc-backview",
	},
	.id_table = backview_id,
};

module_init(backview_init);
