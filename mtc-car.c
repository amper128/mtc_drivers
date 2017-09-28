// CONFIG_HZ = 100
// xloops = 107374

#include <asm-generic/gpio.h>
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
#include <stdbool.h>

#include "mtc_shared.h"

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
	struct mtc_config_data config_data;
	char _gap1[16];
	struct workqueue_struct *car_wq;
	struct mutex car_io_lock;
	char _gap2[4];
	struct mutex car_cmd_lock;
	char _gap3[12];
	struct delayed_work wipecheckclear_work;
	char mcu_version[16];
	char mcu_date[16];
	char mcu_time[16];
	char _gap4[4];
	unsigned char ioctl_buf1[3072];
	unsigned char buffer2[3072];
	char _gap5[4];
};

static struct mtc_car_struct *mtc_car_struct;

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
getPin(int gpio)
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
getPin2(int gpio)
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

static long
car_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	// too many code
	(void)filp;
	(void)cmd;
	(void)arg;

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

EXPORT_SYMBOL_GPL(arm_parrot_boot)

/* все это очень сильно смахивает на SPI, почему не использовали хардверную
 * шину?? */
/* fully decompiled */
static int
arm_send_cmd(unsigned int cmd)
{
	long clk_timeout;
	long timeout_word;
	unsigned int bit;
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
	mtc_car_struct->rev_bytes_count = 0x10000;
	mtc_car_struct->arm_rev_cmd = cmd;

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

	printk("~ arm_rev_8bits err0 %x %x %d\n", mtc_car_struct->arm_rev_status,
	       mtc_car_struct->arm_rev_cmd, bit_n);

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
			mtc_car_struct->arm_rev_status++;
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
			mtc_car_struct->arm_rev_status = 0x20000;
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

					mtc_car_struct->arm_rev_cmd = arm_rev_cmd;

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

	disable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_lock(&mtc_car_struct->car_comm->car_lock);

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

	enable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_unlock(&mtc_car_struct->car_comm->car_lock);
}
EXPORT_SYMBOL_GPL(arm_send)

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

	disable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_lock(&mtc_car_struct->car_comm->car_lock);
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
					mtc_car_struct->rev_bytes_count += 0x100;
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
	enable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_unlock(&mtc_car_struct->car_comm->car_lock);

	return res;
}

EXPORT_SYMBOL_GPL(arm_send_multi)

/*
 * ==================================
 *	     work functions
 * ==================================
 */

// dirty code
static void
car_work(struct work_struct *work)
{
	struct work_struct *v1;		   // r5@1
	mtc_work *car_work_data;	   // r6@1
	int v4;				   // r0@1
	int v5;				   // r1@1
	int v6;				   // r2@1
	int cmd1;			   // r7@3
	int v8;				   // r3@35
	int v9;				   // r3@44
	struct mtc_car_struct *v10;	// r4@44
	int v11;			   // r3@45
	int v12;			   // r3@49
	unsigned __int8 v13;		   // r3@50
	int v14;			   // r3@55
	int v15;			   // r3@62
	signed int v16;			   // r0@63
	struct mtc_car_struct *car_struct; // r12@69 MAPDST
	int v18;			   // r5@70
	int v20;			   // r3@82
	int v21;			   // r5@85
	int v22;			   // r3@89
	int v23;			   // r3@90
	__int32 v24;			   // r2@93
	int v25;			   // r12@93
	__int32 v26;			   // r0@93
	int v27;			   // r0@94
	int v28;			   // r1@97
	int v29;			   // r0@98
	int v30;			   // r7@121
	int v31;			   // r5@124
	int v32;			   // r5@128
	char v33;			   // [sp+7h] [bp-21h]@25
	struct timeval tv;		   // [sp+8h] [bp-20h]@89

	v1 = work;
	car_struct = p_mtc_car_struct_0;
	car_work_data = CONTAINING_RECORD(work, mtc_work, dwork);
	mutex_lock(&p_mtc_car_struct_0->car_cmd_lock);
	while (!car_struct->car_status._gap0[0]) {
		msleep(10u);
	}
	cmd1 = CONTAINING_RECORD(v1, mtc_work, dwork)->cmd1;
	if (cmd1 == 42) {
		printk(log_mtc_on_d, v1[-1].data.counter);
		car_struct = p_mtc_car_struct_0;
		if (car_struct->car_status._gap1[0] &&
		    (vs_send(2, 0xF1, 0, 0), v30 = v1[-1].data.counter,
		     car_struct->car_status._gap1[0])) // cmd1
		{
			car_struct->car_status._gap0[1] = 1;
			car_struct->car_status._gap9[16] = 1;
			audio_add_work(20, 0, 0, 0);
			capture_add_work(46, 1, 0, 0);
			if (v30) {
				backlight_on();
			}
		} else {
			car_struct->car_status._gap0[1] = 1;
			car_struct->car_status._gap9[16] = 1;
			capture_add_work(47, 0, 0, 0);
			backlight_on();
		}
		if (!v1[0xFFFFFFFF].data.counter) // cmd1
		{
			capture_add_work(56, 1000, 0, 0);
		}
		goto LABEL_33;
	}
	if (cmd1 > 0x2A) {
		if (cmd1 != 69) {
			if (cmd1 > 0x45) {
				if (cmd1 == 73) {
					kernel_power_off();
				} else if (cmd1 > 0x49) {
					if (cmd1 == 74) {
						arm_parrot_boot(LOBYTE(v1[-1].data.counter));
					} else if (cmd1 == 0xFFFF) {
						printk(log_TT); // "--TT\n"
						car_add_work_delay(0xFFFF, 0, 0x7D0u);
					}
				} else if (cmd1 == 70) {
					v15 = v1[-1].data.counter;
					if (v15 <= 31) {
						if (v15 == 1) {
						LABEL_96:
							send_ir_key(18);
						} else if (v15 == 2) {
							send_ir_key(26);
						}
					} else {
						v16 = v15 - 32;
						v1[-1].data.counter = v15 - 32;
						if (v15 == 50 || v16 == 26) {
							send_ir_key(v16);
						}
					}
				} else if (cmd1 == 72 && car_struct->car_status.wipe_check) {
					arm_send_multi(0xA124u, 0, 0);
					kernel_restart(cmd);
				}
				goto LABEL_33;
			}
			if (cmd1 == 45) {
				vs_send(2, -118, 0, 0);
				if (car_struct->car_status._gap5[0]) {
					capture_add_work(59, 0, 0, 1);
				} else if (p_mtc_car_struct_0->car_status._gap5[3]) {
					capture_add_work(55, car_struct->car_status._gap5[0],
							 car_struct->car_status._gap5[0], 1);
				}
				car_struct = p_mtc_car_struct_0;
				if (car_struct->car_status._gap0[1]) {
					p_mtc_car_struct_0->car_status._gap0[1] = 0;
					car_struct->car_status.rpt_power = 1;
					vs_send(2, -16, 0, 0);
					LOBYTE(v31) = 40;
					do {
						msleep(0x28u);
						v31 = (v31 - 1);
					} while (car_struct->car_status.rpt_power && v31);
				}
				LOBYTE(v18) = 80;
				printk(log_mtc_off_acc);
				power_soft_off();
				do {
					msleep(0x32u);
					v18 = (v18 - 1);
				} while (car_struct->car_status.rpt_power && v18);
			} else {
				if (cmd1 > 0x2D) {
					if (cmd1 != 67) {
						if (cmd1 == 68) {
							v33 = v1[0xFFFFFFFF].data.counter;
							vs_send(2, -93, &v33, 1);
						}
						goto LABEL_33;
					}
					vs_send(2, 0xA4, 0, 0);
					v12 = v1[-1].data.counter;
					if (v12 <= 31) {
						goto LABEL_33;
					}
					v13 = v12 - 32;
					if (!v13) {
						arm_send_multi(0x9527u, 0, 0);
						goto LABEL_33;
					}
					if (v13 == 40) {
						arm_send_multi(0x9528u, 0, 0);
						goto LABEL_33;
					}
					if (v13 != 99) {
						if (v13 != 94) {
							send_ir_key(v13);
							goto LABEL_33;
						}
					LABEL_135:
						car_avm();
						goto LABEL_33;
					}
				LABEL_97:
					v28 = car_struct->car_status.backlight_status;
					if (car_struct->car_status.backlight_status) {
						v28 = 0;
						v29 = 36;
					} else {
						v29 = 35;
					}
					car_add_work(v29, v28, v28);
					goto LABEL_33;
				}
				if (cmd1 != 43) {
					if (cmd1 == 44) {
						printk(log_mtc_on_acc);
						vs_send(2, -117, 0, 0);
					}
					goto LABEL_33;
				}
				printk(log_mtc_off);
				v20 = car_struct->car_status._gap5[3];
				car_struct->car_status._gap0[1] = 0;
				if (v20) {
					capture_add_work(55, 0, 0, 1);
				}
				if (car_struct->car_status._gap1[0]) {
					p_mtc_car_struct_0->car_status.rpt_power = 1;
					LOBYTE(v32) = 40;
					vs_send(2, 0xF0, 0, 0);
					do {
						msleep(0x32u);
						v32 = (v32 - 1);
					} while (car_struct->car_status.rpt_power && v32);
				}
				power_soft_off();
				LOBYTE(v21) = 80;
				do {
					msleep(0x32u);
					v21 = (v21 - 1);
				} while (car_struct->car_status.rpt_power && v21);
			}
			arm_send(0x20Bu);
			goto LABEL_33;
		}
		do_gettimeofday(&tv);
		v22 = v1[-1].data.counter;
		if (v22 <= 31) {
		LABEL_93:
			v24 = tv.tv_usec;
			v25 = *&car_struct->_gap3[8];
			v26 = tv.tv_sec - *&car_struct->_gap3[4];
			*&car_struct->_gap3[4] = tv.tv_sec;
			*&car_struct->_gap3[8] = v24;
			if (v24 - v25 + 1000000 * v26 <= 29999) {
				goto LABEL_33;
			}
			v27 = v1[-1].data.counter;
			if (v27 > 31) {
				send_ir_key(v27 - 32);
				goto LABEL_33;
			}
			switch (v27) {
			case 1:
				goto LABEL_96;
			case 2:
				send_ir_key(26);
				goto LABEL_33;
			case 3:
				send_ir_key(58);
				goto LABEL_33;
			case 4:
				send_ir_key(57);
				goto LABEL_33;
			case 5:
				send_ir_key(68);
				goto LABEL_33;
			case 6:
				send_ir_key(3);
				goto LABEL_33;
			case 7:
				send_ir_key(1);
				goto LABEL_33;
			case 8:
				send_ir_key(69);
				goto LABEL_33;
			case 9:
				send_ir_key(5);
				goto LABEL_33;
			case 10:
				send_ir_key(13);
				goto LABEL_33;
			case 11:
				send_ir_key(8);
				goto LABEL_33;
			case 12:
				send_ir_key(10);
				goto LABEL_33;
			case 13:
				send_ir_key(9);
				goto LABEL_33;
			default:
				goto LABEL_33;
			case 16:
				send_ir_key(54);
				goto LABEL_33;
			case 17:
				send_ir_key(66);
				goto LABEL_33;
			case 18:
				send_ir_key(50);
				goto LABEL_33;
			case 19:
				send_event_key(28);
				goto LABEL_33;
			case 20:
				send_event_key(139);
				goto LABEL_33;
			case 21:
				goto LABEL_97;
			}
			goto LABEL_97;
		}
		v23 = (v22 - 32);
		if (v23 == 64) {
			car_struct->car_status._gap9[19] = 1;
			key_beep();
			if (car_struct->car_status.rpt_boot_android) {
				audio_add_work(22, 0, 0, 0);
			}
			if (car_struct->car_status._gap1[0]) {
				vs_send(2, -97, 0, 0);
			}
		} else {
			if (v23 != 65) {
				if (v23 == 94) {
					goto LABEL_135;
				}
				goto LABEL_93;
			}
			car_struct->car_status._gap9[19] = 0;
			key_beep();
			if (car_struct->car_status.rpt_boot_android) {
				if (p_mtc_car_struct_0->car_status._gap9[20] &&
				    p_mtc_car_struct_0->car_status._gap0[1] == 1) {
					backlight_off();
				}
				audio_add_work(23, 0, 0, 0);
			}
			if (car_struct->car_status._gap1[0]) {
				vs_send(2, -96, 0, 0);
			}
		}
		goto LABEL_33;
	}
	if (cmd1 == 33) {
		capture_add_work(60, 0, 0, 0);
		goto LABEL_33;
	}
	if (cmd1 <= 0x21) {
		if (cmd1 != 30) {
			if (cmd1 > 0x1E) {
				if (cmd1 == 31) {
					capture_add_work(56, 0, 0, 0);
				} else if (cmd1 == 32) {
					capture_add_work(57, 0, 0, 0);
				}
				goto LABEL_33;
			}
			if (cmd1 != 29) {
				goto LABEL_33;
			}
			v8 = (car_struct->car_status._gap0[1] + 1);
			car_struct->car_status._gap0[1] = v8;
			if (v8) {
				audio_active(v4, v5, v6);
				backlight_on();
				goto LABEL_33;
			}
		LABEL_36:
			backlight_on();
			goto LABEL_33;
		}
		v9 = car_struct->car_status._gap0[1];
		v10 = p_mtc_car_struct_0;
		if (!v9) {
			goto LABEL_33;
		}
		v11 = (v9 - 1);
		p_mtc_car_struct_0->car_status._gap0[1] = v11;
		if (v11) {
			goto LABEL_33;
		}
		audio_deactive();
		if (v10->car_status._gap5[0]) {
			goto LABEL_33;
		}
	LABEL_47:
		backlight_off();
		goto LABEL_33;
	}
	if (cmd1 == 36) {
		goto LABEL_47;
	}
	if (cmd1 > 0x24) {
		if (cmd1 == 37) {
			arm_send(0x202u);
			v14 = car_struct->car_status._gap0[1];
			car_struct->car_status._gap1[0] = 1;
			if (v14) {
				vs_send(2, -14, 0, 0);
				audio_add_work(20, 0, 0, 0);
			}
			if (car_struct->car_status.wipe_flag & 0x10) {
				vs_send(2, -120, 0, 0);
			}
			if (car_struct->car_status._gap9[19]) {
				vs_send(2, -97, 0, 0);
			}
			capture_add_work(58, 2000, 0, 0);
		} else if (cmd1 == 38 && car_struct->car_status._gap9[19]) {
			audio_add_work(22, 0, 0, 0);
		}
		goto LABEL_33;
	}
	if (cmd1 == 34) {
		capture_add_work(59, 0, 0, 0);
		goto LABEL_33;
	}
	if (cmd1 == 35) {
		goto LABEL_36;
	}
LABEL_33:
	kzfree(car_work_data);
	mutex_unlock(off_C0830630);
}

