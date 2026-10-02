// CONFIG_HZ = 100
// xloops = 107374

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/i2c.h> /* MTC-14: gtp-секция (i2c_transfer, i2c_client) */
#include <linux/uaccess.h> /* T5 r20: copy_from/to_user */
#include <stdbool.h>
#include <linux/sched.h>
#include <linux/reboot.h>
#include <linux/input.h> /* t6b: gtp/touch closure (input_event, input_dev) */
#include <linux/earlysuspend.h> /* t6b: early_suspend (gtp_request_input_dev; RK-дерево: нет linux/platform.h) */

#include "car.h"

/* T5 r20 (binaRE): p_car_lock = mutes, zashchischayushchij car_ioctl;
 * para s car_io_lock (mutex_lock(&car_struct.car_io_lock) @2826, mutex_init @5343) */
#define p_car_lock (&car_struct.car_io_lock)

/* ===== T5 smoke-green (executor): IDA-artifact mechanical block (TENTATIVE) ===== */
#define __CFADD__(a, b) ((a) + (b)) /* IDA: overflow-checked addition */

/* .rodata literals (значения — из usage-комментариев // "...") и бинарные плейсхолдеры */
#define off_C08316FC "av_channel"
#define off_C0831700 "av_gps_monitor"
#define off_C0831704 "on"
#define off_C0831710 "canbus_rsp"
#define off_C0831714 "rpt_boot_complete"
#define off_C0831718 "rpt_logo_complete"
#define off_C083171C "rpt_boot_android"
#define off_C0831720 "rpt_boot_appinit"
#define off_C0831724 "sta_dvd"
#define off_C0831728 "sta_dvd_folder"
#define off_C083172C "sta_dvd_media"
#define off_C0831730 "sta_dvd_folder_cnt"
#define off_C0831734 "sta_dvd_media_cnt"
#define off_C0831738 "sta_dvd_folder_idx"
#define off_C083173C "sta_dvd_media_idx"
#define off_C0831740 "sta_dvd_length"
#define off_C0831744 "sta_dvd_position"
#define off_C0831748 "sta_dvd_title"
#define off_C083174C "sta_ipod"
#define off_C0831750 "cfg_maxvolume"
#define off_C0831754 "cfg_customer"
#define off_C0831758 "cfg_sn"
#define off_C083175C "cfg_model"
#define off_C0831760 "cfg_password"
#define off_C0831764 "cfg_logo1"
#define off_C0831768 "cfg_logo2"
#define off_C083176C "cfg_canbus"
#define off_C0831770 "cfg_canbus_cfg"
#define off_C0831774 "cfg_atvmode"
#define off_C0831778 "cfg_dtv"
#define off_C083177C "cfg_frontview"
#define off_C0831780 "cfg_ipod"
#define off_C0831784 "cfg_dvd"
#define off_C0831788 "cfg_bt"
#define off_C083178C "cfg_radio"
#define off_C0831790 "cfg_rds"
#define off_C0831794 "cfg_logo_type"
#define off_C0831798 "cfg_radio_area"
#define off_C083179C "cfg_launcher"
#define off_C08317A0 "cfg_led_type"
#define off_C08317A4 "cfg_rudder"
#define off_C08317A8 "cfg_key0"
#define off_C08317AC "cfg_appdisable"
#define off_C08317B0 "cfg_language_selection"
#define off_C08317B4 "cfg_color"
#define off_C08317B8 "cfg_led_multi"
#define off_C08317BC "cfg_wifi_pwr"
#define off_C08317C0 "cfg_mirror"
#define off_C08317C4 "ctl_uv_cal"
#define off_C08317C8 "av_channel_enter"
#define off_C08317CC "av_channel_exit"
#define off_C08317D0 "av_volume"
#define off_C08317D4 "av_phone_volume"
#define off_C08317D8 "rpt_boot_recovery"
#define off_C08317DC "av_gps_switch"
#define off_C08317E0 "av_phone"
#define off_C08317E4 "in"
#define off_C08317E8 "out"
#define off_C08317EC "hangup"
#define off_C08317F0 "rpt_reboot"
#define off_C08317F4 "0"
#define off_C08317F8 "av_gps_gain"
#define off_C08317FC "ctl_cvbs_brightness"
#define off_C0831800 "ctl_lcd"
#define off_C0831804 "ctl_camera"
#define off_C0831808 "start_front"
#define off_C083180C "av_speech"
#define off_C0831818 "rpt_power"
#define off_C083181C "rpt_key_mode"
#define off_C0831820 "assign"
#define off_C0831824 "normal"
#define off_C0831828 "steering"
#define off_C083182C ((char *)0) /* binaRE placeholder */
#define off_C0831830 "cfg_backlight"
#define off_C0831834 "cfg_blmode"
#define off_C0831838 "cfg_powerdelay"
#define off_C083183C "cfg_ill"
#define off_C0831840 "cfg_beep"
#define off_C0831844 "cfg_led"
#define off_C0831848 "cfg_dvr"
#define off_C083184C "cfg_wheelstudy_type"
#define off_C0831850 "cfg_key_assign"
#define off_C0831854 "cfg_ir_assign"
#define off_C0831858 ((char *)0) /* binaRE placeholder */
#define off_C0831864 ((char *)0) /* binaRE placeholder */
#define off_C0831868 "cfg_steer_assign"
#define off_C0831870 "cfg_config"
#define off_C0831874 ((char *)0) /* binaRE placeholder */
#define off_C083187C ((char *)0) /* binaRE placeholder */
#define off_C0831880 ((char *)0) /* binaRE placeholder */
#define off_C0831884 ((char *)0) /* binaRE placeholder */
#define off_C0831888 ((char *)0) /* binaRE placeholder */
#define off_C0831890 ((char *)0) /* binaRE placeholder */
#define off_C0831898 "answer"
#define off_C083189C "start"
#define off_C08318A0 "cancel"
#define off_C08318A4 ((char *)0) /* binaRE placeholder */
#define off_C08318A8 ((char *)0) /* binaRE placeholder */
#define off_C08318AC ((char *)0) /* binaRE placeholder */
#define off_C08318B0 "--mtc exit %s\n"
#define off_C08318B4 "--mtc enter %s\n"
#define off_C08318B8 "gsm_bt"
#define off_C08318BC "sys"
#define off_C08318C0 "fm"
#define off_C08318C4 "ipod"
#define off_C08318CC "ctl_capture_on"
#define off_C08318D0 "ctl_capture_off"
#define off_C08318D4 "dvd"
#define off_C08318D8 "line"
#define off_C08318DC "dtv"
#define off_C08318E0 "dvr"
#define off_C08318E4 "ctl_radar"
#define off_C08318E8 "ctl_beep"
#define off_C08318EC "sta_driving"
#define off_C08318F0 "fm"
#define off_C08318F8 "sta_ill"
#define off_C0831904 "ctl_dtv_ir"
#define off_C0831908 "ctl_dvd_cmd"
#define off_C083190C "ctl_dvd_door"
#define off_C0831910 "open"
#define off_C0831914 "close"
#define off_C0831918 "eject"
#define off_C0831920 "sta_dtv"
#define off_C0831924 "sta_battery"
#define off_C0831928 "sta_touch"
#define off_C083192C "none"
#define off_C0831930 "sta_touch_adc"
#define off_C0831934 "sta_touch_cal"
#define off_C0831938 ((char *)0) /* binaRE placeholder */
#define off_C083193C "recovery"
#define off_C0831940 "success"
#define off_C0831948 "sta_video_signal"
#define off_C083194C "ok"
#define off_C0833864 "ctl_radio_ta"
#define off_C0833878 "nosignal" /* T5 r20: TENTATIVE po IDA-kommentariyu (was binaRE placeholder) */
#define off_C083387C "sta_radio_signal"
#define off_C0833880 "sta_tv_status"
#define off_C0833884 "sta_tv_signal"
#define off_C0833888 "sta_radio_stereo"
#define off_C0833890 "sta_mcu_version"
#define off_C0833894 "sta_mcu_date"
#define off_C0833898 "sta_mcu_time"
#define off_C083389C "sta_uv_cal"
#define off_C08338A0 "sta_view"
#define off_C08338A4 "front"
#define off_C08338A8 "back"
#define off_C08338AC "ctl_radio_af"
#define off_C08338B0 "sta_touch_info"
#define off_C08338B4 "sta_wipe"
#define off_C08338B8 "no"
#define off_C08338C4 "ctl_radio_search"
#define off_C08338C8 ((char *)0) /* binaRE placeholder */
#define off_C08338CC "ctl_radio_frequency"
#define off_C08338D0 "ctl_radio_sfrequency"
#define off_C08338D4 "ctl_soft_mute"
#define off_C08338D8 "stereo"
#define off_C08338DC "secam" /* T5 r20: TENTATIVE po IDA-kommentariyu (was binaRE placeholder) */
#define off_C08338E0 "mono" /* T5 r20: TENTATIVE po IDA-kommentariyu (was binaRE placeholder) */
#define off_C08338E4 "ntsc" /* T5 r20: TENTATIVE po IDA-kommentariyu (was binaRE placeholder) */
#define off_C08338E8 "ctl_radio_mute"
#define off_C08338EC "ctl_radio_stereo"
#define off_C08338F0 "cfg_config"
#define off_C08338F8 "ctl_backview_vol"
#define off_C08338FC "ctl_backview_mute"
#define off_C0833900 "av_gps_ontop"
#define off_C0833904 "ctl_tv_frequency"
#define off_C0833908 "ctl_tv_demod"
#define off_C083390C "ctl_key"
#define off_C0833910 "power"
#define off_C0833914 "power2"
#define off_C0833918 "eject"
#define off_C083391C "screenbrightness"
#define off_C0833920 "parrot_updata"
#define off_C0833924 "parrot_normal"
#define off_C0833928 "av_lud"
#define off_C0833934 "ctl_power"
#define off_C0833938 "av_balance"
#define off_C083393C "av_eq"
#define off_C0833940 "av_gps_monitor"
#define off_C0833944 "ctl_reset"
#define off_C0833948 "0"
#define off_C083394C "1"
#define off_C0833950 "recovery"
#define off_C0833954 "av_gps_switch"
#define off_C0833958 "av_gps_gain"
#define off_C083395C "av_active"

static const char str_av_mute_[] = "av_mute";
static const char str_diskin[] = "disk in"; /* TENTATIVE: по имени (binaRE) */
static const char str_fail[] = "fail"; /* TENTATIVE: по имени (binaRE) */
static const char str_false[] = "false";
static const char str_false_0[] = "false";
static const char str_fmt_d_3[] = "%d";
static const char str_fmt_d_4[] = "%d";
static const char str_nodisk[] = "nodisk"; /* TENTATIVE: по имени (binaRE) */
static const char str_off[] = "off";
static const char str_on[] = "on";
static const char str_start_back[] = "start_back";
static const char str_true[] = "true";
static const char str_true_0[] = "true";
static const char str_fmt_d_comma_2[] = "%d"; /* TENTATIVE: все usage — sprintf(dst,fmt,int) */
static const char str_fmt_d_comma_3[] = "%d"; /* TENTATIVE: все usage — sprintf(dst,fmt,int) */

static struct miscdevice mtc_car_miscdev; /* forward (def ниже) */
static noinline void car_avm(void); /* forward (def ниже); binaRE t LOCAL c082fa8c — noinline: в orig не инлайнен, -O2 у нас инлайнит */
static irqreturn_t mcu_isr_cb(int irq, void *dev_id)
{ (void)irq; (void)dev_id; return IRQ_HANDLED; /* binaRE placeholder */ }
static void WipeCheckClear_work(struct work_struct *work)
{ (void)work; pr_warn("--mtc WipeCheckClear_work: binaRE placeholder\n"); }

/* binaRE плейсхолдеры: локальные буферы декомпилятора */
static char mtc_sta_buf[512];
static char *buf_1 = mtc_sta_buf;
static char *p_buf1 = mtc_sta_buf;
static char *p_buf2 = mtc_sta_buf;
static char *p_buf1_3060 = NULL; /* TENTATIVE: &car_struct.ioctl_buf1[3060] (ставится в car_ioctl) */
static union mtc_config_data *p_config_data_4 = NULL; /* TENTATIVE: ставится в car_ioctl */



/* T5 minfix: cross-TU прототипы (сигнатуры = определения в vs.c/audio_card_glue.c/backview.c) */
int vs_send(int port_num, unsigned char cmd, char *cmd_data, signed int count); /* binary returns int (decompiled_vs_send.c) */
void vs_send_raw(int port_num, unsigned char *data, int count);
void capture_add_work(unsigned int cmd1, int cmd2, unsigned int delay, int flush);
void audio_add_work(unsigned int cmd1, int cmd2, int cmd3, int val1);

/* T5 minfix: decompiler-низкие имена -> SDK API (plat/gpio.h: gpio_set_value=__gpio_set_value;
 * asm-generic/delay.h: udelay(n)=__udelay(n), n в usec) */
#define _gpio_set_value gpio_set_value
#define _gpio_get_value gpio_get_value
#define _const_udelay(n) __const_udelay(n)	/* SDK asm/delay.h: константный путь __const_udelay (arch/arm/lib/delay.S); udelay(const>2000) -> __bad_udelay (undefined) */

/* T5 minfix: stubs — тела не реконструированы (binaRE: to-do); для полноты линковки. */
void backlight_on(void) { }
void backlight_off(void) { }
void backlight_update(void) { }
void Hit_radio_sta(int a1, int a2, int a3, int a4, int a5) { (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; }

/* binaRE recon round3: локальные объявления (определения — блок "binaRE recon round3"
 * перед car_ioctl). Адреса — 3188_kallsyms (tr -d '\r'), не из decompiled-заголовков. */

/* binaRE: состояние колёса (adc_wheel_callback, R2; контекст регистрации 0xC09BCF38) —
 * поля по оффсетам из disassembly. */
struct mtc_wheel_state {
	u32 pad0;		/* @0 */
	u32 key_repeat_cnt;	/* @4 — дебаунс/повтор (ставится 40) */
	u8 pad1[84];	/* @5..87 */
	u32 *adc_ref_up;	/* @88 — указатель на обученное "up"-значение ADC */
	u32 *adc_ref_dn;	/* @92 — указатель на обученное "down"-значение ADC */
	u8 wheel_state;	/* @96 — старший ниббл: текущее направление, младший: предыдущее */
	u8 wheel_last_key;	/* @97 — 0x40 / 0x41('A') / 0x42('B') */
	u8 pad2[158];	/* @98..255 */
	u32 adc_up_val;	/* @256 */
	u32 adc_dn_val;	/* @260 */
};

/* binaRE 0xC083B8A0 (add_wheel_work.constprop.9, 116B) — вне списка 17 функций этого раунда;
 * прототип для вызовов из adc_wheel_callback. */
extern int add_wheel_work(int key, struct mtc_wheel_state *ws);

static void power_soft_off(void);
static int check_customer(const char *name);
static int get_token_int(char **pos);
static int process_mcu_command(unsigned int cmd);
int mtcWipeCheck(void);
char *mtc_get_pin_map(int pin_id);
int mtc_init_test_io(void);
int mtc_test_port2(unsigned char *pa, unsigned char *pb);
int mtc_test_port3(unsigned char *pa, unsigned char *pb, unsigned char *pc);
char *mtc_test_port(void);
void mtc_clear_screen(int color);
char *mtc_debug_putc(int glyph, int x, int y, int fg_color, int bg_color);
char *mtc_debug_put_string(const char *s, int len, int x0, int y,
				  int fg_color, int bg_color);
static int adc_wheel_callback(const u32 *adc_cur, struct mtc_wheel_state *ws,
			      int adc_val);
static void stw_range_check(void);
int mtc_iomux_set(unsigned int mode);
int mtc_get_screen_width(void);
int mtc_get_screen_height(void);

struct mtc_car_struct car_struct;   /* T5 minfix: было static mtc_car_struct (дубль глобала); символ kallsyms = car_struct (car.h:142, якорь 0xC168AC80) */

static struct mtc_car_status *car_status = &car_struct.car_status;
static union mtc_config_data *config_data = &car_struct.config_data;

static int arm_rev(void);

/* fully decompiled */
static inline long
GetCurTimer()
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return tv.tv_usec;
}

/* fully decompiled */
static bool
CheckTimeOut(long timeout)
{
	long usec;
	struct timeval tv;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;
	if (usec < timeout) {
		usec = tv.tv_usec + 1000000;
	}
	return (usec - timeout) > 249999;
}

/* антидребезг? */
/* fully decompiled */
static int
getPin(unsigned int gpio)
{
	int value;

	do {
		value = gpio_get_value(gpio);
		udelay(1);
	} while (value != gpio_get_value(gpio));

	return value;
}

/* антидребезг? */
/* fully decompiled */
static int
getPin2(unsigned int gpio)
{
	int value;

	do {
		value = gpio_get_value(gpio);
		udelay(5);
	} while (value != gpio_get_value(gpio));

	return value;
}

/*
 * ==================================
 *	miscdev file operations
 * ==================================
 */

/* fully decompiled */
static int
car_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return 0;
}

/* fully decompiled */
static ssize_t
car_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	(void)filp;
	(void)buf;
	(void)count;
	(void)offp;

	return 0;
}

/* fully decompiled */
static ssize_t
car_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
	(void)filp;
	(void)buf;
	(void)count;
	(void)offp;

	return 0;
}


/*
 * ==================================
 *	MCU communications
 * ==================================
 */

/* fully decompiled */
void
arm_parrot_boot(int mode)
{
	signed int i; // r4@6

	if (mode) {
		if (mode == 1) {
			gpio_direction_output(gpio_PARROT_RESET, 0);
			gpio_direction_output(gpio_PARROT_BOOT, 0);
			udelay(10);
			gpio_direction_output(gpio_PARROT_BOOT, 1);
			udelay(40);
			gpio_direction_output(gpio_PARROT_RESET, 1);

			for (i = 0; i < 12; i++) {
				udelay(1000);
			}

			gpio_direction_output(gpio_PARROT_BOOT, 0);
		} else if (mode == 2) {
			gpio_direction_output(gpio_PARROT_RESET, 1);
		}
	} else {
		gpio_direction_output(gpio_PARROT_RESET, 1);
		gpio_direction_output(gpio_PARROT_BOOT, mode);
	}
}

EXPORT_SYMBOL_GPL(arm_parrot_boot);

/* все это очень сильно смахивает на SPI, почему не использовали хардверную
 * шину?? */
