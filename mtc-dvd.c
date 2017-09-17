// tmp defines
#define CONFIG_HAS_EARLYSUSPEND

#include <asm/string.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

enum mtc_gpio {
	gpio_MCU_CLK = 167,
	gpio_DVD_STB = 168,
	gpio_MCU_DIN = 169,
	gpio_MCU_DOUT = 170,
	gpio_DVD_ACK = 174,
	gpio_DVD_DATA = 175,
	gpio_PARROT_RESET = 198,
	gpio_PARROT_BOOT = 199,
};

struct mtc_dvd_drv {
	char _gap0[4];
	u8 play_status;
	char _gap1[23];
	struct input_device *input_dev;
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
	char dvd_byteval_12;
	char _gap13[3];
	struct delayed_work media_work;
	struct delayed_work media_index_work;
	int dvd_intval_13;
	int dvd_intval_14;
	int dvd_intval_15;
	char _gap14[4];
	char dvd_byteval_16;
	char dvd_byteval_17;
	char _gap15[2];
	struct mutex cmd_lock;
	char _gap16[8];
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

static struct mtc_dvd_drv *dvd_dev;

static struct early_suspend dvd_early_suspend;
static struct dev_pm_ops dvd_pm_ops;
static struct platform_driver mtc_dvd_driver;

signed int dvd_poweroff_constprop_10(void);

/* fully decompiled */
void dvd_s_resume(struct early_suspend *h)
{
	(void)h;
	// empty function
}

/* fully decompiled */
void dvd_s_suspend(struct early_suspend *h)
{
	(void)h;

	dvd_poweroff_constprop_10();
}

/* fully decompiled */
int dvd_suspend(struct device *dev)
{
	(void)dev;

	return 0;
}

/* fully decompiled */
int dvd_resume(struct device *dev)
{
	(void)dev;

	return 0;
}

/* fully decompiled */
irqreturn_t dvd_isr(unsigned int irq)
{
	disable_irq_nosync(irq);
	queue_work(dvd_dev->dvd_rev_wq, dvd_dev->dvd_work);

	return IRQ_HANDLED;
}

/* fully decompiled */
int dvd_add_work(int cmd1, int cmd2)
{
	struct mtc_dvd_work *dvd_work; // r0@2

	dvd_work = kmalloc(sizeof(struct mtc_dvd_work), __GFP_IO);

	dvd_work->cmd1 = cmd1;
	dvd_work->cmd2 = cmd2;

	INIT_WORK(dvd_work->work, cmd_work);
	queue_work(mtc_dvd->dvd_wq, &dvd_work->work);

	return 0;
}

/* fully decompiled */
struct list_head *free_folder(struct list_head *folder_list)
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
struct list_head *free_media(struct list_head *media_list)
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

void input_event_func(s16 arg)
{
	char v1;

	v1 = (arg >> 8) & 0xFF;
	vs_send(2, 0x89, &v1, 2);
}

/* fully decompiled */
void cmd_surface(int arg)
{
	if (p_mtc_dvd_drv->surface_flag != arg) {
		p_mtc_dvd_drv->surface_flag = arg;
		if (arg) {
			input_event_func(0x9702);
		} else {
			input_event_func(0x9701);
		}
	}
}

/* fully decompiled */
bool CheckTimeOut_constprop_9(unsigned int timeout)
{
	int usec;
	struct timeval tv;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;

	if (usec < timeout) {
		usec += 1000000;
	}

	return ((usec - timeout) > 49999);
}

int dvd_rev_part_2()
{
	unsigned int v0;  // r4@3
	int v1;		  // r1@4
	int v2;		  // r5@5
	int v3;		  // r5@6
	int v4;		  // r6@6
	unsigned int v5;  // r4@7
	unsigned int v6;  // r7@12
	int v7;		  // r4@14
	unsigned int v8;  // r4@16
	int v9;		  // r1@17
	unsigned int v10; // r4@19
	int v11;	  // r1@20
	int v12;	  // r6@21
	int v14;	  // [sp+0h] [bp-20h]@3
	unsigned int v15; // [sp+4h] [bp-1Ch]@3

	mutex_lock(off_C082A9A4);
	while (2) {
		if (_gpio_get_value(175)) {
			v7 = 0;
		} else {
			gpio_direction_output(174, 0);
			do_gettimeofday(&v14);
			v0 = v15;
			while (1) {
				v2 = _gpio_get_value(175);
				if (v2) {
					break;
				}
				if (CheckTimeOut_constprop_9(v0)) {
					v7 = v2;
					printk(off_C082A9B8, v1);
					gpio_direction_input(174);
					goto LABEL_24;
				}
			}
			v3 = 0;
			v4 = 0;
			while (2) {
				gpio_direction_output(174, 1);
				do_gettimeofday(&v14);
				v5 = v15;
				while (_gpio_get_value(168)) {
					if (CheckTimeOut_constprop_9(v5)) {
						printk(off_C082A9AC[0], v4);
						gpio_direction_input(174);
						v7 = 0;
						goto LABEL_24;
					}
				}
				v3 *= 2;
				if (_gpio_get_value(175)) {
					v3 |= 1u;
				}
				gpio_direction_output(174, 0);
				do_gettimeofday(&v14);
				v6 = v15;
				while (1) {
					v7 = _gpio_get_value(168);
					if (v7) {
						break;
					}
					if (CheckTimeOut_constprop_9(v6)) {
						printk(off_C082A9A8[0], v4);
						gpio_direction_input(174);
						goto LABEL_24;
					}
				}
				v4 = (unsigned __int8)(v4 + 1);
				if (v4 != 16) {
					continue;
				}
				break;
			}
			gpio_direction_input(174);
			do_gettimeofday(&v14);
			v8 = v15;
			while (_gpio_get_value(174)) {
				if (CheckTimeOut_constprop_9(v8)) {
					v7 = 0;
					printk(off_C082A9B0[0], v9);
					goto LABEL_24;
				}
			}
			gpio_direction_output(168, 0);
			do_gettimeofday(&v14);
			v10 = v15;
			while (1) {
				v12 = _gpio_get_value(174);
				if (v12) {
					break;
				}
				if (CheckTimeOut_constprop_9(v10)) {
					v7 = v12;
					printk(off_C082A9B4[0], v11);
					gpio_direction_input(168);
					goto LABEL_24;
				}
			}
			gpio_direction_input(168);
			dvd_add_work(5, v3);
			if (!_gpio_get_value(175)) {
				continue;
			}
			v7 = 1;
		}
		break;
	}
LABEL_24:
	mutex_unlock(off_C082A9A4);
	return v7;
}

/* fully decompiled */
void rev_work(void)
{
	if (p_mtc_dvd_drv->dvd_power_on) {
		dvd_rev_part_2();

		if (p_mtc_dvd_drv->dvd_power_on) {
			enable_irq(p_mtc_dvd_drv->dvd_irq);

			if (p_mtc_dvd_drv->dvd_power_on) {
				dvd_rev_part_2();
			}
		}
	}
}

void stop_work()
{
	int cmd; // r0@4

	if (dvd_dev->dvd_byteval_5) {
		cmd_surface(0);
		if (*(&dvd_dev + 4)) {
			cmd = 0x30F00;
		} else {
			cmd = 0x20F0D;
		}
		cmd_send(cmd);

		queue_delayed_work(dvd_dev->dvd_wq, &dvd_dev->stop_work,
				   msecs_to_jiffies(2000u));
	} else {
		dvd_dev->dvd_byteval_12 = 0;
	}
}

/* fully decompiled */
signed int dvd_poweroff_constprop_10(void)
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

