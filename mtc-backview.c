#include <linux/module.h>
#include <linux/i2c.h>

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


module_init(backview_init);
