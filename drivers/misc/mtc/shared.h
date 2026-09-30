#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <stdbool.h>

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
	gpio_T132_PWR = 235,
	gpio_LCD_CS = 236,
	gpio_T132_RST = 269,
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
	MTC_CMD_AUDIO_ACTIVE_INIT = 0x9005, /* tentative: имя по контексту использования (B8) — audio_active */
	MTC_CMD_AUDIO_DEACTIVE = 0x9006, /* tentative: имя по контексту использования (B8) — audio_deactive */
	MTC_CMD_MUTE_ALL = 0x9007,
	MTC_CMD_UNMUTE_ALL = 0x9008,
	MTC_CMD_VIDEO_CHANNEL = 0x900A,
	MTC_CMD_SET_BALANCE_OLD = 0x9010, /* tentative: имя по контексту использования (B8) — старая ветка Audio_Balance */
	MTC_CMD_PHONE_BT_IN = 0x9020, /* tentative: имя по контексту использования (B8) — Audio_PhoneChannel вход BT */
	MTC_CMD_PHONE_BT_OUT = 0x9021, /* tentative: имя по контексту использования (B8) — Audio_PhoneChannel выход BT */
	MTC_CMD_VOLUME_AUX = 0x9022, /* tentative: имя по контексту использования (B8); не встречено в audio */
	MTC_CMD_VOLUME_MAIN = 0x9023, /* tentative: имя по контексту использования (B8) — audio_active */
	MTC_CMD_SET_MUTE = 0x9024,
	MTC_CMD_AJX_IN = 0x9030, /* tentative: имя по контексту использования (B8) — Audio_AJXChannel mode!=0 */
	MTC_CMD_AJX_OUT = 0x9031, /* tentative: имя по контексту использования (B8) — Audio_AJXChannel mode==0 */
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

/* tentative: имена по контексту использования (B8) — vs_send(channel, cmd, buf, len) */
enum MTC_VS_CMD {
	MTC_VS_CMD_PIN_MUTE = 150,  /* 0x96: pin-mute vs_send(2, 150, &mute, ...) */
	MTC_VS_CMD_TA_CHECK = 155, /* 0x9B: ta_check_start/back vs_send(2, 155, buf, 1) */
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
		char ch_attr[7];	   /* @13..19 was _gap1[7]: атрибуты каналов для Audio_ChInit (pack в 12B ch-команду) */
		char cfg_rds;
		char reserved_21[2];  /* @21..22 was _gap[2]: unused */
		char cfg_frontview;
		char cfg_logo_type;
		char adc_wheel_gate;    /* @25 was _gap3[0]: гейт fallback-обработки колёса — бинар: единственный потребитель
		 * adc_wheel_callback (A:0x00D9 r2 w0). Имя TENTATIVE. */
		char reserved_26;	   /* @26 was _gap3[1]: unused */
		char cfg_rudder;
		char default_ajx_ch;    /* @28 was _gap4[1]: AJX-канал по умолчанию — бинар: Audio_AJXChannel читает СЮДА
		 * (A:0x00DC r2 w0, decompiled_Audio_AJXChannel.c:17) */
		char cfg_dvr;
		char cfg_appdisable;
		char cfg_ill;
		char cfg_customer[16];
		char cfg_model[16];
		char cfg_sn[16];
		char cfg_password[16];
		char cfg_logo1[16];
		char cfg_logo2[16];
		char reserved_128[75]; /* @128..202 was _gap5[75]: unused (возможная IR-область — см. ir_assign_tab) */
		char cfg_wheelstudy_type;
		char reserved_204;	   /* @204 was _gap6[1]: binaRE-доступен (ADV7181D_Init), роль ? */
		char canbus_cfg;
		char reserved_206;	   /* @206 was _gap7[1]: РАСХОЖДЕНИЕ #5 — cfg_ir_assign пишет IR-таблицу отсюда
		 * (mtc-car.c:2468-2479), layout блоба в бинаре другой */
		char cfg_atvmode;
		char ir_assign_tab[120]; /* @208..327 was _gap8[120]: ГИПОТЕЗА — таблица IR-привязок 60×16-бит (naming_report §4, low;
		 * байты @288..295 бинарно доступны car_ioctl/key_beep/rk29sdk_wifi_power) */
		char steer_data[150];
		char reserved_478[2]; /* @478..479 was _gap9[2]: unused */
		char cfg_color[2];
		char cfg_powerdelay;
		char cfg_backlight;
		char ctl_beep;
		char cfg_led[3];
		char reserved_488;	   /* @488 was _gap10[1]: unused */
		char wifi_pwr;
		char reserved_490;	   /* @490 was _gap11[0]: РАСХОЖДЕНИЕ #6 — код OOB-читает [0]/[1] здесь как u/v-cfg
		 * (mtc-backview.c:366-367); реальные u/v в бинаре @493/494 = uv_off_u/v */
		char cfg_mirror;
		char cfg_led_multi;
		char uv_off_u;	     /* @493 was _gap12[0]: источник u_value (binaRE 0x2AD ✓ T132B_UV_Set) */
		char uv_off_v;	     /* @494 was _gap12[1]: источник v_value (binaRE 0x2AE ✓) */
		char cfg_blmode;
		char reserved_496[16]; /* @496..511 was _gap13[16]: хвост блоба, unused */
	} d;
	char arr[512];  /* T5: плоский алиас cfg-байтов (keys.c: .d.arr -> .arr) */
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