		arm_send_multi(0x9101, 0, 0);
		msleep(100u);
		arm_send(0x104);

		result = 0;
	} else {
		result = -1;
	}

	return result;
}

signed int dvd_power(int pwr)
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

		arm_send(0x103);

		dvd_dev->dvd_byteval_19 = 0;
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
		dvd_dev->dvd_byteval_12 = 0;

		free_folder(&dvd_dev->folder_list);
		free_media(&dvd_dev->media_list);

		memset(dvd_dev->array1, 0, 32);
		memset(dvd_dev->array2, 0, 32);
		memset(dvd_dev->array3, 0, 32);
		memset(dvd_dev->array4, 0, 32);

		*(&dvd_dev + 4) = 0; // unknown offset
		dvd_dev->dvd_power_on = 1;
		dvd_dev->dvd_byteval_18 = 0;
		*(pp_mtc_keys_drv + 0x32) = 0; // unknown offset
		*(pp_dvd_probe + 124) = 24;    // unknown offset

		gpio_direction_input(gpio_DVD_DATA);
		gpio_direction_input(gpio_DVD_STB);
		gpio_direction_input(gpio_DVD_ACK);

		arm_send_multi(0x9102, 0, 0);
		arm_send_multi(0x9100, 0, 0);
		msleep(150u);
		arm_send_multi(0x9103, 0, 0);
		msleep(150u);
		enable_irq(dvd_dev->dvd_irq);
		result = 0;
	} else {
		result = dvd_poweroff_constprop_10();
	}

	return result;
}

/* fully decompiled */
signed int dvd_play_cmd(void)
{
	signed int result; // r0@2

	if (dvd_dev->play_status) {
		result = 0x30D00;
	} else {
		result = 0x20D0F;
	}

	return result;
}

