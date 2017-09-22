#include <linux/gpio.h>

#ifndef _MTC_SHARED_H
#define _MTC_SHARED_H

enum mtc_gpio {
	gpio_FCAM_PWR = 161,
	gpio_MCU_CLK = 167,
	gpio_DVD_STB = 168,
	gpio_MCU_DIN = 169,
	gpio_MCU_DOUT = 170,
	gpio_DVD_ACK = 174,
	gpio_DVD_DATA = 175,
	gpio_PARROT_RESET = 198,
	gpio_PARROT_BOOT = 199,
};

enum MTC_CMD {
	MTC_CMD_SHUTDOWN = 0x0755,	// send when ARM reboot
	MTC_CMD_REBOOT = 0x0EFE,	// send when ARM shutdown
};

#endif // _MTC_SHARED_H