/* реализация: радиомодуль, binaRE 0xc08366cc/0xc0836708 (decompiled_ta_check_*.c) */
void ta_check_start(void);
void ta_check_back(void);
extern void arm_send(unsigned int cmd);
int arm_send_multi(unsigned int cmd, int count, unsigned char *buf);
extern int car_comm_init(void);
extern void car_add_work(int a1, int a2, int flush);
extern void car_add_work_delay(int a1, int a2, unsigned int delay);


/* ===== T5 smoke-green (executor): Windows-макросы + IDA API maps + enum'ы + cross-TU прототипы ===== */
#define LOBYTE(w)   ((unsigned char *)&(w))[0]
#define HIBYTE(w)   (((unsigned char *)&(w))[3])
#define BYTE1(w)    (((unsigned char *)&(w))[1])
#define BYTE2(w)    (((unsigned char *)&(w))[2])
#define LOWORD(w)   (*(unsigned short *)&(w))
#define HIWORD(w)   (*(unsigned short *)(((unsigned char *)&(w)) + 2))
#define memzero(p, n)         memset((p), 0, (n))
#define _memzero(p, n)        memset((p), 0, (n))
#define _kmalloc(sz, fl)      kmalloc((sz), (fl))
#define _copy_from_user(d, us, n) copy_from_user((d), (us), (n))
#define _copy_to_user(d, us, n)   copy_to_user((d), (us), (n))
#define kzfree(p)             kfree(p) /* kernel 3.0: kzfree нет */

/* TENTATIVE: порядок/значения по определению enum в audio_card_glue.c (IPOD=6, DVR=7) */
enum MTC_AV_CHANNEL {
	MTC_AV_CHANNEL_GSM_BT = 0, MTC_AV_CHANNEL_SYS = 1, MTC_AV_CHANNEL_DVD = 2,
	MTC_AV_CHANNEL_LINE = 3, MTC_AV_CHANNEL_FM = 4, MTC_AV_CHANNEL_DTV = 5,
	MTC_AV_CHANNEL_IPOD = 6, MTC_AV_CHANNEL_DVR = 7,
};

enum mtc_car_work { CAR_WORK_BL_ON = 35, CAR_WORK_BL_OFF = 36 }; /* по case в car_work */

enum mtc_audio_work {
	AUDIO_WORK_CH_ENTER = 0, AUDIO_WORK_CH_EXIT = 1, AUDIO_WORK_MUTE = 2,
	AUDIO_WORK_VOLUME = 3, AUDIO_WORK_PHONE_VOLUME = 4, AUDIO_WORK_PHONE = 0xA,
}; /* по switch audio-work handler'а (audio_card_glue.c); PHONE — TENTATIVE */

/* cross-TU прототипы (T5): сигнатуры — по определениям в mtc_drivers/ref_kernel (binaRE) */
/* T5: прототипы дособраны вручную по определениям (parse_defs пропустил multi-line сигнатуры) */
int Radio_TA(int enable, int a2);
void Radio_AF(int af);
int Radio_Get_Signal(void);
int Radio_Get_Stereo(void);
void Radio_Set_Search(char arg);
int Radio_Set_Frequency(int freq, int with_data);
void Radio_Set_Mute(int mute);
void Radio_Set_Stereo(int val);
void rds_input(int a1);
void rds_input2(int a1);
void rds_input3(char *rds);
void rds_input3A(char *rds);
void Tv_Set_Frequency(signed int freq);
void Tv_Set_Demod(int a1);
int Tv_Get_Status(void);
void audio_active(void);
void audio_deactive(void);
int getAudioChannel(void);
bool isAudioMute(void);
void audio_flush_work(void);
void audio_channel_switch_unmute(void);
void decorder_power(int pwr); /* def: backview.c */
int codec_active(void);
int codec_deactive(void);
int vs_send(int port_num, unsigned char cmd, char *cmd_data, int count); /* def: vs.c; binary returns int */
void vs_send_raw(int port_num, unsigned char *data, int count); /* def: vs.c */
int mtc_iomux_set(unsigned int mode);
int send_event_key(unsigned int a1);
int key_beep(void);
char key_enter_mode(char result);
int send_ir_key(int result);
void lcd_show(const char *str);
void dvd_send_command(u32 command);
int dvd_get_folder(int result, const char *buf_1, int a3, int a4);
int dvd_get_media(int result, const char *a2, int a3, int a4);
int dvd_get_folder_cnt(char *buf);
int dvd_get_media_cnt(char *buf);
int dvd_get_folder_idx(char *buf);
int dvd_get_media_idx(char *buf);
int dvd_get_length(char *buf);
int dvd_get_position(char *buf);
int dvd_get_media_title(const char *buf);
int check_tv_signal(void); /* TENTATIVE */
int board_boot_mode(void); /* def: ref_kernel mach-rk30/common.c */
int rk_fb_show_logo(void); /* binaRE @0xc06a26dc (def: lcd.c) */
int rk29sdk_wifi_power(int on); /* def: ref_kernel board-rk30-sdk-sdmmc.c */
int sta_touch_adc(char *buf); /* def: car.c (binaRE 0xc0842888, MTC-14) */
struct mtc_keys_data *mtc_keys_data_ptr(void); /* def: keys.c (MTC-14: keys_data .bss @0xC168E474) */
int sta_touch_cal(void *data); /* TENTATIVE */
#endif // _MTC_SHARED_H