/* fully decompiled */
signed int dvd_stop_cmd(void)
{
	signed int result; // r0@2

	if (dvd_dev->play_status) {
		result = 0x30F00;
	} else {
		result = 0x20F0D;
	}
	return result;
}

/* dirty code */
int dvd_send_command_direct(int result)
{
	int v1;				   // r1@1
	struct mtc_dvd_drv **p_dvd_dev;    // r5@1 MAPDST
	struct mtc_dvd_drv *dvd_dev;       // r3@1 MAPDST
	int dvd_flag;			   // r2@4
	bool v5;			   // r6@4
	char v6;			   // zf@4
	char v7;			   // zf@7
	int v8;				   // r2@10
	int v9;				   // r12@17
	int v10;			   // r4@19
	signed int v11;			   // r2@20
	int v12;			   // r6@24
	int v15;			   // r5@25
	struct delayed_work *dwork_1;      // r1@26
	unsigned int timeout_ms;	   // r0@26
	struct workqueue_struct *media_wq; // r5@27
	unsigned __int32 delay_1;	  // r0@27
	struct delayed_work *dwork;	// r1@28
	int v24;			   // ST04_4@29
	struct mtc_dvd_drv *v25;	   // r3@29
	struct delayed_work *v26;	  // r0@30
	struct mtc_dvd_drv *v27;	   // r4@31
	struct workqueue_struct *wq;       // r5@31
	unsigned __int32 delay;		   // r0@31

	v1 = result;
	p_dvd_dev = pp_mtc_dvd_dev_10;
	dvd_dev = *pp_mtc_dvd_dev_10;
	if (!(*pp_mtc_dvd_dev_10)->dvd_power_on) {
		return result;
	}
	if ((*&dvd_dev->_gap7[6] - 1) > 1u) {
		return dvd_add_work(1, v1);
	}
	dvd_flag = *(pp_mtc_dvd_dev_10 + 4);
	v5 = dvd_flag == 0;
	v6 = result == 0x22220;
	if (result == 0x22220) {
		v6 = dvd_flag == 0;
	}
	if (v6) {
		goto LABEL_36;
	}
	result = 0x32200;
	v7 = v1 == 0x32200;
	if (v1 == 0x32200) {
		v7 = dvd_flag == 1;
	}
	v8 = v7 ? 1 : 0;
	if (v7) {
	LABEL_36:
		dvd_dev->dvd_flag_3 = 0;
		dvd_dev->dvd_flag_4 = 0;
		dvd_dev->dvd_flag_5 = 0;
		cancel_delayed_work_sync(&dvd_dev->media_dwork1);
		result = *p_dvd_dev;
		if (((*p_dvd_dev)->_gap8[0] - 2) <= 1u && *(result + 286)) {
			cancel_delayed_work_sync((result + 452));
			dvd_dev = *pp_mtc_dvd_dev_10;
			dwork = &(*pp_mtc_dvd_dev_10)->dwork1;
			dvd_dev->_gap14[24] = 1;
			result =
			    queue_delayed_work(dvd_dev->media_wq, dwork, 0);
		}
		return result;
	}
	v9 = v1 & 0xFF0000;
	if ((v1 & 0xFF0000) == 0x90000) {
		v26 = &dvd_dev->media_dwork;
		dvd_dev->dvd_flag_3 = v8;
		dvd_dev->dvd_flag_4 = (v1 + 1) | 0x90000;
	LABEL_31:
		cancel_delayed_work_sync(v26);
		v27 = *p_dvd_dev;
		v27->_gap18[4] = 1;
		wq = v27->media_wq;
		delay = msecs_to_jiffies(2000u);
		return queue_delayed_work(wq, &v27->media_dwork, delay);
	}
	if (v9 == 0xD0000) {
		dvd_dev->dvd_flag_3 = v8;
		dvd_dev->dvd_flag_4 = v8;
		dvd_dev->dvd_flag_5 = (v1 >> 8) | 0xD0000;
		v26 = &dvd_dev->media_dwork;
		goto LABEL_31;
	}
	v10 = dvd_dev->_gap121[2] & 1;
	if (!(dvd_dev->_gap121[2] & 1)) {
		result = 0x30F00;
		v11 = 0x20F0D;
		if (!v5) {
			v11 = 0x30F00;
		}
		if (v1 == v11) {
			v24 = v1;
			cancel_delayed_work_sync(&dvd_dev->media_dwork1);
			v25 = *p_dvd_dev;
			v25->dvd_flag_3 = v10;
			v25->dvd_flag_4 = v10;
			v25->dvd_flag_5 = v10;
			v25->_gap18[5] = v10;
			result = dvd_add_work(1, v24);
		} else {
			if (v9 != 0xB0000) {
				return dvd_add_work(1, v1);
			}
			v12 = v1 + 1;
			if (v12 <= dvd_dev->media_count) {
				dvd_dev->media_idx = v1;
				dvd_add_work(6, 0xE800);
				dvd_dev = *p_dvd_dev;
				p_dvd_dev = pp_mtc_dvd_dev_10;
				v15 = (*p_dvd_dev)->_gap8[0];
				dvd_dev->dvd_flag_3 = v12 | 0xB0000;
				if (v15 == 1) {
					dvd_dev->dvd_length = 0;
					dvd_dev->dvd_position = 0;
					_memzero(dvd_dev->array1, 32);
					_memzero((*p_dvd_dev)->array2, 32);
					_memzero((*p_dvd_dev)->array3, 32);
					_memzero((*p_dvd_dev)->array4, 32);
					cancel_delayed_work_sync(
					    &(*p_dvd_dev)->media_dwork1);
					dvd_dev = *p_dvd_dev;
					timeout_ms = 500;
					dvd_dev->_gap18[4] = 1;
				} else {
					cancel_delayed_work_sync(
					    &dvd_dev->dwork1);
					dvd_dev = *p_dvd_dev;
					dwork_1 = &(*p_dvd_dev)->dwork1;
					dvd_dev->_gap14[24] = 1;
					queue_delayed_work(dvd_dev->media_wq,
							   dwork_1, 0);
					cancel_delayed_work_sync(
					    &(*p_dvd_dev)->media_dwork1);
					dvd_dev = *p_dvd_dev;
					timeout_ms = 1500;
					dvd_dev->_gap18[5] = 1;
				}
				media_wq = dvd_dev->media_wq;
				delay_1 = msecs_to_jiffies(timeout_ms);
				result = queue_delayed_work(
				    media_wq, &dvd_dev->media_dwork1, delay_1);
			}
		}
	}
	return result;
}