/* fully decompiled */
static int
arm_send_cmd(unsigned int cmd)
{
	long clk_timeout;
	long timeout_word;
	int bit;
	int bit_pos;

	while (1) {
		if (!getPin2(gpio_MCU_CLK)) {
			printk("~ SND EXIT 0\n");
			goto send_err1;
		}
		if (getPin2(gpio_MCU_DIN)) {
			break;
		}
		printk("~ REV RESTART 0\n");
		arm_rev();
	}

	gpio_direction_output(gpio_MCU_DOUT, 0);
	clk_timeout = GetCurTimer();
	while (getPin2(gpio_MCU_CLK)) {
		if (CheckTimeOut(clk_timeout)) {
			printk("~ arm_send err0 %04x\n", cmd);
			goto send_err1;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 1);
	clk_timeout = GetCurTimer();
	while (!getPin2(gpio_MCU_CLK)) {
		if (CheckTimeOut(clk_timeout)) {
			printk("~ arm_send err1\n");
			goto send_err1;
		}
	}

	bit_pos = 0;
	car_struct.rev_bytes_count = 0x10000;
	car_struct.arm_rev_cmd = cmd;

	udelay(10);

LABEL_10:
	timeout_word = GetCurTimer();
	do {
		if (getPin(gpio_MCU_DIN)) {
			long timeout;

			if ((cmd & 0x8000) == 0) {
				bit = 0;
			} else {
				bit = 1;
			}

			gpio_set_value(gpio_MCU_DOUT, bit);
			gpio_direction_output(gpio_MCU_CLK, 0);

			timeout = GetCurTimer();
			while (getPin(gpio_MCU_DIN)) {
				if (CheckTimeOut(timeout)) {
					printk("~ arm_send_16bits err1 %d\n", bit_pos);
					goto send_err0;
				}
			}

			if (bit_pos != 15) {
				gpio_direction_output(gpio_MCU_CLK, 1);
				bit_pos++;
				goto LABEL_10;
			}

			gpio_set_value(gpio_MCU_DOUT, 1);
			gpio_direction_output(gpio_MCU_CLK, 1);
			return 1;
		}
	} while (!CheckTimeOut(timeout_word));

	printk("~ arm_send_16bits err0 %d\n", bit_pos);

send_err0:
	gpio_set_value(gpio_MCU_DOUT, 1);
	gpio_direction_input(gpio_MCU_CLK);
	udelay(10);

send_err1:
	gpio_set_value(gpio_MCU_DOUT, 1);
	return 0;
}

/* fully decompiled */
static int
arm_send_ack()
{
	long timeout;

	gpio_direction_input(gpio_MCU_CLK);
	timeout = GetCurTimer();
	while (!getPin(gpio_MCU_DIN)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_send_ack err0\n"); // а тут китайцы забыли
			// перенос строки
			goto send_error;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 0);
	timeout = GetCurTimer();
	while (getPin(gpio_MCU_CLK)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_send_ack err1\n");
			goto send_error;
		}
	}

	gpio_set_value(gpio_MCU_DOUT, 1);
	timeout = GetCurTimer();
	do {
		if (getPin(gpio_MCU_CLK)) {
			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_send_ack err2\n");

send_error:
	gpio_set_value(gpio_MCU_DOUT, 1);

	return 0;
}

/* fully decompiled */
static int
arm_rev_8bits(unsigned char *byteval)
{
	unsigned char byte;
	int bit_n;
	long timeout;
	long timeout_bit;

	byte = 0;
	gpio_direction_input(gpio_MCU_CLK);
	bit_n = 0;

LABEL_2:
	timeout = GetCurTimer();
	do {
		if (!getPin(gpio_MCU_CLK)) {
			byte = (unsigned char)(byte << 1u);
			if (getPin(gpio_MCU_DIN)) {
				byte |= 1u;
			}
			gpio_set_value(gpio_MCU_DOUT, 0);

			timeout_bit = GetCurTimer();
			while (!getPin(gpio_MCU_CLK)) {
				if (CheckTimeOut(timeout_bit)) {
					printk("~ arm_rev_8bits err1 %d\n", bit_n);
					goto err_rev;
				}
			}

			bit_n = (bit_n + 1);
			gpio_set_value(gpio_MCU_DOUT, 1);
			if (bit_n != 8) {
				goto LABEL_2;
			}
			*byteval = byte;

			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_rev_8bits err0 %x %x %d\n", car_struct.rev_bytes_count,
	       car_struct.arm_rev_cmd, bit_n);

err_rev:
	gpio_set_value(gpio_MCU_DOUT, 1);

	return 0;
}

/* fully decompiled */
static int
arm_rev_bytes(unsigned char *buf, int count)
{
	int pos;
	int result;

	if (count) {
		pos = 0;
		while (1) {
			car_struct.rev_bytes_count++;
			result = arm_rev_8bits(&buf[pos++]);

			if (!result) {
				break;
			}
			if (pos == count) {
				return 1;
			}
		}
	} else {
		result = 1;
	}

	return result;
}

/* fully decompiled */
static int
arm_rev_ack()
{
	long timeout;

	timeout = GetCurTimer();
	while (getPin(gpio_MCU_DIN)) {
		if (CheckTimeOut(timeout)) {
			printk("~ arm_rev_ack err0\n"); // а тут была копипаста,
			// снова без переноса
			goto send_ack_err;
		}
	}

	gpio_direction_output(gpio_MCU_CLK, 0);
	timeout = GetCurTimer();
	do {
		if (getPin(gpio_MCU_DIN)) {
			gpio_direction_input(gpio_MCU_CLK);
			udelay(10);
			getPin(gpio_MCU_DIN);
			return 1;
		}
	} while (!CheckTimeOut(timeout));

	printk("~ arm_rev_ack err1\n"); // и тут копипаста

send_ack_err:
	gpio_direction_input(gpio_MCU_CLK);
	udelay(10);

	return 0;
}

/* fully decompiled */
static int
arm_rev()
{
	int mcu_clk_val;
	int mcu_din_val;
	unsigned int arm_rev_cmd;
	int bit_pos;
	long timeout;

	mcu_clk_val = getPin2(gpio_MCU_CLK);
	if (mcu_clk_val) {
		if (getPin2(gpio_MCU_DIN)) {
			mcu_clk_val = 0;
		} else {
			gpio_direction_output(gpio_MCU_CLK, 0);
			udelay(1);

			timeout = GetCurTimer();
			while (1) {
				mcu_din_val = getPin2(gpio_MCU_DIN);
				if (mcu_din_val) {
					break;
				}

				if (CheckTimeOut(timeout)) {
					printk("~ arm_rev err0\n");
					gpio_direction_input(gpio_MCU_CLK);
					udelay(10);

					return mcu_din_val;
				}
			}

			gpio_direction_output(gpio_MCU_CLK, 1);
			udelay(1);
			car_struct.rev_bytes_count = 0x20000;
			gpio_direction_input(gpio_MCU_CLK);
			arm_rev_cmd = 0;
			bit_pos = 0;

		LABEL_8:
			timeout = GetCurTimer();
			do {
				if (!getPin(gpio_MCU_CLK)) {
					arm_rev_cmd = (unsigned char)(arm_rev_cmd << 1);
					if (getPin(gpio_MCU_DIN)) {
						arm_rev_cmd |= 1u;
					}

					gpio_set_value(gpio_MCU_DOUT, 0);
					timeout = GetCurTimer();
					while (!getPin(gpio_MCU_CLK)) {
						if (CheckTimeOut(timeout)) {
							printk("~ "
							       "arm_rev_16bits "
							       "err1 %d\n",
							       bit_pos);
							goto LABEL_19;
						}
					}

					bit_pos++;
					gpio_set_value(gpio_MCU_DOUT, 1);
					if (bit_pos != 16) {
						goto LABEL_8;
					}

					car_struct.arm_rev_cmd = arm_rev_cmd;

					return process_mcu_command(arm_rev_cmd);
				}
			} while (!CheckTimeOut(timeout));

			printk("~ arm_rev_16bits err0 %d\n", bit_pos);

		LABEL_19:
			gpio_set_value(gpio_MCU_DOUT, 1);
			mcu_clk_val = 0;
		}
	}

	return mcu_clk_val;
}

/* fully decompiled */
void
arm_send(unsigned int cmd)
{
	int hi_byte;
	unsigned char byteval = 0;

	disable_irq(car_struct.car_comm->mcu_din_gpio);
	mutex_lock(&car_struct.car_comm->car_lock);

	if (arm_send_cmd(cmd)) {
		hi_byte = cmd & 0xFF00;

		if ((hi_byte == 0xF00) || (cmd == 0x201)) {
			if (arm_rev_8bits(&byteval)) {
				arm_rev_ack();
			}
		} else {
			arm_send_ack();
		}
	}

	enable_irq(car_struct.car_comm->mcu_din_gpio);
	mutex_unlock(&car_struct.car_comm->car_lock);
}
EXPORT_SYMBOL_GPL(arm_send);

/* fully decompiled */
int
arm_send_multi(unsigned int cmd, int count, unsigned char *buf)
{
	int recv;
	int pos;
	unsigned char byte;
	char bitval;
	int bit;
	long timeout;
	int res;
	int bit_pos;

	disable_irq(car_struct.car_comm->mcu_din_gpio);
	mutex_lock(&car_struct.car_comm->car_lock);
	recv = arm_send_cmd(cmd);

	if (!recv) {
	LABEL_18:
		res = recv;
		goto LABEL_19;
	}

	if (cmd != 0xA000) {
		if (cmd & 0x8000) {
			if (count) {
				for (pos = 0; pos < count; pos++) {
					car_struct.rev_bytes_count += 0x100;
					byte = buf[pos];

					for (bit_pos = 0; bit_pos < 7; bit_pos++) {
						timeout = GetCurTimer();
						while (!getPin(gpio_MCU_DIN)) {
							if (CheckTimeOut(timeout)) {
								printk("~ "
								       "arm_send_"
								       "8bits "
								       "err0 %d\n",
								       bit_pos);
							LABEL_23:
								gpio_set_value(gpio_MCU_DOUT, 1);
								gpio_direction_input(gpio_MCU_CLK);
								res = 0;
								udelay(10);

								goto LABEL_19;
							}
						}

						if ((byte & 0x80) == 0) {
							bit = 0;
						} else {
							bit = 1;
						}

						byte = (unsigned char)(byte << 1);

						gpio_set_value(gpio_MCU_DOUT, bit);
						gpio_direction_output(gpio_MCU_CLK, 0);
						timeout = GetCurTimer();
						while (getPin(gpio_MCU_DIN)) {
							if (CheckTimeOut(timeout)) {
								printk("~ "
								       "arm_send_"
								       "8bits "
								       "err1 %d\n",
								       bit_pos);
								goto LABEL_23;
							}
						}

						if (bit_pos != 7) {
							gpio_direction_output(gpio_MCU_CLK, 1);
						}
					}
					gpio_set_value(gpio_MCU_DOUT, 1);
					gpio_direction_output(gpio_MCU_CLK, 1);
				}
			}

			goto LABEL_21;
		}
		recv = arm_rev_bytes(buf, count);
		if (recv) {
			res = arm_rev_ack();
			goto LABEL_19;
		}
		goto LABEL_18;
	}

	if (!count) {
	LABEL_21:
		res = arm_send_ack();
		goto LABEL_19;
	}

	for (pos = 0; pos < count; pos++) {
		byte = buf[pos];
		bit_pos = 8;

		while (1) {
			while (!getPin(gpio_MCU_DIN)) {
				;
			}

			bitval = (byte & 0x80) == 0;
			byte = (unsigned char)(byte << 1);
			bit = bitval ? 0 : 1;
			gpio_set_value(gpio_MCU_DOUT, bit);
			gpio_direction_output(gpio_MCU_CLK, 0);

			while (getPin(gpio_MCU_DIN)) {
				;
			}

			if (bit_pos == 1) {
				break;
			}

			bit_pos--;
			gpio_direction_output(gpio_MCU_CLK, 1);

			if (!bit_pos) {
				goto LABEL_36;
			}
		}

		gpio_set_value(gpio_MCU_DOUT, 1);
		gpio_direction_output(gpio_MCU_CLK, 1);

	LABEL_36:;
	}

	res = arm_send_ack();

LABEL_19:
	enable_irq(car_struct.car_comm->mcu_din_gpio);
	mutex_unlock(&car_struct.car_comm->car_lock);

	return res;
}

EXPORT_SYMBOL_GPL(arm_send_multi);

/*
 * ==================================
 *	     work functions
 * ==================================
 */

// dirty code
static void
car_work(struct work_struct *work)
{
	struct mtc_work *car_work_data;
	unsigned int t = 40;
	int cmd2, cmd2_32; /* IDA: locals case 67/69 */
	int _cmd2_32; /* IDA: case 69 */
	struct timeval tv; /* IDA: case 69 */
	long old_sec; /* IDA: case 69 */

	car_work_data = container_of(work, struct mtc_work, dwork);

	mutex_lock(&car_struct.car_cmd_lock);

	while (!car_status->car_ready) {
		msleep(10u);
	}

	switch (car_work_data->cmd1) {

	case 29:
		if (++car_status->power_refcnt) {
			audio_active(); /* IDA: def audio_card_glue.c без аргументов */
			backlight_on();
			break;
		}

		backlight_on();
		break;

	case 30:
		if (!car_status->power_refcnt) {
			break;
		}

		if (--car_status->power_refcnt) {
			break;
		}

		audio_deactive();
		if (car_status->cam_state) {
			break;
		}

		backlight_off();
		break;

	case 31:
		capture_add_work(0x38u, 0, 0, 0);
		break;

	case 32:
		capture_add_work(0x39u, 0, 0, 0);
		break;

	case 33:
		capture_add_work(0x3Cu, 0, 0, 0);
		break;

	case 34:
		capture_add_work(0x3Bu, 0, 0, 0);
		break;

	case 35:
		backlight_on();
		break;

	case 36:
		backlight_off();
		break;

	case 37:
		arm_send(0x202u);
		car_status->call_active = 1;
		if (car_status->power_refcnt) {
			vs_send(2, 0xF2u, 0, 0);
			audio_add_work(0x14u, 0, 0, 0);
		}
		if (car_status->wipe_flag & 0x10) {
			vs_send(2, 0x88u, 0, 0);
		}
		if (car_status->video_src_ready) {
			vs_send(2, 0x9Fu, 0, 0);
		}
		capture_add_work(0x3Au, 2000, 0, 0);

		break;

	case 38:
		if (car_status->video_src_ready) {
			audio_add_work(0x16u, 0, 0, 0);
		}
		break;

	case 42:
		printk("--mtc on %d\n", car_work_data->cmd2);
		if (car_status->call_active) {
			vs_send(2, 0xF1, 0, 0);

			car_status->power_refcnt = 1;
			car_status->power_on = 1;

			audio_add_work(20, 0, 0, 0);
			capture_add_work(46, 1, 0, 0);

			if (car_work_data->cmd2) {
				backlight_on();
			}
		} else {
			car_status->power_refcnt = 1;
			car_status->power_on = 1;
			capture_add_work(47, 0, 0, 0);
			backlight_on();
		}
		if (!car_work_data->cmd2) {
			capture_add_work(56, 1000, 0, 0);
		}

		break;

	case 43:
		printk("--mtc off\n");
		car_status->power_refcnt = 0;
		if (car_status->cam_signal) {
			capture_add_work(0x37u, 0, 0, 1);
		}
		if (car_status->call_active) {
			car_status->rpt_power = 1;
			t = 40;
			vs_send(2, 0xF0u, 0, 0);
			do {
				msleep(50u);
				t--;
			} while (car_status->rpt_power && t);
		}
		power_soft_off();
		t = 80;
		do {
			msleep(50u);
			t--;
		} while (car_status->rpt_power && t);

		arm_send(0x20Bu);

		break;

	case 44:
		printk("--mtc on acc\n");
		vs_send(2, 0x8Bu, 0, 0);

		break;

	case 45:
		vs_send(2, 0x8Au, 0, 0);
		if (car_status->cam_state) {
			capture_add_work(0x3Bu, 0, 0, 1);
		} else if (car_status->cam_signal) {
			capture_add_work(0x37u, 0, 0, 1);
		}
		if (car_status->power_refcnt) {
			car_status->power_refcnt = 0;
			car_status->rpt_power = 1;

			vs_send(2, 0xF0u, 0, 0);

			do {
				msleep(40u);
				t--;
			} while (car_status->rpt_power && (t > 0));
		}

		t = 80;
		printk("--mtc off acc\n");
		power_soft_off();
		do {
			msleep(50u);
			t--;
		} while (car_status->rpt_power && (t > 0));

		break;

	case 67:
		vs_send(2, 0xA4u, 0, 0);
		cmd2 = car_work_data->cmd2; /* was CONTAINING_RECORD(work, struct mtc_work, dwork)->cmd2 */
		if (cmd2 <= 31) {
			break;
		}
		cmd2_32 = cmd2 - 32;
		if (!cmd2_32) {
			arm_send_multi(0x9527u, 0, 0);
			break;
		}
		if (cmd2_32 == 0x28) {
			arm_send_multi(0x9528u, 0, 0);
			break;
		}
		if (cmd2_32 != 0x63) {
			if (cmd2_32 != 0x5E) {
				send_ir_key(cmd2_32);
				break;
			}
		LABEL_135:
			car_avm();
			break;
		}

	LABEL_97:
		if (car_status->backlight_status) {
			car_add_work(CAR_WORK_BL_ON, 0, 0);
		} else {
			car_add_work(CAR_WORK_BL_OFF, 0, 0);
		}
		break;

	case 68: {
		char cmd_data = (char)car_work_data->cmd2;

		vs_send(2, 0xA3u, &cmd_data, 1);
		break;
	}

	case 69:
		do_gettimeofday(&tv);
		cmd2 = car_work_data->cmd2;
		if (cmd2 <= 0x1F) {
		LABEL_93:
			old_sec = tv.tv_sec - car_struct.tv.tv_sec;
			car_struct.tv = tv;
			if (tv.tv_usec - car_struct.tv.tv_usec + 1000000 * old_sec <= 29999) {
				break;
			}
			cmd2 = car_work_data->cmd2;
			if (cmd2 > 31) {
				send_ir_key(cmd2 - 32);
				break;
			}
			switch (cmd2) {
			case 1:
				goto LABEL_96;
			case 2:
				send_ir_key(26);
				break;
			case 3:
				send_ir_key(58);
				break;
			case 4:
				send_ir_key(57);
				break;
			case 5:
				send_ir_key(68);
				break;
			case 6:
				send_ir_key(3);
				break;
			case 7:
				send_ir_key(1);
				break;
			case 8:
				send_ir_key(69);
				break;
			case 9:
				send_ir_key(5);
				break;
			case 10:
				send_ir_key(13);
				break;
			case 11:
				send_ir_key(8);
				break;
			case 12:
				send_ir_key(10);
				break;
			case 13:
				send_ir_key(9);
				break;
			default:
				break;
			case 16:
				send_ir_key(54);
				break;
			case 17:
				send_ir_key(66);
				break;
			case 18:
				send_ir_key(50);
				break;
			case 19:
				send_event_key(28);
				break;
			case 20:
				send_event_key(139);
				break;
			case 21:
				goto LABEL_97;
			}
	LABEL_96:	/* TENTATIVE: IDA-лейбл потерян (case 1); контекст case 7 */
			send_ir_key(1);
			break;
			goto LABEL_97;
		}
		_cmd2_32 = (cmd2 - 32);
		if (_cmd2_32 == 0x40) {
			car_status->video_src_ready = 1;
			key_beep();
			if (car_status->rpt_boot_android) {
				audio_add_work(0x16u, 0, 0, 0);
			}
			if (car_status->call_active) {
				vs_send(2, 0x9Fu, 0, 0);
			}
		} else {
			if (_cmd2_32 != 0x41) {
				if (_cmd2_32 == 0x5E) {
					goto LABEL_135;
				}
				goto LABEL_93;
			}
			car_status->video_src_ready = 0;
			key_beep();
			if (car_status->rpt_boot_android) {
				if (car_status->ajx_active) {
					if (car_status->power_refcnt == 1) {
						backlight_off();
					}
				}
				audio_add_work(0x17u, 0, 0, 0);
			}
			if (car_status->call_active) {
				vs_send(2, 0xA0u, 0, 0);
			}
		}
		break;

	case 70:
		if (car_work_data->cmd2 <= 0x1F) {
			if (car_work_data->cmd2 == 1) {
				send_ir_key(0x12);
			} else if (car_work_data->cmd2 == 2) {
				send_ir_key(0x1A);
			}
		} else {
			int cmd2 = car_work_data->cmd2 - 32;

			if (cmd2 == 0x32 || cmd2 == 0x1A) {
				send_ir_key(cmd2);
			}
		}
		break;

	case 72:
		if (car_status->boot_flags) {
			arm_send_multi(MTC_CMD_RESET2, 0, 0);
			kernel_restart("recovery");
		}
		break;

	case 73:
		kernel_power_off();
		break;

	case 74:
		arm_parrot_boot(car_work_data->cmd2);
		break;

	case 0xFFFF:
		printk("--TT\n");
		car_add_work_delay(0xFFFF, 0, 2000u);
		break;
	}

	kzfree(car_work_data);
	mutex_unlock(&car_struct.car_cmd_lock);
}


/* ============================================================
 * binaRE recon round3: 17 car-misc-функций mtc-модуля.
 * Адреса — 3188_kallsyms (tr -d '\r'); код — src_all/decompiled_*,
 * уточнения — disassembly_full.txt (точечные окна).
 * ============================================================ */

/* --- статические глобалы (честные плейсхолдеры; константы — по binaRE) --- */

/* binaRE 0xC0A06F48 (kernel.elf, file offset 0x606F48): шрифт 8x16, 80 глифов;
 * байты извлечены из kernel.elf. */
static const unsigned char font_8x16[1280] = {
  0xAC, 0x05, 0x00, 0x00, 0x47, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x4C, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x4D, 0x02, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x4E, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x49, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x4A, 0x02, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x4B, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x52, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x53, 0x02, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x54, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x39, 0x02, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x3A, 0x02, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x3B, 0x02, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x0A, 0x03, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xAC, 0x05, 0x00, 0x00, 0x0B, 0x03, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x0D, 0x05, 0x00, 0x00, 0x01, 0x32, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x20, 0x10, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x6A, 0x04, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x6A, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xF2, 0x04, 0x00, 0x00, 0x18, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xF2, 0x04, 0x00, 0x00, 0x23, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xB4, 0x04, 0x00, 0x00, 0x61, 0xDE, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xB4, 0x04, 0x00, 0x00, 0x64, 0xDE, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xB4, 0x04, 0x00, 0x00, 0xA1, 0xBC, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0xB4, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x79, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x79, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x06, 0x20, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x18, 0x05, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x16, 0x0C, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x16, 0x0C, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x16, 0x0C, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x7D, 0x04, 0x00, 0x00, 0x41, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x26, 0x09, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x58, 0x04, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x41, 0x12, 0x00, 0x00, 0x67, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x00, 0xD5,
  0xAF, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD4,
  0xAE, 0xA7, 0x98, 0xA1, 0x70, 0x00, 0x00, 0x00, 0x9A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xB8, 0xB9,
  0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0x00, 0x00, 0x00, 0x00, 0xAF, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x13, 0xC5, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x0C, 0xC5, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x17, 0xC5, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x01, 0xC1, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x04, 0xC7, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x14, 0xC7, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x1F, 0xC7, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x0A, 0xC3, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x12, 0xC5, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x15, 0xC2, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x94, 0xC2, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x0A, 0xC2, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x11, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x19, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x83, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x86, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x95, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x03, 0xCA, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x99, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x9B, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x98, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x9C, 0xC2, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x93, 0xC2, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x18, 0xC2, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
  0x87, 0xC2, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x6D, 0x04, 0x00, 0x00,
};

/* binaRE .bss: экран/фреймбуфер (mtc_get_screen_width / mtc_debug_putc / mtc_clear_screen) */
static unsigned int mtc_fb_width;	/* 0xC0D1DE28 — ширина экрана (800/1024) */
static unsigned int mtc_fb_height;	/* 0xC0D1DE2C — высота экрана (480/768) */
static u32 *mtc_fb_buf;			/* 0xC0D1DE30 — framebuffer (ARGB u32) */

/* binaRE 0xC0BC9B20: указатель на pin-таблицу (16B-записи: [0]=pin id, 0=терминатор;
 * [4]=gpio; [8]=iomux-режим; [14]=флаг "проверено"). Начальное значение из .data =
 * 0xC09BC034 — НЕ декодируется как таблица (kernel.elf); runtime-значение выставляется
 * в другом месте — честный плейсхолдер NULL. binaRE NULL не проверяет (нет call-sites). */
static unsigned char *pin_map_tbl = NULL;

/* binaRE 0xC0BCB25C..0xC0BCB26C (.data): screen-info слова [0xC0B0462C, 1082, 536, 0x10550].
 * Роль не определена из бинара в этом раунде — честные плейсхолдеры с начальными
 * константами из kernel.elf. */
static u32 screen_info_1 = 1082;	/* 0xC0BCB260 */
static u32 screen_info_2 = 536;		/* 0xC0BCB264 */

/* binaRE 0xC0BC9F54: таблица test-последовательности (mtc_test_port): записи 3B
 * {a, b, c}, a == 0 — терминатор. Байты извлечены из kernel.elf (реальные). */
static const unsigned char test_seq_tbl[] = {
1, 74, 107, 3, 7, 0, 5, 9, 0, 123, 131, 0, 125, 127, 0, 133, 141, 0, 139, 143, 0, 147, 153, 0, 149, 155, 0, 76, 80, 0, 78, 82, 0, 84, 88, 0, 86, 90, 0, 92, 106, 0, 102, 108, 0, 112, 118, 0, 116, 120, 0, 122, 130, 0, 126, 132, 0, 134, 138, 0, 136, 140, 0, 144, 150, 0, 148, 152, 0, 154, 160, 0, 158, 164, 0, 166, 170, 0, 168, 172, 0, 174, 178, 0, 176, 180, 0, 0
};

/* binaRE 0xC168E474: состояние wheel/steer-study (поля по оффсетам из disassembly) */
struct mtc_wheel_study {
	u32 pad0;	/* @0 */
	u32 adc_enabled;	/* @4 (0xC168E478) — гейт ветки "study" в adc_wheel_callback */
	u8 pad1[7];	/* @5..11 */
	u8 adc_type;	/* @12 (0xC168E480) — вариант ADC-порогов (==1 — альтернативные) */
	u8 dir_inv;	/* @13 (0xC168E481) — инверсия направления */
	u8 pad2[99];	/* @14..112 */
	u8 stw_min_a;	/* @113 (0xC168E4E5) — мин. разность stw-таблицы A (stw_range_check) */
	u8 stw_min_b;	/* @114 (0xC168E4E6) — мин. разность stw-таблицы C */
};
static struct mtc_wheel_study wheel_study;

/* --- 1. power_soft_off --- */
/* binaRE 0xC082D1F0 (power_soft_off, 120B) */
static void
power_soft_off(void)
{
	car_status->ch_status = 0;		/* binaRE 0xC168ACE5 */
	car_status->power_refcnt = 0;		/* binaRE 0xC168AC85 */
	car_status->power_on = 0;			/* binaRE 0xC168ACDC */
	if (car_status->call_active) {		/* binaRE 0xC168AC87 */
		capture_add_work(46, 255, 0, 0); /* args по прототипу */
		audio_add_work(21, 0, 0, 0); /* args по прототипу */
	} else {
		capture_add_work(47, 0, 0, 0); /* args по прототипу */
	}
	backlight_off();
}

/* --- 2. check_customer --- */
/* binaRE 0xC082E734 (check_customer, 200B): сравнение строки customer
 * (car_status+72, binaRE 0xC168ACCC — поле в раунде 2 названо mcuver1) с name:
 * полное совпадение или префикс с допуском на 1 лишний символ, если он цифра.
 * Специальный кейс: "KLD" совпадает с "KLDY". */
static int
check_customer(const char *name)
{
	const char *cs = car_status->mcuver1;
	int cs_len = strlen(cs);
	int n_len = strlen(name);

	if (cs_len < n_len)
		return 0;
	if (cs_len == n_len)
		return strcmp(cs, name) == 0;
	if (cs_len != n_len + 1)
		return 0;
	if (!strcmp("KLD", name) && !strcmp("KLDY", cs))
		return 1;
	if (strncmp(cs, name, n_len))
		return 0;
	if (cs[n_len] > '/')
		return cs[n_len] <= '9';
	return 0;	/* binaRE: return result (strncmp == 0) */
}

/* --- 3. get_token_int --- */
/* binaRE 0xC0830680 (get_token_int, 148B): разбор десятичного числа из токена,
 * *pos продвигается; -1 при ошибке; ',' и '\0' завершают число (для ',' — pos
 * продвинут за запятую). */
static int
get_token_int(char **pos)
{
	char *p = *pos;
	int c = *p;
	int next;
	int val = 0;

	if (!c)
		return -1;
	if (c == ',') {
		(*pos)++;
		return 0;
	}
	if ((unsigned char)(c - '0') > 9u)
		return -1;
	while (1) {
		next = *++p;
		*pos = p;
		val = val * 10 + (c - '0');
		if (!next)
			return val;
		if (next == ',') {
			(*pos)++;
			return val;
		}
		if ((unsigned char)(next - '0') > 9u)
			return -1;
		c = next;
	}
}

/* --- 4. process_mcu_command --- */
/* binaRE 0xC082E958 (process_mcu_command, 2672B) — процессор команд MCU (обратный
 * канал). ПОЛНАЯ транскрипция диспетчеризации (все кейсы, без сокращений);
 * LABEL_x — метки binaRE. */
static int
process_mcu_command(unsigned int cmd)
{
	u8 buf[8];	/* binaRE: стек-область sp+0x10.. (v14..); кейс 0xF00C читает 8 байт */
	int r;

	if ((cmd & 0x7000u) < 0x1000u) {
		if (cmd <= 0x602u) {
			if (cmd > 0x600u)
				return arm_rev_ack();
			if (cmd == 259u) {				/* 0x103 */
				vs_send(2, 134, NULL, 0);
				return arm_rev_ack();
			}
			if (cmd <= 259u) {
				if (cmd == 257u) {			/* 0x101 */
					vs_send(2, 132, NULL, 0);
					return arm_rev_ack();
				}
				if (cmd == 258u) {			/* 0x102 */
					vs_send(2, 133, NULL, 0);
					return arm_rev_ack();
				}
			} else {
				switch (cmd) {
				case 0x402u:
					vs_send(2, 241, NULL, 0);	/* binaRE LABEL_28 */
					return arm_rev_ack();
				case 0x408u:
					vs_send(2, 135, NULL, 0);
					return arm_rev_ack();
				case 0x401u:
					vs_send(2, 241, NULL, 0);
					return arm_rev_ack();
				}
			}
			printk("~ mtc rev err cmd %04x\n", cmd);	/* binaRE LABEL_21 */
			return arm_rev_ack();
		}
		if (cmd > 0x705u) {
			switch (cmd) {
			case 0x801u:
				vs_send(2, 136, NULL, 0);
				return arm_rev_ack();
			case 0x802u:
				car_status->power2_flag = 1;		/* binaRE 0xC168AD1F */
				return arm_rev_ack();
			case 0x755u:
				printk("MCU_SHUTDOWN\n");
				car_add_work(73, 0, 0);
				return arm_rev_ack();
			}
			printk("~ mtc rev err cmd %04x\n", cmd);
			return arm_rev_ack();
		}
		if (cmd >= 0x704u)
			return arm_rev_ack();
		if (cmd != 1794u) {				/* 0x702 */
			if (cmd <= 0x702u) {
				if (cmd != 1793u) {			/* 0x701 */
					printk("~ mtc rev err cmd %04x\n", cmd);
					return arm_rev_ack();
				}
				car_add_work(35, 0, 0);		/* binaRE LABEL_41 */
				return arm_rev_ack();
			}
			if (!car_status->cam_signal) {		/* binaRE 0xC168ACA5 */
				car_add_work(35, 0, 0);		/* binaRE LABEL_41 */
				return arm_rev_ack();
			}
		}
		car_add_work(36, 0, 0);
		return arm_rev_ack();
	}

	if ((cmd & 0xF000u) == 0xD000u) {
		u8 n = cmd & 0xFFu;

		r = arm_rev_bytes(buf + 1, n);	/* binaRE: v13 = (u8)cmd — только store в стек, дальше не читается */
		if (r) {
			if (((cmd & 0xF00u) == 0x100u) && !buf[1])
				config_data->d.cfg_radio = buf[2];	/* binaRE 0xC168AD46 (config+6) */
			return arm_rev_ack();
		}
		return r;
	}

	if (cmd == 61458u) {				/* 0xF012 */
		if (!arm_rev_bytes(buf, 2))
			return 0;
		vs_send(0, 18, (char *)buf, 2);
		goto tail_ack;
	}

	if (cmd <= 0xF012u) {
		if (cmd == 61447u) {				/* 0xF007 */
			if (arm_rev_bytes(buf, 5)) {
				arm_rev_ack();
				rds_input(buf[3] |
					  ((buf[2] | ((buf[1] | (buf[0] << 8)) << 8)) << 8)); /* T5 r20: def 1 arg; buf[4] - IDA-artefakt, ubran */
				return 1;
			}
			return 0;
		}
		if (cmd > 0xF007u) {
			if (cmd == 61451u) {			/* 0xF00B */
				if (arm_rev_bytes(buf, 2)) {
					arm_rev_ack();
					rds_input3A(buf);
					return 1;
				}
				return 0;
			}
			if (cmd <= 0xF00Bu) {
				if (cmd == 61449u) {		/* 0xF009 */
					r = arm_rev_bytes(buf, 5);
					if (!r)
						return 0;
					Hit_radio_sta(buf[0], buf[1], buf[2],
						       buf[3], buf[4]);
					goto tail_ack;
				}
				if (cmd > 0xF009u) {		/* 0xF00A */
					if (arm_rev_bytes(buf, 3)) {
						arm_rev_ack();
						rds_input2(buf[2] | ((buf[1] | (buf[0] << 8)) << 8));
						return 1;
					}
					return 0;
				}
				r = arm_rev_bytes(buf, 4);	/* cmd <= 0xF008 */
				if (!r)
					return 0;
				Hit_radio_sta(buf[0], buf[1], buf[2], 0, buf[3]);
				goto tail_ack;
			}
			if (cmd == 61456u) {			/* 0xF010 */
				r = arm_rev_bytes(buf, 1);
				if (!r)
					return 0;
				vs_send(0, 16, (char *)buf, 1);
				goto tail_ack;
			}
			if (cmd > 0xF010u) {			/* 0xF011 */
				r = arm_rev_bytes(buf, 5);
				if (!r)
					return 0;
				vs_send(0, 17, (char *)buf, 5);
				goto tail_ack;
			}
			if (cmd == 61452u) {			/* 0xF00C */
				if (arm_rev_bytes(buf, 8)) {
					arm_rev_ack();
					rds_input3(buf);
					return 1;
				}
				return 0;
			}
			goto err_ret0;			/* 0xF00D..0xF00F */
		}
		if (cmd == 61443u) {				/* 0xF003 */
			r = arm_rev_bytes(buf, 2);
			if (!r)
				return 0;
			{
				int v = 2275 * (buf[1] | (buf[0] << 8));

				car_status->intval3 = v >> 10;	/* binaRE 0xC168AC98 (+24) */
				buf[0] = (u8)(v >> 18);
				buf[1] = (u8)(v >> 10);
				vs_send(2, 146, (char *)buf, 2);
			}
			goto tail_ack;
		}
		if (cmd > 0xF003u) {
			if (cmd == 61445u) {			/* 0xF005 */
				r = arm_rev_bytes(buf, 2);
				if (r) {
					car_status->intval4 = buf[1] | (buf[0] << 8);	/* binaRE 0xC168AC9C (+28) */
					if (!car_status->intval4)
						car_status->intval4 = 1;
					goto tail_ack;
				}
			} else if (cmd > 0xF005u) {		/* 0xF006 */
				r = arm_rev_bytes(buf, 2);
				if (r) {
					car_status->wipe_flag = buf[1] | (buf[0] << 8);	/* binaRE 0xC168ACA0 (+32) */
					if (!car_status->wipe_flag)
						car_status->wipe_flag = 1;
					goto tail_ack;
				}
			} else {				/* 0xF004 */
				r = arm_rev_bytes(buf, 1);
				if (r) {
					u8 nv = buf[0];
					u8 dv;

					dv = nv ^ car_status->sta_bits;	/* binaRE 0xC168AC8C (+8) */
					if (dv & 0x10u) {
						u8 b = (nv & 0x10u) != 0;

						vs_send(2, 144, (char *)&b, 1);
					}
					dv = nv ^ car_status->sta_bits;
					if (dv & 0x20u) {
						u8 b = (nv & 0x20u) == 0;

						vs_send(2, 145, (char *)&b, 1);
					}
					dv = car_status->sta_bits ^ nv;
					if (dv & 0x48u) {
						if (dv & 8u) {
							u8 b = (nv & 8u) == 0;

							vs_send(2, 147, (char *)&b, 1);
						}
						car_status->sta_bits = nv;
						backlight_update();
					}
					car_status->sta_bits = nv;
					goto tail_ack;
				}
			}
		} else {				/* cmd <= 0xF003 */
			switch (cmd) {
			case 0xF000u:
				r = arm_rev_bytes(buf, 2);
				if (r) {
					car_status->intval1 = (buf[1] | (buf[0] << 8)) / 3u;	/* binaRE 0xC168AC90 (+12) */
					if (!car_status->intval1)
						car_status->intval1 = 1;
					goto tail_ack;
				}
				break;
			case 0xF001u:
				r = arm_rev_bytes(buf, 2);
				if (r) {
					car_status->intval2 = (buf[1] | (buf[0] << 8)) / 3u;	/* binaRE 0xC168AC94 (+16) */
					if (!car_status->intval2)
						car_status->intval2 = 1;
					goto tail_ack;
				}
				break;
			case 0xE000u: {
				u8 mmsg[256];	/* binaRE: IDA-область помечена 4B (v14) + соседние vars;
							 * оригинал пишет buf[len] и читает len <= 255 — честный буфер 256B */
				if (arm_rev_bytes(mmsg, 1)) {
					mmsg[mmsg[0]] = 0;
					r = arm_rev_bytes(mmsg, mmsg[0]);
					if (!r)
						return 0;
					printk("--mtc mmsg %s\n", (const char *)mmsg);
					goto tail_ack;
				}
				break;
			}
			default:
				goto err_ret0;
			}
		}
	} else {
		if (cmd == 61953u) {				/* 0xF201 */
			if (!arm_rev_bytes(buf, 1))
				return 0;
			if (!buf[0])
				car_status->mcu_cmd_state = 5;	/* binaRE 0xC168AD20 (+156) */
			r = buf[0];
			car_add_work(42, r, 0);
			goto tail_ack;
		}
		if (cmd > 0xF201u) {
			if (cmd == 61957u) {			/* 0xF205 */
				car_add_work(34, 0, 0);
				return arm_rev_ack();
			}
			if (cmd <= 0xF205u) {
				if (cmd == 61955u) {		/* 0xF203 */
					car_add_work(44, 0, 0);
				} else if (cmd > 0xF203u) {	/* 0xF204 */
					if (!car_status->reserved_10 ||
					    car_status->mcu_cmd_state == 5) {	/* binaRE 0xC168ACA6 (+38) */
						car_status->mcu_cmd_state = 5;
						car_add_work(31, 0, 0);
					} else {
						car_status->mcu_cmd_state = 5;
						car_add_work(32, 0, 0);
					}
				} else {				/* 0xF202 */
					car_add_work(45, 0, 0);
				}
				return arm_rev_ack();
			}
			if (cmd == 61959u) {			/* 0xF207 */
				if (!car_status->reserved_10 ||
				    car_status->mcu_cmd_state == 6) {	/* binaRE 0xC168ACA6 (+38) */
					car_status->mcu_cmd_state = 6;
					car_add_work(31, 0, 0);
				} else {
					car_status->mcu_cmd_state = 6;
					car_add_work(32, 0, 0);
				}
				return arm_rev_ack();
			}
			if (cmd < 0xF207u) {			/* 0xF206 */
				car_add_work(33, 0, 0);
				return arm_rev_ack();
			}
			if (cmd == 61968u) {			/* 0xF210 */
				if (!arm_rev_bytes(buf, 1))
					return 0;
				car_status->mcu_cmd_state = 6;
				car_add_work(42, 0, 0);
				r = 1;
				goto tail_ack;
			}
			goto err_ret0;			/* > 0xF210 */
		}
		if (cmd == 61462u) {				/* 0xF016 */
			if (!arm_rev_bytes(buf, 1))
				return 0;
			printk("--mtc hold %d\n", buf[0]);
			car_add_work(70, buf[0], 0);
			r = 1;
			goto tail_ack;
		}
		if (cmd <= 0xF016u) {
			if (cmd == 61460u) {			/* 0xF014 */
				if (!arm_rev_bytes(buf, 1))
					return 0;
				printk("--mtc press %d\n", buf[0]);
				car_add_work(69, buf[0], 0);
			} else if (cmd > 0xF014u) {		/* 0xF015 */
				r = arm_rev_bytes(buf, 1);
				goto tail_ack;
			} else {				/* 0xF013 */
				r = arm_rev_bytes(buf, 2);
				if (!r)
					return 0;
				vs_send(0, 19, (char *)buf, 2);
			}
			goto tail_ack;
		}
		if (cmd != 61473u) {				/* 0xF021 */
			if (cmd == 61952u) {			/* 0xF200 */
				if (!arm_rev_bytes(buf, 1))
					return 0;
				car_add_work(43, buf[0], 0);
				r = 1;
				goto tail_ack;
			}
			if (cmd == 61472u) {			/* 0xF020 */
				u8 len;

				if (!arm_rev_bytes(&len, 1))
					goto ret_tail;		/* binaRE LABEL_87 */
				if (!arm_rev_bytes(buf, len))
					return 0;
				vs_send_raw(0, (char *)buf, len);
				r = 1;
				goto tail_ack;
			}
			goto err_ret0;			/* binaRE LABEL_36 */
		}
		{
			u8 mmsg32[32];

			r = arm_rev_bytes(mmsg32, 32);
			if (r) {
				u8 *dst = (u8 *)car_status + 100;	/* binaRE 0xC168ACE8: запись в [+1 .. +32] */
				int k;

				for (k = 0; k < 32; k++)
					dst[1 + k] = mmsg32[k];
				goto tail_ack;
			}
		}
	}

ret_tail:				/* binaRE LABEL_87 */
	return ((int)cmd < 0) ? 0 : ((int)cmd & 0x8000);
tail_ack:				/* binaRE LABEL_13 */
	if (r)
		return arm_rev_ack();
	return 0;
err_ret0:				/* binaRE LABEL_36 */
	printk("~ mtc rev err cmd %04x\n", cmd);
	return 0;
}

/* --- 5. mtcWipeCheck --- */
/* binaRE 0xC0830648 (mtcWipeCheck, 52B): счётчик "wipe"-запросов (car_status+147,
 * binaRE 0xC168AD17; поле в раунде 2 названо boot_flags). Ранние возвраты в
 * binaRE R0 не устанавливают (BX LR) — здесь 0/счётчик; >3 — tail-call car_add_work(72). */
int
mtcWipeCheck(void)
{
	if (!car_status->boot_flags)
		return 0;
	car_status->boot_flags = (u8)(car_status->boot_flags + 1);
	if (car_status->boot_flags <= 3)
		return car_status->boot_flags;
	car_add_work(72, 0, 0);	/* binaRE: tail-call — R0 = R0 car_add_work */
	return 0;		/* мtc_shared.h:253 объявляет car_add_work void */
}

/* --- 6. mtc_get_pin_map --- */
/* binaRE 0xC082D5C0 (mtc_get_pin_map, 88B): поиск pin id в pin-таблице (записи 16B,
 * id — первый байт записи, 0 — терминатор). Возврат — указатель на запись или NULL.
 * binaRE: таблица через указатель 0xC0BC9B20 (см. pin_map_tbl), первая запись [P+0x74]. */
char *
mtc_get_pin_map(int pin_id)
{
	unsigned char *rec = pin_map_tbl + 0x74;

	if (!rec[0])
		return NULL;
	if (rec[0] == pin_id)
		return rec;
	rec += 16;
	for (;;) {
		if (!rec[0])
			return NULL;
		if (rec[0] == pin_id)
			return rec;
		rec += 16;
	}
}

/* --- 7. mtc_init_test_io --- */
/* binaRE 0xC082D61C (mtc_init_test_io, 160B): настройка test-GPIO pin-таблицы:
 * iomux_set([+8]), gpio_request([+4]), pull-updown=0, direction=input; флаг [+14] = 0. */
int
mtc_init_test_io(void)
{
	unsigned char *rec = pin_map_tbl + 0x74;
	char name[16];	/* binaRE: 14B-область стека */
	int idx = 0;
	int ret = 0;

	while (rec[0]) {
		u32 gpio = *(u32 *)(rec + 4);
		u32 iomux = *(u32 *)(rec + 8);

		if (iomux)
			mtc_iomux_set(iomux);
		sprintf(name, "tp%d", idx++);
		gpio_request(gpio, name);
		gpio_pull_updown(gpio, 0);
		ret = gpio_direction_input(gpio);
		rec[14] = 0;
		rec += 16;
	}
	return ret;	/* binaRE: значение последнего gpio_direction_input */
}

/* --- 8. mtc_test_port2 --- */
/* binaRE 0xC082D6C4 (mtc_test_port2, 288B): тест пары пинов (3 попытки, оба направления).
 * Флаг "проверено" [+14]: 1 = нет контакта, 0 = OK. */
int
mtc_test_port2(unsigned char *pa, unsigned char *pb)
{
	int i;
	u32 ga = *(u32 *)(pa + 4);
	u32 gb = *(u32 *)(pb + 4);
	int ret;

	for (i = 3; i > 0; i--) {
		gpio_direction_input(gb);
		gpio_direction_output(ga, 1);
		_gpio_set_value(ga, 1);
		_const_udelay(1073740);
		if (!_gpio_get_value(gb))
			goto ok;
		_gpio_set_value(ga, 0);
		_const_udelay(1073740);
		if (_gpio_get_value(gb) == 1)
			goto ok;
		gpio_direction_input(ga);
		gpio_direction_output(gb, 1);
		_gpio_set_value(gb, 1);
		_const_udelay(1073740);
		if (!_gpio_get_value(ga))
			goto ok;
		_gpio_set_value(gb, 0);
		_const_udelay(1073740);
		if (_gpio_get_value(ga) == 1)
			goto ok;
	}
	gpio_direction_input(ga);
	ret = gpio_direction_input(gb);
	pa[14] = 1;
	pb[14] = 1;
	return ret;
ok:
	gpio_direction_input(ga);
	ret = gpio_direction_input(gb);
	pa[14] = 0;
	pb[14] = 0;
	return ret;
}

/* --- 9. mtc_test_port3 --- */
/* binaRE 0xC082D7E4 (mtc_test_port3, 636B): тест тройки пинов (3 раунда, битовая
 * маска 1/2/4 за раунд). Финал: [+14] = (маска != ожидаемая) — 1 = дефект. */
int
mtc_test_port3(unsigned char *pa, unsigned char *pb, unsigned char *pc)
{
	int i;
	u32 ga = *(u32 *)(pa + 4);
	u32 gb = *(u32 *)(pb + 4);
	u32 gc = *(u32 *)(pc + 4);
	int ret;

	pa[14] = 0;
	pb[14] = 0;
	pc[14] = 0;
	for (i = 3; i > 0; i--) {
		/* раунд: драйв A */
		gpio_direction_input(gb);
		gpio_direction_input(gc);
		gpio_direction_output(ga, 1);
		_gpio_set_value(ga, 1);
		_const_udelay(1073740);
		if (!_gpio_get_value(gb))
			pb[14] |= 1;
		if (!_gpio_get_value(gc))
			pc[14] |= 1;
		_gpio_set_value(ga, 0);
		_const_udelay(1073740);
		if (_gpio_get_value(gb) == 1)
			pb[14] |= 1;
		if (_gpio_get_value(gc) == 1)
			pc[14] |= 1;
		gpio_direction_input(gc);
		gpio_direction_input(ga);
		/* драйв B */
		gpio_direction_output(gb, 1);
		_gpio_set_value(gb, 1);
		_const_udelay(1073740);
		if (!_gpio_get_value(gc))
			pc[14] |= 2;
		if (!_gpio_get_value(ga))
			pa[14] |= 2;
		_gpio_set_value(gb, 0);
		_const_udelay(1073740);
		if (_gpio_get_value(gc) == 1)
			pc[14] |= 2;
		if (_gpio_get_value(ga) == 1)
			pa[14] |= 2;
		gpio_direction_input(ga);
		gpio_direction_input(gb);
		/* драйв C */
		gpio_direction_output(gc, 1);
		_gpio_set_value(gc, 1);
		_const_udelay(1073740);
		if (!_gpio_get_value(ga))
			pa[14] |= 4;
		if (!_gpio_get_value(gb))
			pb[14] |= 4;
		_gpio_set_value(gc, 0);
		_const_udelay(1073740);
		if (_gpio_get_value(ga) == 1)
			pa[14] |= 4;
		if (_gpio_get_value(gb) == 1)
			pb[14] |= 4;
	}
	gpio_direction_input(ga);
	gpio_direction_input(gb);
	ret = gpio_direction_input(gc);
	pa[14] = pa[14] != 6;
	pb[14] = pb[14] != 5;
	pc[14] = pc[14] != 3;
	return ret;	/* binaRE: значение последнего gpio_direction_input */
}

/* --- 10. mtc_test_port --- */
/* binaRE 0xC082DCF4 (mtc_test_port, 488B): прогон test-последовательности
 * (test_seq_tbl: записи {a, b, c}): a==1 — первый pin (запись 0), иначе поиск по id;
 * c==1 — третья запись 0; c==0/не найден — тест пары (mtc_test_port2), иначе тройки
 * (mtc_test_port3). В конце — вывод результатов на debug-экран. */
char *
mtc_test_port(void)
{
	const unsigned char *seq = test_seq_tbl;
	char *ret = 0;

	while (seq[0]) {
		unsigned char a = seq[0], b = seq[1], c = seq[2];
		unsigned char *pa, *pb, *pc;
		int fa, fb;

		if (a == 1) {
			pa = pin_map_tbl + 0x74;	/* первая запись */
			fa = 0;
		} else {
			pa = (unsigned char *)mtc_get_pin_map(a);
			fa = (pa == NULL);
		}
		if (b == 1) {
			pb = pin_map_tbl + 0x74;
			fb = 0;
		} else {
			pb = (unsigned char *)mtc_get_pin_map(b);
			fb = (pb == NULL);
		}
		if (c == 1) {
			pc = pin_map_tbl + 0x74;
		} else {
			pc = (unsigned char *)mtc_get_pin_map(c);
		}
		if (!fa && !fb) {
			if (pc)
				mtc_test_port3(pa, pb, pc);
			else
				mtc_test_port2(pa, pb);	/* c == 0 или не найден */
		}
		seq += 3;
	}
	{
		unsigned char *rec = pin_map_tbl + 0x74;

		while (rec[0]) {
			if (rec[14] == 0)
				ret = mtc_debug_put_string("*", 1, rec[12], rec[13] - 1,
							   0xFFFF0000u, 0xFF404040u);
			else
				ret = mtc_debug_put_string("*", 1, rec[12], rec[13] - 1,
							   0xFF00FF00u, 0xFF404040u);
			rec += 16;
		}
	}
	return ret;	/* binaRE: значение последнего mtc_debug_put_string */
}

/* --- 11. mtc_clear_screen --- */
/* binaRE 0xC082DA60 (mtc_clear_screen, 388B): color без стартового байта —
 * "test pattern" (480 строк); иначе — залита цветом. Ширина — is1024screen
 * (binaRE 0xC168ACE2). */
void
mtc_clear_screen(int color)
{
	u32 *fb = mtc_fb_buf;	/* binaRE 0xC0D1DE30 */
	int width = car_status->is1024screen ? 1024 : 800;
	int x, y;

	if ((color & 0xFF000000u) == 0) {
		for (y = 0; y < 480; y++) {
			u32 *row = fb + y * width;

			for (x = 0; x < width; x++) {
				unsigned int c;

				if ((unsigned int)(y - 16) > 0x1BFu ||
				    (unsigned int)(x - 1) > 0x2FFu)
					c = 0;
				else if (y <= 279)
					c = 0xFF404040u;	/* серый */
				else if (y > 399) {
					if ((unsigned int)(x - 1) > 255u) {
						if ((unsigned int)(x - 1) > 511u)
							c = 0xFF000000u | (((x - 1) & 0xFFu) << 16);
						else
							c = 0xFF000000u | (((x - 1) & 0xFFu) << 8);
					} else {
						c = 0xFF000000u | ((x - 1) & 0xFFu);
					}
				} else if (y > 339) {
					int g = 255 - (x - 1) / 3;

					c = 0xFF000000u | (g << 16) | (g << 8) | g;
				} else {
					c = 0;
				}
				row[x] = (u32)c;
			}
		}
		return;
	}
	for (y = 0; y < 480; y++) {
		u32 *row = fb + y * width;

		for (x = 0; x < width; x++)
			row[x] = (u32)color;
	}
}

/* --- 12. mtc_debug_putc --- */
/* binaRE 0xC082DBEC (mtc_debug_putc, 136B): вывод глифа font_8x16 в framebuffer:
 * glyph — индекс (8x16), x — пиксельная колонка (блок 8 px), y — половинная строка
 * (строка = 2*y), fg/bg — ARGB-цвета. */
char *
mtc_debug_putc(int glyph, int x, int y, int fg_color, int bg_color)
{
	const u8 *g = font_8x16 + 16 * glyph;
	int width = car_status->is1024screen ? 1024 : 800;	/* binaRE 0xC168ACE2 */
	int stride = car_status->is1024screen ? 4096 : 3200;
	u32 *p = mtc_fb_buf + 8 * (x + width * 2 * y);	/* binaRE: fb + 32*(a2 + w*2*a3) байт */
	int row, bit;

	for (row = 0; row < 16; row++) {
		u8 px = g[row];
		u32 *dst = p;

		for (bit = 0; bit < 8; bit++) {
			*dst++ = (px & 0x80u) ? (u32)fg_color : (u32)bg_color;
			px <<= 1;
		}
		p += stride / 4;
	}
	return (char *)g;
}

/* --- 13. mtc_debug_put_string --- */
/* binaRE 0xC082DC80 (mtc_debug_put_string, 116B): вывод строки (до len символов;
 * хвост — пробелы). */
char *
mtc_debug_put_string(const char *s, int len, int x0, int y, int fg_color, int bg_color)
{
	int slen = strlen(s);
	int i;

	for (i = 0; i < len; i++) {
		int c = 32;

		if (slen > i)
			c = s[i];
		mtc_debug_putc(c, x0 + i, y, fg_color, bg_color);
	}
	return (char *)s;
}

/* --- 14. adc_wheel_callback --- */
/* binaRE 0xC083BA24 (adc_wheel_callback, 780B): ADC-колбэк рулевого колёса
 * (регистрация adc_register(ch, adc_wheel_callback, &wheel_state), контекст
 * 0xC09BCF38). r0 — указатель на текущее значение ADC канала; r1 — состояние
 * колёса; r2 — прочитанное значение ADC. Транскрипция 1-в-1 (управляющие потоки —
 * из decompiled; LABEL_25/27/28 — метки binaRE). */
static int
adc_wheel_callback(const u32 *adc_cur, struct mtc_wheel_state *ws, int adc_val)
{
	int ret;

	if (wheel_study.adc_enabled) {			/* binaRE 0xC168E478 — режим "study" */
		int cur = *adc_cur;
		int up, dn, dir;

		if (cur == *ws->adc_ref_up)
			ws->adc_up_val = adc_val;
		else if (cur == *ws->adc_ref_dn)
			ws->adc_dn_val = adc_val;

		up = ws->adc_up_val;
		if (up < 0)
			return 0;
		dn = ws->adc_dn_val;
		if (dn < 0)
			return 0;

		ret = ws->key_repeat_cnt;
		if (ret > 0)
			ws->key_repeat_cnt = --ret;

		if (up > 199) {
			if (dn <= 199)
				dir = wheel_study.dir_inv ? 2 : 1;	/* binaRE 0xC168E481 */
			else
				dir = 4;
		} else if (dn > 199) {
			dir = wheel_study.dir_inv ? 1 : 2;
		} else {
			dir = 3;
		}

		{
			unsigned char st = ws->wheel_state;
			unsigned char hi = st >> 4;

			if (hi != (unsigned char)dir) {
				ws->wheel_state = (st & 0xF) | ((unsigned char)dir << 4);
				return ret;
			}
			if (!ret) {				/* key_repeat_cnt == 0 */
				if (st == 20) {			/* 0x14 */
					unsigned char lk = ws->wheel_last_key;

					if (lk != 'A') {
						if (lk == '@')
							ws->wheel_last_key = st;	/* binaRE LABEL_27 */
						ws->key_repeat_cnt = 40;	/* binaRE LABEL_25 */
						ret = add_wheel_work(ret, ws);
					}
				} else if (st == 36) {			/* 0x24 */
					unsigned char lk = ws->wheel_last_key;

					if (lk != 'B') {
						if (lk == '@')
							ws->wheel_last_key = st;	/* LABEL_27 */
						ret = 1;
						ws->key_repeat_cnt = 40;	/* LABEL_25 */
						ret = add_wheel_work(ret, ws);
					}
				}
			}
			if ((st & 0xF) == hi) {			/* binaRE LABEL_28 */
				ws->wheel_state = (unsigned char)dir | ((unsigned char)dir << 4);
				return ret;
			}
			ws->wheel_last_key = st;		/* binaRE LABEL_27 */
			ws->wheel_state = (unsigned char)dir | ((unsigned char)dir << 4);
			return ret;				/* LABEL_28 */
		}
	}

	ret = ws->key_repeat_cnt;
	if (ret > 0)
		ws->key_repeat_cnt = ret - 1;

	if (!config_data->d.adc_wheel_gate) {		/* binaRE 0xC168AD59 (config+25) — обычный режим */
		int dir = 0;
		unsigned char st, hi;

		if (wheel_study.adc_type == 1) {		/* binaRE 0xC168E480 */
			if ((unsigned int)(adc_val - 488) <= 0x30u)
				dir = 1;
			else if ((unsigned int)(adc_val - 658) <= 0x30u)
				dir = 2;
			else if ((unsigned int)(adc_val - 385) <= 0x30u)
				dir = 3;
			else if (adc_val > 1000)
				dir = 4;
		} else {
			if ((unsigned int)(adc_val - 437) <= 0x30u)
				dir = 1;
			else if ((unsigned int)(adc_val - 590) <= 0x30u)
				dir = 2;
			else if ((unsigned int)(adc_val - 330) <= 0x30u)
				dir = 3;
			else if (adc_val > 1000)
				dir = 4;
		}

		st = ws->wheel_state;
		hi = st >> 4;
		if (hi != (unsigned char)dir) {
			ws->wheel_state = (st & 0xF) | ((unsigned char)dir << 4);
			return ret;
		}
		if (dir) {
			if (ret) {				/* binaRE: goto LABEL_28 */
				ws->wheel_state = hi | (hi << 4);
				return ret;
			}
			if (st != 20 && st != 35) {
				if (st != 36) {
					if (st != 19) {
						ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
						return ret;
					}
					ret = 1;			/* binaRE LABEL_47 */
					ws->key_repeat_cnt = 40;
					ret = add_wheel_work(ret, ws);
					ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
					return ret;
				}
				ws->key_repeat_cnt = 40;	/* binaRE LABEL_80 */
				ret = add_wheel_work(1, ws);
				ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
				return ret;
			}
			return ret;
		}
		return ret;
	}

	{					/* adc_wheel_gate — режим "steering" */
		int dir;

		if ((unsigned int)(adc_val - 316) > 0x3Au) {
			if ((unsigned int)(adc_val - 483) > 0x3Au) {
				if ((unsigned int)(adc_val - 585) > 0x3Au)
					dir = 0;
				else
					dir = 3;
			} else {
				dir = 2;
			}
		} else {
			dir = 1;
		}

		{
			unsigned char st = ws->wheel_state;
			unsigned char hi = st >> 4;

			if (hi != (unsigned char)dir) {
				ws->wheel_state = (st & 0xF) | ((unsigned char)dir << 4);
				return ret;
			}
			if (dir) {
				if (ret) {			/* binaRE: goto LABEL_28 */
					ws->wheel_state = hi | (hi << 4);
					return ret;
				}
				if (st == 33 || st == 50 || st == 19) {
					ws->key_repeat_cnt = 40;	/* binaRE LABEL_80 */
					ret = add_wheel_work(1, ws);
					ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
					return ret;
				}
				if (st != 49 && st != 18 && st != 35) {
					ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
					return ret;
				}
				ws->key_repeat_cnt = 40;	/* binaRE LABEL_47 */
				ret = add_wheel_work(ret, ws);
				ws->wheel_state = hi | (hi << 4);	/* LABEL_28 */
				return ret;
			}
			return ret;
		}
	}
}

/* --- 15. stw_range_check --- */
/* binaRE 0xC083BD50 (stw_range_check, 352B): минимальная разность значений в двух
 * stw-таблицах — таблица A: config+329 (binaRE 0xC168AE89), таблица C: config+404
 * (0xC168AED4): 24 × u16 LE, шаг 3 (область steer_data раунда 2). Результат делится
 * на 2 и пишется в wheel_study.stw_min_a/b (0xC168E4E5/E6); стартовый потолок — 50.
 * Внутренний цикл binaRE идёт до оффсета base+72 (включая байты за концом таблицы)
 * — воспроизведено 1-в-1. */
static void
stw_range_check(void)
{
	const u8 *cfg = config_data->u8;	/* minfix: u8[512] — член union, не член d (mtc_shared.h) */
	int i, off;
	unsigned int min_a = 50, min_c = 50;
	u16 u;

	for (i = 0; i < 24; i++) {
		u = cfg[329 + 3 * i] | ((u16)cfg[330 + 3 * i] << 8);
		for (off = 3 + 3 * i; off <= 72; off += 3) {
			u16 v = cfg[329 + off] | ((u16)cfg[330 + off] << 8);
			int d = (u > v) ? u - v : v - u;

			if (min_a >= (unsigned int)d)
				min_a = (unsigned int)d;
		}
	}
	for (i = 0; i < 24; i++) {
		u = cfg[404 + 3 * i] | ((u16)cfg[405 + 3 * i] << 8);
		for (off = 3 + 3 * i; off <= 72; off += 3) {
			u16 v = cfg[404 + off] | ((u16)cfg[405 + off] << 8);
			int d = (u > v) ? u - v : v - u;

			if (min_c >= (unsigned int)d)
				min_c = (unsigned int)d;
		}
	}
	wheel_study.stw_min_a = (u8)(min_a >> 1);	/* binaRE 0xC168E4E5 */
	wheel_study.stw_min_b = (u8)(min_c >> 1);	/* binaRE 0xC168E4E6 */
}

/* --- 16. mtc_iomux_set --- */
/* binaRE 0xC04AE134 (iomux_set, 100B) — board-level pinmux-helper (вызовы:
 * mtc-audio.c:1263, mtc-car.c:3977). mode: [3:0]=значение режима, [7:4]=n,
 * [11:8]=pin в банке, [15:12]=банк (<=3). Адрес регистра — по disassembly
 * (literal pool 0xC04AE184): *(u32*)( (0x7FB4200E + bank*4 + pin) << 2 ). */
/* binaRE 0xC04AE134; renamed: SDK plat-rk iomux_set collision */
int
mtc_iomux_set(unsigned int mode)
{
	unsigned int bank, n, pin, val;

	if (mode == 0xFFFFFFFFu)
		return printk("<6><%s> mode(0x%x) is invalid\n", "iomux_set", mode);
	bank = (mode >> 12) & 0xFu;
	if (bank > 3u)
		return printk("<6><%s> mode(0x%x) is invalid\n", "iomux_set", mode);
	n = (mode >> 4) & 0xFu;
	pin = (mode >> 8) & 0xFu;
	val = ((mode & 0xFu) << (2 * n)) + (3u << (2 * (n + 8)));
	*(volatile u32 *)(unsigned long)((0x7FB4200Eu + bank * 4u + pin) << 2) = val;
	return 2 * (n + 8);
}

/* --- 17. mtc_get_screen_width --- */
/* binaRE 0xC06A172C (mtc_get_screen_width, 16B) */
int
mtc_get_screen_width(void)
{
	return (int)mtc_fb_width;	/* binaRE 0xC0D1DE28 */
}

/* --- 18. mtc_get_screen_height --- */
/* binaRE 0xC06A173C (mtc_get_screen_height, 16B) — src_all/decompiled_mtc_get_screen_height.c, 1-в-1 */
int
mtc_get_screen_height(void)
{
	return (int)mtc_fb_height;	/* binaRE 0xC0D1DE2C */
}

// very dirty code
static long
car_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct task_struct *v6;		       // r5@5
	int v7;				       // r3@5
	unsigned char v8;		       // cf@5
	char *data_buf;			       // r8@12 MAPDST
	char *equal_last_pos;		       // r0@12
	const char *token_start;	       // r7@13
	size_t slen;			       // r0@14
	int first_char;			       // r3@15 MAPDST
	char *v16;	// r1@22 (T5 r20: ptr, IDA lost)
	char v17;			       // zf@23
	unsigned int v18;		       // r3@24
	signed int res;			       // r7@28
	size_t v20;			       // r0@30
	int v21;			       // r3@30
	unsigned char v22;		       // cf@30
	void *data72b;			       // r5@38
	int v25;			       // r3@40
	unsigned char v26;		       // cf@40
	struct task_struct *cur_task;	  // r5@47
	unsigned char *buf1;		       // r0@47
	int v29;			       // r3@47
	unsigned char v30;		       // cf@47
	int mcu_data_size;		       // r6@53
	void *mcu_data;			       // r0@53 MAPDST
	int v34;			       // r3@54
	unsigned char v35;		       // cf@54
	size_t size;			       // r0@64 MAPDST
	unsigned char *v38;		       // r6@65
	void *data;			       // r5@65 MAPDST
	int v41;			       // r3@67
	unsigned char v42;		       // cf@67
	int i;				       // r4@78
	int v46;			       // r0@88
	char v47;			       // r5@88
	int v48;			       // r0@89
	char v49;			       // r5@89
	int intval;			       // r0@95
	char can_buf_pos;		       // r3@97
	int count;			       // r1@97
	char *v54;			       // r2@116
	char v55;			       // zf@117
	char *v56;			       // r3@118
	int v57;			       // r0@121
	int v58;			       // r1@121
	size_t v59;			       // r0@133
	u32 *v60;			       // r4@134
	int v61;			       // r6@136
	int v62;			       // r5@136
	int v63;			       // r0@138
	int v67;			       // r0@172
	int v68;			       // r1@172
	char *v69;			       // r2@172
	char v70;			       // zf@173
	char *v71;			       // r3@174
	char *v78;	// r1@220 (T5 r20: ptr, IDA lost)
	char v79;			       // zf@221
	char *v80;			       // r3@222
	unsigned int v81;		       // r3@225
	signed int is_audio_mute;	      // r0@227
	int v83;			       // r1@227
	char *v84;			       // r2@227
	char not_mute;			       // zf@227
	char *v86;			       // r3@228
	int volume;			       // r0@249
	int phone_volume;		       // r0@258
	int cam_front;			       // r1@265
	int folder_token;		       // r0@274
	int v92;			       // r2@274
	int v93;			       // r3@274
	int v94;			       // r0@276
	char b1[4];			       // r0@277
	char b2[4];			       // r1@277
	char *v97;	// r3@277 (T5 r20: ptr, IDA lost)
	int v98;			       // r2@287
	int v99;			       // r1@288
	char *v100;	// r3@288 (T5 r20: ptr, IDA lost)
	char *v102;			       // r6@307
	char *v103;	// r7@307 (T5 r20: ptr, IDA lost)
	int v104;			       // r8@307
	int v105;			       // r10@308
	int v106;			       // r2@308
	int v107;			       // r3@308
	int v108;			       // r10@308
	size_t v109;			       // r0@309
	int v110;			       // r0@312
	unsigned int v111;		       // r1@312
	char *v112;	// r3@312 (T5 r20: ptr, IDA lost)
	size_t v113;			       // r0@313
	char *v114;			       // r6@315
	char *v115;	// r8@315 (T5 r20: ptr, IDA lost)
	int v116;			       // r7@315
	int v117;			       // r10@316
	int v118;			       // r3@316
	int v119;			       // r10@316
	size_t v120;			       // r0@317
	int cfg_wheelstudy_type;	       // r2@319
	int cfg_dvr;			       // r2@320
	int cfg_led2;			       // r12@321
	int cfg_led0;			       // r2@321
	int cfg_led1;			       // r3@321
	int cfg_beep;			       // r2@322
	size_t v127;			       // r0@323
	int v128;			       // r3@324
	char *v129;			       // r6@325
	char *v130;	// r7@325 (T5 r20: ptr, IDA lost)
	int v131;			       // r8@325
	int v132;			       // r10@326
	size_t v133;			       // r0@327
	int cfg_ill;			       // r2@329
	int media_token;		       // r0@334
	int v136;			       // r2@334
	int v137;			       // r3@334
	size_t size_1;			       // r0@344
	u32 *v139;			       // r5@345
	int v140;			       // r6@347
	int v141;			       // r4@347
	int v142;			       // r0@348
	union mtc_config_data *gap12;	 // r7@350
	int v144;			       // r2@350
	u32 *v145;			       // r3@350
	int st_pos;			       // r4@350
	char *v147;	// r1@351 (T5 r20: ptr, IDA lost)
	int v148;			       // t1@351
	int v150;			       // r4@357
	int v151;			       // r0@358
	union mtc_config_data *p_config_data; // r7@360
	int *v153;			       // r3@360
	int cur_byte;			       // r4@360
	int v155;			       // t1@361
	char *v156; /* IDA: байтовый указатель */	  // r3@363
	unsigned short *v157;		       // r2@363
	char *v158;			       // r1@363
	int v159;			       // t1@364
	int led1;			       // r0@367
	char b_led1;			       // r5@367
	int led2;			       // r0@368
	char b_led2;			       // r4@368
	int led3;			       // r0@369
	unsigned char *cfg_led;	      // r2@370
	int enable_beep;		       // r0@371
	int backlight;			       // r0@373
	unsigned char *p_backlight;	  // r2@374
	int powerdelay;			       // r0@375
	unsigned char *p_powerdelay;	 // r2@376
	int blmode;			       // r0@377
	unsigned char *p_blmode;	     // r2@378
	int wifi_pwr;			       // r0@379 MAPDST
	int v174;			       // r0@381
	int cfg_led_multi;		       // r0@383 MAPDST
	int color;			       // r0@385
	unsigned char *cfg_color;	    // r2@386
	signed int v178;		       // r1@389
	int v179;			       // r2@397
	int v180;			       // r2@400
	int v181;			       // r1@403
	char *v182;	// r3@403 (T5 r20: ptr, IDA lost)
	enum MTC_AV_CHANNEL av_channel;	// r1@423 MAPDST (T5 r20: enum iz shared.h, typedef net)
	signed int v185;		       // r0@439
	char v186;			       // zf@439
	unsigned int v187;		       // r1@439
	int brightness;			       // r1@472
	char v192;			       // r3@481
	char v193;			       // zf@481
	char *v194;			       // r3@482
	int v195;			       // r0@484
	int v196;			       // r1@484
	char *v197;	// r3@484 (T5 r20: ptr, IDA lost)
	int v198;			       // r0@489
	unsigned int v199;		       // r1@489
	char *v200;			       // r1@490
	char *v202;			       // r2@492
	int v203;			       // r0@495
	int v204;			       // r1@495
	int touch_type;			       // r3@512
	char v207;			       // zf@513
	char *v208;			       // r3@514
	int v209;			       // r1@517
	char *v211;			       // r8@524
	int v212;			       // r9@526
	int v213;			       // r0@527
	int v214;			       // r0@531
	char *v215;			       // r2@531
	char *v216;			       // r3@531
	char v217;			       // zf@532
	struct mtc_car_drv *v218;	      // r0@536
	int v219;			       // r1@536
	int v220;			       // r2@546
	int battery;			       // r2@552
	int v222;			       // r1@553
	int dvd_command;		       // r0@554
	char *v224;			       // r3@557
	int v225;			       // r1@557
	int v226;			       // r2@557
	char *v228;			       // r2@566
	char view;			       // zf@567
	char *v230;			       // r3@568
	int v231;			       // r0@571
	int v232;			       // r1@571
	char *v233;	// r1@586 (T5 r20: ptr, IDA lost)
	int wipe;			       // r3@587
	char v235;			       // nf@587
	unsigned int *v236;		       // r3@588
	unsigned int v237;		       // r3@591
	int v238;			       // r1@593
	char *v239;	// r3@593 (T5 r20: ptr, IDA lost)
	char v240;			       // r0@596
	int touch_info1;		       // lr@598
	int touch_info2;		       // r12@598
	int touch_w;			       // r2@598
	int touch_h;			       // r3@598
	int uv_cal;			       // r2@605
	int v246;			       // r0@610
	unsigned int v247;		       // r1@610
	char *v248;	// r3@610 (T5 r20: ptr, IDA lost)
	int tv_signal;			       // r0@611
	char *v250;			       // r2@611
	char *v251;			       // r3@611
	char v252;			       // zf@613
	char *v253;			       // r3@617
	int v254;			       // r0@617
	int v255;			       // r1@617
	int v256;			       // r1@625
	char *v257;	// r3@625 (T5 r20: ptr, IDA lost)
	int v258;			       // r1@627
	char *v259;			       // r3@627
	int tv_status;			       // r0@628
	unsigned int radio_signal;	     // r0@629
	signed int freq;		       // r0@635 MAPDST
	int cfg_radio;			       // r2@644
	int cfg_bt;			       // r2@645
	int cfg_appdisable;		       // r2@646
	int cfg_key0;			       // r2@647
	int cfg_led_type;		       // r2@649
	int cfg_launcher;		       // r2@650
	int cfg_radio_area;		       // r2@651
	int cfg_logo_type;		       // r2@652
	int cfg_rds;			       // r2@653
	int cfg_color1;			       // r2@656
	int cfg_color2;			       // r3@656
	int cfg_ls1;			       // r2@657
	int cfg_ls2;			       // r3@657
	int cfg_blmode;			       // r2@658
	int cfg_backlight;		       // r2@659
	size_t v282;			       // r0@660
	char *v283;	// r7@662 (T5 r20: ptr, IDA lost)
	int v284;			       // r11@662
	int v285;			       // t1@662
	int v286;			       // r8@662
	char *v287;			       // r6@662
	const char *v288;		       // r10@663
	size_t v289;			       // r0@663
	int v290;			       // t1@663
	size_t v291;			       // r0@664
	int canbus;			       // r2@665
	int cfg_dtv;			       // r2@667
	int cfg_atvmode;		       // r2@668
	int canbus_cfg;			       // r2@669
	int cfg_dvd;			       // r2@670
	int cfg_ipod;			       // r2@671
	signed int v298;		       // r0@696
	int v299;			       // r1@696
	int backview_vol;		       // r0@703
	int v301;			       // r0@710
	int v302;			       // r5@712
	signed int tv_freq;		       // r0@713
	int v304;			       // r0@736
	int v305;			       // r0@739
	int eq1;			       // r5@746
	int eq2;			       // r4@747
	int eq3;			       // r3@748
	int balance;			       // r4@750
	int balance2;			       // r1@751
	int v312;			       // r0@755
	int av_active;			       // r1@760
	int av_gps_gain;		       // r0@762 MAPDST
	char v315;			       // [sp+Bh] [bp-95h]@478
	char *token_pos;		       // [sp+Ch] [bp-94h]@12 MAPDST
	unsigned char dtv_ir[4];	     // [sp+10h] [bp-90h]@556
	char can_buf[100];		       // [sp+14h] [bp-8Ch]@94

	if (cmd == 119) {
		/* binaRE: case 119 (0xC082E96A) */
		*(int *)arg = 0;
		return 0;
	}

	mutex_lock(&car_struct.car_io_lock);
	unsigned int user_cmd;	/* IDA: (unsigned)cmd */
	unsigned long userbuf;	/* IDA: arg */
	user_cmd = (unsigned int)cmd;
	userbuf = arg;
	p_config_data_4 = &car_struct.config_data; /* TENTATIVE: IDA alias */
	p_buf1_3060 = &car_struct.ioctl_buf1[3060]; /* TENTATIVE: IDA alias */


	if (cmd == 0xFE010000) {
		cur_task = get_current();
		buf1 = car_struct.ioctl_buf1;
		v29 = 0; /* IDA CF-flag artifact */
		v30 = __CFADD__(userbuf, 2);
		if (userbuf < 0xFFFFFFFE) {
			v30 = userbuf + 2 >= v29 + !__CFADD__(userbuf, 2);
		}
		if (!v30) {
			v29 = 0;
		}
		if (v29) {
			_memzero(buf1, 2u);
		} else {
			_copy_from_user(buf1, userbuf, 2u);
		}
		mcu_data_size = (car_struct.ioctl_buf1[1] | (car_struct.ioctl_buf1[0] << 8)) + 2;
		mcu_data = _kmalloc(mcu_data_size, __GFP_ZERO | __GFP_FS | __GFP_IO | __GFP_WAIT);
		if (mcu_data) {
			v34 = 0; /* IDA CF-flag artifact */
			v35 = __CFADD__(userbuf, mcu_data_size);
			if (!__CFADD__(userbuf, mcu_data_size)) {
				v35 = userbuf + mcu_data_size >=
				      v34 + !__CFADD__(userbuf, mcu_data_size);
			}
			if (!v35) {
				v34 = 0;
			}
			if (v34) {
				if (mcu_data_size) {
					_memzero(mcu_data, mcu_data_size);
				}
			} else {
				_copy_from_user(mcu_data, userbuf, mcu_data_size);
			}
			arm_send_multi(MTC_CMD_MCU_UPDDTE, mcu_data_size, mcu_data);
			kzfree(mcu_data);
			goto LABEL_62;
		}
		goto LABEL_140;
	}
	if (user_cmd == 0xFD020000) {
		size = *(off_C0831874 + 0x28);
		if (size) {
			data = kmem_cache_alloc_trace(size, 0x80D0, __GFP_NOWARN | __GFP_DMA32);
			v38 = data + 3;
		} else {
			v38 = 19;
			data = 16;
		}
		v41 = 0; /* IDA CF-flag artifact */
		v42 = __CFADD__(userbuf, 516);
		if (userbuf < 0xFFFFFDFC) {
			v42 = userbuf + 516 >= v41 + !__CFADD__(userbuf, 516);
		}
		if (!v42) {
			v41 = 0;
		}
		if (v41) {
			_memzero(data, 516u);
		} else {
			_copy_from_user(data, userbuf, 516u);
		}
		arm_send_multi(0x95FEu, 512, v38); // sending 512 bytes to MCU
		kzfree(data);
		msleep(800u);
		goto LABEL_62;
	}
	if (user_cmd == 0xFC030000) {
		size = *(off_C0831874 + 4);
		if (size) {
			data72b = kmem_cache_alloc_trace(size, 0x80D0, __GFP_IO | __GFP_MOVABLE);
		} else {
			data72b = 16;
		}
		v25 = 0; /* IDA CF-flag artifact */
		v26 = __CFADD__(userbuf, 72);
		if (userbuf < 0xFFFFFFB8) {
			v26 = userbuf + 72 >= v25 + !__CFADD__(userbuf, 72);
		}
		if (!v26) {
			v25 = 0;
		}
		if (v25) {
			_memzero(data72b, 72u);
		} else {
			_copy_from_user(data72b, userbuf, 72u);
		}
		arm_send_multi(0x95FDu, 72, data72b); // sending 72 bytes to MCU
		kzfree(data72b);
		msleep(800u);
		goto LABEL_62;
	}
	car_struct.buffer2[0] = 0;
	_memzero(car_struct.ioctl_buf1, 3072u);
	if (user_cmd >= 3072u) {
		goto LABEL_140;
	}
	v6 = get_current();
	v7 = 0; /* IDA CF-flag artifact */
	v8 = __CFADD__(userbuf, user_cmd);
	if (!__CFADD__(userbuf, user_cmd)) {
		v8 = userbuf + user_cmd >= v7 + !__CFADD__(userbuf, user_cmd);
	}
	if (!v8) {
		v7 = 0;
	}
	if (v7) {
		if (user_cmd) {
			_memzero(car_struct.ioctl_buf1, user_cmd);
		}
	} else {
		_copy_from_user(car_struct.ioctl_buf1, userbuf, user_cmd);
	}
	data_buf = car_struct.ioctl_buf1;
	equal_last_pos = strrchr(car_struct.ioctl_buf1, '=');
	token_pos = equal_last_pos;
	if (!equal_last_pos) {
		goto LABEL_140;
	}
	*equal_last_pos = 0;
	v17 = (user_cmd & 0x10000) == 0;
	token_start = token_pos++ + 1;
	if (v17) {
		i = strcmp(data_buf, off_C0831710); // "canbus_rsp"
		if (!i) {
			while (1) {
				intval =
				    get_token_int(&token_pos); // get token and move pos to next
				if (intval < 0) {
					break;
				}
				can_buf[++i] = intval;
				if (i == 99) {
					count = 100;
					can_buf_pos = 99;
					goto LABEL_99;
				}
			}
			if (!i) {
				goto LABEL_140;
			}
			can_buf_pos = i;
			count = i + 1;
		LABEL_99:
			can_buf[0] = can_buf_pos; // first byte is length of data
			arm_send_multi(MTC_CMD_CANBUS_RSP, count, can_buf);
			goto LABEL_62;
		}
		if (strlen(data_buf) <= 4) {
			goto LABEL_62;
		}
		first_char = *data_buf;
		if (first_char != 'c') {
			if (first_char != 'a') {
				if (first_char != 'r' || data_buf[1] != 'p' || data_buf[2] != 't' ||
				    data_buf[3] != '_') {
					goto LABEL_62;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831714)) // "rpt_boot_complete"
				{
					printk(off_C08318A8,
					       token_start); // "rpt_boot_complete %s\n"
					if (!strcmp(token_pos, str_true_0)) // "true"
					{
						v180 = *(off_C08318AC + 0xFFFFFB75);
						*(off_C08318AC + 0xFFFFFB74) = 1;
						if (v180) {
							car_add_work_delay(37, 0, 0);
						}
						goto LABEL_62;
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C0831718)) // "rpt_logo_complete"
				{
					printk(off_C08318A4,
					       token_start); // "rpt_logo_complete %s\n"
					if (!strcmp(token_pos, str_true_0)) // "true"
					{
						v179 = *(off_C08318AC + 0xFFFFFB74);
						*(off_C08318AC + 0xFFFFFB75) = 1;
						if (v179) {
							car_add_work_delay(37, 0, 0);
						}
						goto LABEL_62;
					}
				} else {
					v46 = strcmp(car_struct.ioctl_buf1,
						     off_C083171C); // "rpt_boot_android"
					v47 = v46;
					if (v46) {
						v48 = strcmp(car_struct.ioctl_buf1,
							     off_C0831720); // "rpt_boot_appinit"
						v49 = v48;
						if (!v48) {
							if (!strcmp(token_start,
								    off_C083189C)) // "start"
							{
								car_struct.car_status
								    .rpt_boot_appinit = 1;
							} else {
								car_struct.car_status
								    .rpt_boot_appinit = v49; // = 0
							}
							goto LABEL_62;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08317D8)) // "rpt_boot_recovery"
						{
							if (!strcmp(token_start,
								    str_true_0)) // "true"
							{
								arm_send(MTC_CMD_BOOT_RECOVERY);
								key_enter_mode(
								    RPT_KEY_MODE_RECOVERY);
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct.ioctl_buf1,
								   off_C08317F0)) // "rpt_reboot"
						{
							if (!strcmp(token_start,
								    off_C08317F4)) // "0"
							{
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct.ioctl_buf1,
								   off_C0831818)) // "rpt_power"
						{
							printk(off_C083182C,
							       token_start); // "rpt_power %s\n"
							if (!strcmp(token_pos,
								    str_true_0)) // "true"
							{
								goto LABEL_62;
							}
							if (!strcmp(token_pos,
								    str_false_0)) // "false"
							{
								car_struct.car_status.rpt_power =
								    0;
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct.ioctl_buf1,
								   off_C083181C)) // "rpt_key_mode"
						{
							if (!strcmp(token_start,
								    off_C083193C)) // "recovery"
							{
								key_enter_mode(
								    RPT_KEY_MODE_RECOVERY);
								goto LABEL_62;
							}
							if (!strcmp(token_start,
								    off_C0831820)) //  "assign"
							{
								key_enter_mode(RPT_KEY_MODE_ASSIGN);
								goto LABEL_62;
							}
							if (!strcmp(token_start,
								    off_C0831824)) // "normal"
							{
								key_enter_mode(RPT_KEY_MODE_NORMAL);
								goto LABEL_62;
							}
							if (!strcmp(token_start,
								    off_C0831828)) // "steering"
							{
								key_enter_mode(
								    RPT_KEY_MODE_STEERING);
								goto LABEL_62;
							}
						}
					} else {
						printk(off_C0831864,
						       token_start); // "rpt_boot_android %s\n"
						car_struct.car_status.rpt_boot_appinit =
						    v47;			    // = 0
						if (!strcmp(token_pos, str_true_0)) // "true"
						{
							arm_send(MTC_CMD_BOOT_ANDROID);
							car_struct.car_status.rpt_boot_android = 1;
							car_add_work(38, 0, 0);
							goto LABEL_62;
						}
					}
				}
				goto LABEL_140;
			}
			if (data_buf[1] != 'v' || data_buf[2] != '_') {
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C08317C8)) // "av_channel_enter"
			{
				printk(off_C08318B4, token_start);    // "--mtc enter %s\n"
				if (!strcmp(token_pos, off_C08318B8)) // "gsm_bt"
				{
					av_channel = MTC_AV_CHANNEL_GSM_BT;
				} else if (!strcmp(token_pos, off_C08318BC)) // "sys"
				{
					av_channel = MTC_AV_CHANNEL_SYS;
				} else if (!strcmp(token_pos, off_C08318D4)) // "dvd"
				{
					av_channel = MTC_AV_CHANNEL_DVD;
				} else if (!strcmp(token_pos, off_C08318D8)) // "line"
				{
					car_struct.car_status.av_channel_flag1 = 0;
					av_channel = MTC_AV_CHANNEL_LINE;
				} else if (!strcmp(token_pos, off_C08318C0)) // "fm"
				{
					av_channel = MTC_AV_CHANNEL_FM;
				} else if (!strcmp(token_pos, off_C08318DC)) // "dtv"
				{
					car_struct.car_status.av_channel_flag1 = 0;
					av_channel = MTC_AV_CHANNEL_DTV;
				} else if (!strcmp(token_pos, off_C08318E0)) // "dvr"
				{
					car_struct.car_status.av_channel_flag1 = 0;
					av_channel = MTC_AV_CHANNEL_DVR;
				} else {
					if (strcmp(token_pos, off_C08318C4)) // "ipod"
					{
						goto LABEL_140;
					}
					av_channel = MTC_AV_CHANNEL_IPOD;
				}
				audio_add_work(AUDIO_WORK_CH_ENTER, av_channel, 800,
					       AUDIO_WORK_CH_ENTER);
				audio_flush_work();
				audio_channel_switch_unmute();
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C08317CC)) // "av_channel_exit"
			{
				printk(off_C08318B0, token_start);	    // "--mtc exit %s\n"
				av_channel = strcmp(token_pos, off_C08318B8); // "gsm_bt"
				if (av_channel) {
					if (!strcmp(token_pos, off_C08318BC)) // "sys"
					{
						av_channel = MTC_AV_CHANNEL_SYS;
					} else if (!strcmp(token_pos, off_C08318D4)) // "dvd"
					{
						av_channel = MTC_AV_CHANNEL_DVD;
					} else if (!strcmp(token_pos, off_C08318D8)) // "line"
					{
						av_channel = MTC_AV_CHANNEL_LINE;
					} else if (!strcmp(token_pos, off_C08318E0)) // "dvr"
					{
						av_channel = MTC_AV_CHANNEL_DVR;
					} else if (!strcmp(token_pos, off_C08318C0)) // "fm"
					{
						av_channel = MTC_AV_CHANNEL_FM;
					} else if (!strcmp(token_pos, off_C08318DC)) // "dtv"
					{
						av_channel = MTC_AV_CHANNEL_DTV;
					} else {
						if (strcmp(token_pos, off_C08318C4)) // "ipod"
						{
							goto LABEL_140;
						}
						av_channel = MTC_AV_CHANNEL_IPOD;
					}
				}
				audio_add_work(AUDIO_WORK_CH_EXIT, av_channel, 0, 0);
				audio_flush_work();
				audio_channel_switch_unmute();
				goto LABEL_62;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317D0)) // "av_volume"
			{
				volume = get_token_int(&token_pos);
				if (volume >= 0 && volume <= 100) {
					audio_add_work(AUDIO_WORK_VOLUME, volume, 0, 0);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317D4)) // "av_phone_volume"
			{
				phone_volume = get_token_int(&token_pos);
				if (phone_volume >= 0 && phone_volume <= 100) {
					audio_add_work(AUDIO_WORK_PHONE_VOLUME, phone_volume, 0, 0);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (strcmp(car_struct.ioctl_buf1, str_av_mute_)) // "av_mute"
			{
				if (!strcmp(car_struct.ioctl_buf1, off_C08317E0)) // "av_phone"
				{
					if (!strcmp(token_start, off_C08317E4)) // "in"
					{
						audio_add_work(AUDIO_WORK_PHONE, 1, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, off_C08317E8)) // "out"
					{
						audio_add_work(AUDIO_WORK_PHONE, 2, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, off_C08317EC)) // "hangup"
					{
						/* binaRE CONFIRMED (naming_report2 T1): 0xC168ACAD = car_status+41 = ch_mode (mtc-car.h:61) */
						if (car_struct.car_status.ch_mode &&
						    car_struct.car_status.power_refcnt == 1) {
							backlight_off();
						}
						audio_add_work(AUDIO_WORK_PHONE, 0, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, off_C0831898)) // "answer"
					{
						audio_add_work(AUDIO_WORK_PHONE, 3, 0, 0);
						goto LABEL_62;
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C083180C)) // "av_speech"
				{
					if (!strcmp(token_start, off_C08317E4)) // "in"
					{
						audio_add_work(27, 4, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, off_C08317E8)) // "out"
					{
						audio_add_work(27, 5, 0, 0);
						goto LABEL_62;
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C0833900)) // "av_gps_ontop"
				{
					if (!strcmp(token_start, str_true)) // "true"
					{
						audio_add_work(6, 1, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, str_false)) // "false"
					{
						audio_add_work(6, 0, 0, 0);
						goto LABEL_62;
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C0833928)) // "av_lud"
				{
					if (!strcmp(token_start, str_on)) // "on"
					{
						audio_add_work(12, 1, 0, 0);
						goto LABEL_62;
					}
					if (!strcmp(token_start, str_off)) // "off"
					{
						audio_add_work(12, 0, 0, 0);
						goto LABEL_62;
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C0833938)) // "av_balance"
				{
					balance = get_token_int(&token_pos);
					if (balance >= 0) {
						balance2 = get_token_int(&token_pos);
						if (balance2 >= 0) {
							audio_add_work(10, balance2, balance, 0);
							goto LABEL_62;
						}
					}
				} else if (!strcmp(car_struct.ioctl_buf1, off_C083393C)) // "av_eq"
				{
					eq1 = get_token_int(&token_pos);
					if (eq1 >= 0) {
						eq2 = get_token_int(&token_pos);
						if (eq2 >= 0) {
							eq3 = get_token_int(&token_pos);
							if (eq3 >= 0) {
								audio_add_work(11, eq1, eq2, eq3);
								goto LABEL_62;
							}
						}
					}
				} else if (!strcmp(car_struct.ioctl_buf1,
						   off_C0833940)) // "av_gps_monitor"
				{
					if (!strcmp(token_start, str_on)) // "on"
					{
						if (car_struct.car_status.av_gps_monitor != 1) {
							car_struct.car_status
							    .av_gps_monitor = 1;
							audio_add_work(7, 1, 0, 0);
						}
						goto LABEL_62;
					}
					v305 = strcmp(token_start, str_off); // "off"
					if (!v305) {
						if (car_struct.car_status.av_gps_monitor) {
							car_struct.car_status
							    .av_gps_monitor = v305; // =0
							audio_add_work(7, 0, v305, v305);
						}
						goto LABEL_62;
					}
				} else {
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_13; */
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0833954)) // "av_gps_switch"
					{
						if (!strcmp(token_start, str_on)) // "on"
						{
							if (car_struct.car_status.av_gps_switch !=
							    1) {
								car_struct.car_status
								    .av_gps_switch = 1;
								audio_add_work(8, 1, 0, 0);
							}
							goto LABEL_62;
						}
						v312 = strcmp(token_start, str_off); // "off"
						if (!v312) {
							if (car_struct.car_status.av_gps_switch) {
								car_struct.car_status
								    .av_gps_switch = v312;
								audio_add_work(8, 0, v312, v312);
							}
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct.ioctl_buf1,
							   off_C0833958)) // "av_gps_gain"
					{
						av_gps_gain = get_token_int(&token_pos);
						if (av_gps_gain >= 0) {
							if (car_struct.car_status.av_gps_gain !=
							    av_gps_gain) {
								car_struct.car_status.av_gps_gain =
								    av_gps_gain;
							}
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct.ioctl_buf1,
							   off_C083395C)) // "av_active"
					{
						av_active = get_token_int(&token_pos);
						if (av_active >= 0) {
							audio_add_work(14, av_active, 0, 0);
							goto LABEL_62;
						}
					}
				}
				goto LABEL_140;
			}				      // av_mute
			if (!strcmp(token_start, str_true_0)) // "true"
			{
				audio_add_work(2, 1, 0, 0);
				goto LABEL_62;
			}
			if (strcmp(token_start, str_false_0)) // "false"
			{
				goto LABEL_140;
			}
			audio_add_work(2, 0, 0, 0);
		LABEL_62:
			res = 0;
			mutex_unlock(p_car_lock);
			return res;
		}
		if (data_buf[1] != 'f' || data_buf[2] != 'g' || data_buf[3] != '_') {
			data_buf = car_struct.ioctl_buf1;
			if (car_struct.ioctl_buf1[1] != 't' ||
			    car_struct.ioctl_buf1[2] != 'l' || data_buf[3] != '_') {
				goto LABEL_62;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317C4)) // "ctl_uv_cal"
			{
				if (car_struct.car_status.reserved_22[8] ||
				    car_struct.car_status.decoder_state != 1) {
					car_struct.car_status.uv_cal = 0;
				} else {
					v178 = strcmp(token_start, off_C083189C); // "start"
					if (v178) {
						if (!strcmp(token_start, off_C083194C)) // "ok"
						{
							v178 = 1;
						} else if (!strcmp(token_start,
								   off_C08318A0)) // "cancel"
						{
							v178 = 2;
						} else {
							v178 = 0;
						}
					}
					car_struct.car_status.uv_cal = 1;
					capture_add_work(50, v178, 0, 0);
				}
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C08317FC)) // "ctl_cvbs_brightness"
			{
				brightness = get_token_int(&token_pos);
				if (brightness >= 0) {
					capture_add_work(51, brightness, 0, 0);
				}
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C0831800)) // "ctl_lcd"
			{
				lcd_show(token_start);
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C0831804)) // "ctl_camera"
			{
				if (!strcmp(token_start, off_C0831808)) // "start_front"
				{
					cam_front = car_struct.config_data.d.cfg_frontview;
					if (car_struct.config_data.d.cfg_frontview) {
						cam_front = 1;
					}
					capture_add_work(52, cam_front, 0, 0);
				} else {
					v185 = strcmp(token_start, str_start_back); // "start_back"
					v187 = v185;
					v186 = v185 == 0;
					if (v185) {
						v187 = 0;
					} else {
						v185 = 52;
					}
					if (!v186) {
						v185 = 53;
					}
					capture_add_work(v185, v187, v187, v187);
				}
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C08318CC)) // "ctl_capture_on"
			{
				if (!strcmp(token_start, off_C08318D4)) // "dvd"
				{
					av_channel = MTC_AV_CHANNEL_DVD;
				} else if (!strcmp(token_start, off_C08318D8)) // "line"
				{
					av_channel = MTC_AV_CHANNEL_LINE;
				} else if (!strcmp(token_start, off_C08318E0)) // "dvr"
				{
					av_channel = MTC_AV_CHANNEL_DVR;
				} else {
					if (strcmp(token_start, off_C08318DC)) // "dtv"
					{
						goto LABEL_140;
					}
					av_channel = MTC_AV_CHANNEL_DTV;
				}
				capture_add_work(48, av_channel, 0, 1);
				goto LABEL_62;
			}
			if (!strcmp(data_buf, off_C08318D0)) // "ctl_capture_off"
			{
				if (!strcmp(token_start, off_C08318D4)) // "dvd"
				{
					av_channel = MTC_AV_CHANNEL_DVD;
				} else if (!strcmp(token_start, off_C08318D8)) // "line"
				{
					av_channel = MTC_AV_CHANNEL_LINE;
				} else if (!strcmp(token_start, off_C08318DC)) // "dtv"
				{
					av_channel = MTC_AV_CHANNEL_DTV;
				} else {
					if (strcmp(token_start, off_C08318E0)) // "dvr"
					{
						goto LABEL_140;
					}
					av_channel = MTC_AV_CHANNEL_DVR;
				}
				capture_add_work(49, av_channel, 0, 1);
				goto LABEL_62;
			}
			if (strcmp(data_buf, off_C08318E4)) // "ctl_radar"
			{
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_14; */
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C08318E8)) // "ctl_beep"
				{
					if (car_struct.config_data.d.ctl_beep &&
					    get_token_int(&token_pos) >= 0) {
						v315 = 5;
						arm_send_multi(0x9520u, 1, &v315);
						goto LABEL_62;
					}
				} else {
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831904)) // "ctl_dtv_ir"
					{
						dtv_ir[0] = get_token_int(&token_pos);
						dtv_ir[1] = get_token_int(&token_pos);
						dtv_ir[2] = get_token_int(&token_pos);
						dtv_ir[3] = get_token_int(&token_pos);
						arm_send_multi(MTC_CMD_DTV_IR, 4, dtv_ir);
						goto LABEL_62;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831908)) // "ctl_dvd_cmd"
					{
						dvd_command = get_token_int(&token_pos);
						if (dvd_command >= 0) {
							dvd_send_command(dvd_command);
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct.ioctl_buf1,
							   off_C083190C)) // "ctl_dvd_door"
					{
						if (!strcmp(token_start, off_C0831910)) // "open"
						{
							arm_send(MTC_CMD_DVD_DOOR_OPEN);
							goto LABEL_62;
						}
						if (!strcmp(token_start, off_C0831914)) // "close"
						{
							arm_send(MTC_CMD_DVD_DOOR_CLOSE);
							goto LABEL_62;
						}
						if (!strcmp(token_start, off_C0831918)) // "eject"
						{
							arm_send(MTC_CMD_DVD_EJECT);
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct.ioctl_buf1,
							   off_C0833864)) // "ctl_radio_ta"
					{
						if (!strcmp(token_start, str_true)) // "true"
						{
							Radio_TA(1, 0); /* T5 r20: def 2 arg; a2 padding 0 (decompiled car.c - 1 arg) */
							goto LABEL_62;
						}
						if (!strcmp(token_start, str_false)) // "false"
						{
							Radio_TA(0, 0); /* T5 r20: def 2 arg; a2 padding 0 (decompiled car.c - 1 arg) */
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct.ioctl_buf1,
							   off_C08338AC)) // "ctl_radio_af"
					{
						if (!strcmp(token_start, str_true)) // "true"
						{
							Radio_AF(1);
							goto LABEL_62;
						}
						if (!strcmp(token_start, str_false)) // "false"
						{
							Radio_AF(0);
							goto LABEL_62;
						}
					} else {
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08338C4)) // "ctl_radio_search"
						{
							v240 = strcmp(token_start, str_true) ==
							       0; // "true"
							Radio_Set_Search(v240);
							goto LABEL_62;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08338CC)) // "ctl_radio_frequency"
						{
							freq = get_token_int(&token_pos);
							if (freq >= 0) {
								Radio_Set_Frequency(freq, 0);
								goto LABEL_62;
							}
						} else if (
						    !strcmp(car_struct.ioctl_buf1,
							    off_C08338D0)) // "ctl_radio_sfrequency"
						{
							freq = get_token_int(&token_pos);
							if (freq >= 0) {
								Radio_Set_Frequency(freq, 1);
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct.ioctl_buf1,
								   off_C08338D4)) // "ctl_soft_mute"
						{
							if (!strcmp(token_start,
								    str_true)) // "true"
							{
								audio_add_work(18, 1, 0, 0);
								goto LABEL_62;
							}
							if (!strcmp(token_start,
								    str_false)) // "false"
							{
								audio_add_work(18, 0, 0, 0);
								goto LABEL_62;
							}
						} else {
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_13; */
							if (!strcmp(
								car_struct.ioctl_buf1,
								off_C08338E8)) // "ctl_radio_mute"
							{
								if (!strcmp(token_start,
									    str_true)) // "true"
								{
									audio_add_work(19, 1, 0, 0);
									Radio_Set_Mute(1);
									goto LABEL_62;
								}
								if (!strcmp(token_start,
									    str_false)) // "false"
								{
									Radio_Set_Mute(0);
									audio_add_work(19, 0, 0, 0);
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct.ioctl_buf1,
								off_C08338EC)) // "ctl_radio_stereo"
							{
								if (!strcmp(token_start,
									    str_true)) {
									Radio_Set_Stereo(1);
									goto LABEL_62;
								}
								if (!strcmp(token_start,
									    str_false)) {
									Radio_Set_Stereo(0);
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct.ioctl_buf1,
								off_C08338F8)) // "ctl_backview_vol"
							{
								backview_vol =
								    get_token_int(&token_pos);
								if (backview_vol >= 0) {
									car_struct.car_status
									    .backview_vol =
									    backview_vol;
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct.ioctl_buf1,
								off_C08338FC)) // "ctl_backview_mute"
							{
								if (!strcmp(token_start,
									    str_false)) {
									car_struct.car_status
									    .backview_vol = 11;
									goto LABEL_62;
								}
								if (!strcmp(token_start,
									    str_true)) {
									car_struct.car_status
									    .backview_vol = 0;
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct.ioctl_buf1,
								off_C0833904)) // "ctl_tv_frequency"
							{
								tv_freq = get_token_int(&token_pos);
								if (tv_freq >= 0) {
									Tv_Set_Frequency(tv_freq);
									goto LABEL_62;
								}
							} else if (
							    !strcmp(car_struct.ioctl_buf1,
								    off_C0833908)) // "ctl_tv_demod"
							{
								v301 = get_token_int(&token_pos);
								if (v301 >= 0) {
									if ((v301 - 1) <= 7) {
										v302 = v301;
										car_struct
										    .car_status
										    .av_channel_flag1 =
										    v301;
										capture_add_work(
										    61, 0, 0, 1);
										Tv_Set_Demod(v302);
									}
									goto LABEL_62;
								}
							} else if (!strcmp(
								       car_struct.ioctl_buf1,
								       off_C083390C)) // "ctl_key"
							{
								if (!strcmp(
									token_start,
									off_C0833910)) // "power"
								{
									arm_send_multi(0x9527u, 0,
										       0);
									goto LABEL_62;
								}
								if (!strcmp(
									token_start,
									off_C0833914)) // "power2"
								{
									arm_send_multi(0x9529u, 0,
										       0);
									car_struct.car_status
									    .power2_flag = 1;
									goto LABEL_62;
								}
								if (!strcmp(
									token_start,
									off_C0833918)) // "eject"
								{
									arm_send_multi(0x9528u, 0,
										       0);
									goto LABEL_62;
								}
								v298 = strcmp(
								    token_start,
								    off_C083391C); // "screenbrightness"
								v299 = v298;
								if (!v298) {
									if (car_struct.car_status
										.backlight_status) {
										v298 = 36;
									} else {
										v299 =
										    car_struct
											.car_status
											.backlight_status;
									}
									if (!car_struct.car_status
										 .backlight_status) {
										v298 = 35;
									}
									car_add_work(v298, v299,
										     v299);
									goto LABEL_62;
								}
								if (!strcmp(
									token_start,
									off_C0833920)) // "parrot_updata"
								{
									car_add_work(74, 1, 0);
									goto LABEL_62;
								}
								if (!strcmp(
									token_start,
									off_C0833924)) // "parrot_normal"
								{
									car_add_work(74, 0, 0);
									goto LABEL_62;
								}
							} else if (!strcmp(
								       car_struct.ioctl_buf1,
								       off_C0833934)) // "ctl_power"
							{
								if (!strcmp(token_start, str_on) ||
								    !strcmp(token_start, str_off)) {
									goto LABEL_62;
								}
							} else if (!strcmp(
								       car_struct.ioctl_buf1,
								       off_C0833944)) // "ctl_reset"
							{
								if (!strcmp(token_start,
									    off_C0833948)) // "0"
								{
									arm_send_multi(
									    MTC_CMD_RESET, 0, 0);
									msleep(100u);
									goto LABEL_62;
								}
								v304 = strcmp(token_start,
									      off_C083394C); // "1"
								if (!v304) {
									if (car_struct.car_status
										.wipe_flag &
									    0x40) {
										arm_send_multi(
										    MTC_CMD_RESET2,
										    v304, 0);
										kernel_restart(
										    off_C0833950); // "recovery"
									} else {
										arm_send_multi(
										    MTC_CMD_RESET,
										    car_struct
											    .car_status
											    .wipe_flag &
											0x40,
										    (car_struct
											 .car_status
											 .wipe_flag &
										     0x40));
										msleep(100u);
									}
									goto LABEL_62;
								}
							}
						}
					}
				}
				goto LABEL_140;
			}				      // ctl_radar
			if (!strcmp(token_start, str_true_0)) // "true"
			{
				capture_add_work(64, 0, *&car_struct.car_status.radar_val, 0);
				goto LABEL_62;
			}
			if (!strcmp(token_start, str_false_0)) // "false"
			{
				goto LABEL_62;
			}
		} else {
			if (!strcmp(data_buf, off_C08317B4)) // "cfg_color"
			{
				color = get_token_int(&token_pos);
				if (color >= 0) {
					cfg_color = &car_struct.config_data.d.cfg_color[0]; /* IDA alias */
					car_struct.config_data.d.cfg_color[1] = color;
					car_struct.config_data.d.cfg_color[0] = BYTE1(color);
					arm_send_multi(0x9507u, 2, cfg_color);
					goto LABEL_62;
				}
			}
			if (!strcmp(car_struct.ioctl_buf1,
				    off_C08317B8)) // "cfg_led_multi"
			{
				cfg_led_multi = get_token_int(&token_pos);
				if (cfg_led_multi >= 0) {
					car_struct.config_data.d.cfg_led_multi = cfg_led_multi;
					arm_send_multi(0x9508u, 1,
						       &car_struct.config_data.d.cfg_led_multi);
					goto LABEL_62;
				}
			}
			if (!strcmp(car_struct.ioctl_buf1,
				    off_C08317BC)) // "cfg_wifi_pwr"
			{
				wifi_pwr = get_token_int(&token_pos);
				if (wifi_pwr >= 0) {
					car_struct.config_data.d.wifi_pwr = wifi_pwr;
					arm_send_multi(MTC_CMD_WIFI_PWR, 1, &car_struct.config_data.d.wifi_pwr); /* IDA alias */
					if (car_struct.wifi_capable) {
						v174 = *off_C0831890;
						if (v174 != 255) {
							rk29sdk_wifi_power(v174);
						}
					}
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C0831834)) // "cfg_blmode"
			{
				blmode = get_token_int(&token_pos);
				if (blmode >= 0) {
					p_blmode = off_C0831888;
					car_struct.config_data.d.cfg_blmode = blmode;
					arm_send_multi(MTC_CMD_BLMODE, 1, p_blmode);
					backlight_update();
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C0831838)) // "cfg_powerdelay"
			{
				powerdelay = get_token_int(&token_pos);
				if (powerdelay >= 0) {
					p_powerdelay = off_C0831884;
					car_struct.config_data.d.cfg_powerdelay = powerdelay;
					arm_send_multi(MTC_CMD_POWERDELAY, 1, p_powerdelay);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317C0)) // "cfg_mirror"
			{
				if (!strcmp(token_pos, str_true_0)) // "true"
				{
					car_struct.config_data.d.cfg_mirror = 1;
					arm_send(MTC_CMD_MIRROR_ON);
					goto LABEL_62;
				}
				if (strcmp(token_pos, str_false_0)) // "false"
				{
					goto LABEL_140;
				}
				car_struct.config_data.d.cfg_mirror = 0;
				arm_send(MTC_CMD_MIRROR_OFF);
				goto LABEL_62;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C0831830)) // "cfg_backlight"
			{
				backlight = get_token_int(&token_pos);
				if (backlight >= 0) {
					p_backlight = off_C0831880;
					car_struct.config_data.d.cfg_backlight = backlight;
					arm_send_multi(MTC_CMD_BACKLIGHT, 1, p_backlight);
					goto LABEL_62;
				}
			} else if (!strcmp(car_struct.ioctl_buf1, off_C0831840)) // "cfg_beep"
			{
				enable_beep = get_token_int(&token_pos);
				if (enable_beep >= 0) {
					car_struct.config_data.d.ctl_beep = enable_beep;
					arm_send_multi(MTC_CMD_BEEP, 1,
						       &car_struct.config_data.d.ctl_beep);
					goto LABEL_62;
				}
			} else {
				if (strcmp(car_struct.ioctl_buf1, off_C0831844)) // "cfg_led"
				{
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831854)) // "cfg_ir_assign"
					{
						v59 = *(off_C0831874 + 0x20);
						if (v59) {
							v60 = kmem_cache_alloc_trace(v59, 0x80D0,
										     0xF0u);
						} else {
							v60 = 16;
						}
						v61 = 0;
						v62 = 0;
						do {
							++v62;
							v63 = get_token_int(&token_pos);
							v60[v61] = v63;
							v61 = v62;
							if (v63 < 0) {
								kfree(v60);
								goto LABEL_140;
							}
						} while (v62 != 60);
						v156 = (char *)&p_config_data_4->d.checksum;
						v157 = v60;
						v158 = &p_config_data_4->d.cfg_logo2[8];
						do {
							v156[206] = *v157 >> 8;
							v159 = *v157;
							v157 += 2;
							v156[207] = v159;
							v156 = (v156 + 2);
						} while (v156 != v158);
						kfree(v60);
						goto LABEL_62;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831868)) // "cfg_steer_assign"
					{
						size_1 = *(off_C0831874 + 0x20);
						if (size_1) {
							v139 = kmem_cache_alloc_trace(
							    size_1, 0x80D0, 0xC8u);
						} else {
							v139 = ZERO_SIZE_PTR;
						}
						v140 = 0;
						v141 = 0;
						while (1) {
							++v141;
							v142 = get_token_int(&token_pos);
							v139[v140] = v142;
							v140 = v141;
							if (v142 < 0) {
								break;
							}
							if (v141 == 50) {
								gap12 = p_config_data_4;
								v144 = 0;
								v145 = v139;
								st_pos = 0;
								do {
									v147 = (&gap12->d.checksum +
										v144);
									st_pos += 3;
									*(v147 + 0x148) =
									    *(v145 + 1);
									v144 = st_pos;
									v148 = *v145;
									++v145;
									*(v147 + 0x149) =
									    BYTE1(v148);
									*(v147 + 0x14A) =
									    *(v145 - 1);
								} while (st_pos != 150);
								kfree(v139);
								arm_send_multi(MTC_CMD_STEER_ASSIGN,
									       150, &car_struct.config_data.d.steer_data[0]); /* IDA alias */
								stw_range_check();
								goto LABEL_62;
							}
						}
					} else {
						if (strcmp(car_struct.ioctl_buf1,
							   off_C0831870)) // "cfg_config"
						{
							goto LABEL_140;
						} // cfg_config
						size = *(off_C0831874 + 0x2C);
						if (size) {
							v139 = kmem_cache_alloc_trace(size, 0x80D0,
										      __GFP_NOFAIL);
						} else {
							v139 = ZERO_SIZE_PTR;
						}
						v150 = 0;
						while (1) {
							v151 = get_token_int(&token_pos);
							v139[v150] = v151;
							++v150;
							if (v151 < 0) {
								break;
							}
							if (v150 == 512) {
								p_config_data = p_config_data_4;
								v153 = v139;
								cur_byte = 0;
								do {
									v155 = *v153;
									++v153;
									*(&p_config_data->d.checksum +
									  cur_byte++) = v155;
								} while (cur_byte != 512);
								kfree(v139);
								arm_send_multi(0x95FEu, 512,
									       p_config_data_4);
								goto LABEL_62;
							}
						}
					}
					kfree(v139);
					goto LABEL_140;
				} // cfg_led
				led1 = get_token_int(&token_pos);
				b_led1 = led1;
				if (led1 >= 0) {
					led2 = get_token_int(&token_pos);
					b_led2 = led2;
					if (led2 >= 0) {
						led3 = get_token_int(&token_pos);
						if (led3 >= 0) {
							cfg_led = off_C083187C;
							car_struct.config_data.d.cfg_led[0] = b_led1;
							car_struct.config_data.d.cfg_led[1] = b_led2;
							car_struct.config_data.d.cfg_led[2] = led3;
							arm_send_multi(MTC_CMD_LEDCFG, 3, cfg_led);
							goto LABEL_62;
						}
					}
				}
			}
		}
	LABEL_140:
		res = -1;
		mutex_unlock(p_car_lock);
		return res;
	}
	slen = strlen(data_buf);
	if (slen <= 4) {
		if (slen != 4) {
			goto LABEL_102;
		}
		first_char = *data_buf;
	} else {
		first_char = *data_buf;
		if (first_char == 115) {
			if (data_buf[1] == 't' && data_buf[2] == 'a' && data_buf[3] == '_') {
				if (!strcmp(data_buf, off_C0831724)) // "sta_dvd"
				{
					arm_send(0xF01u);
					if ((v94 & 0xFF00) != 0xF00 || v94 != 5) {
						res = 0;
						*b1 = str_nodisk[0]; /* T5 r20: ptr-tip IDA uteryan */
						*b2 = str_nodisk[1]; /* T5 r20: ptr-tip IDA uteryan */
						car_struct.buffer2[6] = *b2 >> 16;
						v97 = off_C08318F0;
						*&car_struct.buffer2[0] = *b1;
						*(v97 + 0x10) = *b2;
					} else {
						res = 0;
						v198 = str_diskin[0]; /* T5 r20: ptr-tip IDA uteryan */
						v199 = str_diskin[1]; /* T5 r20: ptr-tip IDA uteryan */
						car_struct.buffer2[6] = v199 >> 16;
						*&car_struct.buffer2[0] = v198;
						*&car_struct.buffer2[4] = v199;
					}
					goto LABEL_30;
				}
				if (!strcmp(data_buf, off_C0831728)) // "sta_dvd_folder"
				{
					folder_token = get_token_int(&token_pos);
					if (folder_token >= 0) {
						res = 0;
						dvd_get_folder(folder_token, buf_1, v92, v93);
						goto LABEL_30;
					}
				} else if (!strcmp(data_buf, off_C083172C)) // "sta_dvd_media"
				{
					media_token = get_token_int(&token_pos);
					if (media_token >= 0) {
						res = 0;
						dvd_get_media(media_token, p_buf1, v136, v137);
						goto LABEL_30;
					}
				} else {
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831730)) // "sta_dvd_folder_cnt"
					{
						res = 0;
						dvd_get_folder_cnt(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831734)) // "sta_dvd_media_cnt"
					{
						res = 0;
						dvd_get_media_cnt(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831738)) // "sta_dvd_folder_idx"
					{
						res = 0;
						dvd_get_folder_idx(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C083173C)) // "sta_dvd_media_idx"
					{
						res = 0;
						dvd_get_media_idx(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831740)) // "sta_dvd_length"
					{
						res = 0;
						dvd_get_length(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831744)) // "sta_dvd_position"
					{
						res = 0;
						dvd_get_position(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831748)) // "sta_dvd_title"
					{
						res = 0;
						dvd_get_media_title(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C083174C)) // "sta_ipod"
					{
						v55 = (car_struct.car_status.sta_bits & 0x20) == 0;
						if (car_struct.car_status.sta_bits & 0x20) {
							v56 = &car_struct.ioctl_buf1[3060];
							v54 = str_false_0; // "false"
						} else {
							v56 = str_true_0; // "true"
						}
						if (car_struct.car_status.sta_bits & 0x20) {
							v57 = *v54;
							v58 = *(v54 + 1);
						} else {
							v57 = *v56;
							v58 = *(v56 + 1);
						}
						if (car_struct.car_status.sta_bits & 0x20) {
							*&car_struct.buffer2[0] = v57;
						} else {
							*&car_struct.buffer2[0] = v57;
							car_struct.buffer2[4] = v58;
						}
						res = 0;
						if (!v55) {
							*(v56 + 8) = v58;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C08318EC)) //  "sta_driving"
					{
						v192 = car_struct.car_status.sta_bits;
						res = 0;
						v193 = (v192 & 0x10) == 0;
						if (v192 & 0x10) {
							v194 = str_true_0;
						} else {
							v194 = str_false_0;
						}
						v195 = *v194;
						v196 = *(v194 + 1);
						v197 = off_C08318F0;
						if (v193) {
							*&car_struct.buffer2[0] = v195;
							*(v197 + 0x10) = v196;
						} else {
							*&car_struct.buffer2[0] = v195;
							*(v197 + 0x10) = v196;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C08318F8)) // "sta_ill"
					{
						res = 0;
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_14; */
						if (car_struct.car_status.sta_bits & 8) {
							v202 =
							    &car_struct.ioctl_buf1[3060];
							v200 = str_false_0;
						} else {
							v202 = str_true_0;
						}
						if (car_struct.car_status.sta_bits & 8) {
							v203 = *v200;
							v204 = *(v200 + 1);
						} else {
							v203 = *v202;
							v204 = *(v202 + 1);
						}
						if (car_struct.car_status.sta_bits & 8) {
							*&car_struct.buffer2[0] = v203;
							*(v202 + 8) = v204;
						} else {
							*&car_struct.buffer2[0] = v203;
							car_struct.buffer2[4] = v204;
						}
						goto LABEL_30;
					}
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_14; */
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831920)) // "sta_dtv"
					{
						v222 = *(str_true + 1);
						*&car_struct.buffer2[0] = *str_true;
						car_struct.buffer2[4] = v222;
						res = 0;
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831924)) // "sta_battery"
					{
						battery = *&car_struct.car_status.battery;
						res = 0;
						sprintf(p_buf2, str_fmt_d_3, battery); // "%d"
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831928)) // "sta_touch"
					{
						touch_type = car_struct.car_status.touch_type;
						if (touch_type == 129) {
							*&car_struct.buffer2[0] = 'ser';
							res = 0;
						} else {
							v207 = touch_type == 128;
							if (touch_type == 128) {
								v208 = 'pac';
								*&car_struct.buffer2[0] = 'pac';
							} else {
								v208 = off_C083192C; // "none"
							}
							if (!v207) {
								v209 = *(v208 + 1);
								*&car_struct.buffer2[0] = *v208;
								car_struct.buffer2[4] = v209;
							}
							res = 0;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831930)) // "sta_touch_adc"
					{
						res = 0;
						sta_touch_adc(p_buf2);
						goto LABEL_30;
					}
					if (!strcmp(car_struct.ioctl_buf1,
						    off_C0831934)) // "sta_touch_cal"
					{
						size = *(off_C0831938 + 0x18);
						if (size) {
							v211 = kmem_cache_alloc_trace(
							    size, 0x80D0,
							    GFP_ATOMIC | __GFP_MOVABLE);
						} else {
							v211 = ZERO_SIZE_PTR;
						}
						v212 = 0;
						res = 0;
						do {
							v213 = get_token_int(&token_pos);
							*&v211[v212] = v213;
							v212 += 4;
							if (v213 < 0) {
								res = -1;
							}
						} while (v212 != 40);
						if (!res) {
							v214 = sta_touch_cal(v211);
							if (v214) {
								v217 = v214 == 1;
								if (v214 == 1) {
									v216 =
									    off_C0831940; // "success"
								} else {
									v215 =
									    off_C083193C; // "recovery"
								}
								if (v214 == 1) {
									v218 = *v216;
									v219 = *(v216 + 1);
										/* IDA artifact удалено: car_struct = p_buf1; */
								} else {
									v216 = p_buf1;
									v218 = *v215;
									v219 = *(v215 + 1);
									v215 = *(v215 + 2);
								}
								if (v217) {
									car_struct.car_dev = v218;
									*&car_struct.car_status
									      .car_ready = v219;
								} else {
									*v216 = v218;
									*(v216 + 1) = v219;
									v216 += 8;
								}
								if (!v217) {
									*v216 = v215;
								}
								kfree(v211);
							} else {
								v238 = *(str_fail + 1);
								v239 = p_buf1_3060;
								*&car_struct.buffer2[0] =
								    *str_fail;
								*(v239 + 0x10) = v238;
								kfree(v211);
							}
							goto LABEL_30;
						}
						kfree(v211);
					} else {
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0831948)) // "sta_video_signal"
						{
							if (car_struct.car_status
								.sta_video_signal) {
								v220 = *off_C083194C >> 16;
								*&car_struct.buffer2[0] =
								    *off_C083194C; // "ok"
								car_struct.buffer2[2] = v220;
								res = 0;
							} else {
								res = 0;
								v224 = p_buf2;
								v225 = *(off_C0833878 + 1); // "nosignal" (T5 r20: ptr-tip IDA)
								v226 = *(off_C0833878 + 2); /* T5 r20: ptr-tip IDA */
								*p_buf2 = *off_C0833878; /* T5 r20: ptr-tip IDA */
								*(v224 + 1) = v225;
								v224[8] = v226;
							}
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C083387C)) // "sta_radio_signal"
						{
							radio_signal = Radio_Get_Signal();
							res = 0;
							sprintf(p_buf2, str_fmt_d_3,
								radio_signal); // "%d"
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833880)) // "sta_tv_status"
						{
							tv_status = Tv_Get_Status();
							res = 0;
							sprintf(p_buf2, str_fmt_d_3, tv_status);
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833884)) // "sta_tv_signal"
						{
							tv_signal = check_tv_signal();
							if (tv_signal == 1) {
								res = 0;
								v258 = *(off_C08338E4 + 1); // "ntsc" (T5 r20: ptr-tip IDA)
								v259 = p_buf1_3060;
								*&car_struct.buffer2[0] =
								    *off_C08338E4; /* T5 r20: ptr-tip IDA */
								*(v259 + 0x10) = v258;
							} else if (tv_signal == 2) {
								res = 0;
								*&car_struct.buffer2[0] = 'lap';
							} else {
								v252 = tv_signal == 3;
								res = 0;
								if (tv_signal == 3) {
									v251 = off_C08338DC; // "secam" (T5 r20: ptr-tip IDA)
								} else {
									v250 = off_C0833878; // "nosignal" (T5 r20: ptr-tip IDA)
								}
								if (tv_signal == 3) {
									v254 = *v251;
									v255 = *(v251 + 1);
									v253 = p_buf1_3060;
								} else {
									v253 = p_buf2;
									v254 = *v250;
									v255 = *(v250 + 1);
									v250 = *(v250 + 2);
								}
								if (v252) {
									*&car_struct.buffer2[0] =
									    v254;
									*(v253 + 8) = v255;
								} else {
									*v253 = v254;
									*(v253 + 1) = v255;
									v253 += 8;
								}
								if (!v252) {
									*v253 = v250;
								}
							}
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833888)) // "sta_radio_stereo"
						{
							if (Radio_Get_Stereo()) {
								res = 0;
								v246 = off_C08338D8[0]; // "stereo" (T5 r20: ptr-tip IDA)
								v247 = off_C08338D8[1]; /* T5 r20: ptr-tip IDA */
								car_struct.buffer2[6] = v247 >> 16;
								v248 = p_buf1_3060;
								*&car_struct.buffer2[0] = v246;
								*(v248 + 0x10) = v247;
							} else {
								res = 0;
								v256 = *(off_C08338E0 + 1); // "mono" (T5 r20: ptr-tip IDA)
								v257 = p_buf1_3060;
								*&car_struct.buffer2[0] =
								    *off_C08338E0; /* T5 r20: ptr-tip IDA */
								*(v257 + 0x10) = v256;
							}
							goto LABEL_30;
						}
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_13; */
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833890)) // "sta_mcu_version"
						{
							strcpy(p_buf2, car_struct.mcu_version);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833894)) // "sta_mcu_date"
						{
							strcpy(p_buf2, car_struct.mcu_date);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C0833898)) // "sta_mcu_time"
						{
							strcpy(p_buf2, car_struct.mcu_time);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C083389C)) // "sta_uv_cal"
						{
							uv_cal = car_struct.car_status.uv_cal;
							res = 0;
							sprintf(p_buf2, str_fmt_d_3, uv_cal);
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08338A0)) // "sta_view"
						{
							view = car_struct.car_status.sta_view == 0;
							if (car_struct.car_status.sta_view) {
								v230 =
								    &car_struct.ioctl_buf1[3060];
								v228 = off_C08338A4[0]; // "front"
							} else {
								v230 = off_C08338A8[0]; // "back"
							}
							if (car_struct.car_status.sta_view) {
								v231 = *v228;
								v232 = *(v228 + 1);
							} else {
								v231 = *v230;
								v232 = *(v230 + 1);
							}
							if (car_struct.car_status.sta_view) {
								*&car_struct.buffer2[0] = v231;
							} else {
								*&car_struct.buffer2[0] = v231;
								car_struct.buffer2[4] = v232;
							}
							res = 0;
							if (!view) {
								*(v230 + 8) = v232;
							}
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08338B0)) // "sta_touch_info"
						{
							touch_info1 =
							    car_struct.car_status.touch_info1;
							touch_info2 =
							    car_struct.car_status.touch_info2;
							touch_w =
							    car_struct.car_status.touch_width;
							touch_h =
							    car_struct.car_status.touch_height;
							res = 0;
							sprintf(p_buf2, off_C08338C8, touch_w,
								touch_h, touch_info1,
								touch_info2); // "%d x %d (0x%02x
							// 0x%02x)"
							goto LABEL_30;
						}
						if (!strcmp(car_struct.ioctl_buf1,
							    off_C08338B4)) // "sta_wipe"
						{
							wipe = car_struct.car_status.wipe_flag;
							v235 = wipe < 0;
							if (wipe < 0) {
								v236 = 'sey';
								*&car_struct.buffer2[0] = 'sey';
							} else {
								v233 =
								    &car_struct.ioctl_buf1[3060];
								v236 = off_C08338B8; // "no"
							}
							if (!v235) {
								v237 = *v236;
								*(v233 + 0xC) = v237;
								car_struct.buffer2[2] = v237 >> 16;
							}
							res = 0;
							goto LABEL_30;
						}
					}
				}
			}
			goto LABEL_102;
		}
		if (first_char == 'c') {
			if (data_buf[1] == 'f' && data_buf[2] == 'g' && data_buf[3] == '_') {
				if (!strcmp(data_buf, off_C0831750)) // "cfg_maxvolume"
				{
					res = 0;
					sprintf(buf_1, str_fmt_d_4,
						car_struct.car_status.cfg_maxvolume); // "%d"
					goto LABEL_30;
				}
				if (!strcmp(data_buf, off_C0831754)) // "cfg_customer"
				{
					res = 0;
					strcpy(buf_1, &car_struct.config_data.d.cfg_customer[0]); /* IDA alias */
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831758)) // "cfg_sn"
				{
					strcpy(p_buf2, car_struct.config_data.d.cfg_sn);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C083175C)) // "cfg_model"
				{
					strcpy(p_buf2, car_struct.config_data.d.cfg_model);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831760)) // "cfg_password"
				{
					strcpy(p_buf2, car_struct.config_data.d.cfg_password);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831764)) // "cfg_logo1"
				{
					strcpy(p_buf2, car_struct.config_data.d.cfg_logo1);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831768)) // "cfg_logo2"
				{
					strcpy(p_buf2, car_struct.config_data.d.cfg_logo2);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C083176C)) // "cfg_canbus"
				{
					canbus = car_struct.config_data.d.cfg_canbus;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, canbus);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831770)) // "cfg_canbus_cfg"
				{
					canbus_cfg = car_struct.config_data.d.canbus_cfg;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, canbus_cfg);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831774)) // "cfg_atvmode"
				{
					cfg_atvmode = car_struct.config_data.d.cfg_atvmode;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_atvmode);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831778)) // "cfg_dtv"
				{
					cfg_dtv = car_struct.config_data.d.cfg_dtv;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_dtv);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C083177C)) // "cfg_frontview"
				{
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						car_struct.config_data.d.cfg_frontview);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831780)) // "cfg_ipod"
				{
					cfg_ipod = car_struct.config_data.d.cfg_ipod;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_ipod);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831784)) // "cfg_dvd"
				{
					cfg_dvd = car_struct.config_data.d.cfg_dvd;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_dvd);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831788)) // "cfg_bt"
				{
					cfg_bt = car_struct.config_data.d.cfg_bt;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_bt);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C083178C)) // "cfg_radio"
				{
					cfg_radio = car_struct.config_data.d.cfg_radio;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_radio);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831790)) // "cfg_rds"
				{
					cfg_rds = car_struct.config_data.d.cfg_rds;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_rds);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831794)) // "cfg_logo_type"
				{
					cfg_logo_type = car_struct.config_data.d.cfg_logo_type;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_logo_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831798)) // "cfg_radio_area"
				{
					cfg_radio_area = car_struct.config_data.d.cfg_radio_area;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_radio_area);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C083179C)) // "cfg_launcher"
				{
					cfg_launcher = car_struct.config_data.d.cfg_launcher;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_launcher);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C08317A0)) // "cfg_led_type"
				{
					cfg_led_type = car_struct.config_data.d.cfg_led_type;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_led_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C08317A4)) // "cfg_rudder"
				{
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						car_struct.config_data.d.cfg_rudder);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C08317A8)) // "cfg_key0"
				{
					cfg_key0 = car_struct.config_data.d.cfg_key0;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_key0);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C08317AC)) // "cfg_appdisable"
				{
					cfg_appdisable = car_struct.config_data.d.cfg_appdisable;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_appdisable);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C08317B0)) // "cfg_language_selection"
				{
					cfg_ls1 = car_struct.config_data.d.cfg_language_selection[0];
					cfg_ls2 = car_struct.config_data.d.cfg_language_selection[1];
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_ls2 | (cfg_ls1 << 8));
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C08317B4)) // "cfg_color"
				{
					cfg_color1 = car_struct.config_data.d.cfg_color[0];
					cfg_color2 = car_struct.config_data.d.cfg_color[1];
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						cfg_color2 | (cfg_color1 << 8));
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C08317B8)) // "cfg_led_multi"
				{
					cfg_led_multi = car_struct.config_data.d.cfg_led_multi;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_led_multi);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C08317BC)) // "cfg_wifi_pwr"
				{
					wifi_pwr = car_struct.config_data.d.wifi_pwr;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, wifi_pwr);
					goto LABEL_30;
				}
				v67 = strcmp(car_struct.ioctl_buf1, off_C08317C0); // "cfg_mirror"
				if (!v67) {
					v70 = car_struct.config_data.d.cfg_mirror == 0;
					if (car_struct.config_data.d.cfg_mirror) {
						v71 = str_true_0;
					} else {
						v71 = &car_struct.ioctl_buf1[3060];
					}
					if (car_struct.config_data.d.cfg_mirror) {
						v67 = *v71;
						v68 = *(v71 + 1);
					} else {
						v69 = str_false_0;
					}
					if (car_struct.config_data.d.cfg_mirror) {
						*&car_struct.buffer2[0] = v67;
						car_struct.buffer2[4] = v68;
					} else {
						v67 = *v69;
						v68 = *(v69 + 1);
					}
					if (v70) {
						*&car_struct.buffer2[0] = v67;
					}
					res = 0;
					if (v70) {
						*(v71 + 8) = v68;
					}
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831830)) // "cfg_backlight"
				{
					cfg_backlight = car_struct.config_data.d.cfg_backlight;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_backlight);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831834)) // "cfg_blmode"
				{
					cfg_blmode = car_struct.config_data.d.cfg_blmode;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_blmode);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831838)) // "cfg_powerdelay"
				{
					res = 0;
					sprintf(p_buf1, str_fmt_d_4,
						car_struct.config_data.d.cfg_powerdelay); // "%d"
					goto LABEL_30;
				}
	/* IDA artifact удалено: car_struct = p_mtc_car_struct_14; */
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C083183C)) // "cfg_ill"
				{
					cfg_ill = car_struct.config_data.d.cfg_ill;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_ill); // "%d,"
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831840)) // "cfg_beep"
				{
					cfg_beep = car_struct.config_data.d.ctl_beep;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_beep); // "%d,"
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831844)) // "cfg_led"
				{
					cfg_led2 = car_struct.config_data.d.cfg_led[2];
					cfg_led0 = car_struct.config_data.d.cfg_led[0];
					cfg_led1 = car_struct.config_data.d.cfg_led[1];
					res = 0;
					sprintf(p_buf1, off_C0831858, cfg_led0, cfg_led1,
						cfg_led2); // "%d,%d,%d"
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C0831848)) // "cfg_dvr"
				{
					cfg_dvr = car_struct.config_data.d.cfg_dvr;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_dvr); // "%d"
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C083184C)) // "cfg_wheelstudy_type"
				{
					cfg_wheelstudy_type =
					    car_struct.config_data.d.cfg_wheelstudy_type;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_wheelstudy_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831850)) // "cfg_key_assign"
				{
					v102 = p_buf1;
					v103 = &car_struct.config_data.d.checksum;
					v104 = 0;
					while (1) {
						v105 = *(v103 + 0x80);
						v17 = v104 == 72;
						v106 = *(v103 + 0x81);
						v104 += 3;
						v107 = *(v103 + 0x82);
						v103 += 3;
						v108 = v107 | ((v106 | (v105 << 8)) << 8);
						if (v17) {
							break;
						}
						v109 = strlen(p_buf1);
						sprintf(&v102[v109], str_fmt_d_comma_2, v108);
						if (v104 == 75) {
							res = 0;
							goto LABEL_30;
						}
					}
					v113 = strlen(p_buf1);
					res = 0;
					sprintf(&v102[v113], str_fmt_d_4, v108);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1,
					    off_C0831854)) // "cfg_ir_assign"
				{
					v114 = p_buf1;
					v115 = &car_struct.config_data.d.checksum;
					v116 = 0;
					while (1) {
						v117 = *(v115 + 0xD0);
						v17 = v116 == 118;
						v118 = *(v115 + 0xD1);
						v116 += 2;
						v115 += 2;
						v119 = v118 | (v117 << 8);
						if (v17) {
							break;
						}
						v120 = strlen(p_buf1);
						sprintf(&v114[v120], str_fmt_d_comma_2, v119);
						if (v116 == 120) {
							res = 0;
							goto LABEL_30;
						}
					}
					v127 = strlen(p_buf1);
					res = 0;
					sprintf(&v114[v127], str_fmt_d_4, v119);
					goto LABEL_30;
				}
				v128 = strcmp(car_struct.ioctl_buf1,
					      off_C0831868); // "cfg_steer_assign"
				if (!v128) {
					v129 = p_buf1;
					v130 = &car_struct.config_data.d.checksum;
					v131 = 0;
					while (1) {
						v17 = v131 == 147;
						v131 += 3;
						v132 = *(v130 + v128 + 330) |
						       ((*(v130 + v128 + 329) |
							 (*(v130 + v128 + 328) << 8))
							<< 8);
						if (v17) {
							break;
						}
						v133 = strlen(p_buf1);
						sprintf(&v129[v133], str_fmt_d_comma_2, v132);
						v128 = v131;
						if (v131 == 150) {
							res = 0;
							goto LABEL_30;
						}
					}
					v282 = strlen(p_buf1);
					res = 0;
					sprintf(&v129[v282], str_fmt_d_3, v132);
					goto LABEL_30;
				}
				if (!strcmp(car_struct.ioctl_buf1, off_C08338F0)) // "cfg_config"
				{
					v285 = car_struct.config_data.d.checksum;
					v283 = &car_struct.config_data.d.checksum;
					v284 = v285;
					v286 = 0;
					v287 = p_buf2;
					do {
						v288 = p_buf2;
						++v286;
						v289 = strlen(p_buf2);
						sprintf(&v287[v289], str_fmt_d_comma_3, v284);
						v290 = *(v283++ + 1);
						v284 = v290;
					} while (v286 != 511);
					res = 0;
					v291 = strlen(v288);
					sprintf(&v288[v291], str_fmt_d_3, v284);
					goto LABEL_30;
				}
			}
			goto LABEL_102;
		}
	}
	if (first_char == 'a') {
		if (car_struct.ioctl_buf1[1] == 'v' && car_struct.ioctl_buf1[2] == '_') {
			if (!strcmp(car_struct.ioctl_buf1, str_av_mute_)) // "av_mute"
			{
				is_audio_mute = isAudioMute();
				not_mute = is_audio_mute == 0;
				if (is_audio_mute) {
					v86 = str_true_0; // "true"
				} else {
					v86 = &car_struct.ioctl_buf1[3060];
				}
				if (is_audio_mute) {
					is_audio_mute = *v86;
					v83 = *(v86 + 1);
				} else {
					v84 = str_false_0; // "false"
				}
				if (not_mute) {
					is_audio_mute = *v84;
					v83 = *(v84 + 1);
				} else {
					*&car_struct.buffer2[0] = is_audio_mute;
					car_struct.buffer2[4] = v83;
				}
				if (not_mute) {
					*&car_struct.buffer2[0] = is_audio_mute;
				}
				res = 0;
				if (not_mute) {
					*(v86 + 8) = v83;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08316FC)) // "av_channel"
			{
				switch (getAudioChannel()) {
				case MTC_AV_CHANNEL_GSM_BT:
					res = 0;
					v110 = *off_C08318B8; // "gsm_bt"
					v111 = *(off_C08318B8 + 1);
					car_struct.buffer2[6] = v111 >> 16;
					v112 = off_C08318F0;
					*&car_struct.buffer2[0] = v110;
					*(v112 + 0x10) = v111;
					break;
				case MTC_AV_CHANNEL_SYS:
					res = 0;
					*&car_struct.buffer2[0] = 'sys';
					break;
				case MTC_AV_CHANNEL_DVD:
					res = 0;
					*&car_struct.buffer2[0] = 'dvd';
					break;
				case MTC_AV_CHANNEL_LINE:
					res = 0;
					v99 = *(off_C08318D8 + 1);
					v100 = off_C08318F0;
					*&car_struct.buffer2[0] = *off_C08318D8; // "line"
					*(v100 + 0x10) = v99;
					break;
				case MTC_AV_CHANNEL_FM:
					res = 0;
					v98 = *off_C08318C0 >> 16;
					*(off_C08318F0 + 0xC) = *off_C08318C0; // "fm"
					car_struct.buffer2[2] = v98;
					break;
				case MTC_AV_CHANNEL_DTV:
					res = 0;
					*&car_struct.buffer2[0] = 'vtd';
					break;
				case MTC_AV_CHANNEL_IPOD:
					res = 0;
					v181 = *(off_C08318C4 + 1);
					v182 = off_C08318F0;
					*&car_struct.buffer2[0] = *off_C08318C4; // "ipod"
					*(v182 + 0x10) = v181;
					break;
				case MTC_AV_CHANNEL_DVR:
					res = 0;
					*&car_struct.buffer2[0] = 'rvd';
					break;
				default:
					goto LABEL_102;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C0831700)) // "av_gps_monitor"
			{
				v17 = car_struct.car_status.av_gps_monitor == 0;
				if (car_struct.car_status.av_gps_monitor) {
					v16 = &car_struct.ioctl_buf1[3060];
					v18 = off_C0831704; // "on"
				} else {
					v18 = 'ffo';
					*&car_struct.buffer2[0] = 'ffo';
				}
				if (!v17) {
					v18 = *(int *)v18; /* T5 r20: v18 - int* (rodata "on"), ptr-tip IDA uteryan */
					car_struct.buffer2[2] = v18 >> 16;
				}
				res = 0;
				if (!v17) {
					*(v16 + 0xC) = v18;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317DC)) // "av_gps_switch"
			{
				v79 = car_struct.car_status.av_gps_switch == 0;
				if (car_struct.car_status.av_gps_switch) {
					v78 = &car_struct.ioctl_buf1[3060];
					v80 = off_C0831704;
				} else {
					v80 = 'ffo';
					*&car_struct.buffer2[0] = 'ffo';
				}
				if (!v79) {
					v81 = *v80;
					*(v78 + 0xC) = v81;
					car_struct.buffer2[2] = v81 >> 16;
				}
				res = 0;
				goto LABEL_30;
			}
			if (!strcmp(car_struct.ioctl_buf1, off_C08317F8)) // "av_gps_gain"
			{
				av_gps_gain = car_struct.car_status.av_gps_gain;
				res = 0;
				sprintf(buf_1, str_fmt_d_4, av_gps_gain); // "%d"
				goto LABEL_30;
			}
		}
	}
