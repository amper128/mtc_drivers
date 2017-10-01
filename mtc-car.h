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
	char _gap3[12];
	char battery;
	char _gap4[11];
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
	char _gap8[25];
	char _gap9[22];
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
	char wipe_check;
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
	char _gap2[4];
	struct mutex car_cmd_lock;
	char _gap3[12];
	struct delayed_work wipecheckclear_work;
	char mcu_version[16];
	unsigned char mcu_date[16];
	unsigned char mcu_time[16];
	char _gap4[4];
	unsigned char ioctl_buf1[3072];
	unsigned char buffer2[3072];
	char _gap5[4];
};

#endif
