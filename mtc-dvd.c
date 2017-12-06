#include <asm/string.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <stdint.h>

#include "mtc_shared.h"
#include "mtc-car.h"

struct mtc_dvd_drv {
	char _gap0[4];
	u8 play_status;
	u8 _gap1[23];
	struct input_dev *input_dev;
	char _gap2[24];
	void (*callback)(void);
	char _gap3[40];
	int dvd_power_on;
	unsigned int dvd_irq;
	char _gap4[4];
	struct workqueue_struct *dvd_rev_wq;
	struct work_struct dvd_work;
	char _gap5[108];
	int dvd_intval_2;
	char _gap6[4];
	struct workqueue_struct *dvd_wq;
	char _gap7[6];
	u16 dvd_u16val_3;
	char _gap8[8];
	struct list_head folder_list;
	struct list_head media_list;
	char dvd_byteval_4;
	char unk_byte;
	char dvd_byteval_5;
	char dvd_byteval_6;
	char _gap9[1];
	char array1[32];
	char array2[32];
	char array3[32];
	char array4[32];
	char _gap10[1];
	u16 folders_count;
	u16 media_count;
	u16 dvd_u16val_7;
	s16 folder_idx;
	u16 dvd_u16val_8;
	s16 media_idx;
	u16 dvd_length;
	u16 dvd_position;
	char _gap11[10];
	u16 dvd_u16val_9;
	u16 surface_flag;
	u16 dvd_u16val_10;
	char dvd_byteval_11;
	char _gap12[1];
	struct delayed_work stop_work;
	char do_media_index;
	char _gap13[3];
	struct delayed_work media_work;
	struct delayed_work media_index_work;
	u32 dvd_intval_13;
	int dvd_intval_14;
	int dvd_intval_15;
	char _gap14[4];
	char dvd_byteval_16;
	char dvd_byteval_17;
	char _gap15[2];
	struct mutex cmd_lock;
	char _gap16[4];
	int dvd_command_byte2;
	char dvd_byteval_18;
	char dvd_byteval_19;
	char _gap17[2];
};

struct mtc_dvd_work {
	u32 cmd1;
	u32 cmd2;
	char gap0[12];
	struct work_struct work;
};

static struct platform_driver mtc_dvd_driver;
static struct early_suspend dvd_early_suspend;
static struct mutex dvd_rev_mutex;
static int dvd_value1;
static u8 dvd_cmd_bit_count = 24u;
static int dvd_value2;
static int dvd_value3;
static int dvd_flag1;

static struct mtc_dvd_drv *dvd_dev;
static struct dev_pm_ops dvd_pm_ops;

static void cmd_work(struct work_struct *work);
static int dvd_poweroff(void);

/*
 * =========================================
 *	прочие функции
 * =========================================
 */

/* fully decompiled */
static bool
CheckTimeOut(long timeout)
{
	struct timeval tv;
	long usec;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;

	if (usec < timeout) {
		usec += 1000000;
	}

	return ((usec - timeout) > 49999);
}

/*
 * =========================================
 *	working with DVD drive
 * =========================================
 */

/* fully decompiled */
static void
dvd_s_resume(struct early_suspend *h)
{
	(void)h;
	// empty function
}

/* fully decompiled */
static void
dvd_s_suspend(struct early_suspend *h)
{
	(void)h;

	dvd_poweroff();
}

/* fully decompiled */
static int
dvd_suspend(struct device *dev)
{
	(void)dev;

	return 0;
}

/* fully decompiled */
static int
dvd_resume(struct device *dev)
{
	(void)dev;

	return 0;
}

/* fully decompiled */
static irqreturn_t
dvd_isr(int irq, void *data)
{
	(void)data;
	disable_irq_nosync((unsigned int)irq);
	queue_work(dvd_dev->dvd_rev_wq, &dvd_dev->dvd_work);

	return IRQ_HANDLED;
}

/* fully decompiled */
static int
dvd_add_work(u32 cmd1, u32 cmd2)
{
	struct mtc_dvd_work *dvd_work; // r0@2

	dvd_work = kmalloc(sizeof(struct mtc_dvd_work), __GFP_IO);

	dvd_work->cmd1 = cmd1;
	dvd_work->cmd2 = cmd2;

	INIT_WORK(&dvd_work->work, cmd_work);
	queue_work(dvd_dev->dvd_wq, &dvd_work->work);

	return 0;
}