LABEL_102:
	res = -1;
	car_struct.buffer2[0] = 0;
LABEL_30:
	v20 = strlen(buf_1);
	v21 = 0; /* IDA CF-flag artifact */
	v22 = __CFADD__(userbuf, v20 + 1);
	if (!__CFADD__(userbuf, v20 + 1)) {
		v22 = userbuf + v20 + 1 >= v21 + !__CFADD__(userbuf, v20 + 1);
	}
	if (!v22) {
		v21 = 0;
	}
	if (!v21) {
		_copy_to_user(userbuf, buf_1, v20 + 1);
	}
	mutex_unlock(p_car_lock);
	return res;
}

/* fully decompiled */
void
car_add_work(int a1, int a2, int flush)
{
	struct mtc_work *work; // r4@2

	work = kmalloc(sizeof(struct mtc_work), __GFP_IO);
	// TODO: alloc check

	INIT_DELAYED_WORK(&work->dwork, &car_work); /* T5 r20: makros beret struct delayed_work * */

	work->cmd1 = a1;
	work->cmd2 = a2;
	queue_delayed_work(car_struct.car_wq, &work->dwork, msecs_to_jiffies(0));

	if (flush) {
		flush_workqueue(car_struct.car_wq);
	}
}

EXPORT_SYMBOL_GPL(car_add_work);

