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
	MTC_CMD_DVD_DOOR_OPEN = 0x0101,
	MTC_CMD_DVD_DOOR_CLOSE = 0x0102,
	MTC_CMD_DVD_EJECT = 0x0105,

	MTC_CMD_BOOT_ANDROID = 0x204,
	MTC_CMD_BOOT_RECOVERY = 0x208,

	MTC_CMD_SHUTDOWN = 0x0755,	// send when ARM reboot
	MTC_CMD_REBOOT = 0x0EFE,	// send when ARM shutdown

	MTC_CMD_DTV_IR = 0x9523,

	MTC_CMD_CANBUS_RSP = 0xC000,
};

enum RPT_KEY_MODE {
	RPT_KEY_MODE_NORMAL = 0,
	RPT_KEY_MODE_ASSIGN = 1,
	RPT_KEY_MODE_STEERING = 2,
	RPT_KEY_MODE_RECOVERY = 3,
};

extern void arm_send(unsigned int cmd);

#endif // _MTC_SHARED_H
