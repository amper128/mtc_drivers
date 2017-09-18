#include <linux/gpio.h>

#ifndef _MTC_SHARED_H
#define _MTC_SHARED_H

enum mtc_gpio {
	gpio_MCU_CLK = 167,
	gpio_DVD_STB = 168,
	gpio_MCU_DIN = 169,
	gpio_MCU_DOUT = 170,
	gpio_DVD_ACK = 174,
	gpio_DVD_DATA = 175,
	gpio_PARROT_RESET = 198,
	gpio_PARROT_BOOT = 199,
};

#endif // _MTC_SHARED_H