/* fully decompiled */
struct list_head *
free_folder(struct list_head *folder_list)
{
	struct list_head *v1; // r6@1
	struct list_head *v2; // r3@1
	struct list_head *v3; // r4@2
	struct list_head *i;  // r5@2

	v1 = folder_list;
	v2 = folder_list->next;
	if (folder_list != folder_list->next) {
		v3 = v2->next;
		for (i = v2->next;; i = v3) {
			list_del(v2);
			folder_list = kfree(v2);
			v2 = i;
			v3 = v3->next;
			if (v1 == i) {
				break;
			}
		}
	}
	return folder_list;
}

/* fully decompiled */
struct list_head *
free_media(struct list_head *media_list)
{
	struct list_head *v1; // r6@1
	struct list_head *v2; // r3@1
	struct list_head *v3; // r4@2
	struct list_head *i;  // r5@2

	v1 = media_list;
	v2 = media_list->next;
	if (media_list != media_list->next) {
		v3 = v2->next;
		for (i = v2->next;; i = v3) {
			list_del(v2);
			media_list = kfree(v2);
			v2 = i;
			v3 = v3->next;
			if (v1 == i) {
				break;
			}
		}
	}
	return media_list;
}

/* fully decompiled */
static void
input_event_func(u16 cmd)
{
	u8 buf[2];

	buf[0] = (cmd >> 8) & 0xFF;
	buf[1] = cmd & 0xFF;

	vs_send(2, 0x89, buf, 2);
}

/* fully decompiled */
static void
cmd_surface(u16 cmd)
{
	if (dvd_dev->surface_flag != cmd) {
		dvd_dev->surface_flag = cmd;
		if (cmd) {
			input_event_func(0x9702);
		} else {
			input_event_func(0x9701);
		}
	}
}

/* fully decompiled */
static int
dvd_rev_part_2(void)
{
	int dvd_data;
	u32 cmd;
	int bit_pos;
	int stb_val;
	int dvd_ack;
	struct timeval tv;

	mutex_lock(&dvd_rev_mutex);
	while (2) {
		if (gpio_get_value(gpio_DVD_DATA)) {
			stb_val = 0;
		} else {
			gpio_direction_output(gpio_DVD_ACK, 0);
			do_gettimeofday(&tv);
			while (1) {
				dvd_data = gpio_get_value(gpio_DVD_DATA);
				if (dvd_data) {
					break;
				}
				if (CheckTimeOut(tv.tv_usec)) {
					stb_val = dvd_data;
					printk("!!!!!!! DREV ERROR 0\n");
					gpio_direction_input(gpio_DVD_ACK);
					goto LABEL_24;
				}
			}

			cmd = 0;
			bit_pos = 0;
			while (2) {
				gpio_direction_output(gpio_DVD_ACK, 1);
				do_gettimeofday(&tv);
				while (gpio_get_value(gpio_DVD_STB)) {
					if (CheckTimeOut(tv.tv_usec)) {
						printk("!!!!!!! DREV ERROR 1 "
						       "-%d\n",
						       bit_pos);
						gpio_direction_input(
						    gpio_DVD_ACK);
						stb_val = 0;
						goto LABEL_24;
					}
				}

				cmd *= 2;
				if (gpio_get_value(gpio_DVD_DATA)) {
					cmd |= 1u;
				}

				gpio_direction_output(gpio_DVD_ACK, 0);
				do_gettimeofday(&tv);
				while (1) {
					stb_val = gpio_get_value(gpio_DVD_STB);
					if (stb_val) {
						break;
					}
					if (CheckTimeOut(tv.tv_usec)) {
						printk("!!!!!!! DREV ERROR 2 "
						       "-%d\n",
						       bit_pos);
						gpio_direction_input(
						    gpio_DVD_ACK);
						goto LABEL_24;
					}
				}

				bit_pos = (bit_pos + 1);
				if (bit_pos != 16) {
					continue;
				}

				break;
			}

			gpio_direction_input(gpio_DVD_ACK);
			do_gettimeofday(&tv);
			while (gpio_get_value(gpio_DVD_ACK)) {
				if (CheckTimeOut(tv.tv_usec)) {
					stb_val = 0;
					printk("!!!!!!! DREV ERROR 3\n");
					goto LABEL_24;
				}
			}

			gpio_direction_output(gpio_DVD_STB, 0);
			do_gettimeofday(&tv);
			while (1) {
				dvd_ack = gpio_get_value(gpio_DVD_ACK);
				if (dvd_ack) {
					break;
				}
				if (CheckTimeOut(tv.tv_usec)) {
					stb_val = dvd_ack;
					printk("!!!!!!! DREV ERROR 4\n");
					gpio_direction_input(gpio_DVD_STB);
					goto LABEL_24;
				}
			}

			gpio_direction_input(gpio_DVD_STB);
			dvd_add_work(5, cmd);
			if (!gpio_get_value(gpio_DVD_DATA)) {
				continue;
			}

			stb_val = 1;
		}

		break;
	}

LABEL_24:
	mutex_unlock(&dvd_rev_mutex);

	return stb_val;
}

