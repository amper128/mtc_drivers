#include <linux/mutex.h>

#include "shared.h"

#ifndef _MTC_CAR_H
#define _MTC_CAR_H

/* binaRE (RK3188, verification_report.md §2 E4): значения подтверждены машинным кодом:
 * AM=8 (dec Audio_FadeIn/FadeOut: mtc_customer==8), JY=6 (audio_active/audio_deactive),
 * KLD=4 (audio_active: iomux_set(0x1A51));
 * YZ_RM_ZT=1, HMF=2, MX=9 — значения подтверждены binaRE (customer_values.md):
 * tef6606_i2c_probe/decorder_power */
enum MTC_CUSTOMER {
	MTC_CUSTOMER_YZ_RM_ZT = 1,
	MTC_CUSTOMER_HMF = 2,
	MTC_CUSTOMER_KLD = 4,
	MTC_CUSTOMER_JY = 6,
	MTC_CUSTOMER_AM = 8,
	MTC_CUSTOMER_MX = 9,
};

struct mtc_car_comm {
	unsigned int mcu_din_gpio;
	struct workqueue_struct *mcc_rev_wq;
	struct work_struct work;
	struct mutex car_lock;
};

/* Имена гэп-полей по analyzer2/naming_report.md §1 (бинарные якоря проверены по
 * абсолютным адресам; car_status @ 0xC168AC84). Layout байт-exact: sizeof == 160.
 * reserved_N — без semantic/бинарного смысла (пометка — если бинарный доступ есть). */
struct mtc_car_status {
	char car_ready;	     /* @0  was _gap0[0]: boot-инит-флаг (wait while(!); SET 1 boot/recovery) */
	char power_refcnt;	  /* @1  was _gap0[1]: refcount power-on сессий (++/-- cmd 29/30; SET 1/0 cmd 43) */
	char rpt_power;
	char call_active;	   /* @3  was _gap1[0]: входящий звонок (SET 1 + arm 0x202; гейт vs_uart) */
	char rpt_boot_android;
	char input_ready;	   /* @5  was _gap2[0]: input-устройства зарегистрированы (mtc-keys) */
	char audio_ready;	   /* @6  was _gap2[1]: audio-подсистема готова (гейт audio-ворков) */
	char touch_type;
	char sta_bits;	      /* @8  was _gap3[0]: ioctl-статус-биты: 0x08=ill, 0x10=driving, 0x20=iPod */
	char reserved_3;	    /* @9  was _gap3[1]: unused */
	char reserved_4;	    /* @10 was _gap3[2]: unused */
	int intval1;
	int intval2;
	char battery;
	char reserved_5;	    /* @21 was _gap4[0]: unused */
	char reserved_6;	    /* @22 was _gap4[1]: unused */
	char reserved_7;	    /* @23 was _gap4[2]: unused */
	int intval3;
	int intval4;
	char wipe_flag;
	char backlight_status;
	char cam_state;	     /* @34 was _gap5[0]: состояние видеоисточника/камеры (бинар R/W; код — только READ) */
	char reserved_8;	    /* @35 was _gap5[1]: binaRE-active (capture_work), роль ? */
	char reserved_9;	    /* @36 was _gap5[2]: binaRE-active (T132B_Page_Write), роль ? */
	char cam_signal;	    /* @37 was _gap5[3]: видеосигнал/камера подключена */
	char reserved_10;	   /* @38 was _gap5[4]: binaRE-active (camera), роль ? */
	char reserved_11;	   /* @39 was _gap5[5]: binaRE-active (video_Channel), роль ? */
	char sta_view;
	char ch_mode;
	char key_mode;
	char reserved_12;	   /* @43 was _gap7[0]: binaRE-active (ioctl+camera+I2C), роль ? */
	char backview_vol;
	char sta_video_signal;
	char av_channel_flag1;
	char video_mode;	    /* @47 was _gap8[0]: режим/источник видео (SET 0xFF при boot, wipe_flag&8) */
	char reserved_13;	   /* @48 was _gap8[1]: binaRE-active (capture_work), роль ? */
	char mcu_clk;
	char decoder_state;	 /* @50 was _gap81[0]: состояние декодера (dvd/ADV7181D); OOB-чтение кодом как _gap8[3] (mtc-car.c:1908) */
	char reserved_14;	   /* @51 was _gap81[1]: unused */
	char radar_val;	     /* @52 was _gap81[2]: значение радара (ctl_radar); OOB-чтение кодом как _gap8[5] (mtc-car.c:2338) */
	char reserved_15;	   /* @53 was _gap81[3]: unused */
	char reserved_16;	   /* @54 was _gap81[4]: unused */
	char reserved_17;	   /* @55 was _gap81[5]: unused */
	char mcuver2[16];
	char mcuver1[16];
	char power_on;	      /* @88 was _gap9[0]: флаг «включено» (зеркало power_refcnt; SET 1 при boot) */
	char radio_rds_flag;	 /* @89 was _gap9[1]: RDS/radio-флаг (SET 0 в radio-init) */
	char reserved_18;	   /* @90 was _gap9[2]: unused (SET 0 в radio-init) */
	char video_src_ready; /* @91 was _gap9[3]: видео-источник доступен (пары SET 1/0 при смене источника) */
	char ajx_active;	    /* @92 was _gap9[4]: внешний/AJX-канал аудиопровода активен (binaRE +92 ✓) */
	char reserved_19;	   /* @93 was _gap9[5]: binaRE-active (camera/tv), роль ? */
	char is1024screen;
	char reserved_20;	   /* @95 was _gap10[0]: binaRE-active (gtp_init_panel), роль ? */
	char reserved_21;	   /* @96 was _gap10[1]: binaRE-active (gtp_init_panel), роль ? */
	char ch_status;
	char u_value;	       /* @98 was _gap101[0]: U-значение T132B (binaRE 0xCE6 ✓ = T132B_UV_Set) */
	char v_value;	       /* @99 was _gap101[1]: V-значение T132B (binaRE 0xCE7 ✓) */
	char uv_cal;
	char reserved_22[32]; /* @101..132 was _gap11[32]: unused (OOB-чтение бывшего _gap9[21] @109 = [8]) */
	char rpt_boot_appinit;
	char cfg_maxvolume;
	char reserved_54;	   /* @135 was _gap12[0]: binaRE-active (touch init), роль ? */
	int touch_width;
	int touch_height;
	char touch_info1;
	char touch_info2;
	char mtc_customer;
	char boot_flags;
	char reserved_55[4];  /* @148..151 was _gap13[4]: binaRE-active (ioctl/audio), роль ? */
	char av_gps_switch;
	char av_gps_monitor;
	char av_gps_gain;
	char power2_flag;	   /* @155 was _gap14[0]: флаг «запрошено выключение питания» (ctl power2, arm 0x9529) */
	char mcu_cmd_state;	 /* @156 was _gap14[1]: состояние MCU-команд (SET 5/6 при boot) */
	char reserved_59[3];  /* @157..159 was _gap14[2..4]: unused */
};