// very dirty code
static int
car_ioctl(int a1, int a2, unsigned int a3)
{
	struct mtc_car_struct *car_struct;     // r6@1 MAPDST
	int user_cmd;			       // r7@1
	unsigned int userbuf;		       // r4@1
	struct task_struct *v6;		       // r5@5
	int v7;				       // r3@5
	unsigned __int8 v8;		       // cf@5
	char *data_buf;			       // r8@12 MAPDST
	char *equal_last_pos;		       // r0@12
	const char *token_start;	       // r7@13
	size_t slen;			       // r0@14
	int first_char;			       // r3@15 MAPDST
	int v16;			       // r1@22
	char v17;			       // zf@23
	unsigned int v18;		       // r3@24
	signed int res;			       // r7@28
	size_t v20;			       // r0@30
	int v21;			       // r3@30
	unsigned __int8 v22;		       // cf@30
	void *data72b;			       // r5@38
	int v25;			       // r3@40
	unsigned __int8 v26;		       // cf@40
	struct task_struct *cur_task;	  // r5@47
	unsigned __int8 *buf1;		       // r0@47
	int v29;			       // r3@47
	unsigned __int8 v30;		       // cf@47
	int mcu_data_size;		       // r6@53
	void *mcu_data;			       // r0@53 MAPDST
	int v34;			       // r3@54
	unsigned __int8 v35;		       // cf@54
	size_t size;			       // r0@64 MAPDST
	unsigned __int8 *v38;		       // r6@65
	void *data;			       // r5@65 MAPDST
	int v41;			       // r3@67
	unsigned __int8 v42;		       // cf@67
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
	_DWORD *v60;			       // r4@134
	int v61;			       // r6@136
	int v62;			       // r5@136
	int v63;			       // r0@138
	int v67;			       // r0@172
	int v68;			       // r1@172
	char *v69;			       // r2@172
	char v70;			       // zf@173
	char *v71;			       // r3@174
	int v78;			       // r1@220
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
	int v97;			       // r3@277
	int v98;			       // r2@287
	int v99;			       // r1@288
	int v100;			       // r3@288
	char *v102;			       // r6@307
	int v103;			       // r7@307
	int v104;			       // r8@307
	int v105;			       // r10@308
	int v106;			       // r2@308
	int v107;			       // r3@308
	int v108;			       // r10@308
	size_t v109;			       // r0@309
	int v110;			       // r0@312
	unsigned int v111;		       // r1@312
	int v112;			       // r3@312
	size_t v113;			       // r0@313
	char *v114;			       // r6@315
	int v115;			       // r8@315
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
	int v130;			       // r7@325
	int v131;			       // r8@325
	int v132;			       // r10@326
	size_t v133;			       // r0@327
	int cfg_ill;			       // r2@329
	int media_token;		       // r0@334
	int v136;			       // r2@334
	int v137;			       // r3@334
	size_t size_1;			       // r0@344
	_DWORD *v139;			       // r5@345
	int v140;			       // r6@347
	int v141;			       // r4@347
	int v142;			       // r0@348
	struct mtc_config_data *gap12;	 // r7@350
	int v144;			       // r2@350
	_DWORD *v145;			       // r3@350
	int st_pos;			       // r4@350
	int v147;			       // r1@351
	int v148;			       // t1@351
	int v150;			       // r4@357
	int v151;			       // r0@358
	struct mtc_config_data *p_config_data; // r7@360
	int *v153;			       // r3@360
	int cur_byte;			       // r4@360
	int v155;			       // t1@361
	struct mtc_config_data *v156;	  // r3@363
	unsigned __int16 *v157;		       // r2@363
	char *v158;			       // r1@363
	int v159;			       // t1@364
	int led1;			       // r0@367
	char b_led1;			       // r5@367
	int led2;			       // r0@368
	char b_led2;			       // r4@368
	int led3;			       // r0@369
	unsigned __int8 *cfg_led;	      // r2@370
	int enable_beep;		       // r0@371
	int backlight;			       // r0@373
	unsigned __int8 *p_backlight;	  // r2@374
	int powerdelay;			       // r0@375
	unsigned __int8 *p_powerdelay;	 // r2@376
	int blmode;			       // r0@377
	unsigned __int8 *p_blmode;	     // r2@378
	int wifi_pwr;			       // r0@379 MAPDST
	int v174;			       // r0@381
	int cfg_led_multi;		       // r0@383 MAPDST
	int color;			       // r0@385
	unsigned __int8 *cfg_color;	    // r2@386
	signed int v178;		       // r1@389
	int v179;			       // r2@397
	int v180;			       // r2@400
	int v181;			       // r1@403
	int v182;			       // r3@403
	MTC_AV_CHANNEL av_channel;	     // r1@423 MAPDST
	signed int v185;		       // r0@439
	char v186;			       // zf@439
	unsigned int v187;		       // r1@439
	int brightness;			       // r1@472
	char v192;			       // r3@481
	char v193;			       // zf@481
	char *v194;			       // r3@482
	int v195;			       // r0@484
	int v196;			       // r1@484
	int v197;			       // r3@484
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
	int v233;			       // r1@586
	int wipe;			       // r3@587
	char v235;			       // nf@587
	unsigned int *v236;		       // r3@588
	unsigned int v237;		       // r3@591
	int v238;			       // r1@593
	int v239;			       // r3@593
	char v240;			       // r0@596
	int touch_info1;		       // lr@598
	int touch_info2;		       // r12@598
	int touch_w;			       // r2@598
	int touch_h;			       // r3@598
	int uv_cal;			       // r2@605
	int v246;			       // r0@610
	unsigned int v247;		       // r1@610
	int v248;			       // r3@610
	int tv_signal;			       // r0@611
	char *v250;			       // r2@611
	char *v251;			       // r3@611
	char v252;			       // zf@613
	char *v253;			       // r3@617
	int v254;			       // r0@617
	int v255;			       // r1@617
	int v256;			       // r1@625
	int v257;			       // r3@625
	int v258;			       // r1@627
	int v259;			       // r3@627
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
	int v283;			       // r7@662
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
	unsigned __int8 dtv_ir[4];	     // [sp+10h] [bp-90h]@556
	char can_buf[100];		       // [sp+14h] [bp-8Ch]@94

	car_struct = p_mtc_car_struct_12;
	user_cmd = a2;
	userbuf = a3;
	mutex_lock(&p_mtc_car_struct_12->car_io_lock);
	if (user_cmd == 0xFE010000) {
		cur_task = get_current();
		buf1 = car_struct->ioctl_buf1;
		v29 = *(cur_task + 2);
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
		mcu_data_size = (car_struct->ioctl_buf1[1] | (car_struct->ioctl_buf1[0] << 8)) + 2;
		mcu_data = _kmalloc(mcu_data_size, __GFP_ZERO | __GFP_FS | __GFP_IO | __GFP_WAIT);
		if (mcu_data) {
			v34 = *(cur_task + 2);
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
		v41 = *(get_current() + 2);
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
		v25 = *(get_current() + 2);
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
	car_struct->buffer2[0] = 0;
	_memzero(car_struct->ioctl_buf1, 3072u);
	if (user_cmd >= 3072u) {
		goto LABEL_140;
	}
	v6 = get_current();
	v7 = *(v6 + 2);
	v8 = __CFADD__(userbuf, user_cmd);
	if (!__CFADD__(userbuf, user_cmd)) {
		v8 = userbuf + user_cmd >= v7 + !__CFADD__(userbuf, user_cmd);
	}
	if (!v8) {
		v7 = 0;
	}
	if (v7) {
		if (user_cmd) {
			_memzero(car_struct->ioctl_buf1, user_cmd);
		}
	} else {
		_copy_from_user(car_struct->ioctl_buf1, userbuf, user_cmd);
	}
	car_struct = p_mtc_car_struct_12;
	data_buf = p_mtc_car_struct_12->ioctl_buf1;
	equal_last_pos = strrchr(p_mtc_car_struct_12->ioctl_buf1, '=');
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
				car_struct = p_mtc_car_struct_12;
				if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
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
				} else if (!strcmp(car_struct->ioctl_buf1,
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
					v46 = strcmp(car_struct->ioctl_buf1,
						     off_C083171C); // "rpt_boot_android"
					v47 = v46;
					if (v46) {
						v48 = strcmp(car_struct->ioctl_buf1,
							     off_C0831720); // "rpt_boot_appinit"
						v49 = v48;
						if (!v48) {
							if (!strcmp(token_start,
								    off_C083189C)) // "start"
							{
								car_struct->car_status
								    .rpt_boot_appinit = 1;
							} else {
								car_struct->car_status
								    .rpt_boot_appinit = v49; // = 0
							}
							goto LABEL_62;
						}
						if (!strcmp(car_struct->ioctl_buf1,
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
						} else if (!strcmp(car_struct->ioctl_buf1,
								   off_C08317F0)) // "rpt_reboot"
						{
							if (!strcmp(token_start,
								    off_C08317F4)) // "0"
							{
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct->ioctl_buf1,
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
								car_struct->car_status.rpt_power =
								    0;
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct->ioctl_buf1,
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
						car_struct->car_status.rpt_boot_appinit =
						    v47;			    // = 0
						if (!strcmp(token_pos, str_true_0)) // "true"
						{
							arm_send(MTC_CMD_BOOT_ANDROID);
							car_struct->car_status.rpt_boot_android = 1;
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
					car_struct->car_status.av_channel_flag1 = 0;
					av_channel = MTC_AV_CHANNEL_LINE;
				} else if (!strcmp(token_pos, off_C08318C0)) // "fm"
				{
					av_channel = MTC_AV_CHANNEL_FM;
				} else if (!strcmp(token_pos, off_C08318DC)) // "dtv"
				{
					car_struct->car_status.av_channel_flag1 = 0;
					av_channel = MTC_AV_CHANNEL_DTV;
				} else if (!strcmp(token_pos, off_C08318E0)) // "dvr"
				{
					car_struct->car_status.av_channel_flag1 = 0;
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
			car_struct = p_mtc_car_struct_12;
			if (!strcmp(p_mtc_car_struct_12->ioctl_buf1, off_C08317D0)) // "av_volume"
			{
				volume = get_token_int(&token_pos);
				if (volume >= 0 && volume <= 100) {
					audio_add_work(AUDIO_WORK_VOLUME, volume, 0, 0);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C08317D4)) // "av_phone_volume"
			{
				phone_volume = get_token_int(&token_pos);
				if (phone_volume >= 0 && phone_volume <= 100) {
					audio_add_work(AUDIO_WORK_PHONE_VOLUME, phone_volume, 0, 0);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (strcmp(car_struct->ioctl_buf1, str_av_mute_)) // "av_mute"
			{
				if (!strcmp(car_struct->ioctl_buf1, off_C08317E0)) // "av_phone"
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
						if (car_struct->car_status._gap6[0] &&
						    car_struct->car_status._gap0[1] == 1) {
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
				} else if (!strcmp(car_struct->ioctl_buf1,
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
				} else if (!strcmp(car_struct->ioctl_buf1,
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
				} else if (!strcmp(car_struct->ioctl_buf1,
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
				} else if (!strcmp(car_struct->ioctl_buf1,
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
				} else if (!strcmp(car_struct->ioctl_buf1, off_C083393C)) // "av_eq"
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
				} else if (!strcmp(car_struct->ioctl_buf1,
						   off_C0833940)) // "av_gps_monitor"
				{
					if (!strcmp(token_start, str_on)) // "on"
					{
						if (car_struct->car_status.av_gps_monitor != 1) {
							p_mtc_car_struct_13->car_status
							    .av_gps_monitor = 1;
							audio_add_work(7, 1, 0, 0);
						}
						goto LABEL_62;
					}
					v305 = strcmp(token_start, str_off); // "off"
					if (!v305) {
						if (car_struct->car_status.av_gps_monitor) {
							p_mtc_car_struct_13->car_status
							    .av_gps_monitor = v305; // =0
							audio_add_work(7, 0, v305, v305);
						}
						goto LABEL_62;
					}
				} else {
					car_struct = p_mtc_car_struct_13;
					if (!strcmp(p_mtc_car_struct_13->ioctl_buf1,
						    off_C0833954)) // "av_gps_switch"
					{
						if (!strcmp(token_start, str_on)) // "on"
						{
							if (car_struct->car_status.av_gps_switch !=
							    1) {
								car_struct->car_status
								    .av_gps_switch = 1;
								audio_add_work(8, 1, 0, 0);
							}
							goto LABEL_62;
						}
						v312 = strcmp(token_start, str_off); // "off"
						if (!v312) {
							if (car_struct->car_status.av_gps_switch) {
								car_struct->car_status
								    .av_gps_switch = v312;
								audio_add_work(8, 0, v312, v312);
							}
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct->ioctl_buf1,
							   off_C0833958)) // "av_gps_gain"
					{
						av_gps_gain = get_token_int(&token_pos);
						if (av_gps_gain >= 0) {
							if (car_struct->car_status.av_gps_gain !=
							    av_gps_gain) {
								car_struct->car_status.av_gps_gain =
								    av_gps_gain;
							}
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct->ioctl_buf1,
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
			car_struct = p_mtc_car_struct_12;
			data_buf = p_mtc_car_struct_12->ioctl_buf1;
			if (car_struct->ioctl_buf1[1] != 't' ||
			    p_mtc_car_struct_12->ioctl_buf1[2] != 'l' || data_buf[3] != '_') {
				goto LABEL_62;
			}
			if (!strcmp(p_mtc_car_struct_12->ioctl_buf1, off_C08317C4)) // "ctl_uv_cal"
			{
				if (car_struct->car_status._gap9[21] ||
				    car_struct->car_status._gap8[3] != 1) {
					car_struct->car_status.uv_cal = 0;
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
					car_struct->car_status.uv_cal = 1;
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
					cam_front = car_struct->config_data.cfg_frontview;
					if (car_struct->config_data.cfg_frontview) {
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
				car_struct = p_mtc_car_struct_14;
				if (!strcmp(p_mtc_car_struct_14->ioctl_buf1,
					    off_C08318E8)) // "ctl_beep"
				{
					if (car_struct->config_data.ctl_beep &&
					    get_token_int(&token_pos) >= 0) {
						v315 = 5;
						arm_send_multi(0x9520u, 1, &v315);
						goto LABEL_62;
					}
				} else {
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831904)) // "ctl_dtv_ir"
					{
						dtv_ir[0] = get_token_int(&token_pos);
						dtv_ir[1] = get_token_int(&token_pos);
						dtv_ir[2] = get_token_int(&token_pos);
						dtv_ir[3] = get_token_int(&token_pos);
						arm_send_multi(MTC_CMD_DTV_IR, 4, dtv_ir);
						goto LABEL_62;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831908)) // "ctl_dvd_cmd"
					{
						dvd_command = get_token_int(&token_pos);
						if (dvd_command >= 0) {
							dvd_send_command(dvd_command);
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct->ioctl_buf1,
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
					} else if (!strcmp(car_struct->ioctl_buf1,
							   off_C0833864)) // "ctl_radio_ta"
					{
						if (!strcmp(token_start, str_true)) // "true"
						{
							Radio_TA(1);
							goto LABEL_62;
						}
						if (!strcmp(token_start, str_false)) // "false"
						{
							Radio_TA(0);
							goto LABEL_62;
						}
					} else if (!strcmp(car_struct->ioctl_buf1,
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
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C08338C4)) // "ctl_radio_search"
						{
							v240 = strcmp(token_start, str_true) ==
							       0; // "true"
							Radio_Set_Search(v240);
							goto LABEL_62;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C08338CC)) // "ctl_radio_frequency"
						{
							freq = get_token_int(&token_pos);
							if (freq >= 0) {
								Radio_Set_Frequency(freq, 0);
								goto LABEL_62;
							}
						} else if (
						    !strcmp(car_struct->ioctl_buf1,
							    off_C08338D0)) // "ctl_radio_sfrequency"
						{
							freq = get_token_int(&token_pos);
							if (freq >= 0) {
								Radio_Set_Frequency(freq, 1);
								goto LABEL_62;
							}
						} else if (!strcmp(car_struct->ioctl_buf1,
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
							car_struct = p_mtc_car_struct_13;
							if (!strcmp(
								p_mtc_car_struct_13->ioctl_buf1,
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
								car_struct->ioctl_buf1,
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
								car_struct->ioctl_buf1,
								off_C08338F8)) // "ctl_backview_vol"
							{
								backview_vol =
								    get_token_int(&token_pos);
								if (backview_vol >= 0) {
									car_struct->car_status
									    .backview_vol =
									    backview_vol;
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct->ioctl_buf1,
								off_C08338FC)) // "ctl_backview_mute"
							{
								if (!strcmp(token_start,
									    str_false)) {
									car_struct->car_status
									    .backview_vol = 11;
									goto LABEL_62;
								}
								if (!strcmp(token_start,
									    str_true)) {
									car_struct->car_status
									    .backview_vol = 0;
									goto LABEL_62;
								}
							} else if (
							    !strcmp(
								car_struct->ioctl_buf1,
								off_C0833904)) // "ctl_tv_frequency"
							{
								tv_freq = get_token_int(&token_pos);
								if (tv_freq >= 0) {
									Tv_Set_Frequency(tv_freq);
									goto LABEL_62;
								}
							} else if (
							    !strcmp(car_struct->ioctl_buf1,
								    off_C0833908)) // "ctl_tv_demod"
							{
								v301 = get_token_int(&token_pos);
								if (v301 >= 0) {
									if ((v301 - 1) <= 7) {
										v302 = v301;
										car_struct
										    ->car_status
										    .av_channel_flag1 =
										    v301;
										capture_add_work(
										    61, 0, 0, 1);
										Tv_Set_Demod(v302);
									}
									goto LABEL_62;
								}
							} else if (!strcmp(
								       car_struct->ioctl_buf1,
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
									car_struct->car_status
									    ._gap14[0] = 1;
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
									if (car_struct->car_status
										.backlight_status) {
										v298 = 36;
									} else {
										v299 =
										    car_struct
											->car_status
											.backlight_status;
									}
									if (!car_struct->car_status
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
								       car_struct->ioctl_buf1,
								       off_C0833934)) // "ctl_power"
							{
								if (!strcmp(token_start, str_on) ||
								    !strcmp(token_start, str_off)) {
									goto LABEL_62;
								}
							} else if (!strcmp(
								       car_struct->ioctl_buf1,
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
									if (car_struct->car_status
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
											    ->car_status
											    .wipe_flag &
											0x40,
										    (car_struct
											 ->car_status
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
				capture_add_work(64, 0, *&car_struct->car_status._gap8[5], 0);
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
					cfg_color = p_cfg_color;
					car_struct->config_data.cfg_color[1] = color;
					car_struct->config_data.cfg_color[0] = BYTE1(color);
					arm_send_multi(0x9507u, 2, cfg_color);
					goto LABEL_62;
				}
			}
			car_struct = p_mtc_car_struct_12;
			if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
				    off_C08317B8)) // "cfg_led_multi"
			{
				cfg_led_multi = get_token_int(&token_pos);
				if (cfg_led_multi >= 0) {
					car_struct->config_data.cfg_led_multi = cfg_led_multi;
					arm_send_multi(0x9508u, 1,
						       &car_struct->config_data.cfg_led_multi);
					goto LABEL_62;
				}
			}
			car_struct = p_mtc_car_struct_12;
			if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
				    off_C08317BC)) // "cfg_wifi_pwr"
			{
				wifi_pwr = get_token_int(&token_pos);
				if (wifi_pwr >= 0) {
					car_struct->config_data.wifi_pwr = wifi_pwr;
					arm_send_multi(MTC_CMD_WIFI_PWR, 1, p_wifi_pwr);
					if (car_struct->_gap4[0]) {
						v174 = *off_C0831890;
						if (v174 != 255) {
							rk29sdk_wifi_power(v174);
						}
					}
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C0831834)) // "cfg_blmode"
			{
				blmode = get_token_int(&token_pos);
				if (blmode >= 0) {
					p_blmode = off_C0831888;
					car_struct->config_data.cfg_blmode = blmode;
					arm_send_multi(MTC_CMD_BLMODE, 1, p_blmode);
					backlight_update();
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C0831838)) // "cfg_powerdelay"
			{
				powerdelay = get_token_int(&token_pos);
				if (powerdelay >= 0) {
					p_powerdelay = off_C0831884;
					car_struct->config_data.cfg_powerdelay = powerdelay;
					arm_send_multi(MTC_CMD_POWERDELAY, 1, p_powerdelay);
					goto LABEL_62;
				}
				goto LABEL_140;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C08317C0)) // "cfg_mirror"
			{
				if (!strcmp(token_pos, str_true_0)) // "true"
				{
					car_struct->config_data.cfg_mirror = 1;
					arm_send(MTC_CMD_MIRROR_ON);
					goto LABEL_62;
				}
				if (strcmp(token_pos, str_false_0)) // "false"
				{
					goto LABEL_140;
				}
				car_struct->config_data.cfg_mirror = 0;
				arm_send(MTC_CMD_MIRROR_OFF);
				goto LABEL_62;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C0831830)) // "cfg_backlight"
			{
				backlight = get_token_int(&token_pos);
				if (backlight >= 0) {
					p_backlight = off_C0831880;
					car_struct->config_data.cfg_backlight = backlight;
					arm_send_multi(MTC_CMD_BACKLIGHT, 1, p_backlight);
					goto LABEL_62;
				}
			} else if (!strcmp(car_struct->ioctl_buf1, off_C0831840)) // "cfg_beep"
			{
				enable_beep = get_token_int(&token_pos);
				if (enable_beep >= 0) {
					car_struct->config_data.ctl_beep = enable_beep;
					arm_send_multi(MTC_CMD_BEEP, 1,
						       &car_struct->config_data.ctl_beep);
					goto LABEL_62;
				}
			} else {
				if (strcmp(car_struct->ioctl_buf1, off_C0831844)) // "cfg_led"
				{
					if (!strcmp(car_struct->ioctl_buf1,
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
						v156 = p_config_data_4;
						v157 = v60;
						v158 = &p_config_data_4->cfg_logo2[8];
						do {
							v156->_gap7[0] = *v157 >> 8;
							v159 = *v157;
							v157 += 2;
							v156->_gap7[1] = v159;
							v156 = (v156 + 2);
						} while (v156 != v158);
						kfree(v60);
						goto LABEL_62;
					}
					if (!strcmp(car_struct->ioctl_buf1,
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
									v147 = (&gap12->checksum +
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
									       150, p_steer_data);
								stw_range_check();
								goto LABEL_62;
							}
						}
					} else {
						if (strcmp(car_struct->ioctl_buf1,
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
									*(&p_config_data->checksum +
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
							car_struct->config_data.cfg_led[0] = b_led1;
							car_struct->config_data.cfg_led[1] = b_led2;
							car_struct->config_data.cfg_led[2] = led3;
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
						*b1 = *str_nodisk[0];
						*b2 = *(str_nodisk[0] + 1);
						car_struct->buffer2[6] = *b2 >> 16;
						v97 = off_C08318F0;
						*&car_struct->buffer2[0] = *b1;
						*(v97 + 0x10) = *b2;
					} else {
						res = 0;
						v198 = *str_diskin[0];
						v199 = *(str_diskin[0] + 1);
						car_struct->buffer2[6] = v199 >> 16;
						*&car_struct->buffer2[0] = v198;
						*&car_struct->buffer2[4] = v199;
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
					car_struct = p_mtc_car_struct_12;
					if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
						    off_C0831730)) // "sta_dvd_folder_cnt"
					{
						res = 0;
						dvd_get_folder_cnt(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831734)) // "sta_dvd_media_cnt"
					{
						res = 0;
						dvd_get_media_cnt(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831738)) // "sta_dvd_folder_idx"
					{
						res = 0;
						dvd_get_folder_idx(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C083173C)) // "sta_dvd_media_idx"
					{
						res = 0;
						dvd_get_media_idx(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831740)) // "sta_dvd_length"
					{
						res = 0;
						dvd_get_length(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831744)) // "sta_dvd_position"
					{
						res = 0;
						dvd_get_position(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831748)) // "sta_dvd_title"
					{
						res = 0;
						dvd_get_media_title(p_buf1);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C083174C)) // "sta_ipod"
					{
						v55 = (car_struct->car_status._gap3[0] & 0x20) == 0;
						if (car_struct->car_status._gap3[0] & 0x20) {
							v56 = &car_struct->ioctl_buf1[3060];
							v54 = str_false_0; // "false"
						} else {
							v56 = str_true_0; // "true"
						}
						if (car_struct->car_status._gap3[0] & 0x20) {
							v57 = *v54;
							v58 = *(v54 + 1);
						} else {
							v57 = *v56;
							v58 = *(v56 + 1);
						}
						if (car_struct->car_status._gap3[0] & 0x20) {
							*&car_struct->buffer2[0] = v57;
						} else {
							*&car_struct->buffer2[0] = v57;
							car_struct->buffer2[4] = v58;
						}
						res = 0;
						if (!v55) {
							*(v56 + 8) = v58;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C08318EC)) //  "sta_driving"
					{
						v192 = car_struct->car_status._gap3[0];
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
							*&car_struct->buffer2[0] = v195;
							*(v197 + 0x10) = v196;
						} else {
							*&car_struct->buffer2[0] = v195;
							*(v197 + 0x10) = v196;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C08318F8)) // "sta_ill"
					{
						res = 0;
						car_struct = p_mtc_car_struct_14;
						if (car_struct->car_status._gap3[0] & 8) {
							v202 =
							    &p_mtc_car_struct_14->ioctl_buf1[3060];
							v200 = str_false_0;
						} else {
							v202 = str_true_0;
						}
						if (car_struct->car_status._gap3[0] & 8) {
							v203 = *v200;
							v204 = *(v200 + 1);
						} else {
							v203 = *v202;
							v204 = *(v202 + 1);
						}
						if (car_struct->car_status._gap3[0] & 8) {
							*&p_mtc_car_struct_14->buffer2[0] = v203;
							*(v202 + 8) = v204;
						} else {
							*&p_mtc_car_struct_14->buffer2[0] = v203;
							car_struct->buffer2[4] = v204;
						}
						goto LABEL_30;
					}
					car_struct = p_mtc_car_struct_14;
					if (!strcmp(p_mtc_car_struct_14->ioctl_buf1,
						    off_C0831920)) // "sta_dtv"
					{
						v222 = *(str_true + 1);
						*&car_struct->buffer2[0] = *str_true;
						car_struct->buffer2[4] = v222;
						res = 0;
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831924)) // "sta_battery"
					{
						battery = *&car_struct->car_status.battery;
						res = 0;
						sprintf(p_buf2, str_fmt_d_3, battery); // "%d"
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831928)) // "sta_touch"
					{
						touch_type = car_struct->car_status.touch_type;
						if (touch_type == 129) {
							*&car_struct->buffer2[0] = 'ser';
							res = 0;
						} else {
							v207 = touch_type == 128;
							if (touch_type == 128) {
								v208 = 'pac';
								*&car_struct->buffer2[0] = 'pac';
							} else {
								v208 = off_C083192C; // "none"
							}
							if (!v207) {
								v209 = *(v208 + 1);
								*&car_struct->buffer2[0] = *v208;
								car_struct->buffer2[4] = v209;
							}
							res = 0;
						}
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
						    off_C0831930)) // "sta_touch_adc"
					{
						res = 0;
						sta_touch_adc(p_buf2);
						goto LABEL_30;
					}
					if (!strcmp(car_struct->ioctl_buf1,
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
									car_struct = p_buf1;
								} else {
									v216 = p_buf1;
									v218 = *v215;
									v219 = *(v215 + 1);
									v215 = *(v215 + 2);
								}
								if (v217) {
									car_struct->car_dev = v218;
									*&car_struct->car_status
									      ._gap0[0] = v219;
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
								*&car_struct->buffer2[0] =
								    *str_fail;
								*(v239 + 0x10) = v238;
								kfree(v211);
							}
							goto LABEL_30;
						}
						kfree(v211);
					} else {
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0831948)) // "sta_video_signal"
						{
							if (car_struct->car_status
								.sta_video_signal) {
								v220 = *off_C083194C >> 16;
								*&car_struct->buffer2[0] =
								    *off_C083194C; // "ok"
								car_struct->buffer2[2] = v220;
								res = 0;
							} else {
								res = 0;
								v224 = p_buf2;
								v225 = *(off_C0833878[0] +
									 1); // "nosignal"
								v226 = *(off_C0833878[0] + 2);
								*p_buf2 = *off_C0833878[0];
								*(v224 + 1) = v225;
								v224[8] = v226;
							}
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C083387C)) // "sta_radio_signal"
						{
							radio_signal = Radio_Get_Signal();
							res = 0;
							sprintf(p_buf2, str_fmt_d_3,
								radio_signal); // "%d"
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0833880)) // "sta_tv_status"
						{
							tv_status = Tv_Get_Status();
							res = 0;
							sprintf(p_buf2, str_fmt_d_3, tv_status);
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0833884)) // "sta_tv_signal"
						{
							tv_signal = check_tv_signal();
							if (tv_signal == 1) {
								res = 0;
								v258 = *(off_C08338E4[0] +
									 1); // "ntsc"
								v259 = p_buf1_3060;
								*&car_struct->buffer2[0] =
								    *off_C08338E4[0];
								*(v259 + 0x10) = v258;
							} else if (tv_signal == 2) {
								res = 0;
								*&car_struct->buffer2[0] = 'lap';
							} else {
								v252 = tv_signal == 3;
								res = 0;
								if (tv_signal == 3) {
									v251 = off_C08338DC
									    [0]; // "secam"
								} else {
									v250 = off_C0833878
									    [0]; // "nosignal"
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
									*&car_struct->buffer2[0] =
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
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0833888)) // "sta_radio_stereo"
						{
							if (Radio_Get_Stereo()) {
								res = 0;
								v246 = *off_C08338D8[0]; // "stereo"
								v247 = *(off_C08338D8[0] + 1);
								car_struct->buffer2[6] = v247 >> 16;
								v248 = p_buf1_3060;
								*&car_struct->buffer2[0] = v246;
								*(v248 + 0x10) = v247;
							} else {
								res = 0;
								v256 = *(off_C08338E0[0] +
									 1); // "mono"
								v257 = p_buf1_3060;
								*&car_struct->buffer2[0] =
								    *off_C08338E0[0];
								*(v257 + 0x10) = v256;
							}
							goto LABEL_30;
						}
						car_struct = p_mtc_car_struct_13;
						if (!strcmp(p_mtc_car_struct_13->ioctl_buf1,
							    off_C0833890)) // "sta_mcu_version"
						{
							strcpy(p_buf2, car_struct->mcu_version);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0833894)) // "sta_mcu_date"
						{
							strcpy(p_buf2, car_struct->mcu_date);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C0833898)) // "sta_mcu_time"
						{
							strcpy(p_buf2, car_struct->mcu_time);
							res = 0;
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C083389C)) // "sta_uv_cal"
						{
							uv_cal = car_struct->car_status.uv_cal;
							res = 0;
							sprintf(p_buf2, str_fmt_d_3, uv_cal);
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C08338A0)) // "sta_view"
						{
							view = car_struct->car_status.sta_view == 0;
							if (car_struct->car_status.sta_view) {
								v230 =
								    &car_struct->ioctl_buf1[3060];
								v228 = off_C08338A4[0]; // "front"
							} else {
								v230 = off_C08338A8[0]; // "back"
							}
							if (car_struct->car_status.sta_view) {
								v231 = *v228;
								v232 = *(v228 + 1);
							} else {
								v231 = *v230;
								v232 = *(v230 + 1);
							}
							if (car_struct->car_status.sta_view) {
								*&car_struct->buffer2[0] = v231;
							} else {
								*&car_struct->buffer2[0] = v231;
								car_struct->buffer2[4] = v232;
							}
							res = 0;
							if (!view) {
								*(v230 + 8) = v232;
							}
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C08338B0)) // "sta_touch_info"
						{
							touch_info1 =
							    car_struct->car_status.touch_info1;
							touch_info2 =
							    car_struct->car_status.touch_info2;
							touch_w =
							    car_struct->car_status.touch_width;
							touch_h =
							    car_struct->car_status.touch_height;
							res = 0;
							sprintf(p_buf2, off_C08338C8, touch_w,
								touch_h, touch_info1,
								touch_info2); // "%d x %d (0x%02x
							// 0x%02x)"
							goto LABEL_30;
						}
						if (!strcmp(car_struct->ioctl_buf1,
							    off_C08338B4)) // "sta_wipe"
						{
							wipe = car_struct->car_status.wipe_flag;
							v235 = wipe < 0;
							if (wipe < 0) {
								v236 = 'sey';
								*&car_struct->buffer2[0] = 'sey';
							} else {
								v233 =
								    &car_struct->ioctl_buf1[3060];
								v236 = off_C08338B8; // "no"
							}
							if (!v235) {
								v237 = *v236;
								*(v233 + 0xC) = v237;
								car_struct->buffer2[2] = v237 >> 16;
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
						car_struct->car_status.cfg_maxvolume); // "%d"
					goto LABEL_30;
				}
				if (!strcmp(data_buf, off_C0831754)) // "cfg_customer"
				{
					res = 0;
					strcpy(buf_1, p_cfg_customer);
					goto LABEL_30;
				}
				car_struct = p_mtc_car_struct_12;
				if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
					    off_C0831758)) // "cfg_sn"
				{
					strcpy(p_buf2, car_struct->config_data.cfg_sn);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C083175C)) // "cfg_model"
				{
					strcpy(p_buf2, car_struct->config_data.cfg_model);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831760)) // "cfg_password"
				{
					strcpy(p_buf2, car_struct->config_data.cfg_password);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831764)) // "cfg_logo1"
				{
					strcpy(p_buf2, car_struct->config_data.cfg_logo1);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831768)) // "cfg_logo2"
				{
					strcpy(p_buf2, car_struct->config_data.cfg_logo2);
					res = 0;
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C083176C)) // "cfg_canbus"
				{
					canbus = car_struct->config_data.cfg_canbus;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, canbus);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831770)) // "cfg_canbus_cfg"
				{
					canbus_cfg = car_struct->config_data.canbus_cfg;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, canbus_cfg);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831774)) // "cfg_atvmode"
				{
					cfg_atvmode = car_struct->config_data.cfg_atvmode;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_atvmode);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831778)) // "cfg_dtv"
				{
					cfg_dtv = car_struct->config_data.cfg_dtv;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_dtv);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C083177C)) // "cfg_frontview"
				{
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						car_struct->config_data.cfg_frontview);
					goto LABEL_30;
				}
				car_struct = p_mtc_car_struct_12;
				if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
					    off_C0831780)) // "cfg_ipod"
				{
					cfg_ipod = car_struct->config_data.cfg_ipod;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_ipod);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831784)) // "cfg_dvd"
				{
					cfg_dvd = car_struct->config_data.cfg_dvd;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_dvd);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831788)) // "cfg_bt"
				{
					cfg_bt = car_struct->config_data.cfg_bt;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_bt);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C083178C)) // "cfg_radio"
				{
					cfg_radio = car_struct->config_data.cfg_radio;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_radio);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831790)) // "cfg_rds"
				{
					cfg_rds = car_struct->config_data.cfg_rds;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_rds);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831794)) // "cfg_logo_type"
				{
					cfg_logo_type = car_struct->config_data.cfg_logo_type;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_logo_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831798)) // "cfg_radio_area"
				{
					cfg_radio_area = car_struct->config_data.cfg_radio_area;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_radio_area);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C083179C)) // "cfg_launcher"
				{
					cfg_launcher = car_struct->config_data.cfg_launcher;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_launcher);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C08317A0)) // "cfg_led_type"
				{
					cfg_led_type = car_struct->config_data.cfg_led_type;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_led_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C08317A4)) // "cfg_rudder"
				{
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						car_struct->config_data.cfg_rudder);
					goto LABEL_30;
				}
				car_struct = p_mtc_car_struct_12;
				if (!strcmp(p_mtc_car_struct_12->ioctl_buf1,
					    off_C08317A8)) // "cfg_key0"
				{
					cfg_key0 = car_struct->config_data.cfg_key0;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_key0);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C08317AC)) // "cfg_appdisable"
				{
					cfg_appdisable = car_struct->config_data.cfg_appdisable;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_appdisable);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C08317B0)) // "cfg_language_selection"
				{
					cfg_ls1 = car_struct->config_data.cfg_language_selection[0];
					cfg_ls2 = car_struct->config_data.cfg_language_selection[1];
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_ls2 | (cfg_ls1 << 8));
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C08317B4)) // "cfg_color"
				{
					cfg_color1 = car_struct->config_data.cfg_color[0];
					cfg_color2 = car_struct->config_data.cfg_color[1];
					res = 0;
					sprintf(p_buf2, str_fmt_d_3,
						cfg_color2 | (cfg_color1 << 8));
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C08317B8)) // "cfg_led_multi"
				{
					cfg_led_multi = car_struct->config_data.cfg_led_multi;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_led_multi);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C08317BC)) // "cfg_wifi_pwr"
				{
					wifi_pwr = car_struct->config_data.wifi_pwr;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, wifi_pwr);
					goto LABEL_30;
				}
				v67 = strcmp(car_struct->ioctl_buf1, off_C08317C0); // "cfg_mirror"
				if (!v67) {
					v70 = car_struct->config_data.cfg_mirror == 0;
					if (car_struct->config_data.cfg_mirror) {
						v71 = str_true_0;
					} else {
						v71 = &car_struct->ioctl_buf1[3060];
					}
					if (car_struct->config_data.cfg_mirror) {
						v67 = *v71;
						v68 = *(v71 + 1);
					} else {
						v69 = str_false_0;
					}
					if (car_struct->config_data.cfg_mirror) {
						*&car_struct->buffer2[0] = v67;
						car_struct->buffer2[4] = v68;
					} else {
						v67 = *v69;
						v68 = *(v69 + 1);
					}
					if (v70) {
						*&car_struct->buffer2[0] = v67;
					}
					res = 0;
					if (v70) {
						*(v71 + 8) = v68;
					}
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831830)) // "cfg_backlight"
				{
					cfg_backlight = car_struct->config_data.cfg_backlight;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_backlight);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831834)) // "cfg_blmode"
				{
					cfg_blmode = car_struct->config_data.cfg_blmode;
					res = 0;
					sprintf(p_buf2, str_fmt_d_3, cfg_blmode);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831838)) // "cfg_powerdelay"
				{
					res = 0;
					sprintf(p_buf1, str_fmt_d_4,
						car_struct->config_data.cfg_powerdelay); // "%d"
					goto LABEL_30;
				}
				car_struct = p_mtc_car_struct_14;
				if (!strcmp(p_mtc_car_struct_14->ioctl_buf1,
					    off_C083183C)) // "cfg_ill"
				{
					cfg_ill = car_struct->config_data.cfg_ill;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_ill); // "%d,"
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831840)) // "cfg_beep"
				{
					cfg_beep = car_struct->config_data.ctl_beep;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_beep); // "%d,"
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831844)) // "cfg_led"
				{
					cfg_led2 = car_struct->config_data.cfg_led[2];
					cfg_led0 = car_struct->config_data.cfg_led[0];
					cfg_led1 = car_struct->config_data.cfg_led[1];
					res = 0;
					sprintf(p_buf1, off_C0831858, cfg_led0, cfg_led1,
						cfg_led2); // "%d,%d,%d"
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1, off_C0831848)) // "cfg_dvr"
				{
					cfg_dvr = car_struct->config_data.cfg_dvr;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_dvr); // "%d"
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C083184C)) // "cfg_wheelstudy_type"
				{
					cfg_wheelstudy_type =
					    car_struct->config_data.cfg_wheelstudy_type;
					res = 0;
					sprintf(p_buf1, str_fmt_d_4, cfg_wheelstudy_type);
					goto LABEL_30;
				}
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831850)) // "cfg_key_assign"
				{
					v102 = p_buf1;
					v103 = &car_struct->config_data.checksum;
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
				if (!strcmp(car_struct->ioctl_buf1,
					    off_C0831854)) // "cfg_ir_assign"
				{
					v114 = p_buf1;
					v115 = &car_struct->config_data.checksum;
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
				v128 = strcmp(car_struct->ioctl_buf1,
					      off_C0831868); // "cfg_steer_assign"
				if (!v128) {
					v129 = p_buf1;
					v130 = &car_struct->config_data.checksum;
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
				if (!strcmp(car_struct->ioctl_buf1, off_C08338F0)) // "cfg_config"
				{
					v285 = car_struct->config_data.checksum;
					v283 = &car_struct->config_data.checksum;
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
		car_struct = p_mtc_car_struct_12;
		if (car_struct->ioctl_buf1[1] == 'v' && p_mtc_car_struct_12->ioctl_buf1[2] == '_') {
			if (!strcmp(p_mtc_car_struct_12->ioctl_buf1, str_av_mute_)) // "av_mute"
			{
				is_audio_mute = isAudioMute();
				not_mute = is_audio_mute == 0;
				if (is_audio_mute) {
					v86 = str_true_0; // "true"
				} else {
					v86 = &car_struct->ioctl_buf1[3060];
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
					*&car_struct->buffer2[0] = is_audio_mute;
					car_struct->buffer2[4] = v83;
				}
				if (not_mute) {
					*&car_struct->buffer2[0] = is_audio_mute;
				}
				res = 0;
				if (not_mute) {
					*(v86 + 8) = v83;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C08316FC)) // "av_channel"
			{
				switch (getAudioChannel()) {
				case MTC_AV_CHANNEL_GSM_BT:
					res = 0;
					v110 = *off_C08318B8; // "gsm_bt"
					v111 = *(off_C08318B8 + 1);
					car_struct->buffer2[6] = v111 >> 16;
					v112 = off_C08318F0;
					*&car_struct->buffer2[0] = v110;
					*(v112 + 0x10) = v111;
					break;
				case MTC_AV_CHANNEL_SYS:
					res = 0;
					*&car_struct->buffer2[0] = 'sys';
					break;
				case MTC_AV_CHANNEL_DVD:
					res = 0;
					*&car_struct->buffer2[0] = 'dvd';
					break;
				case MTC_AV_CHANNEL_LINE:
					res = 0;
					v99 = *(off_C08318D8 + 1);
					v100 = off_C08318F0;
					*&car_struct->buffer2[0] = *off_C08318D8; // "line"
					*(v100 + 0x10) = v99;
					break;
				case MTC_AV_CHANNEL_FM:
					res = 0;
					v98 = *off_C08318C0 >> 16;
					*(off_C08318F0 + 0xC) = *off_C08318C0; // "fm"
					car_struct->buffer2[2] = v98;
					break;
				case MTC_AV_CHANNEL_DTV:
					res = 0;
					*&car_struct->buffer2[0] = 'vtd';
					break;
				case MTC_AV_CHANNEL_IPOD:
					res = 0;
					v181 = *(off_C08318C4 + 1);
					v182 = off_C08318F0;
					*&car_struct->buffer2[0] = *off_C08318C4; // "ipod"
					*(v182 + 0x10) = v181;
					break;
				case MTC_AV_CHANNEL_DVR:
					res = 0;
					*&car_struct->buffer2[0] = 'rvd';
					break;
				default:
					goto LABEL_102;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C0831700)) // "av_gps_monitor"
			{
				v17 = car_struct->car_status.av_gps_monitor == 0;
				if (car_struct->car_status.av_gps_monitor) {
					v16 = &car_struct->ioctl_buf1[3060];
					v18 = off_C0831704; // "on"
				} else {
					v18 = 'ffo';
					*&car_struct->buffer2[0] = 'ffo';
				}
				if (!v17) {
					v18 = *v18;
					car_struct->buffer2[2] = v18 >> 16;
				}
				res = 0;
				if (!v17) {
					*(v16 + 0xC) = v18;
				}
				goto LABEL_30;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C08317DC)) // "av_gps_switch"
			{
				v79 = car_struct->car_status.av_gps_switch == 0;
				if (car_struct->car_status.av_gps_switch) {
					v78 = &car_struct->ioctl_buf1[3060];
					v80 = off_C0831704;
				} else {
					v80 = 'ffo';
					*&car_struct->buffer2[0] = 'ffo';
				}
				if (!v79) {
					v81 = *v80;
					*(v78 + 0xC) = v81;
					car_struct->buffer2[2] = v81 >> 16;
				}
				res = 0;
				goto LABEL_30;
			}
			if (!strcmp(car_struct->ioctl_buf1, off_C08317F8)) // "av_gps_gain"
			{
				av_gps_gain = car_struct->car_status.av_gps_gain;
				res = 0;
				sprintf(buf_1, str_fmt_d_4, av_gps_gain); // "%d"
				goto LABEL_30;
			}
		}
	}
LABEL_102:
	res = -1;
	car_struct->buffer2[0] = 0;
LABEL_30:
	v20 = strlen(buf_1);
	v21 = *(v6 + 2);
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

	INIT_DELAYED_WORK(work->dwork, &car_work);

	work->cmd1 = a1;
	work->cmd2 = a2;
	queue_delayed_work(mtc_car_struct->car_wq, &work->dwork, msecs_to_jiffies(0));

	if (flush) {
		flush_workqueue(mtc_car_struct->car_wq);
	}
}

EXPORT_SYMBOL_GPL(car_add_work)

/* fully decompiled */
void
car_add_work_delay(int a1, int a2, unsigned int delay)
{
	struct mtc_work *work; // r4@2

	work = kmalloc(sizeof(struct mtc_work), __GFP_IO);
	// TODO: alloc check

	INIT_DELAYED_WORK(work->dwork, &car_work);

	work->cmd1 = a1;
	work->cmd2 = a2;
	queue_delayed_work(mtc_car_struct->car_wq, &work->dwork, msecs_to_jiffies(delay));
}

EXPORT_SYMBOL_GPL(car_add_work_delay)

/* fully decompiled */
static void
mtc_car_work(struct work_struct *work)
{
	(void)work;

	mutex_lock(&mtc_car_struct->car_comm->car_lock);
	udelay(20);

	while (!getPin(gpio_MCU_DIN) && arm_rev()) {
		;
	}

	enable_irq(mtc_car_struct->car_comm->mcu_din_gpio);
	mutex_unlock(&mtc_car_struct->car_comm->car_lock);
}

static void
car_avm(void)
{
	unsigned char buf[7];

	if (mtc_car_struct->config_data.cfg_canbus == 0xA) {
		key_beep();

		buf[0] = 0x06; // length?
		buf[1] = 0x2E;
		buf[2] = 0xC6u;
		buf[3] = 0x02;
		buf[4] = 0x02;
		buf[5] = 0x01;
		buf[6] = 0x34;

		if (mtc_car_struct->car_status._gap5[0]) { // ?
			printk("mBackView\n");
			arm_send_multi(0xC000u, 7, buf);
		} else {
			arm_send_multi(0xC000u, 7, buf);
		}
	} else if (mtc_car_struct->config_data.cfg_canbus == 0x25) {
		key_beep();

		buf[0] = 0x05; // length?
		buf[1] = 0x2E;
		buf[2] = 0xC7;
		buf[3] = 0x01;
		buf[4] = 0x01;
		buf[5] = 0x36;

		if (mtc_car_struct->car_status._gap5[0]) { // ?
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

	mtc_car_struct->car_comm = car_comm;

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

EXPORT_SYMBOL_GPL(car_comm_init)

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
	const char *screen_width;	      // r0@1
	char check;			       // zf@1
	struct miscdevice *_p_car_miscdev;     // r0@5
	bool mcu_clk_val;		       // r0@5
	char mtc_bootmode;		       // r0@6
	char mtc_boot_mode;		       // r0@6
	void(__cdecl * p_work_func)(void *);   // r3@7
	struct workqueue_struct *wq;	   // r4@7
	unsigned __int32 delay_60s;	    // r0@7
	int ver_part;			       // r5@8
	int pos;			       // r4@8
	char v16;			       // r0@8
	char v17;			       // r0@8
	int cur_byte;			       // r7@9
	int check_hmf;			       // r0@21
	struct mtc_car_struct *mtc_car_struct; // r4@21 MAPDST
	int check_c_kz;			       // r0@37
	struct mtc_config_data *config_data;   // r5@61
	unsigned int cfg_backlight;	    // r3@61
	const char *s_mtc_config;	      // r0@61
	int config_byte;		       // r4@61
	int cfg_canbus;			       // r3@68
	char canbus_is_7_or_40;		       // zf@68
	char cfg_maxvolume;		       // r3@75
	int v32;			       // r0@79
	char wipe_flag;			       // r3@81
	char v35;			       // r1@81
	char v37;			       // lr@85
	const char *_log_car_probe_end;	// r0@92

	printk(log_mtc_car);
	mtc_car_struct = p_mtc_car_struct_21;
	gpio_request(gpio_FCAM_PWR, gpio_fcam_pwr_nm);
	gpio_pull_updown(gpio_FCAM_PWR, 0);
	gpio_direction_output(gpio_FCAM_PWR, 0);
	_mutex_init(&mtc_car_struct->car_io_lock, nm_car_io_lock, &mtc_car_struct->car_cmd_lock);
	_mutex_init(&mtc_car_struct->car_cmd_lock, nm_car_cmd_lock, &mtc_car_struct->_gap3[4]);
	_memzero(&mtc_car_struct->car_status, 160u);
	screen_width = mtc_get_screen_width();
	check = screen_width == 1024;
	if (screen_width == 1024) {
		screen_width = log_mtc_resolution_1024x600;
		mtc_car_struct->car_status.is1024screen = 1;
	}
	if (!check) {
		screen_width = log_mtc_resolution_800x480;
	}
	printk(screen_width);
	mtc_car_struct->car_status.key_mode = 0;
	*&mtc_car_struct->car_status._gap4[3] = 0x3FF;
	*&mtc_car_struct->car_status._gap4[7] = 0x3FF;
	*&mtc_car_struct->car_status._gap3[4] = 0x3FF;
	*&mtc_car_struct->car_status._gap3[8] = 0x3FF;
	_p_car_miscdev = p_mtc_car_miscdev;
	mtc_car_struct->car_status.sta_view = 1;
	misc_register(_p_car_miscdev);
	gpio_request(gpio_MCU_CLK, gpio_mcu_clk_nm);
	mtc_car_struct = p_mtc_car_struct_21;
	gpio_pull_updown(gpio_MCU_CLK, 0);
	gpio_direction_input(gpio_MCU_CLK);
	mcu_clk_val = _gpio_get_value(gpio_MCU_CLK) == 0;
	mtc_car_struct->car_status._gap8[2] = mcu_clk_val;
	if (mcu_clk_val) {
		return 0;
	}
	*&mtc_car_struct->_gap3[8] = 0;
	*&mtc_car_struct->_gap3[4] = 0;
	mtc_car_struct->car_wq =
	    _alloc_workqueue_key(car_wq_nm, WQ_MEM_RECLAIM | WQ_UNBOUND, 1, 0, 0);
	car_comm_init();
	mtc_bootmode = board_boot_mode();
	arm_send(mtc_bootmode & 0xF | MTC_CMD_BOOTMODE);
	mtc_boot_mode = board_boot_mode();
	printk(log_mtc_bootmode_d, mtc_boot_mode & 0xF);
	if (!(board_boot_mode() & 0xF)) {
		mtc_car_struct->wipecheckclear_work.work.entry.next =
		    &mtc_car_struct->wipecheckclear_work.work.entry;
		mtc_car_struct->wipecheckclear_work.work.entry.prev =
		    &mtc_car_struct->wipecheckclear_work.work.entry;
		p_work_func = p_wipechecker_work;
		mtc_car_struct->car_status.wipe_check = 1;
		mtc_car_struct->wipecheckclear_work.work.data.counter = 0x500;
		mtc_car_struct->wipecheckclear_work.work.func = p_work_func;
		init_timer_key(&mtc_car_struct->wipecheckclear_work.timer, 0, 0);
		wq = mtc_car_struct->car_wq;
		delay_60s = msecs_to_jiffies(60000u);
		queue_delayed_work(wq, &mtc_car_struct->wipecheckclear_work, delay_60s);
	}
	mtc_car_struct = p_mtc_car_struct_21;
	arm_send(0x201u);
	ver_part = 0;
	pos = 0;
	mtc_car_struct->car_status.wipe_flag = v16;
	arm_send(0xF02u);
	mtc_car_struct->car_status.backview_vol = 11;
	mtc_car_struct->car_status._gap3[0] = v17;
	rk_fb_show_logo();
	_memzero(&mtc_car_struct->config_data, 512u);
	printk(log_mtc_config_pre);
	arm_send_multi(MTC_CMD_MCUVER, 16, mtc_car_struct->mcu_version);
	do {
		cur_byte = p_mcu_version[pos];
		if (!p_mcu_version[pos]) {
			break;
		}
		if (cur_byte == '-') {
			ver_part = (ver_part + 1);
		} else if (ver_part == 1) {
			mtc_car_struct->car_status._gap9[strlen(p_mcu_version - 748)] = cur_byte;
		} else if (ver_part == 2) {
			mtc_car_struct->car_status._gap8[strlen(p_mcu_version - 764) + 9] =
			    cur_byte;
		}
		++pos;
	} while (pos != 16);
	if (check_customer(customer_YZ) || check_customer(customer_RM) ||
	    check_customer(customer_ZT)) {
		p_mtc_car_struct_21->car_status.mtc_customer = 1;
	} else {
		check_hmf = check_customer(customer_HMF);
		mtc_car_struct = p_mtc_car_struct_21;
		if (check_hmf) {
			p_mtc_car_struct_21->car_status.mtc_customer = 2;
			decorder_power(1);
			decorder_power(0);
		} else if (check_customer(customer_KGL)) {
			mtc_car_struct->car_status.mtc_customer = 3;
		} else if (check_customer(customer_KLD)) {
			mtc_car_struct->car_status.mtc_customer = 4;
		} else if (check_customer(customer_FY)) {
			mtc_car_struct->car_status.mtc_customer = 5;
		} else if (check_customer(customer_HZC)) {
			mtc_car_struct->car_status.mtc_customer = 16;
		} else if (check_customer(customer_JY)) {
			mtc_car_struct->car_status.mtc_customer = 6;
		} else if (check_customer(customer_MX)) {
			mtc_car_struct->car_status.mtc_customer = 9;
		} else if (check_customer(customer_AM)) {
			mtc_car_struct->car_status.mtc_customer = 8;
		} else {
			check_c_kz = check_customer(customer_KZ);
			mtc_car_struct = p_mtc_car_struct_21;
			if (check_c_kz) {
				p_mtc_car_struct_21->car_status.mtc_customer = 18;
			} else if (check_customer(customer_KSP)) {
				mtc_car_struct->car_status.mtc_customer = 7;
			} else if (check_customer(customer_HLA)) {
				mtc_car_struct->car_status.mtc_customer = 10;
			} else if (check_customer(customer_JWT)) {
				mtc_car_struct->car_status.mtc_customer = 11;
			} else if (check_customer(customer_KED)) {
				mtc_car_struct->car_status.mtc_customer = 12;
			} else if (check_customer(customer_SH)) {
				mtc_car_struct->car_status.mtc_customer = 13;
			} else if (check_customer(customer_HH)) {
				mtc_car_struct->car_status.mtc_customer = 14;
			} else if (check_customer(customer_KY)) {
				mtc_car_struct->car_status.mtc_customer = 15;
			} else if (check_customer(customer_LM)) {
				mtc_car_struct->car_status.mtc_customer = 17;
			} else if (check_customer(customer_YM)) {
				mtc_car_struct->car_status.mtc_customer = 19;
			} else if (check_customer(customer_YMZ)) {
				p_mtc_car_struct_21->car_status.mtc_customer = 20;
			}
		}
	}
	mtc_car_struct = p_mtc_car_struct_21;
	if (p_mtc_car_struct_21->car_status.mtc_customer != 4) {
		iomux_set(0x1A51u);
	}
	config_data = p_config_data_35;
	arm_send_multi(0x1510u, 1, p_cfg_backlight);
	cfg_backlight = mtc_car_struct->config_data.cfg_backlight;
	s_mtc_config = log_mtc_config;
	config_byte = 0;
	if (cfg_backlight <= 9) {
		p_mtc_car_struct_21->config_data.cfg_backlight = 10;
	}
	printk(s_mtc_config);
	arm_send_multi(MTC_CMD_MCUDATE, 16, p_mcu_date);
	arm_send_multi(MTC_CMD_MCUTIME, 16, p_mcu_time);
	arm_send_multi(MTC_CMD_GET_MCUCONFIG, 512, p_config_data_35);
	printk(log_mtc_mcu_config);
	do {
		printk(str_fmt_02x, *(&config_data->checksum + config_byte));
		check = (config_byte++ & 0xF) == 15;
		if (check) {
			printk(str_newline_0);
		}
	} while (config_byte != 512);
	stw_range_check();
	cfg_canbus = p_mtc_car_struct_21->config_data.cfg_canbus;
	canbus_is_7_or_40 = cfg_canbus == 7;
	if (cfg_canbus != 7) {
		canbus_is_7_or_40 = cfg_canbus == 40;
	}
	if (canbus_is_7_or_40 || cfg_canbus == 21 || cfg_canbus == 36) {
		p_mtc_car_struct_21->car_status.cfg_maxvolume = 40;
	} else {
		if (cfg_canbus == 15) {
			cfg_maxvolume = 35;
		} else {
			cfg_maxvolume = 30;
		}
		p_mtc_car_struct_21->car_status.cfg_maxvolume = cfg_maxvolume;
	}
	mtc_car_struct = p_mtc_car_struct_21;
	if (p_mtc_car_struct_21->car_status.mtc_customer == 4) {
		p_mtc_car_struct_21->_gap4[0] = 1;
		v32 = *off_C09BC724;
		if (v32 != 255) {
			rk29sdk_wifi_power(v32);
		}
	}
	wipe_flag = mtc_car_struct->car_status.wipe_flag;
	mtc_car_struct = p_mtc_car_struct_21;
	v35 = wipe_flag & 1;
	mtc_car_struct = p_mtc_car_struct_21;
	if (mtc_car_struct->car_status.wipe_flag & 1) {
		v35 = 1;
	}
	p_mtc_car_struct_21->car_status._gap0[1] = v35;
	mtc_car_struct->car_status._gap9[16] = v35;
	if (wipe_flag & 8) {
		if (wipe_flag & 2) {
			v37 = 5;
		} else {
			v37 = 6;
		}
		mtc_car_struct->car_status._gap14[1] = v37;
		mtc_car_struct->car_status._gap8[0] = -1;
		capture_add_work(56, 1, 0, 1);
	} else if (mtc_car_struct->car_status._gap0[1]) {
		if (mtc_car_struct->car_status.mtc_customer == 8) {
			msleep(1000u);
		}
		backlight_on();
	}
	mtc_car_struct = p_mtc_car_struct_21;
	_log_car_probe_end = log_car_probe_end;
	p_mtc_car_struct_21->car_status._gap0[0] = 1;
	printk(_log_car_probe_end);
	if (mtc_car_struct->config_data.cfg_bt == 3) {
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
	queue_work(mtc_car_struct->car_comm->mcc_rev_wq, &mtc_car_struct->car_comm->work);

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

int __init
car_init()
{
	platform_driver_register(&mtc_car_driver);
	return 0;
}

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC CAR driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-car");