/* fully decompiled */
static void
rev_work(struct work_struct *work)
{
	(void)work;

	if (dvd_dev->dvd_power_on) {
		dvd_rev_part_2();

		if (dvd_dev->dvd_power_on) {
			enable_irq(dvd_dev->dvd_irq);

			if (dvd_dev->dvd_power_on) {
				dvd_rev_part_2();
			}
		}
	}
}

/* fully decompiled */
static void
cmd_send(u32 cmd)
{
	struct timeval tv;
	long usec1;
	long usec2;

	disable_irq_nosync(dvd_dev->dvd_irq);
	if (dvd_dev->dvd_power_on) {
		mutex_lock(&dvd_rev_mutex);

		gpio_direction_output(gpio_DVD_DATA, 0);
		do_gettimeofday(&tv);

		while (gpio_get_value(gpio_DVD_ACK)) {
			if (CheckTimeOut(tv.tv_usec)) {
				printk("!!!!!!! DSND ERROR 0\n");
				gpio_direction_input(gpio_DVD_DATA);
				goto LABEL_28;
			}
		}

		gpio_direction_output(gpio_DVD_DATA, 1);
		while (!gpio_get_value(gpio_DVD_ACK)) {
			if (CheckTimeOut(tv.tv_usec)) {
				printk("!!!!!!! DSND ERROR 1\n");
				gpio_direction_input(gpio_DVD_DATA);
				goto LABEL_28;
			}
		}

		if (dvd_cmd_bit_count > 0) {
			u8 bit_pos = 0;
			int bit;
			const char *err_str;
			unsigned int err_code;

		LABEL_12:
			bit = cmd & (0x800000 >> bit_pos);
			if (cmd & (0x800000 >> bit_pos)) {
				bit = 1;
			}

			gpio_direction_output(gpio_DVD_DATA, bit);
			gpio_direction_output(gpio_DVD_STB, 0);
			do_gettimeofday(&tv);
			usec1 = tv.tv_usec;
			do {
				if (!gpio_get_value(gpio_DVD_ACK)) {
					gpio_direction_output(gpio_DVD_STB, 1);
					do_gettimeofday(&tv);
					usec2 = tv.tv_usec;
					while (!gpio_get_value(gpio_DVD_ACK)) {
						if (CheckTimeOut(usec2)) {
							err_str = "!!!!!!! "
								  "DSND ERROR "
								  "3 -%d\n";
							err_code = bit_pos;
							goto LABEL_32;
						}
					}

					bit_pos++;
					if (dvd_cmd_bit_count >
					    bit_pos) // dvd_cmd_bit_count
					{
						goto LABEL_12;
					}
					goto LABEL_21;
				}
			} while (!CheckTimeOut(usec1));
			err_code = bit_pos;
			err_str = "!!!!!!! DSND ERROR 2 -%d\n";
		LABEL_32:
			printk(err_str, err_code);
			gpio_direction_input(gpio_DVD_STB);
			gpio_direction_input(gpio_DVD_DATA);
		} else {
		LABEL_21:
			gpio_direction_input(gpio_DVD_DATA);
			gpio_direction_input(gpio_DVD_STB);
			gpio_direction_output(gpio_DVD_ACK, 0);

			do_gettimeofday(&tv);
			while (gpio_get_value(gpio_DVD_STB)) {
				if (CheckTimeOut(tv.tv_usec)) {
					printk("!!!!!!! DSND ERROR 4\n");
					gpio_direction_input(gpio_DVD_ACK);
					goto LABEL_28;
				}
			}

			gpio_direction_output(gpio_DVD_ACK, 1);
			do_gettimeofday(&tv);
			do {
				if (gpio_get_value(gpio_DVD_STB)) {
					gpio_direction_input(gpio_DVD_ACK);
					goto LABEL_28;
				}
			} while (!CheckTimeOut(tv.tv_usec));

			printk("!!!!!!! DSND ERROR 5\n");
			gpio_direction_input(gpio_DVD_ACK);
		}
	LABEL_28:
		mutex_unlock(&dvd_rev_mutex);
		if (dvd_dev->dvd_power_on) {
			enable_irq(dvd_dev->dvd_irq);
			if (dvd_dev->dvd_power_on) {
				dvd_rev_part_2();
			}
		}
	}
}