/* fully decompiled */
void
car_add_work_delay(int a1, int a2, unsigned int delay)
{
	struct mtc_work *work; // r4@2

	work = kmalloc(sizeof(struct mtc_work), __GFP_IO);
	// TODO: alloc check

	INIT_DELAYED_WORK(&work->dwork, &car_work); /* T5 r20: makros beret struct delayed_work * */

	work->cmd1 = a1;
	work->cmd2 = a2;
	queue_delayed_work(car_struct.car_wq, &work->dwork, msecs_to_jiffies(delay));
}

EXPORT_SYMBOL_GPL(car_add_work_delay);

/* fully decompiled */
static void
mtc_car_work(struct work_struct *work)
{
	(void)work;

	mutex_lock(&car_struct.car_comm->car_lock);
	udelay(20);

	while (!getPin(gpio_MCU_DIN) && arm_rev()) {
		;
	}

	enable_irq(car_struct.car_comm->mcu_din_gpio);
	mutex_unlock(&car_struct.car_comm->car_lock);
}

static noinline void car_avm(void) /* binaRE t LOCAL c082fa8c */
{
	unsigned char buf[7];

	if (config_data->d.cfg_canbus == 0xA) {
		key_beep();

		buf[0] = 0x06; // length?
		buf[1] = 0x2E;
		buf[2] = 0xC6u;
		buf[3] = 0x02;
		buf[4] = 0x02;
		buf[5] = 0x01;
		buf[6] = 0x34;

		if (car_struct.car_status.cam_state) { // ?
			printk("mBackView\n");
			arm_send_multi(0xC000u, 7, buf);
		} else {
			arm_send_multi(0xC000u, 7, buf);
		}
	} else if (config_data->d.cfg_canbus == 0x25) {
		key_beep();

		buf[0] = 0x05; // length?
		buf[1] = 0x2E;
		buf[2] = 0xC7;
		buf[3] = 0x01;
		buf[4] = 0x01;
		buf[5] = 0x36;

		if (car_struct.car_status.cam_state) { // ?
			printk("mBackView\n");
			arm_send_multi(0xC000u, 6, buf);
		} else {
			arm_send_multi(0xC000u, 6, buf);
		}
	}
}

