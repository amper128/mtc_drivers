#include <linux/mutex.h>

#include "mtc_shared.h"

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

struct mtc_car_status {
	char _gap0[2];
	char rpt_power;
	char _gap1[1];
	char rpt_boot_android;
	char _gap2[2];
	char touch_type;
	char _gap3[3];
	int intval1;
	int intval2;
	char battery;
	char _gap4[3];
	int intval3;
	int intval4;
	char wipe_flag;
	char backlight_status;
	char _gap5[6];
	char sta_view;
	char ch_mode;
	char key_mode;
	char _gap7[1];
	char backview_vol;
	char sta_video_signal;
	char av_channel_flag1;
	char _gap8[2];
	char mcu_clk;
	char _gap81[6];
	char mcuver2[16];
	char mcuver1[16];
	char _gap9[6];
	char is1024screen;
	char _gap10[2];
	char ch_status;
	char _gap101[2];
	char uv_cal;
	char _gap11[32];
	char rpt_boot_appinit;
	char cfg_maxvolume;
	char _gap12[1];
	int touch_width;
	int touch_height;
	char touch_info1;
	char touch_info2;
	char mtc_customer;
	char boot_flags;
	char _gap13[4];
	char av_gps_switch;
	char av_gps_monitor;
	char av_gps_gain;
	char _gap14[5];
};

struct mtc_car_struct {
	struct mtc_car_drv *car_dev;
	struct mtc_car_status car_status;
	struct mtc_car_comm *car_comm;
	int rev_bytes_count;
	unsigned int arm_rev_cmd;
	char _gap0[16];
	union mtc_config_data config_data;
	char _gap1[16];
	struct workqueue_struct *car_wq;
	struct mutex car_io_lock;
	struct mutex car_cmd_lock;
	struct timeval tv;
	struct delayed_work wipecheckclear_work;
	char mcu_version[16];
	unsigned char mcu_date[16];
	unsigned char mcu_time[16];
	char _gap2[4];
	unsigned char ioctl_buf1[3072];
	unsigned char buffer2[3072];
	char _gap3[4];
	struct mtc_audio_struct *audio;
};

extern struct mtc_car_struct car_struct;

#endif