/* fully decompiled */
static void
media_index_work(struct work_struct *work)
{
	(void)work;

	if (dvd_dev->dvd_power_on) {
		if (dvd_dev->do_media_index == 1) {
			queue_delayed_work(dvd_dev->dvd_wq,
					   &dvd_dev->media_index_work,
					   msecs_to_jiffies(1000u));
		} else {
			if (dvd_dev->dvd_byteval_11 & 1) {
				dvd_dev->dvd_intval_13 = 0;
			} else {
				u32 dvd_cmd_1 = dvd_dev->dvd_intval_13;

				if (dvd_cmd_1) {
					if ((dvd_flag1)&0xFF) {
						u32 cmd_b3 = dvd_cmd_1 << 24;
						cmd_send((dvd_cmd_1 & 0xFF00) |
							 0x620000);
						cmd_send((cmd_b3 >> 16) |
							 0x630000);
					} else {
						cmd_send(dvd_cmd_1 & 0xFF00 |
							 0xB0000 | ((dvd_cmd_1 ^
								     0xB00u) >>
								    8));
						cmd_send(dvd_cmd_1 ^ 0xC |
							 (dvd_cmd_1 << 8) &
							     0xFFFF |
							 0xC0000);
					}

					dvd_dev->dvd_intval_13 = 0;
					dvd_dev->dvd_byteval_6 = 1;
				}
			}

			dvd_dev->dvd_byteval_17 = 0;
		}
	}
}

void media_work(struct work_struct *work)
{
	// too many code
}

static void
stop_work(struct work_struct *work)
{
	int cmd;

	(void)work;

	if (dvd_dev->dvd_byteval_5) { // ?
		cmd_surface(0);

		if (*(&dvd_dev + 4)) { // ?
			cmd = 0x30F00;
		} else {
			cmd = 0x20F0D;
		}
		cmd_send(cmd);

		queue_delayed_work(dvd_dev->dvd_wq, &dvd_dev->stop_work,
				   msecs_to_jiffies(2000u));
	} else {
		dvd_dev->dvd_byteval_12 = 0; // ?
	}
}

/* fully decompiled */
static int
dvd_poweroff(void)
{
	signed int result; // r0@2

	if (dvd_dev->dvd_power_on) {
		dvd_dev->dvd_power_on = 0;
		cancel_delayed_work_sync(&dvd_dev->stop_work);
		cancel_delayed_work_sync(&dvd_dev->media_work);
		cancel_delayed_work_sync(&dvd_dev->media_index_work);
		flush_workqueue(dvd_dev->dvd_wq);

		disable_irq_nosync(dvd_dev->dvd_irq);
		gpio_direction_input(gpio_DVD_DATA);
		gpio_direction_input(gpio_DVD_STB);
		gpio_direction_input(gpio_DVD_ACK);

		arm_send_multi(MTC_CMD_AUDIO_DVD_OFF, 0, 0);
		msleep(100u);
		arm_send(MTC_CMD_DVD_PWR_OFF);

		result = 0;
	} else {
		result = -1;
	}

	return result;
}