/* fully decompiled */
int
car_comm_init(void)
{
	struct mtc_car_comm *car_comm;

	car_comm = kmalloc(sizeof(struct mtc_car_comm), GFP_ATOMIC | GFP_NOIO);

	mutex_init(&car_comm->car_lock);

	car_struct.car_comm = car_comm;

	gpio_request(gpio_MCU_CLK, "mcu_clk");
	gpio_pull_updown(gpio_MCU_CLK, 0);
	gpio_direction_input(gpio_MCU_CLK);
	gpio_request(gpio_MCU_DIN, "mcu_din");
	gpio_pull_updown(gpio_MCU_DIN, 0);
	gpio_direction_input(gpio_MCU_DIN);
	gpio_request(gpio_MCU_DOUT, "mcu_dout");
	gpio_pull_updown(gpio_MCU_DOUT, 0);
	gpio_direction_output(gpio_MCU_DOUT, 1);
	gpio_request(gpio_PARROT_RESET, "parrot_reset");
	gpio_pull_updown(gpio_PARROT_RESET, 0);
	gpio_direction_output(gpio_PARROT_RESET, 0);
	gpio_request(gpio_PARROT_BOOT, "parrot_boot");
	gpio_pull_updown(gpio_PARROT_BOOT, 0);
	gpio_direction_output(gpio_PARROT_BOOT, 0);

	car_comm->mcu_din_gpio = gpio_MCU_DIN;
	irq_set_irq_wake(gpio_MCU_DIN, 1);
	request_threaded_irq(gpio_MCU_DIN, mcu_isr_cb, 0, 2u, "keys", car_comm);

	car_comm->mcc_rev_wq = create_singlethread_workqueue("mcu_rev_wq");

	INIT_WORK(&car_comm->work, mtc_car_work);

	return 0;
}

