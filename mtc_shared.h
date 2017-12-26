#include <linux/gpio.h>
#include <linux/workqueue.h>

#ifndef _MTC_SHARED_H
#define _MTC_SHARED_H

enum mtc_gpio {
	gpio_FCAM_PWR = 161,
	gpio_MCU_CLK = 167,
	gpio_DVD_STB = 168,
	gpio_MCU_DIN = 169,
	gpio_MCU_DOUT = 170,
	gpio_CODEC_PWR = 172,
	gpio_DVD_ACK = 174,
	gpio_DVD_DATA = 175,
	gpio_PARROT_RESET = 198,
	gpio_PARROT_BOOT = 199,
	gpio_LCD_DAT = 228,
	gpio_LCD_CLK = 229,
	gpio_LCD_CS = 236,
};

enum MTC_CMD {
	MTC_CMD_DVD_DOOR_OPEN = 0x0101,
	MTC_CMD_DVD_DOOR_CLOSE = 0x0102,
	MTC_CMD_DVD_PWR_ON = 0x0103,
	MTC_CMD_DVD_PWR_OFF = 0x0104,
	MTC_CMD_DVD_EJECT = 0x0105,

	MTC_CMD_GET_DEVICE_STATUS = 0x201,

	MTC_CMD_BOOT_ANDROID = 0x204,
	MTC_CMD_BOOT_RECOVERY = 0x208,

	MTC_CMD_MIRROR_ON = 0x421,
	MTC_CMD_MIRROR_OFF = 0x422,

	MTC_CMD_BOOTMODE = 0x500,

	MTC_CMD_SHUTDOWN = 0x0755, // send when ARM reboot
	MTC_CMD_REBOOT = 0x0EFE,   // send when ARM shutdown

	MTC_CMD_DVD_STATUS = 0x0F01,
	MTC_CMD_PWR_STATUS = 0x0F02,

	MTC_CMD_GET_TV_STATUS = 0x1405,

	MTC_CMD_GET_BACKLIGHT = 0x1510,

	MTC_CMD_MCUVER = 0x1530,
	MTC_CMD_MCUDATE = 0x1531,
	MTC_CMD_MCUTIME = 0x1532,
	MTC_CMD_GET_MCUCONFIG = 0x15FF,

	MTC_CMD_SET_VOLUME = 0x9000,
	MTC_CMD_SET_BALANCE = 0x9001,
	MTC_CMD_SET_EQUALIZER = 0x9002,
	MTC_CMD_SOFT_MUTE = 0x9003,
	MTC_CMD_SET_CHANNEL = 0x9004,
	MTC_CMD_MUTE_ALL = 0x9007,
	MTC_CMD_UNMUTE_ALL = 0x9008,
	MTC_CMD_VIDEO_CHANNEL = 0x900A,
	MTC_CMD_SET_MUTE = 0x9024,
	MTC_CMD_AUDIO_DVD_ON = 0x9100,
	MTC_CMD_AUDIO_DVD_OFF = 0x9101,
	MTC_CMD_FM_STEREO_ON = 0x9201,
	MTC_CMD_FM_STEREO_OFF = 0x9202,
	MTC_CMD_RADIO_MUTE = 0x9204,
	MTC_CMD_RADIO_UNMUTE = 0x9205,
	MTC_CMD_RADIO_ON = 0x9206,
	MTC_CMD_RADIO_OFF = 0x9207,
	MTC_CMD_RADIO_SEARCH = 0x920C,

	MTC_CMD_TV_ON = 0x9400,
	MTC_CMD_TV_OFF = 0x9401,
	MTC_CMD_TV_FREQ = 0x9403,
	MTC_CMD_TV_DEMOD = 0x9404,

	MTC_CMD_LEDCFG = 0x9500,
	MTC_CMD_BEEP = 0x9501,
	MTC_CMD_BACKLIGHT = 0x9502,
	MTC_CMD_WIFI_PWR = 0x9503,

	MTC_CMD_COLOR = 0x9507,
	MTC_CMD_LED_MULTI = 0x9508,

	MTC_CMD_BLMODE = 0x950A,
	MTC_CMD_POWERDELAY = 0x950B,

	MTC_CMD_STEER_ASSIGN = 0x950F,

	MTC_CMD_DTV_IR = 0x9523,

	MTC_CMD_STUDY_WHEEL_KEY = 0x9527,
	MTC_CMD_STUDY_CANBUS_KEY = 0x9528,

	MTC_CMD_CONFIG_DATA = 0x95FE, // sending 512 bytes of configuration data

	MTC_CMD_MCU_UPDDTE = 0xA000,

	MTC_CMD_RESET = 0xA123,
	MTC_CMD_RESET2 = 0xA124, // send when reboot to recovery

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
union mtc_config_data {
	struct {
		char checksum;
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
	} d;
	u8 u8[512];
};

/* config example:
 *
 * 65 0f 00 00 01 02 03 03 00 00 00 ff ff 0e 04 0b
 * 0a 06 0a 06 01 00 00 00 02 00 00 00 03 00 00 00
 * 6d 74 63 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 53 30 37 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 30 30 30 30 30 30 30 30 00 00 00 00 00 00 00 00
 * 31 32 36 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 48 79 75 6e 64 61 69 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 10 d0 10 30 10 2a 10 c0 10 82 10 80 10 40 10 90
 * 10 52 10 20 10 a0 10 60 10 28 10 e0 10 10 5a a5
 * 10 a8 10 8a 10 4a 10 00 10 f8 10 38 10 b8 10 ca
 * 10 58 10 9a 10 02 10 68 10 62 10 98 10 b0 10 d2
 * 10 fa 10 da 10 f2 10 ea 10 7a 10 5a 10 72 10 6a
 * 10 12 5a a5 5a a5 10 aa 5a a5 5a a5 5a a5 5a a5
 * 5a a5 5a a5 10 ba 5a a5 10 3a 5a a5 5a a5 5a a5
 * 5a a5 5a a5 5a a5 10 b2 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 00 00 08 03 32 00 00 00 00 00 00 00
 * 00 00 08 01 25 08 01 ab 00 00 00 00 00 00 08 02
 * 47 00 00 00 08 02 0f 00 00 00 00 00 00 00 00 00
 * 00 00 00 08 03 81 00 00 00 08 02 b1 08 02 7d 00
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 00 b4 03 ff 00 00 00 ff 00 00 00 00 00 00 00 01
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 *
 */

/* used in all workqueue functions */
struct mtc_work {
	int cmd1;
	int cmd2;
	int cmd3;
	int val1;
	int val2;
	struct delayed_work dwork;
};

/* mtc_car functions */
void arm_parrot_boot(int mode);
extern void arm_send(unsigned int cmd);
int arm_send_multi(unsigned int cmd, int count, unsigned char *buf);
extern int car_comm_init(void);
extern void car_add_work(int a1, int a2, int flush);
extern void car_add_work_delay(int a1, int a2, unsigned int delay);

#endif // _MTC_SHARED_H