/* contained unknown fields */
static int
dvd_power(int pwr)
{
	signed int result; // r0@2

	printk("--mtc dvd %d\n", pwr);

	if (pwr) {			     // power_on?
		if (dvd_dev->dvd_power_on) { // already powered on?
			dvd_dev->dvd_power_on = 0;
			cancel_delayed_work_sync(&dvd_dev->stop_work);
			cancel_delayed_work_sync(&dvd_dev->media_work);
			cancel_delayed_work_sync(&dvd_dev->media_index_work);
			flush_workqueue(dvd_dev->dvd_wq);
			disable_irq_nosync(dvd_dev->dvd_irq);
		}

		arm_send(MTC_CMD_DVD_PWR_ON);

		dvd_dev->dvd_command_byte2 = 0;
		dvd_dev->dvd_intval_13 = 0;
		dvd_dev->dvd_intval_14 = 0;
		dvd_dev->dvd_intval_15 = 0;
		dvd_dev->dvd_byteval_16 = 0;
		dvd_dev->dvd_byteval_17 = 0;
		dvd_dev->dvd_intval_2 = 0;
		dvd_dev->dvd_byteval_4 = 0;
		dvd_dev->dvd_u16val_3 = 0;
		dvd_dev->dvd_byteval_5 = 0;
		dvd_dev->dvd_byteval_6 = 0;
		dvd_dev->dvd_u16val_9 = 0;
		dvd_dev->surface_flag = 0;
		dvd_dev->dvd_u16val_10 = 0;
		dvd_dev->dvd_byteval_11 = 0;
		dvd_dev->media_count = 0;
		dvd_dev->dvd_length = 0;
		dvd_dev->dvd_position = 0;
		dvd_dev->dvd_u16val_7 = 0;
		dvd_dev->folder_idx = -1;
		dvd_dev->dvd_u16val_8 = 0;
		dvd_dev->media_idx = 0;
		dvd_dev->do_media_index = 0;

		free_folder(&dvd_dev->folder_list);
		free_media(&dvd_dev->media_list);

		memset(dvd_dev->array1, 0, 32);
		memset(dvd_dev->array2, 0, 32);
		memset(dvd_dev->array3, 0, 32);
		memset(dvd_dev->array4, 0, 32);

		dvd_flag1 = 0;
		dvd_dev->dvd_power_on = 1;
		dvd_dev->dvd_command_byte2 = 0;
		car_struct.car_status._gap81[0] = 0;
		dvd_cmd_bit_count = 24;

		gpio_direction_input(gpio_DVD_DATA);
		gpio_direction_input(gpio_DVD_STB);
		gpio_direction_input(gpio_DVD_ACK);

		arm_send_multi(0x9102, 0, 0);
		arm_send_multi(MTC_CMD_AUDIO_DVD_ON, 0, 0);
		msleep(150u);
		arm_send_multi(0x9103, 0, 0);
		msleep(150u);
		enable_irq(dvd_dev->dvd_irq);
		result = 0;
	} else {
		result = dvd_poweroff();
	}

	return result;
}

/* fully decompiled */
static u32
dvd_play_cmd(void)
{
	u32 cmd;

	if (dvd_dev->play_status) {
		cmd = 0x30D00;
	} else {
		cmd = 0x20D0F;
	}

	return cmd;
}

/* fully decompiled */
static u32
dvd_stop_cmd(void)
{
	u32 cmd; // r0@2

	if (dvd_dev->play_status) {
		cmd = 0x30F00;
	} else {
		cmd = 0x20F0D;
	}

	return cmd;
}