void cmd_work(int a1)
{
	// too many code...
}

/* dirty code */
int dvd_get_folder(int result, const char *buf_1, int a3, int a4)
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
int dvd_get_media(int result, const char *a2, int a3, int a4)
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
int dvd_get_media_cnt(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->media_count);
}

/* fully decompiled */
int dvd_get_folder_cnt(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->folders_count);
}

/* fully decompiled */
int dvd_get_media_idx(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->media_idx);
}

/* fully decompiled */
int dvd_get_folder_idx(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->folder_idx);
}

/* dirty code */
int dvd_get_media_title(const char *buf)
{
	int v1;				// r3@0
	struct mtc_dvd_drv **p_dvd_dev; // r6@1
	signed int v4;			// r4@1
	int v5;				// r7@1
	size_t s_len;			// r0@2
	int v7;				// r3@2
	size_t pos;			// r0@3

	p_dvd_dev = pp_mtc_dvd_dev_14;
	v4 = 1;
	v5 = (*pp_mtc_dvd_dev_14)->array1[0];
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
int dvd_get_length(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->dvd_length);
}

/* fully decompiled */
int dvd_get_position(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->dvd_position);
}

/* dirty code */
void dvd_send_command(int command)
{
	int v1; // r3@7

	if (p_mtc_dvd_drv->dvd_power_on) {
		if (*(pp_mtc_dvd_dev_15 + 4) == 1) {
			v1 = command & 0xFF0000;
			if ((command & 0xFF0000) == 0x20000) {
				command = (command & 0xFF00) | 0x30000;
			} else {
				if (v1 == 0x30000) {
					dvd_send_command_direct(0x100000);
					return;
				}

				if (v1 == 0xF0000) {
					dvd_send_command_direct(
					    (command & 0xFF00) | 0x570000);
					return;
				}
			}
		}

		dvd_send_command_direct(command);
		return;
	}

	if ((command & 0xFF0000) == 0xF0000) {
		p_mtc_dvd_drv->_gap19[20] = command & 0xFF00;
	}
}

/* fully decompiled */
static int dvd_probe(struct platform_device *pdev)
{
	pr_info("dvd_probe\n");

	dvd_dev = kmalloc(sizeof(struct mtc_dvd_drv), 0x280);
	// TODO: alloc check

	mutex_init(&dvd_dev->cmd_lock);

	dvd_dev->dvd_wq = create_singlethread_workqueue("dvd_rev_wq");

	INIT_DELAYED_WORK(dvd_dev->rev_dwork, rev_work);
	INIT_DELAYED_WORK(dvd_dev->stop_dwork, stop_work);

	INIT_DELAYED_WORK(dvd_dev->media_work, media_work);
	INIT_DELAYED_WORK(dvd_dev->media_index_work, media_index_work);

	list_init(dvd_dev->folder_list);
	list_init(dvd_dev->media_list);

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

	dvd_dev->media_wq = create_singlethread_workqueue("dvd_wq");

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
static int __devexit dvd_remove(struct platform_device *pdev)
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
