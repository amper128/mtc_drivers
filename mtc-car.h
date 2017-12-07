#include <linux/mutex.h>

#include "mtc_shared.h"

#ifndef _MTC_CAR_H
#define _MTC_CAR_H

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
	char _gap6[1];
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
	char _gap10[5];
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