/* dirty code */
static void
dvd_send_command_direct(u32 command)
{
	int dvd_flag;		 // r2@4
	bool v5;		 // r6@4
	char v6;		 // zf@4
	char v7;		 // zf@7
	int v8;			 // r2@10
	int v10;		 // r12@17
	int v11;		 // r4@19
	signed int cmd;		 // r2@20
	int media_count;	 // r6@24
	int v16;		 // r5@25
	unsigned int timeout_ms; // r0@26

	if (!dvd_dev->dvd_power_on) {
		return;
	}

	if ((dvd_dev->dvd_u16val_3 - 1) > 1u) {
	LABEL_3:
		dvd_add_work(1, command);
		return;
	}

	dvd_flag = (dvd_flag1 & 0xFF);
	v5 = dvd_flag == 0;
	v6 = command == 0x22220;
	if (command == 0x22220) {
		v6 = dvd_flag == 0;
	}
	if (v6) {
		goto LABEL_36;
	}
	v7 = command == 0x32200;
	if (command == 0x32200) {
		v7 = dvd_flag == 1;
	}
	v8 = v7 ? 1 : 0;
	if (v7) {
	LABEL_36:
		dvd_dev->dvd_intval_13 = 0;
		dvd_dev->dvd_intval_14 = 0;
		dvd_dev->dvd_intval_15 = 0;
		cancel_delayed_work_sync(&dvd_dev->media_index_work);
		if ((dvd_dev->dvd_byteval_4 - 2) <= 1u &&
		    *&dvd_dev->dvd_byteval_5) {
			cancel_delayed_work_sync(&dvd_dev->stop_work);
			dvd_dev->do_media_index = 1;
			queue_delayed_work(dvd_dev->dvd_wq, &dvd_dev->stop_work,
					   0);
		}
		return;
	}
	v10 = command & 0xFF0000;
	if ((command & 0xFF0000) == 0x90000) {
		dvd_dev->dvd_intval_13 = v8;
		dvd_dev->dvd_intval_14 = (command + 1) | 0x90000;
	LABEL_31:
		cancel_delayed_work_sync(&dvd_dev->media_work);
		dvd_dev->dvd_byteval_16 = 1;
		queue_delayed_work(dvd_dev->dvd_wq, &dvd_dev->media_work,
				   msecs_to_jiffies(2000u));
		return;
	}
	if (v10 == 0xD0000) {
		dvd_dev->dvd_intval_13 = v8;
		dvd_dev->dvd_intval_14 = v8;
		dvd_dev->dvd_intval_15 = (command >> 8) | 0xD0000;
		goto LABEL_31;
	}
	v11 = dvd_dev->dvd_byteval_11 & 1;
	if (!(dvd_dev->dvd_byteval_11 & 1)) {
		cmd = 0x20F0D;
		if (!v5) {
			cmd = 0x30F00;
		}
		if (command == cmd) {
			cancel_delayed_work_sync(&dvd_dev->media_index_work);
			dvd_dev->dvd_intval_13 = v11;
			dvd_dev->dvd_intval_14 = v11;
			dvd_dev->dvd_intval_15 = v11;
			dvd_dev->dvd_byteval_17 = v11;
			dvd_add_work(1, command);
		} else {
			if (v10 != 0xB0000) {
				goto LABEL_3;
			}
			media_count = command + 1;
			if (media_count <= dvd_dev->media_count) {
				dvd_dev->media_idx = command;
				dvd_add_work(6, 0xE800);
				v16 = dvd_dev->dvd_byteval_4;
				dvd_dev->dvd_intval_13 = media_count | 0xB0000;

				if (v16 == 1) {
					dvd_dev->dvd_length = 0;
					dvd_dev->dvd_position = 0;
					memset(dvd_dev->array1, 0, 32);
					memset(dvd_dev->array2, 0, 32);
					memset(dvd_dev->array3, 0, 32);
					memset(dvd_dev->array4, 0, 32);
					cancel_delayed_work_sync(
					    &dvd_dev->media_index_work);
					timeout_ms = 500;
					dvd_dev->dvd_byteval_16 = 1;
				} else {
					cancel_delayed_work_sync(
					    &dvd_dev->stop_work);
					dvd_dev->do_media_index = 1;
					queue_delayed_work(dvd_dev->dvd_wq,
							   &dvd_dev->stop_work,
							   0);
					cancel_delayed_work_sync(
					    &dvd_dev->media_index_work);
					timeout_ms = 1500;
					dvd_dev->dvd_byteval_17 = 1;
				}

				queue_delayed_work(
				    dvd_dev->dvd_wq, &dvd_dev->media_index_work,
				    msecs_to_jiffies(timeout_ms));
			}
		}
	}
}

static void
cmd_work(struct work_struct *work)
{
	// too many code...
}

