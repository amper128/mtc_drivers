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

	MTC_CMD_MIRROR_ON = 0x421,
	MTC_CMD_MIRROR_OFF = 0x422,

	MTC_CMD_BOOTMODE = 0x500,

	MTC_CMD_SHUTDOWN = 0x0755,	// send when ARM reboot
	MTC_CMD_REBOOT = 0x0EFE,	// send when ARM shutdown

	MTC_CMD_MCUVER = 0x1530,
	MTC_CMD_MCUDATE = 0x1531,
	MTC_CMD_MCUTIME = 0x1532,
	MTC_CMD_MCUCONFIG = 0x15FF,

	MTC_CMD_FM_STEREO_ON = 0x9201,
	MTC_CMD_FM_STEREO_OFF = 0x9202,

	MTC_CMD_LEDCFG = 0x9500,
	MTC_CMD_BEEP = 0x9501,
	MTC_CMD_BACKLIGHT = 0x9502,
	MTC_CMD_WIFI_PWR = 0x9503,

	MTC_CMD_BLMODE = 0x950A,
	MTC_CMD_POWERDELAY = 0x950B,

	MTC_CMD_STEER_ASSIGN = 0x950F,

	MTC_CMD_DTV_IR = 0x9523,

	MTC_CMD_CONFIG_DATA = 0x95FE,	// sending 512 bytes of configuration data

	MTC_CMD_MCU_UPDDTE = 0xA000,

	MTC_CMD_RESET = 0xA123,
	MTC_CMD_RESET2 = 0xA124,	// send when reboot to recovery

	MTC_CMD_CANBUS_RSP = 0xC000,
};

enum RPT_KEY_MODE {
	RPT_KEY_MODE_NORMAL = 0,
	RPT_KEY_MODE_ASSIGN = 1,
	RPT_KEY_MODE_STEERING = 2,
	RPT_KEY_MODE_RECOVERY = 3,
};

/* 512-byte config data */
/* may contains unknown fields */
struct mtc_config_data
{
	char _gap0[1];
	char cfg_canbus;
	char cfg_dtv;
	char cfg_ipod;
	char cfg_dvd;
	char cfg_bt;
	char cfg_radio;
	char cfg_radio_area;
	char cfg_launcher;
	char cfg_led_type;
	char cfg_key0;
	char cfg_language_selection[2];
	char _gap1[7];
	char cfg_rds;
	char _gap[2];
	char cfg_frontview;
	char cfg_logo_type;
	char _gap3[2];
	char cfg_rudder;
	char _gap4[1];
	char cfg_dvr;
	char cfg_appdisable;
	char cfg_ill;
	char cfg_customer[16];
	char cfg_model[16];
	char cfg_sn[16];
	char cfg_password[16];
	char cfg_logo1[16];
	char cfg_logo2[16];
	char _gap5[75];
	char cfg_wheelstudy_type;
	char _gap6[1];
	char canbus_cfg;
	char _gap7[1];
	char cfg_atvmode;
	char _gap8[120];
	char steer_data[150];
	char _gap9[2];
	char cfg_color[2];
	char cfg_powerdelay;
	char cfg_backlight;
	char ctl_beep;
	char cfg_led[3];
	char _gap10[1];
	char wifi_pwr;
	char _gap11[1];
	char cfg_mirror;
	char cfg_led_multi;
	char _gap12[2];
	char cfg_blmode;
	char _gap13[16];
};

extern void arm_send(unsigned int cmd);

#endif // _MTC_SHARED_H