/* Layout-офсеты бинарные якоря: car_status @ +4, config_data @ +0xC0 (base 0xC168AC80).
 * reserved_<N> — N = байт-офсет от начала структуры (вычислен при binaRE-проверенных
 * размерах: mutex=24B, timer_list=32B, delayed_work=44B, timeval=8B — RK3188 32-bit). */
struct mtc_car_struct {
	struct mtc_car_drv *car_dev;
	struct mtc_car_status car_status;
	struct mtc_car_comm *car_comm;
	int rev_bytes_count;
	unsigned int arm_rev_cmd;
	char reserved_176[16];	 /* @0xB0 (176..191) was _gap0[16]: между arm_rev_cmd (@172..175) и config_data (@0xC0=192);
	                              метка исправлена (reserved_164 — ошибочно: 164 это offset указателя car_comm); layout НЕ менялся */
	union mtc_config_data config_data;
	char reserved_704[16];	 /* @704 was _gap1[16]: после config_data, перед car_wq */
	struct workqueue_struct *car_wq;
	struct mutex car_io_lock;
	struct mutex car_cmd_lock;
	struct timeval tv;
	struct delayed_work wipecheckclear_work;
	char mcu_version[16];
	unsigned char mcu_date[16];
	unsigned char mcu_time[16];
	/* was _gap2[4] @872..875 (0x368..0x36B): первый байт — wifi-флаг (binaRE A:0x368: car_probe W /
	 * rk29sdk_wifi_power R; в коде обращался как _gap4[0], которого не было в header).
	 * Offset-вычислен из текущего layout (B9-размеры выше); байт лежит ровно в гэпе, layout не ломается. */
	char wifi_capable;	   /* @0x368 (872): wifi-поддержка/GPIO (KLD, customer==4; mtc-car.c:2377/4011) */
	char reserved_873[3];	 /* @873..875: остаток бывшего _gap2[4] */
	unsigned char ioctl_buf1[3072];
	unsigned char buffer2[3072];
	char reserved_7020[4];	 /* was _gap3[4]: перед audio* (офсет зависит от размеров kernel-структур) */
	struct mtc_audio_struct *audio;
};

extern struct mtc_car_struct car_struct;

/* ===== binaRE MTC-14 t6b: gtp (touch) judge-символы — GLOBAL, def: car.c ===== */
struct i2c_client;
struct gtp_dev;
int gtp_reset_guitar(struct i2c_client *client, int ms); /* judge c083df60 */
int gtp_i2c_read(struct i2c_client *client, const unsigned char *buf, unsigned short len); /* judge c083d2ac/c083dfdc */
int gtp_i2c_write(struct i2c_client *client, const unsigned char *buf, unsigned short len); /* judge c083d1e0/c083e0d8 */
int gtp_i2c_test(struct i2c_client *client); /* judge c083d23c/c083e078 */
int isTouchDisable(void); /* judge c082e80c */
char **get_panel(unsigned short w, unsigned short h, int vendor, int flag); /* judge c083e378 */
unsigned int gtp_write_panel(struct gtp_dev *dev); /* judge c09c60b4 */
int gtp_init_panel(struct gtp_dev *dev); /* judge c09c61a8 */

#endif