/* dirty code */
int
dvd_get_folder(int result, const char *buf_1, int a3, int a4)
{
	struct list_head *folder_list; // r7@1
	size_t pos;		       // r0@8
	int v7;			       // r4@8
	size_t pos_1;		       // r0@8
	int v9;			       // r6@8
	int v10;		       // r7@8
	size_t v11;		       // r0@9
	int v12;		       // t1@9
	size_t v13;		       // r0@10
	int v14;		       // [sp+0h] [bp-18h]@1

	v14 = a4;
	folder_list = (*pp_mtc_dvd_dev_12)->folder_list[0];
	if ((*pp_mtc_dvd_dev_12)->folder_list != folder_list) {
		result = result;
		if (LOWORD(folder_list[1].next) == result) {
		LABEL_7:
			if (folder_list) {
				pos = strlen(buf_1);
				LOBYTE(v7) = 0;
				sprintf(&buf_1[pos], str_fmt_d_comma,
					LOWORD(folder_list[1].next));
				pos_1 = strlen(buf_1);
				sprintf(&buf_1[pos_1], str_fmt_d_comma,
					HIWORD(folder_list[1].next));
				v9 = &folder_list[1].prev;
				v10 = LOBYTE(folder_list[1].prev);
				do {
					v11 = strlen(buf_1);
					v7 = (v7 + 1);
					sprintf(&buf_1[v11], str_fmt_d_comma,
						v10);
					v12 = *(v9++ + 1);
					v10 = v12;
				} while (v7 != 31);
				v13 = strlen(buf_1);
				result =
				    sprintf(&buf_1[v13], str_fmt_d, v10, v14);
			}
		} else {
			while (1) {
				folder_list = folder_list->next;
				if ((*pp_mtc_dvd_dev_12)->folder_list ==
				    folder_list) {
					break;
				}
				if (LOWORD(folder_list[1].next) == result) {
					goto LABEL_7;
				}
			}
		}
	}
	return result;
}

/* dirty code */
int
dvd_get_media(int result, const char *a2, int a3, int a4)
{
	const char *v4;		      // r5@1
	struct list_head *media_list; // r2@1
	list_head *next;	      // r4@1 MAPDST
	size_t v8;		      // r0@8
	int v9;			      // r2@8
	int v10;		      // r6@8
	int v11;		      // r7@8
	int v12;		      // t1@8
	size_t v13;		      // r0@9
	int v14;		      // t1@9
	size_t v15;		      // r0@10
	int v16;		      // [sp+0h] [bp-18h]@1

	v16 = a4;
	v4 = a2;
	media_list = (*pp_mtc_dvd_dev_13)->media_list;
	next = media_list->next;
	if (media_list->next != media_list) {
		result = result;
		if (LOWORD(next[1].next) == result) {
		LABEL_7:
			if (next) {
				v8 = strlen(a2);
				v9 = LOWORD(next[1].next);
				LOBYTE(next) = 0;
				sprintf(&v4[v8], str_fmt_d_comma_0, v9);
				v12 = BYTE2(next[1].next);
				v10 = &next[1].next + 2;
				v11 = v12;
				do {
					v13 = strlen(v4);
					next = (next + 1);
					sprintf(&v4[v13], str_fmt_d_comma_0,
						v11);
					v14 = *(v10++ + 1);
					v11 = v14;
				} while (next != 31);
				v15 = strlen(v4);
				result =
				    sprintf(&v4[v15], str_fmt_d_0, v11, v16);
			}
		} else {
			while (1) {
				next = next->next;
				if (next == media_list) {
					break;
				}
				if (LOWORD(next[1].next) == result) {
					goto LABEL_7;
				}
			}
		}
	}
	return result;
}

/* fully decompiled */
static int
dvd_get_media_cnt(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->media_count);
}

/* fully decompiled */
static int
dvd_get_folder_cnt(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->folders_count);
}

/* fully decompiled */
static int
dvd_get_media_idx(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->media_idx);
}

/* fully decompiled */
static int
dvd_get_folder_idx(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->folder_idx);
}

/* dirty code */
static int
dvd_get_media_title(const char *buf)
{
	int v1;				// r3@0
	struct mtc_dvd_drv **p_dvd_dev; // r6@1
	signed int v4;			// r4@1
	int v5;				// r7@1
	size_t s_len;			// r0@2
	int v7;				// r3@2
	size_t pos;			// r0@3

	v4 = 1;
	v5 = dvd_dev->array1[0];
	do {
		s_len = strlen(buf);
		sprintf(&buf[s_len], str_fmt_d_comma_1, v5);
		v7 = *p_dvd_dev + v4++;
		v5 = *(v7 + 289);
	} while (v4 != 128);
	pos = strlen(buf);

	return sprintf(&buf[pos], "%d", v5);
}

/* fully decompiled */
static int
dvd_get_length(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->dvd_length);
}

/* fully decompiled */
static int
dvd_get_position(char *buf)
{
	return sprintf(buf, "%d", dvd_dev->dvd_position);
}