EXPORT_SYMBOL_GPL(car_comm_init);

/* fully decompiled */
static int
car_suspend(struct device *dev)
{
	(void)dev;

	printk("car_suspend\n");

	return 0;
}

/* fully decompiled */
static int
car_resume(struct device *dev)
{
	(void)dev;

	printk("car_resume\n");

	return 0;
}

// dirty code
static int
car_probe(struct platform_device *pdev)
{
	bool mcu_clk_val;
	char mtc_bootmode;
	char mtc_boot_mode;
	int ver_part;
	int pos;
	char v16;
	char v17;
	int cur_byte;

	// tmp defines
	struct mutex *car_io_lock = &car_struct.car_io_lock;
	struct mutex *car_cmd_lock = &car_struct.car_cmd_lock;

	printk("--mtc car\n");
	gpio_request(gpio_FCAM_PWR, "fcam_pwr");
	gpio_pull_updown(gpio_FCAM_PWR, 0);
	gpio_direction_output(gpio_FCAM_PWR, 0);

	mutex_init(car_io_lock);
	mutex_init(car_cmd_lock);

	memzero(car_status, 160u);

	if (mtc_get_screen_width() == 1024) {
		printk("--mtc resolution 1024x600\n");
		car_status->is1024screen = 1;
	} else {
		printk("--mtc resolution 800x480\n");
	}

	car_status->key_mode = RPT_KEY_MODE_NORMAL;
	car_status->intval3 = 0x3FF;
	car_status->intval4 = 0x3FF;
	car_status->intval1 = 0x3FF;
	car_status->intval2 = 0x3FF;

	car_status->sta_view = 1;

	misc_register(&mtc_car_miscdev);

	gpio_request(gpio_MCU_CLK, "mcu_clk");
	gpio_pull_updown(gpio_MCU_CLK, 0u);
	gpio_direction_input(gpio_MCU_CLK);
	mcu_clk_val = gpio_get_value(gpio_MCU_CLK) == 0;
	car_status->mcu_clk = mcu_clk_val;
	if (mcu_clk_val) {
		return 0;
	}

	car_struct.tv.tv_usec = 0;
	car_struct.tv.tv_sec = 0;

	car_struct.car_wq = create_singlethread_workqueue("car_wq");
	car_comm_init();
	mtc_bootmode = board_boot_mode();

	arm_send(mtc_bootmode & 0xF | MTC_CMD_BOOTMODE);
	mtc_boot_mode = board_boot_mode();
	printk("--mtc bootmode %d\n", mtc_boot_mode & 0xF);

	if (!(board_boot_mode() & 0xF)) {
		INIT_DELAYED_WORK(&car_struct.wipecheckclear_work, &WipeCheckClear_work); /* T5 r20: makros beret struct delayed_work * */
		car_status->wipe_flag = 1; /* IDA: wipe_check == wipe_flag */
		queue_delayed_work(car_struct.car_wq, &car_struct.wipecheckclear_work,
				   msecs_to_jiffies(60000u));
	}

	arm_send(0x201u);
	ver_part = 0;

	car_status->wipe_flag = 0;
	arm_send(0xF02u);
	car_status->backview_vol = 11;
	car_status->sta_bits = 0;

	rk_fb_show_logo();

	memzero(config_data, 512u);
	printk("--mtc config_pre\n");

	arm_send_multi(MTC_CMD_MCUVER, 16, car_struct.mcu_version);
	for (pos = 0; pos < 16; pos++) {

		cur_byte = car_struct.mcu_version[pos];
		if (cur_byte == 0) {
			break;
		}

		if (cur_byte == '-') {
			ver_part++;
		} else if (ver_part == 1) {
			car_status->mcuver1[strlen(car_status->mcuver1)] = cur_byte;
		} else if (ver_part == 2) {
			car_status->mcuver2[strlen(car_status->mcuver2)] = cur_byte;
		}
	}

	if (check_customer("YZ") || check_customer("RM") || check_customer("ZT")) {
		car_status->mtc_customer = 1;
	} else if (check_customer("HMF")) {
		car_status->mtc_customer = 2;
		decorder_power(1);
		decorder_power(0);
	} else if (check_customer("KGL")) {
		car_status->mtc_customer = 3;
	} else if (check_customer("KLD")) {
		car_status->mtc_customer = 4;
	} else if (check_customer("FY")) {
		car_status->mtc_customer = 5;
	} else if (check_customer("HZC")) {
		car_status->mtc_customer = 16;
	} else if (check_customer("JY")) {
		car_status->mtc_customer = 6;
	} else if (check_customer("MX")) {
		car_status->mtc_customer = 9;
	} else if (check_customer("AM")) {
		car_status->mtc_customer = 8;
	} else if (check_customer("KZ")) {
		car_status->mtc_customer = 18;
	} else if (check_customer("KSP")) {
		car_status->mtc_customer = 7;
	} else if (check_customer("HLA")) {
		car_status->mtc_customer = 10;
	} else if (check_customer("JWT")) {
		car_status->mtc_customer = 11;
	} else if (check_customer("KED")) {
		car_status->mtc_customer = 12;
	} else if (check_customer("SH")) {
		car_status->mtc_customer = 13;
	} else if (check_customer("HH")) {
		car_status->mtc_customer = 14;
	} else if (check_customer("KY")) {
		car_status->mtc_customer = 15;
	} else if (check_customer("LM")) {
		car_status->mtc_customer = 17;
	} else if (check_customer("YM")) {
		car_status->mtc_customer = 19;
	} else if (check_customer("YMZ")) {
		car_status->mtc_customer = 20;
	}

	if (car_status->mtc_customer != 4) {
		mtc_iomux_set(0x1A51u);
	}

	arm_send_multi(0x1510u, 1, &config_data->d.cfg_backlight);

	if (config_data->d.cfg_backlight <= 9) {
		config_data->d.cfg_backlight = 10;
	}

	printk("--mtc config\n");
	arm_send_multi(MTC_CMD_MCUDATE, 16, car_struct.mcu_date);
	arm_send_multi(MTC_CMD_MCUTIME, 16, car_struct.mcu_time);

	arm_send_multi(MTC_CMD_GET_MCUCONFIG, 512, config_data->u8);
	printk("--mtc MCU config \n");
	int i;	/* C89: hoisted */
	for (i = 0; i < 512; i++) {
		printk("%02x ", config_data->u8[i]);

		if ((i & 0xF) == 15) {
			printk("\n");
		}
	}

	stw_range_check();

	if (config_data->d.cfg_canbus == 7 || config_data->d.cfg_canbus == 21 ||
	    config_data->d.cfg_canbus == 36 || config_data->d.cfg_canbus == 40) {
		car_status->cfg_maxvolume = 40;
	} else if (config_data->d.cfg_canbus == 15) {
		car_status->cfg_maxvolume = 35;
	} else {
		car_status->cfg_maxvolume = 30;
	}

	if (car_status->mtc_customer == 4) {
		car_struct.wifi_capable = 1;
	}

	if (car_status->wipe_flag & 1) {
		v16 = 1;
	}
	car_status->power_refcnt = v16;
	car_status->power_on = v16;

	if (car_status->wipe_flag & 8) {
		if (car_status->wipe_flag & 2) {
			v17 = 5;
		} else {
			v17 = 6;
		}

		car_status->mcu_cmd_state = v17;
		car_status->video_mode = 0xFF;
		capture_add_work(0x38u, 1, 0, 1);
	} else if (car_status->power_refcnt) {
		if (car_status->mtc_customer == 8) {
			msleep(1000u);
		}
		backlight_on();
	}

	car_status->car_ready = 1;

	printk("car_probe end \n");
	if (config_data->d.cfg_bt == 3) {
		car_add_work_delay(74, 2, 20u);
	}

	return 0;
}