/* fully decompiled */
static void
dvd_send_command(u32 command)
{
	u32 dvd_cmd_b3 = command & 0xFF0000;

	if (dvd_dev->dvd_power_on) {
		if (dvd_flag1 == 1) {
			if (dvd_cmd_b3 == 0x20000) {
				command = (command & 0xFF00) | 0x30000;
			} else {
				if (dvd_cmd_b3 == 0x30000) {
					dvd_send_command_direct(0x100000);
					return;
				}

				if (dvd_cmd_b3 == 0xF0000) {
					dvd_send_command_direct(
					    (command & 0xFF00) | 0x570000);
					return;
				}
			}
		}

		dvd_send_command_direct(command);
		return;
	}

	if (dvd_cmd_b3 == 0xF0000) {
		dvd_dev->dvd_command_byte2 = command & 0xFF00;
	}
}

/* fully decompiled */
static int
dvd_probe(struct platform_device *pdev)
{
	pr_info("dvd_probe\n");

	dvd_dev = kmalloc(sizeof(struct mtc_dvd_drv), 0x280);
	// TODO: alloc check

	mutex_init(&dvd_dev->cmd_lock);

	dvd_dev->dvd_wq = create_singlethread_workqueue("dvd_rev_wq");

	INIT_DELAYED_WORK(&dvd_dev->dvd_work, rev_work);
	INIT_DELAYED_WORK(&dvd_dev->stop_work, stop_work);

	INIT_DELAYED_WORK(&dvd_dev->media_work, media_work);
	INIT_DELAYED_WORK(&dvd_dev->media_index_work, media_index_work);

	INIT_LIST_HEAD(&dvd_dev->folder_list);
	INIT_LIST_HEAD(&dvd_dev->media_list);

	gpio_request(gpio_DVD_DATA, "dvd_data");
	gpio_pull_updown(gpio_DVD_DATA, 0);
	gpio_direction_input(gpio_DVD_DATA);
	gpio_request(gpio_DVD_STB, "dvd_stb");
	gpio_pull_updown(gpio_DVD_STB, 0);
	gpio_direction_input(gpio_DVD_STB);

	gpio_request(gpio_DVD_ACK, "dvd_ack");
	gpio_pull_updown(gpio_DVD_ACK, 0);
	gpio_direction_input(gpio_DVD_ACK);

	dvd_dev->dvd_irq = gpio_DVD_DATA;
	request_threaded_irq(gpio_DVD_DATA, dvd_isr, NULL, IRQF_TRIGGER_FALLING,
			     "dvd", dvd_dev);
	disable_irq_nosync(dvd_dev->dvd_irq);

	dvd_dev->dvd_wq = create_singlethread_workqueue("dvd_wq");

	/* creating input device */
	dvd_dev->input_dev = input_allocate_device();
	dvd_dev->input_dev->phys = "gpio-keys/input1";
	dvd_dev->input_dev->id.vendor = 1;
	dvd_dev->input_dev->id.bustype = BUS_HOST;
	dvd_dev->input_dev->name = pdev->name;
	dvd_dev->input_dev->dev.power.suspend_timer.base = &pdev->dev; // ?
	dvd_dev->input_dev->id.version = 256;
	dvd_dev->input_dev->id.product = 1;
	input_set_capability(dvd_dev->input_dev, EV_KEY, 0x10e); //?
	input_set_capability(dvd_dev->input_dev, EV_KEY, KEY_WAKEUP);
	input_register_device(dvd_dev->input_dev);

	register_early_suspend(&dvd_early_suspend);

	return 0;
}

/* fully decompiled */
static int __devexit
dvd_remove(struct platform_device *pdev)
{
	(void)pdev;

	return 0;
}

/* recovered structures */

static struct early_suspend dvd_early_suspend = {
    .suspend = dvd_s_suspend, .resume = dvd_s_resume,
};

static struct dev_pm_ops dvd_pm_ops = {
    .suspend = dvd_suspend, .resume = dvd_resume,
};

static struct platform_driver mtc_dvd_driver = {
    .probe = dvd_probe,
    .remove = __devexit_p(dvd_remove),
    .driver =
	{
	    .name = "mtc-dvd", .pm = &dvd_pm_ops,
	},
};

module_platform_driver(mtc_dvd_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC DVD driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-dvd");