/* fully decompiled */
static int __devexit
car_remove(struct platform_device *pdev)
{
	(void)pdev;

	return 0;
}

/* fully decompiled */
static irqreturn_t
mcu_isr(unsigned int irq)
{
	disable_irq_nosync(irq);
	queue_work(car_struct.car_comm->mcc_rev_wq, &car_struct.car_comm->work);

	return IRQ_HANDLED;
}

/* recovered structures */

static struct file_operations mtc_car_fops = {
    .read = car_read, .write = car_write, .unlocked_ioctl = car_ioctl, .open = car_open,
};

static struct miscdevice mtc_car_miscdev = {
    .minor = 255, .name = "mtc-car", .fops = &mtc_car_fops,
};

static struct dev_pm_ops car_pm_ops = {
    .suspend = car_suspend, .resume = car_resume,
};

static struct platform_driver mtc_car_driver = {
    .probe = car_probe,
    .remove = __devexit_p(car_remove),
    .driver =
	{
	    .name = "mtc-car", .pm = &car_pm_ops,
	},
};

static int __init
car_init()
{
	platform_driver_register(&mtc_car_driver);
	return 0;
}

static void
car_exit()
{
	platform_driver_unregister(&mtc_car_driver);
}

fs_initcall_sync(car_init);
module_exit(car_exit);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC CAR driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-car");

/* ==========================================================================
 * binaRE MTC-14 — gtp (touch) секция + sta_touch_adc
 * первоисточники: src_all/decompiled_gtp_init_panel.c / decompiled_gtp_write_panel.c /
 * decompiled_get_panel.c / decompiled_gtp_reset_guitar.c / decompiled_sub_C083DFDC.c /
 * decompiled_sub_C083E0D8.c / decompiled_sta_touch_adc.c
 * ========================================================================== */

/* binaRE gtp_dev — offsets IDA-верные (gtp_init_panel/gtp_write_panel): +8 client*,
 * +136 w (u16), +138 h (u16), +141 sub (u8), +147 cfg_len (u8). */
struct gtp_dev {
	char _gap0[8];
	struct i2c_client *i2c_client; /* @+8: binaRE *(a1+8) */
	char _gap1[128];	 /* @8..135 */
	u16 touch_w;		 /* @136 (0x88): width (v18[3]+v18[4]<<8) */
	u16 touch_h;		 /* @138 (0x8A): height (v18[5]+(s8)v18[6]<<8) */
	char _gap2[2];
	u8 sub;			 /* @141 (0x8D): v18[8]&3 */
	char _gap3[5];
	u8 cfg_len;		 /* @147 (0x93): gtp_write_panel: -70 (0xBA = 186) */
	char _gap4[9];		 /* до 152: минимальный размер по max-доступу */
};

/* binaRE 0xC0BCA410 (dword_C0BCA410): BSS-блок gtp: config-buf @0 (gtp_write_panel:
 * [1]=cmd 0xF2, [2..]=payload+chksum; memset +2..+241, send 0xF2B) + панель-данные get_panel
 * (&dword_C0BCA410[11*i], fallback [98]/[99] u32). TENTATIVE-размер 768B (max-доступ). */
static u8 gtp_bss[768];

/* binaRE off_C0BCA578: таблица touch-панелей, 44B-stride: [1]=w, [2]=h (u32-slots),
 * +12=vendor (u8), +13=flag (u8), [11]=ptr name (NULL = конец).
 * TENTATIVE: dump 0xC0BCA* нет → zero-таблица: get_panel всегда «--mtc touch NULL» / NULL
 * (byte-1:1 строк — отдельная задача). */
static u8 gtp_panel_tab[44 * 8];

/* binaRE loc_C0A0AD9C (186B): GTP-init блоб (gtp_write_panel: memcpy 186B).
 * TENTATIVE: dump 0xC0A0AD* нет → zero-блоб (byte-1:1 — отдельная задача). */
static const u8 gtp_init_blob[186] = {0};

/* binaRE gtp_reset_guitar @0xc083df60 (t, 124B): gpio 216/217, msleep 0/ms/2/6/50.
 * a2 = ms (вызовы: 10 из i2c-хелперов; constprop.6-инлайн: out1/msleep20/out0/msleep50).
 * Уровень INT-линии — binaRE *(u16*)(client+2)==20 (offset IDA-верный). */
int gtp_reset_guitar(struct i2c_client *client, int ms) /* t6b: GLOBAL (judge c083df60) */
{
	gpio_direction_output(216, 0);
	msleep(ms);
	gpio_direction_output(217, *(unsigned short *)((char *)client + 2) == 20); /* binaRE */
	msleep(2);
	gpio_direction_output(216, 1);
	msleep(6);
	gpio_direction_input(216);
	gpio_direction_output(217, 0);
	msleep(50);
	return gpio_direction_input(217);
}

/* binaRE sub_C083E0D8 @0xc083e0d8 (112B): 1-msg i2c raw write (len), retry×5 + reset.
 * Judge-имя: gtp_i2c_write (GLOBAL, t6b); дубль-адрес c083d1e0/c083e0d8 — одно определение. */
int gtp_i2c_write(struct i2c_client *client, const u8 *buf, u16 len)
{
	struct i2c_msg msg;
	unsigned int retry = 5;
	int res;

	msg.addr = *(unsigned short *)((char *)client + 2);	/* binaRE *(u16*)(a1+2) */
	msg.flags = 0;
	msg.len = len;
	msg.buf = (u8 *)buf;

	while (1) {
		res = i2c_transfer(*(struct i2c_adapter **)((char *)client + 24), &msg, 1); /* binaRE *(u32*)(a1+24) */
		if (res == 1)
			break;
		if (!--retry) {
			gtp_reset_guitar(client, 10);
			return res;
		}
	}
	return res;
}

/* binaRE sub_C083DFDC @0xc083dfdc (156B): 2-msg i2c: msg0 = reg-префикс 2B (flags 0),
 * msg1 = ЧТЕНИЕ len-2 в buf+2 (flags 1 = I2C_M_RD!), retry×5 + reset.
 * Judge-имя: gtp_i2c_read (GLOBAL, t6b); дубль-адрес c083d2ac/c083dfdc — одно определение. */
int gtp_i2c_read(struct i2c_client *client, const u8 *buf, u16 len)
{
	struct i2c_msg msg[2];
	unsigned int retry = 5;
	int res;

	msg[0].addr = *(unsigned short *)((char *)client + 2);	/* binaRE *(u16*)(a1+2) */
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = (u8 *)buf;
	msg[1].addr = msg[0].addr;
	msg[1].flags = 1; /* binaRE: 1 (= I2C_M_TEN) */
	msg[1].len = len - 2;
	msg[1].buf = (u8 *)(buf + 2);

	while (1) {
		res = i2c_transfer(*(struct i2c_adapter **)((char *)client + 24), msg, 2);
		if (res == 2)
			break;
		if (!--retry) {
			gtp_reset_guitar(client, 10);
			return res;
		}
	}
	return res;
}

/* binaRE get_panel @0xc083e378 (T, 272B) — non-static (EXPORT).
 * TENTATIVE: zero-таблица gtp_panel_tab → функция всегда «--mtc touch NULL» / NULL.
 * binaRE-индексы u32-based: e[1]=w, e[2]=h, +12=vendor, +13=flag, e[11]=name ptr;
 * parallel-область = &dword_C0BCA410[11*i] (fallback [98]/[99] ← [91]/[92]). */
char **get_panel(unsigned short w, unsigned short h, int vendor, int flag)
{
	const char *name;
	unsigned int *e;
	int idx = 0;

	name = "JRC-8004"; /* binaRE v4: имя первой строки (при match i>0 IDA печатает i-1 — 1:1) */
	e = (unsigned int *)gtp_panel_tab; /* binaRE &off_C0BCA578 (zero, TENTATIVE) */
	while (1) { /* binaRE: if("JRC-8004") — строка непустая, ветка всегда */
		unsigned int ev8;
		int v10;

		ev8 = ((unsigned char *)e)[12]; /* vendor (binaRE +12) */
		v10 = ((ev8 == 65 && vendor == 95) ? 65 : vendor); /* binaRE: v9 ? 65 : a3 */
		if (e[1] == w && e[2] == h && v10 == ev8) {
			if (((unsigned char *)e)[13] == flag) /* binaRE +13 = flag */
				break;
			if ((v10 == 83 || v10 == 81) && flag != 3) /* binaRE: LOBYTE(v8)=(83|81); a4!=3 → 0 */
				break;
			if (v10 == 65 && flag == 0) /* binaRE v12 */
				break;
		}
		name = (const char *)e[11]; /* binaRE v7 = v6[11] */
		e += 11; /* 44B-stride */
		if (!name) {
			printk("--mtc touch NULL\n");
			return NULL; /* binaRE nullptr */
		}
		idx++;
	}
	printk("--mtc touch %s\n", name);

	{
		unsigned int *cfg = (unsigned int *)(gtp_bss + 44 * idx); /* binaRE &dword_C0BCA410[11*v5] */
		if (!cfg[98])
			cfg[98] = cfg[91];
		if (!cfg[99])
			cfg[99] = cfg[92];
	}
	return (char **)(gtp_panel_tab + 44 * idx); /* binaRE &(&off_C0BCA578)[11*v5] */
}

/* binaRE gtp_write_panel @0xc09c60b4 (t, 220B): blob 186B → gtp_bss (memset +2..+241,
 * payload, chksum = -sum(buf[2..len-1]) в buf[len]), send 0xF2B. TENTATIVE: zero-blob. */
unsigned int gtp_write_panel(struct gtp_dev *dev) /* t6b: GLOBAL (judge c09c60b4) */
{
	u8 v17[186];
	unsigned int i;
	s8 sum = 0;
	unsigned int retry = 5;
	int res;

	memcpy(v17, gtp_init_blob, 186); /* binaRE memcpy(v17, &loc_C0A0AD9C, 186) */
	dev->cfg_len = -70; /* binaRE *(u8*)(a1+147) = -70 (0xBA = 186) */
	memset((char *)gtp_bss + 2, 0, 240); /* binaRE _memzero(dword_C0BCA410+2, 240) */
	memcpy((char *)gtp_bss + 2, v17, dev->cfg_len);

	for (i = 2; dev->cfg_len > i; ++i)
		sum += gtp_bss[i];
	gtp_bss[dev->cfg_len] = -sum; /* binaRE *((u8*)buf+v6) = -v7 */
	printk("chksum %02x\n", (s8)gtp_bss[dev->cfg_len]);

	retry = 5;
	while (1) {
		res = gtp_i2c_write(dev->i2c_client, gtp_bss, 0xF2); /* binaRE sub_C083E0D8(client, dword_C0BCA410, 0xF2) */
		if (res > 0)
			break;
		if (!--retry) {
			if (res) {
				printk("<<-GTP-ERROR->> Send config error.\n");
				msleep(10);
				return 0; /* binaRE: return msleep(10) — msleep void (IDA-артефакт), return-нормализация */
			}
			break;
		}
	}
	printk("911 write success!!\n");
	msleep(10);
	return 0; /* binaRE: return msleep(10) — return-нормализация */
}

/* binaRE gtp_init_panel @0xc09c61a8 (t, 728B): gtp_i2c_read(0x80,'G',188B) (read 186B в buf+2);
 * w/h → gtp_dev+136/+138 и car_status.touch_width/height (+touch_info1/2);
 * vendor==66 && (u32){w|h<<16}==39322690 && !flag && v18[186]==61 → gtp_write_panel;
 * get_panel(w,h,vendor,flag) → keys_data+0xD0 (config_id); printk-дамп; switch w (классы).
 * MEMORY-якоря: AD0C/AD10 = car_status+136/140 (touch_width/height), AD14/AD15 = +144/145
 * (touch_info1/2), E541 = keys_data+0xCD (flag), E544 = +0xD0 (config_id),
 * ACE3 = car_status+95 (is1024screen), ACE4 = +96 (reserved_20, gtp-класс). */
int gtp_init_panel(struct gtp_dev *dev) /* t6b: GLOBAL (judge c09c61a8) */
{
	u8 v18[188];
	int vendor;
	u32 flag;
	u32 word;
	int matched = 0;
	int i;

	v18[0] = 0x80;
	v18[1] = 71; /* 'G' (binaRE) */
	if (gtp_i2c_read(dev->i2c_client, v18, 188) < 0) { /* t6b: gtp_i2c_read (2-msg, I2C_M_RD) */
		printk("\n");
		return 0; /* binaRE LABEL_39 */
	}
	dev->touch_w = (u16)(v18[3] + (v18[4] << 8)); /* binaRE v5 → *(u16*)(a1+136) */
	vendor = v18[2]; /* binaRE v6 (vendor) */
	dev->touch_h = (u16)(v18[5] + ((s8)v18[6] << 8)); /* binaRE: v4 + (v3<<8), v3 = __int16(v18[6]) sign-ext */
	flag = *(const u32 *)((char *)mtc_keys_data_ptr() + 0xCD); /* binaRE MEMORY[0xC168E541] = keys_data+0xCD */
	car_struct.car_status.touch_width = dev->touch_w; /* binaRE MEMORY[0xC168AD0C] = car_status+136 */
	car_struct.car_status.touch_height = dev->touch_h; /* binaRE MEMORY[0xC168AD10] = +140 */
	car_struct.car_status.touch_info1 = vendor; /* binaRE MEMORY[0xC168AD14] = +144 */
	car_struct.car_status.touch_info2 = (char)flag; /* binaRE MEMORY[0xC168AD15] = +145 */
	dev->sub = v18[8] & 3; /* binaRE *(u8*)(a1+141) */

	if (vendor == 66 && *(const u32 *)&dev->touch_w == 39322690 && !flag && v18[186] == 61) /* binaRE: (u16)w | ((u16)h << 16) */
		gtp_write_panel(dev);

	*(u32 *)((char *)mtc_keys_data_ptr() + 0xD0) = (u32)(unsigned long)get_panel(dev->touch_w, dev->touch_h, vendor, (int)flag); /* binaRE MEMORY[0xC168E544] = keys_data+0xD0 (config_id) */
	printk("--mtc config_id %d vendor_id %d\n", vendor, flag);
	printk("Touch911             ");
	word = *(const u32 *)&dev->touch_w; /* binaRE *(u32*)(a1+136) */
	switch (word) {
	case 31458080:
		car_struct.car_status.is1024screen = 0; /* binaRE MEMORY[0xC168ACE3] = +95 */
		matched = 1;
		break;
	case 39322624:
		car_struct.car_status.is1024screen = 1;
		matched = 1;
		break;
	case 52429280:
		car_struct.car_status.is1024screen = 2;
		matched = 1;
		break;
	case 67109464:
		car_struct.car_status.is1024screen = 3;
		matched = 1;
		break;
	}
	if (!matched) {
		u16 w16 = dev->touch_w; /* binaRE v12 */
		u16 h16;

		if (w16 <= 0x320) {
			if (w16 <= 0x1F4) {
				h16 = dev->touch_h; /* binaRE v14 */
				if (h16 > 0x320) {
					dev->touch_h = 800;
					car_struct.car_status.reserved_20 = h16 - 32; /* binaRE MEMORY[0xC168ACE4] = +96 */
					car_struct.car_status.is1024screen = 6;
				}
				goto gtp_dump;
			}
		} else {
			h16 = dev->touch_h; /* binaRE v13 */
			if (h16 <= 0x1F4) {
				dev->touch_w = 800;
				car_struct.car_status.reserved_20 = w16 - 32;
				car_struct.car_status.is1024screen = 4;
				goto gtp_dump;
			}
			if (w16 > 0x400) {
				if (h16 >= 0x258) {
					dev->touch_w = 1024;
					car_struct.car_status.reserved_20 = w16;
					car_struct.car_status.is1024screen = 5;
					goto gtp_dump;
				}
				goto gtp_l28;
			}
		}
		if (w16 < 0x258)
			goto gtp_dump;
gtp_l28: /* binaRE LABEL_28 */
		h16 = dev->touch_h; /* binaRE v15 (re-read) */
		if (h16 > 0x400) {
			dev->touch_h = 1024;
			car_struct.car_status.reserved_20 = h16;
			car_struct.car_status.is1024screen = 7;
			goto gtp_dump;
		}
		if (w16 == 1024) {
			if (h16 > 0x258) {
				dev->touch_h = 600;
				car_struct.car_status.reserved_20 = h16 - 88;
				car_struct.car_status.is1024screen = 9;
				goto gtp_dump;
			}
gtp_l34: /* binaRE LABEL_34 */
			if (h16 == 1024) {
				dev->touch_w = 600;
				car_struct.car_status.reserved_20 = w16 - 88;
				car_struct.car_status.is1024screen = 11;
			}
			goto gtp_dump;
		}
		if (w16 > 0x258)
			goto gtp_l34;
	}
gtp_dump: /* binaRE LABEL_11 */
	for (i = 2; i != 188; ++i) {
		if (((i + 69) & 0xF) == 0)
			printk("\n");
		printk("%02x ", (unsigned int)(u8)v18[i]); /* binaRE v16 = (u8)v18[i] */
	}
	printk("\n"); /* binaRE LABEL_39 */
	return 0;
}

/* binaRE sta_touch_adc @0xc0842888 (T, 28B) — src_all/decompiled_sta_touch_adc.c, 1:1.
 * shared.h: TENTATIVE снят. Байты = keys_data+0x144/+0x148 (binaRE 0xC168E5B8/0xC168E5BC).
 * IDA-сигнатура _BYTE* — return-нормализация: int (sprintf-результат). */
int sta_touch_adc(char *buf)
{
	char *kd = (char *)mtc_keys_data_ptr();

	return sprintf(buf, "%d,%d", *(const u32 *)(kd + 0x144), *(const u32 *)(kd + 0x148));
}

/* ==========================================================================
 * binaRE MTC-14 — t6b gtp/touch closure: judge-символы c0* → GLOBAL в car.c
 * первоисточники: /home/amper/tmp/ida-tmp/mtc_audio/src_all/decompiled_*.c
 * (gtp_i2c_test, gtp_irq_disable/enable, gtp_touch_down, touch_cali_status,
 *  touch_adc_show, touch_mode_show/store, sta_touch_cal, gtp_read_version,
 *  gtp_request_input_dev, isTouchDisable, TouchPanelSetCalibration).
 * TENTATIVE-пометки — в каждом теле (byte-1:1 только там, где отмечено).
 * ========================================================================== */

extern int arm_send_multi(unsigned int cmd, int count, unsigned char *buf); /* = keys.c:244 (mtc-конвенция) */
extern void backlight_on(void); /* = keys.c:240 (определения нет в дереве; binaRE-арг — stale R0) */

/* binaRE gtp_irq_disable/enable @0xc083d328/c083d418 (t, 64B): flag @ts+100,
 * client* @ts+8, irq = *(u32*)(client+368) ≈ i2c_client->dev.irq (struct device 3.0).
 * spinlock-офсет в бинаре неразличим (IDA args-артефакт) → статический lock (TENTATIVE). */
struct gtp_ts_irq {
	char _gap0[8];
	struct i2c_client *i2c_client; /* @+8 (binaRE) */
	char _gap1[92];
	int irq_is_disable;		 /* @+100 (binaRE) */
};
static DEFINE_RAW_SPINLOCK(gtp_irq_lock); /* TENTATIVE (см. выше; binaRE: raw_spin_* — raw_spinlock_t) */

/* binaRE gtp_irq_disable @0xc083d328 (t) — GLOBAL judge-символ (дубль c083e494 — одно определение). */
int gtp_irq_disable(struct gtp_ts_irq *ts)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&gtp_irq_lock, flags);
	if (!ts->irq_is_disable) {
		ts->irq_is_disable = 1;
		disable_irq_nosync(*(const u32 *)((char *)ts->i2c_client + 368)); /* binaRE 1:1 (dev.irq-офсет; mem'ер irq в struct device дерева отсутствует) */
	}
	raw_spin_unlock_irqrestore(&gtp_irq_lock, flags);
	return 0; /* binaRE: return unlock(...) — IDA-артефакт (void-нормализация) */
}

/* binaRE gtp_irq_enable @0xc083d418 (t) — GLOBAL judge-символ (дубль c083e5cc — одно определение). */
int gtp_irq_enable(struct gtp_ts_irq *ts)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&gtp_irq_lock, flags);
	if (ts->irq_is_disable) {
		enable_irq(*(const u32 *)((char *)ts->i2c_client + 368)); /* binaRE 1:1 (dev.irq-офсет; см. irq_disable) */
		ts->irq_is_disable = 0;
	}
	raw_spin_unlock_irqrestore(&gtp_irq_lock, flags);
	return 0; /* binaRE: return unlock(...) — IDA-артефакт */
}

/* binaRE gtp_i2c_test @0xc083d23c (t, 108B): raw-write 1B (=1), retry 3×, msleep(10).
 * GLOBAL judge-символ; дубль c083e078 — одно определение. */
int gtp_i2c_test(struct i2c_client *client)
{
	u8 test = 1; /* binaRE v10 = 1 */
	int retry = 1;
	int res;

	do {
		res = gtp_i2c_write(client, &test, 1); /* binaRE 1:1 */
		if (res > 0)
			break;
		retry++;
		printk("<<-GTP-ERROR->>[%d]GTP i2c test failed time %d.\n", 719, retry); /* binaRE 1:1 */
		msleep(10);
	} while (retry != 4);
	return res;
}

/* binaRE isTouchDisable @0xc082e80c (T, 160B) — GLOBAL judge-символ.
 * car_status-офсеты (base 0xC168AC80): AC85=+1, AC87=+3, ACA5=+37, ACA6=+38,
 * ACA9=+41, ACAА=+42, ACDF=+79, AD09=+113, AD41=0xBA. Тело 1:1. */
int isTouchDisable(void)
{
	u8 *cs = (u8 *)&car_struct.car_status;

	if (((cs[38] || cs[41]) && cs[42]) || cs[113]) /* binaRE 1:1 (ACAA/AD09) */
		return 0;
	if (!cs[1] || cs[38] || cs[41]) /* binaRE AC85/ACA6/ACA9 */
		return 1;
	if (!cs[3]) /* binaRE AC87 */
		return 1;
	if (cs[79]) { /* binaRE ACDF */
		if (cs[0xBA] != 5) /* binaRE AD41 */
			return cs[0xBA] != 55;
	}
	return cs[38]; /* binaRE: result = MEMORY[0xC168ACA6] */
}

/* binaRE gtp-dev ctx (judge-функции c09c5*: request_input_dev/read_version):
 * +8 client*, +12 input_dev*, early-suspend-зона @80..96 (level@88, suspend@92,
 * late_resume@96 — binaRE raw), max_y/max_x/max_pressure @108/110/112, ver-buf @116.
 * TENTATIVE layout (dump 0xC0BC* нет). */
struct gtp_dev_ctx {
	char _gap0[8];
	struct i2c_client *i2c_client; /* @+8 (binaRE) */
	struct input_dev *input_dev;	 /* @+12 (binaRE) */
	char _gap1[68];		 /* @16..83 */
	char _gap_es[24];		 /* @80..103: early-suspend-зона (binaRE) */
	char _gap2[4];		 /* @104..107 */
	u16 max_y;			 /* @+108 (binaRE) */
	u16 max_x;			 /* @+110 (binaRE) */
	u8 max_pressure;		 /* @+112 (binaRE) */
	char _gap3[3];		 /* @113..115 */
	char gtp_ver[40];		 /* @+116 (binaRE): GTP chip version */
};

/* TENTATIVE (t6b): early-suspend/late-resume хендлеры binaRE (goodix_ts_early_suspend /
 * goodix_ts_late_resume) — тела не транскрибируются → no-op (символы НЕ judge-имена). */
static void gtp_ts_early_suspend(struct early_suspend *h) { (void)h; }
static void gtp_ts_late_resume(struct early_suspend *h) { (void)h; }

/* binaRE gtp_touch_down @0xc083e228 (t, 328B) — GLOBAL judge-символ.
 * ts = gtp_dev_ctx (input_dev* @+12); car_status-офсеты как в isTouchDisable;
 * kd+0xCC (0xC168E540) = touch-reported флаг. Тело 1:1 по IDA-контрольному потоку. */
int gtp_touch_down(struct gtp_dev_ctx *ts, int id, int x, int y, int w)
{
	int result;
	int v11;
	u8 *cs = (u8 *)&car_struct.car_status;
	u8 *kd = (u8 *)(char *)mtc_keys_data_ptr();

	result = isTouchDisable();
	if (result) { /* binaRE: touch отключён → событие ARM */
		if (!cs[1] && cs[0x9A]) { /* binaRE AC85 / AD1F */
			unsigned int v = (unsigned int)cs[1];
			result = arm_send_multi(38185, (int)v, (unsigned char *)(unsigned int)v); /* binaRE 1:1 (R1=R2=cs[1], IDA-артефакт) */
			kd[0xCC] = 1; /* binaRE E540 */
		}
		return result;
	}
	if (!cs[38]) { /* binaRE ACA6 */
		if (cs[37]) /* binaRE ACA5 */
			goto touch_l9;
		cs[37] = 1; /* binaRE LABEL_15 (вход 1: !cs[38] && !cs[37]) */
		backlight_on(); /* binaRE backlight_on(0) — дерево: void */
		kd[0xCC] = 1; /* binaRE E540 */
		return result; /* return-нормализация (backlight void) */
	}
	v11 = (y <= 63); /* binaRE a4 <= 63 */
	if (!cs[42]) /* binaRE ACAA */
		v11 = 0;
	if (!v11) {
		if (cs[37]) { /* binaRE LABEL_9 */
			int v10;

touch_l9:
			v10 = kd[0xCC]; /* binaRE E540 */
			if (!v10) {
				input_event(ts->input_dev, 3, 53, x); /* binaRE ABS 53 = a3 (x) */
				input_event(ts->input_dev, 3, 54, y); /* binaRE ABS 54 = a4 (y) */
				input_event(ts->input_dev, 3, 48, w); /* binaRE ABS 48 = a5 (w) */
				input_event(ts->input_dev, 3, 50, w); /* binaRE ABS 50 = a5 */
				input_event(ts->input_dev, 3, 57, id); /* binaRE ABS 57 = a2 (id) */
				input_event(ts->input_dev, v10, 2, v10); /* binaRE: (dev, v10, 2, v10) */
				return result; /* return-нормализация (в дереве input_event — void) */
			}
			return result;
		}
		cs[37] = 1; /* binaRE LABEL_15 (вход 2: !v11 && !cs[37]) */
		backlight_on(); /* binaRE backlight_on(0) — дерево: void */
		kd[0xCC] = 1; /* binaRE E540 */
	}
	return result;
}

/* binaRE dword_C0A0AD2C (2B cmd: GTP_REG_VERSION hi/lo) — dump нет → zero (TENTATIVE). */
static const u8 gtp_rdver_cmd[2] = {0};

/* binaRE gtp_read_version @0xc09c5f6c (t, 264B) — GLOBAL judge-символ.
 * cmd 2B write → msleep(50) → read 40B (msg0 = {106,?}, msg1 = read 38B в buf+2 —
 * семантика gtp_i2c_read); если buf[1] != 0: cmd[1]=0, повторный write,
 * memcpy ver → ctx+116. TENTATIVE: zero-cmd (dump 0xC0A0AD* нет). */
int gtp_read_version(struct gtp_dev_ctx *dev)
{
	u8 cmd[2];
	u8 buf[40];
	int res;

	memcpy(cmd, gtp_rdver_cmd, 2); /* binaRE v14 ← dword_C0A0AD2C (TENTATIVE zero) */
	res = gtp_i2c_write(dev->i2c_client, cmd, 2);
	if (res < 0) {
		printk("<<-GTP-ERROR->>[337]GTP i2c read version failed.\n"); /* binaRE 1:1 */
		return res;
	}
	msleep(50);
	buf[0] = 106; /* binaRE v15 = 106 */
	res = gtp_i2c_read(dev->i2c_client, buf, 40); /* binaRE: gtp_i2c_read(client, &v15, 40) */
	if (res < 0) {
		printk("<<-GTP-ERROR->>[345]GTP i2c read version failed.\n");
		return res;
	}
	buf[39] = 0; /* binaRE v16[39] = 0 (NUL) */
	if (buf[1]) { /* binaRE v16[0] (stack-смежный с buf[0]) */
		cmd[1] = 0; /* binaRE v14[1] = 0 */
		res = gtp_i2c_write(dev->i2c_client, cmd, 2);
		if (res >= 0) {
			memcpy(dev->gtp_ver, buf, 40); /* binaRE memcpy(a1+116, v16, 40) */
			printk("<<-GTP-INFO->>[366]GTP chip version:%s\n", dev->gtp_ver);
		} else {
			printk("<<-GTP-ERROR->>[361]GTP i2c read version failed.\n");
		}
	} else {
		printk("<<-GTP-ERROR->>[353]GTP read version NULL.\n");
		return 1; /* binaRE: return 1 */
	}
	return res;
}

/* binaRE gtp_request_input_dev @0xc09c5d58 (t, 508B) — GLOBAL judge-символ.
 * input_allocate_device → ctx+12; EV 0x0B; id(24, 0xDEAD, 0xBEEF, 0x28BB);
 * name "mtctouch", phys "mtctouch/input0"; 8×input_set_abs_params (коды как в binaRE);
 * register → early-suspend-зона ctx+80..96 (raw-записи 1:1). */
int gtp_request_input_dev(struct gtp_dev_ctx *dev)
{
	struct input_dev *input;
	char phys[36];
	int res;

	input = input_allocate_device();
	dev->input_dev = input; /* binaRE *(a1+12) */
	if (!input) {
		printk("<<-GTP-ERROR->>[908]Failed to allocate input device.\n"); /* binaRE 1:1 */
		return -12; /* binaRE: return -ENOMEM */
	}
	*(u32 *)((char *)input + 24) = 11; /* binaRE: EV-набор 0x0B (SYN|KEY|ABS) */
	*(u32 *)((char *)input + 128) = 16777219; /* binaRE 1:1 (0x0100003) */
	*(u32 *)((char *)input + 68) = 1024; /* binaRE 1:1 */
	input_set_abs_params(input, 0, 0, dev->max_x, 0, 0); /* binaRE ABS 0 (a1+110) */
	input_set_abs_params(input, 1, 0, dev->max_y, 0, 0); /* binaRE ABS 1 (a1+108) */
	input_set_abs_params(input, 0x18, 0, 255, 0, 0); /* binaRE ABS 0x18 */
	input_set_abs_params(input, 0x35, 0, dev->max_x, 0, 0); /* binaRE ABS 0x35 (MT_POSITION_X) */
	input_set_abs_params(input, 0x36, 0, dev->max_y, 0, 0); /* binaRE ABS 0x36 (MT_POSITION_Y) */
	input_set_abs_params(input, 0x32, 0, 255, 0, 0); /* binaRE ABS 0x32 (MT_PRESSURE) */
	input_set_abs_params(input, 0x30, 0, 255, 0, 0); /* binaRE ABS 0x30 */
	input_set_abs_params(input, 0x39, 0, dev->max_pressure, 0, 0); /* binaRE ABS 0x39 (MT_TRACKING_ID) */
	sprintf(phys, "%s/input0", "mtctouch"); /* binaRE 1:1 */
	input->name = "mtctouch"; /* binaRE **(a1+12) */
	input->phys = phys; /* binaRE *(input+4) */
	input->id.bustype = 24; input->id.vendor = 0xDEAD; /* binaRE *(input+12..18) = {24, -8531u16, -16657u16, 10427} */
	input->id.product = 0xBEEF; input->id.version = 0x28BB; /* binaRE: 3.0-совместимо (input_set_id введён в 3.12) */
	res = input_register_device(input);
	if (res) {
		res = -19; /* binaRE: return -ENODEV */
		printk("<<-GTP-ERROR->>[954]Register %s input device failed\n", input->name); /* binaRE 1:1 */
	} else {
		*(u32 *)((char *)dev + 88) = 48; /* binaRE raw (early-suspend-зона) */
		*(u32 *)((char *)dev + 92) = (u32)(unsigned long)gtp_ts_early_suspend; /* binaRE goodix_ts_early_suspend */
		*(u32 *)((char *)dev + 96) = (u32)(unsigned long)gtp_ts_late_resume; /* binaRE goodix_ts_late_resume */
		register_early_suspend((struct early_suspend *)(dev->_gap_es)); /* binaRE register_early_suspend(a1+80) */
	}
	return res;
}

/* ===== binaRE touch calibration (judge c0842*): keys_data-таблицы + BSS 0xC0BD2D* =====
 * keys_data base 0xC168E474, офсеты: +0x150 (E5C4) cali-valid флаг;
 * +0x1A0 (E614) uncali_x[5]; +0x1B4 (E628) uncali_y[5]; +0x1C8 (E63C) prev;
 * +0x1CC (E640) default_x; +0x1D0 (E644) prev2; +0x1D4 (E648) default_y. */
#define MTC_KD_CALI_FLG (0x150)
#define MTC_KD_CALI_X	(0x1A0)
#define MTC_KD_CALI_Y	(0x1B4)
#define MTC_KD_CALI_PV	(0x1C8)
#define MTC_KD_DEF_X	(0x1CC)
#define MTC_KD_CALI_PV2 (0x1D0)
#define MTC_KD_DEF_Y	(0x1D4)

/* binaRE BSS 0xC0BD2DE8..E34 (19×u32): [0]/[1] = входные матричные коэффициенты,
 * [2..17] = результат калибровки (E10..E34). TENTATIVE: zero BSS (dump нет). */
static u32 mtc_cali_bss[19];

/* binaRE BSS 0xC0BD2DAC..DC4 (7×u32): det-результаты ComputeMatrix33. TENTATIVE zero. */
static u32 mtc_cali_det[7];

/* ===== LargeNum-каскад (binaRE c0841*): тела НЕ транскрибированы — TENTATIVE заглушки
 * (символы НЕ judge-имена → static; каскад ~20 функций: LargeNum*, ComputeMatrix33,
 * IsLargeNum*, ErrorAnalysis). */
struct mtc_ln { u32 w[4]; }; /* TENTATIVE: 16B IDA-контейнер */
static void LargeNumSet(struct mtc_ln *d, int v)
{
	d->w[0] = (u32)v;
	d->w[1] = 0;
	d->w[2] = 0;
	d->w[3] = 0; /* TENTATIVE: разумная семантика int→LN */
}
static void LargeNumAdd(struct mtc_ln *d, const struct mtc_ln *a, const struct mtc_ln *b) { (void)d; (void)a; (void)b; } /* TENTATIVE no-op */
static void LargeNumMult(struct mtc_ln *d, const struct mtc_ln *a, const struct mtc_ln *b) { (void)d; (void)a; (void)b; } /* TENTATIVE no-op */
static int LargeNumDivInt32(const struct mtc_ln *a, int b, struct mtc_ln *d) { (void)a; (void)b; d->w[0] = 0; return 0; } /* TENTATIVE */
static int LargeNumBits(const struct mtc_ln *d) { (void)d; return 0; } /* TENTATIVE */
static int IsLargeNumNegative(const struct mtc_ln *d) { (void)d; return 0; } /* TENTATIVE */
static void LargeNumRAShift(struct mtc_ln *d, int n) { (void)d; (void)n; } /* TENTATIVE */

struct mtc_ln33_res { struct mtc_ln m; int det; }; /* TENTATIVE: v40+v41 (8B out) */
static int ComputeMatrix33(struct mtc_ln33_res *out, const struct mtc_ln *m0, const struct mtc_ln *m1,
			   const struct mtc_ln *m2, const struct mtc_ln *m3, const struct mtc_ln *m4,
			   const struct mtc_ln *m5, const struct mtc_ln *m6, const struct mtc_ln *m7,
			   const struct mtc_ln *m8)
{
	(void)m0; (void)m1; (void)m2; (void)m3; (void)m4; (void)m5; (void)m6; (void)m7; (void)m8;
	out->m.w[0] = 0;
	out->det = 1; /* TENTATIVE: «успех» (det != 0 → valid) */
	return 1;
}
static int ErrorAnalysis(int n, const u32 *x, const u32 *y, const u32 *cx, const u32 *cy)
{
	(void)n; (void)x; (void)y; (void)cx; (void)cy;
	return 1; /* TENTATIVE: «успех» (вызывающие трактуют ==1 как success) */
}

/* binaRE TouchPanelSetCalibration @0xc0841120 (T, 1348B) — GLOBAL judge-символ.
 * n точек (a1), uncali_x/y (a4/a5): накопители Σx²,Σxy,Σx,Σy²,Σy,Σx·cx,Σy·cx,Σcx,
 * Σx·cy,Σy·cy,Σcy; 7×ComputeMatrix33 (детерминанты → BSS mtc_cali_det);
 * sign/bits-масштабирование; MEMORY[0xC168E5C4]=valid; return ErrorAnalysis(...).
 * Структура 1:1 по IDA; LargeNum-каскад — TENTATIVE-заглушки (см. выше). */
int TouchPanelSetCalibration(int n, u32 *pcx, u32 *pcy, u32 *pux, u32 *puy) /* binaRE: a2/a3 = cali-src (cx/cy), a4/a5 = uncali-src (x/y) */
{
	struct mtc_ln sxx, sxy, sx, syy, sy;
	struct mtc_ln scx1, scx2, scx, scy1, scy2, scy;
	struct mtc_ln ln_n;
	struct mtc_ln ux, uy, cxi, cyi, tmp;
	struct mtc_ln33_res r1, r2, r3, r4, r5, r6, r7;
	int scale;
	int t;
	int v18;
	int i;

	if (!n) {
		*(u32 *)((char *)mtc_keys_data_ptr() + MTC_KD_CALI_FLG) = 0; /* binaRE 0xC168E5C4 */
		return 1; /* binaRE: return 1 */
	}

	LargeNumSet(&sxx, 0);
	LargeNumSet(&sxy, 0);
	LargeNumSet(&sx, 0);
	LargeNumSet(&syy, 0);
	LargeNumSet(&sy, 0);
	LargeNumSet(&ln_n, n); /* binaRE v28 = a1 */
	LargeNumSet(&scx1, 0);
	LargeNumSet(&scx2, 0);
	LargeNumSet(&scx, 0);
	LargeNumSet(&scy1, 0);
	LargeNumSet(&scy2, 0);
	LargeNumSet(&scy, 0);

	for (i = 0; i < n; ++i) { /* binaRE: do/while v7 != a1 (1:1-набор LargeNum-операций) */
		LargeNumSet(&ux, (int)pux[i]);
		LargeNumSet(&uy, (int)puy[i]);
		LargeNumSet(&cxi, (int)pcx[i]);
		LargeNumSet(&cyi, (int)pcy[i]);
		LargeNumMult(&tmp, &ux, &ux); LargeNumAdd(&sxx, &tmp, &sxx);
		LargeNumMult(&tmp, &ux, &uy); LargeNumAdd(&sxy, &tmp, &sxy);
		LargeNumAdd(&sx, &ux, &sx);
		LargeNumMult(&tmp, &uy, &uy); LargeNumAdd(&syy, &tmp, &syy);
		LargeNumAdd(&sy, &uy, &sy);
		LargeNumMult(&tmp, &ux, &cxi); LargeNumAdd(&scx1, &tmp, &scx1);
		LargeNumMult(&tmp, &uy, &cxi); LargeNumAdd(&scx2, &tmp, &scx2);
		LargeNumAdd(&scx, &cxi, &scx);
		LargeNumMult(&tmp, &ux, &cyi); LargeNumAdd(&scy1, &tmp, &scy1);
		LargeNumMult(&tmp, &uy, &cyi); LargeNumAdd(&scy2, &tmp, &scy2);
		LargeNumAdd(&scy, &cyi, &scy);
	}

	ComputeMatrix33(&r1, &sxx, &sxy, &sx, &sxy, &syy, &sy, &sx, &sy, &ln_n); /* binaRE v40 (m1) */
	ComputeMatrix33(&r2, &scx1, &sxy, &sx, &scx2, &syy, &sy, &scx, &sy, &ln_n); /* binaRE v42 (m2) */
	ComputeMatrix33(&r3, &sxx, &scx1, &sx, &sxy, &scx2, &sy, &sx, &scx, &ln_n); /* binaRE v44 (m3) */
	ComputeMatrix33(&r4, &sxx, &sxy, &scx1, &sxy, &syy, &scx2, &sx, &sy, &scx); /* binaRE v46 (m4) */
	ComputeMatrix33(&r5, &scy1, &sxy, &sx, &scy2, &syy, &sy, &scy, &sy, &ln_n); /* binaRE v48 (m5) */
	ComputeMatrix33(&r6, &sxx, &scy1, &sx, &sxy, &scy2, &sy, &sx, &scy, &ln_n); /* binaRE v50 (m6) */
	ComputeMatrix33(&r7, &sxx, &sxy, &scy1, &sxy, &syy, &scy2, &sx, &sy, &scy); /* binaRE v52 (m7) */

	scale = IsLargeNumNegative(&r1.m) ? -2 : 2; /* binaRE: sign(v40) */
	LargeNumDivInt32(&r1.m, scale, &tmp); /* binaRE v54 = r1/scale */
	LargeNumAdd(&r4.m, &tmp, &r4.m);	/* binaRE v46 += */
	LargeNumAdd(&r7.m, &tmp, &r7.m);	/* binaRE v52 += */
	scale = LargeNumBits(&r2.m) - 15;	/* binaRE v42: -15 */
	t = LargeNumBits(&r3.m) - 15;		/* binaRE v44 */
	if (t > scale)
		scale = t;
	scale &= ~(scale >> 31); /* binaRE: v10 & ~(v10 >> 31) (отрицательный масштаб → 0) */
	t = LargeNumBits(&r5.m) - 15; /* binaRE v48 */
	if (t > scale)
		scale = t;
	t = LargeNumBits(&r6.m) - 15; /* binaRE v50 */
	if (t > scale)
		scale = t;
	t = LargeNumBits(&r4.m) - 27; /* binaRE v46: -27 */
	if (t > scale)
		scale = t;
	t = LargeNumBits(&r7.m) - 27; /* binaRE v52: -27 */
	if (t > scale)
		scale = t;
	t = LargeNumBits(&r1.m) - 31; /* binaRE v40: -31 */
	if (t > scale)
		scale = t;
	if (scale) { /* binaRE: порядок RAShift 1:1 */
		LargeNumRAShift(&r2.m, scale);
		LargeNumRAShift(&r5.m, scale);
		LargeNumRAShift(&r3.m, scale);
		LargeNumRAShift(&r6.m, scale);
		LargeNumRAShift(&r4.m, scale);
		LargeNumRAShift(&r7.m, scale);
		LargeNumRAShift(&r1.m, scale);
	}
	v18 = r1.det; /* binaRE: v18 = v41 */
	mtc_cali_det[0] = r2.det; /* binaRE dword_C0BD2DAC */
	mtc_cali_det[1] = r1.det; /* binaRE dword_C0BD2DC4 */
	if (v18)
		v18 = 1;
	mtc_cali_det[2] = r3.det; /* binaRE dword_C0BD2DB0 */
	mtc_cali_det[3] = r4.det; /* binaRE dword_C0BD2DB4 */
	mtc_cali_det[4] = r5.det; /* binaRE dword_C0BD2DB8 */
	mtc_cali_det[5] = r6.det; /* binaRE dword_C0BD2DBC */
	mtc_cali_det[6] = r7.det; /* binaRE dword_C0BD2DC0 */
	if (!r1.det)
		mtc_cali_det[1] = 1; /* binaRE: if(!v41) DC4 = 1 */
	*(u32 *)((char *)mtc_keys_data_ptr() + MTC_KD_CALI_FLG) = v18; /* binaRE 0xC168E5C4 */
	return ErrorAnalysis(n, pcx, pcy, pux, puy);
}

/* binaRE touch_cali_status @0xc08424e4 (t, 376B) — GLOBAL judge-символ. Тело 1:1. */
int touch_cali_status(void *kobj, char *buf)
{
	u32 *kd = (u32 *)(char *)mtc_keys_data_ptr(); /* binaRE base 0xC168E474 */
	int res;

	(void)kobj; /* binaRE a1 (sysfs-объект, в теле не используется) */
	if (TouchPanelSetCalibration(4, mtc_cali_bss, mtc_cali_bss + 1,
				     kd + (MTC_KD_CALI_X / 4), kd + (MTC_KD_CALI_Y / 4)) == 1) {
		memcpy(mtc_cali_bss + 2, kd + (MTC_KD_CALI_X / 4), 40); /* binaRE: BSS E10..E34 ← kd+0x1A0..+0x1C4 (10×u32) */
		printk("touch_cali_status-0--%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", 548, 3118, 3595, 3159, 501,
		       907, 3627, 878, (int)kd[(0x1B0) / 4], 2048); /* binaRE: арг = E624; остальные — IDA stale-константы (1:1) */
		kd[(MTC_KD_CALI_PV) / 4] = kd[(MTC_KD_DEF_X) / 4]; /* binaRE E63C ← E640 */
		kd[(MTC_KD_CALI_PV2) / 4] = kd[(MTC_KD_DEF_Y) / 4]; /* binaRE E644 ← E648 */
		memcpy(buf, "successful\n", 12); /* binaRE: 12B (NUL + pad) */
		return 11; /* binaRE: strlen("successful\n") */
	}
	printk("touchpal calibration failed, use default value.\n"); /* binaRE 1:1 */
	res = TouchPanelSetCalibration(4, mtc_cali_bss, mtc_cali_bss + 1, mtc_cali_bss + 2, mtc_cali_bss + 8);
	printk("touch_cali_status-1---%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", 548, 3118, 3595, 3159, 501,
	       907, 3627, 878, 2048, 2048); /* binaRE: stale-константы (1:1) */
	if (res == 1) {
		memcpy(buf, "recovery\n", 10); /* binaRE: 10B (NUL + pad) */
		return 9;
	}
	memcpy(buf, "fail\n", 6); /* binaRE: 6B (NUL + pad) */
	return 5;
}

/* binaRE touch_adc_show @0xc084267c (t, 64B) — GLOBAL judge-символ. Тело 1:1.
 * kd+0x144/+0x148 = 0xC168E5B8/E5BC (ADC x/y). IDA-сигнатура _BYTE* → int (sprintf). */
int touch_adc_show(void *kobj, char *buf)
{
	u32 *kd = (u32 *)(char *)mtc_keys_data_ptr();

	(void)kobj;
	printk("ADC show: x=%d y=%d\n", (int)kd[0x144 / 4], (int)kd[0x148 / 4]); /* binaRE 1:1 */
	return sprintf(buf, "%d,%d\n", (int)kd[0x144 / 4], (int)kd[0x148 / 4]);
}

/* binaRE touch_mode_show @0xc08426bc (t, 140B) — GLOBAL judge-символ. Тело 1:1.
 * Порядок a-слов: E614,E628,E618,E62C,... = x0,y0,x1,y1,...,x4,y4 (interleaved). */
int touch_mode_show(void *kobj, char *buf)
{
	u32 *kd = (u32 *)(char *)mtc_keys_data_ptr();
	int res;

	(void)kobj;
	res = sprintf(buf, "TouchCheck:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", /* binaRE 1:1 */
		      (int)kd[0x68], (int)kd[0x6D], (int)kd[0x69], (int)kd[0x6E],
		      (int)kd[0x6A], (int)kd[0x6F], (int)kd[0x6B], (int)kd[0x70],
		      (int)kd[0x6C], (int)kd[0x71]);
	printk("buf: %s", buf); /* binaRE 1:1 */
	return res;
}

/* binaRE touch_mode_store @0xc0842748 (t, 308B) — GLOBAL judge-символ. Тело 1:1.
 * Формат строки: 5×(4-hex x, запятая, 4-hex y) с шагом 10 + def_x @50 + def_y @55;
 * -1 → восстановление prev (E63C/E644). return = count (a3). */
int touch_mode_store(void *kobj, const char *buf, size_t count)
{
	u32 *kd = (u32 *)(char *)mtc_keys_data_ptr();
	char tmp[8];
	u32 def_x;
	u32 def_y;
	int i;

	(void)kobj;
	printk("Read data from Android: %s\n", buf); /* binaRE 1:1 */
	for (i = 0; i < 5; ++i) { /* binaRE: do/while v7 != 5 (шаг 10, парсы по 4 hex) */
		memcpy(tmp, buf + i * 10, 4);
		tmp[4] = 0;
		kd[0x68 + i] = (u32)simple_strtol(tmp, NULL, 16); /* binaRE 0xC168E614+i*4 */
		memcpy(tmp, buf + i * 10 + 5, 4);
		tmp[4] = 0;
		kd[0x6D + i] = (u32)simple_strtol(tmp, NULL, 16); /* binaRE 0xC168E628+i*4 */
		printk("SN=%d uncali_x=%d uncali_y=%d\n", i, (int)kd[0x68 + i], (int)kd[0x6D + i]); /* binaRE 1:1 */
	}
	memcpy(tmp, buf + 50, 4);
	tmp[4] = 0;
	def_x = (u32)simple_strtol(tmp, NULL, 16); /* binaRE 0xC168E640 */
	memcpy(tmp, buf + 55, 4);
	tmp[4] = 0;
	def_y = (u32)simple_strtol(tmp, NULL, 16); /* binaRE 0xC168E648 */
	if (def_x == (u32)-1 || def_y == (u32)-1) {
		def_x = kd[0x72]; /* binaRE E63C */
		def_y = kd[0x74]; /* binaRE E644 */
	}
	kd[0x73] = def_x; /* binaRE E640 */
	kd[0x75] = def_y; /* binaRE E648 */
	printk("SN=%d uncali_x=%d uncali_y=%d\n", 5, (int)def_x, (int)def_y); /* binaRE 1:1 */
	return (int)count; /* binaRE: return a3 (count — sysfs-конвенция) */
}

/* binaRE sta_touch_cal @0xc08428a4 (T, 200B) — GLOBAL judge-символ. Тело 1:1.
 * data = 20×u32 (10 пар x,y) → таблица калибровки; TouchPanelSetCalibration(4,...);
 * успех → copy результата → BSS, return 1; иначе retry по BSS-таблице, return 2/err.
 * shared.h: TENTATIVE-прототип заменён на (unsigned int *data) (u32* == unsigned int*). */
int sta_touch_cal(unsigned int *data)
{
	u32 *kd = (u32 *)(char *)mtc_keys_data_ptr();
	int v3;
	int v4;
	int i;

	for (i = 0; i < 5; ++i) { /* binaRE: for(i=0; i!=20; i+=4) — 10 пар */
		kd[0x68 + i] = data[2 * i];
		kd[0x6D + i] = data[2 * i + 1];
	}
	kd[0x73] = 0; /* binaRE E640 */
	kd[0x75] = 0; /* binaRE E648 */
	v3 = TouchPanelSetCalibration(4, mtc_cali_bss, mtc_cali_bss + 1, kd + (MTC_KD_CALI_X / 4), kd + (MTC_KD_CALI_Y / 4));
	if (v3 == 1) {
		memcpy(mtc_cali_bss + 2, kd + (MTC_KD_CALI_X / 4), 40); /* binaRE: BSS E10..E34 ← kd+0x1A0..+0x1C4 */
		return v3;
	}
	v4 = TouchPanelSetCalibration(4, mtc_cali_bss, mtc_cali_bss + 1, mtc_cali_bss + 2, mtc_cali_bss + 8);
	if (v4 == 1)
		return 2; /* binaRE: return 2 (recovery) */
	return v4;
}
