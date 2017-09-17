#include <linux/module.h>
#include <linux/irq.h>
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

enum mtc_gpio
{
	gpio_MCU_CLK = 167,
	gpio_DVD_STB = 168,
	gpio_MCU_DIN = 169,
	gpio_MCU_DOUT = 170,
	gpio_DVD_ACK = 174,
	gpio_DVD_DATA = 175,
	gpio_PARROT_RESET = 198,
	gpio_PARROT_BOOT = 199,
};


struct mtc_dvd_drv
{
	char _gap0[4];
	unsigned char dvd_flag_1;
	char _gap1[23];
	struct workqueue_struct *mtc_wq;
	char _gap2[24];
	void (*callback)(void);
	char _gap3[40];
	int dvd_power_on;
	unsigned int dvd_irq;
	char _gap4[4];
	struct workqueue_struct *dvd_wq;
	struct work_struct *dvd_work;
	char _gap5[8];
	void (*rev_work_cb)(void);
	char _gap6[116];
	struct workqueue_struct *media_wq;
	char _gap7[16];
	struct list_head *folder_list[2];
	struct list_head *media_list[2];
	char _gap8[5];
	char array1[32];
	char array2[32];
	char array3[32];
	char array4[32];
	char _gap9[1];
	s16 folders_count;
	s16 media_count;
	char _gap10[2];
	s16 folder_idx;
	char _gap11[2];
	s16 media_idx;
	s16 dvd_length;
	s16 dvd_position;
	char _gap12[12];
	s16 surface_flag;
	char _gap121[4];
	struct delayed_work *dwork1;
	char _gap13[8];
	void (*stop_work_cb)(void);
	struct timer_list *stop_timer;
	char _gap14[28];
	struct delayed_work *media_dwork;
	char _gap15[8];
	void (__cdecl *media_work_cb)(void);
	struct timer_list *media_timer;
	char _gap16[24];
	struct delayed_work *media_dwork1;
	struct timer_list *timer1;
	struct timer_list *timer2;
	struct delayed_work *media_index_work;
	struct timer_list *timer3;
	char _gap17[24];
	int dvd_flag_3;
	int dvd_flag_4;
	int dvd_flag_5;
	char _gap18[8];
	struct mutex *cmd_lock;
	char _gap19[32];
};

static struct mtc_dvd_drv *p_mtc_dvd_drv;

/* fully decompiled */
void
dvd_s_resume(void)
{
	// empty function
}

/* fully decompiled */
int
dvd_suspend(void)
{
	return 0;
}

/* fully decompiled */
int
dvd_resume(void)
{
	return 0;
}

/* fully decompiled */
irqreturn_t
dvd_isr(int irq)
{
	disable_irq_nosync(irq);
	queue_work(p_mtc_dvd_drv->dvd_wq, &p_mtc_dvd_drv->dvd_work);
	return IRQ_HANDLED;
}

int
dvd_add_work(int a1, int a2)
{
	int v2; // r5@1
	int v3; // r4@1
	int v4; // r0@2
	int *v5; // r2@4
	int v6; // r2@4
	int (*v7)(); // r3@4

	v2 = a1;
	v3 = a2;
	if ( *(_DWORD *)(dword_C082A600 + 24) ) {
		v4 = kmem_cache_alloc_trace();
	} else {
		v4 = 16;
	}
	v5 = (int *)dword_C082A604;
	*(_DWORD *)v4 = v2;
	*(_DWORD *)(v4 + 24) = v4 + 24;
	v6 = *v5;
	*(_DWORD *)(v4 + 28) = v4 + 24;
	v7 = off_C082A608;
	*(_DWORD *)(v4 + 20) = 1280;
	*(_DWORD *)(v4 + 4) = v3;
	*(_DWORD *)(v4 + 32) = v7;
	queue_work(*(_DWORD *)(v6 + 248), v4 + 20);
	return 0;
}

/* fully decompiled */
struct list_head *
free_folder(struct list_head *folder_list)
{
	struct list_head *v1; // r6@1
	struct list_head *v2; // r3@1
	struct list_head *v3; // r4@2
	struct list_head *i; // r5@2

	v1 = folder_list;
	v2 = folder_list->next;
	if ( folder_list != folder_list->next )
	{
		v3 = v2->next;
		for ( i = v2->next; ; i = v3 )
		{
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
	struct list_head *i; // r5@2

	v1 = media_list;
	v2 = media_list->next;
	if ( media_list != media_list->next )
	{
		v3 = v2->next;
		for ( i = v2->next; ; i = v3 )
		{
			list_del(v2);
			media_list = kfree(v2);
			v2 = i;
			v3 = v3->next;
			if ( v1 == i )
			{
				break;
			}
		}
	}
	return media_list;
}

void
input_event_func(s16 arg)
{
	char v1;

	v1 = (arg >> 8) & 0xFF;
	vs_send(2, 0x89, &v1, 2);
}

/* fully decompiled */
void
cmd_surface(int arg)
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
bool
CheckTimeOut_constprop_9(unsigned int timeout)
{
	int usec;
	struct timeval tv;

	do_gettimeofday(&tv);
	usec = tv.tv_usec;

	if (usec < timeout) {
		usec += 1000000;
	}

	return usec - timeout > 49999;
}

int
dvd_rev_part_2()
{
	unsigned int v0; // r4@3
	int v1; // r1@4
	int v2; // r5@5
	int v3; // r5@6
	int v4; // r6@6
	unsigned int v5; // r4@7
	unsigned int v6; // r7@12
	int v7; // r4@14
	unsigned int v8; // r4@16
	int v9; // r1@17
	unsigned int v10; // r4@19
	int v11; // r1@20
	int v12; // r6@21
	int v14; // [sp+0h] [bp-20h]@3
	unsigned int v15; // [sp+4h] [bp-1Ch]@3

	mutex_lock(off_C082A9A4);
	while ( 2 )
	{
		if ( _gpio_get_value(175) )
		{
			v7 = 0;
		}
		else
		{
			gpio_direction_output(174, 0);
			do_gettimeofday(&v14);
			v0 = v15;
			while ( 1 )
			{
				v2 = _gpio_get_value(175);
				if ( v2 )
				{
					break;
				}
				if ( CheckTimeOut_constprop_9(v0) )
				{
					v7 = v2;
					printk(off_C082A9B8, v1);
					gpio_direction_input(174);
					goto LABEL_24;
				}
			}
			v3 = 0;
			v4 = 0;
			while ( 2 )
			{
				gpio_direction_output(174, 1);
				do_gettimeofday(&v14);
				v5 = v15;
				while ( _gpio_get_value(168) )
				{
					if ( CheckTimeOut_constprop_9(v5) )
					{
						printk(off_C082A9AC[0], v4);
						gpio_direction_input(174);
						v7 = 0;
						goto LABEL_24;
					}
				}
				v3 *= 2;
				if ( _gpio_get_value(175) )
				{
					v3 |= 1u;
				}
				gpio_direction_output(174, 0);
				do_gettimeofday(&v14);
				v6 = v15;
				while ( 1 )
				{
					v7 = _gpio_get_value(168);
					if ( v7 )
					{
						break;
					}
					if ( CheckTimeOut_constprop_9(v6) )
					{
						printk(off_C082A9A8[0], v4);
						gpio_direction_input(174);
						goto LABEL_24;
					}
				}
				v4 = (unsigned __int8)(v4 + 1);
				if ( v4 != 16 )
				{
					continue;
				}
				break;
			}
			gpio_direction_input(174);
			do_gettimeofday(&v14);
			v8 = v15;
			while ( _gpio_get_value(174) )
			{
				if ( CheckTimeOut_constprop_9(v8) )
				{
					v7 = 0;
					printk(off_C082A9B0[0], v9);
					goto LABEL_24;
				}
			}
			gpio_direction_output(168, 0);
			do_gettimeofday(&v14);
			v10 = v15;
			while ( 1 )
			{
				v12 = _gpio_get_value(174);
				if ( v12 )
				{
					break;
				}
				if ( CheckTimeOut_constprop_9(v10) )
				{
					v7 = v12;
					printk(off_C082A9B4[0], v11);
					gpio_direction_input(168);
					goto LABEL_24;
				}
			}
			gpio_direction_input(168);
			dvd_add_work(5, v3);
			if ( !_gpio_get_value(175) )
			{
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
void
rev_work(void)
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

/* fully decompiled */
signed int
dvd_poweroff_constprop_10(void)
{
	signed int result;

	if (p_mtc_dvd_drv->dvd_power_on) {
		p_mtc_dvd_drv->dvd_power_on = 0;

		cancel_delayed_work_sync(p_mtc_dvd_drv->dwork1);
		cancel_delayed_work_sync(p_mtc_dvd_drv->media_dwork);
		cancel_delayed_work_sync(p_mtc_dvd_drv->media_dwork1);
		flush_workqueue(p_mtc_dvd_drv->media_wq);

		disable_irq_nosync(p_mtc_dvd_drv->dvd_irq);

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


signed int
dvd_power(int pwr)
{
	signed int result; // r0@2

	printk("--mtc dvd %d", pwr);

	if (pwr) {
		if (p_mtc_dvd_drv->dvd_power_on) {
			p_mtc_dvd_drv->dvd_power_on = 0;
			cancel_delayed_work_sync(p_mtc_dvd_drv->dwork1);
			cancel_delayed_work_sync(p_mtc_dvd_drv->media_dwork);
			cancel_delayed_work_sync(p_mtc_dvd_drv->media_dwork1);
			flush_workqueue(p_mtc_dvd_drv->media_wq);
			disable_irq_nosync(p_mtc_dvd_drv->dvd_irq);
		}

		arm_send(0x103);

		/* unknown flags */
		p_mtc_dvd_drv->_gap19[25] = 0;
		p_mtc_dvd_drv->dvd_flag_3 = 0;
		p_mtc_dvd_drv->dvd_flag_4 = 0;
		p_mtc_dvd_drv->dvd_flag_5 = 0;
		p_mtc_dvd_drv->_gap18[4] = 0;
		p_mtc_dvd_drv->_gap18[5] = 0;
		p_mtc_dvd_drv->_gap6[108] = 0;
		p_mtc_dvd_drv->_gap8[0] = 0;
		p_mtc_dvd_drv->_gap7[6] = 0;
		p_mtc_dvd_drv->_gap8[2] = 0;
		p_mtc_dvd_drv->_gap8[3] = 0;
		p_mtc_dvd_drv->_gap12[10] = 0;
		p_mtc_dvd_drv->surface_flag = 0;
		p_mtc_dvd_drv->_gap121[0] = 0;
		p_mtc_dvd_drv->_gap121[2] = 0;
		p_mtc_dvd_drv->media_count = 0;
		p_mtc_dvd_drv->dvd_length = 0;
		p_mtc_dvd_drv->dvd_position = 0;
		p_mtc_dvd_drv->_gap10[0] = 0;
		p_mtc_dvd_drv->folder_idx = -1;
		p_mtc_dvd_drv->_gap11[0] = 0;
		p_mtc_dvd_drv->media_idx = 0;
		p_mtc_dvd_drv->_gap14[24] = 0;

		free_folder(p_mtc_dvd_drv->folder_list[0]);
		free_media(p_mtc_dvd_drv->media_list[0]);

		__memzero(p_mtc_dvd_drv->array1, 32);
		__memzero(p_mtc_dvd_drv->array2, 32);
		__memzero(p_mtc_dvd_drv->array3, 32);
		__memzero(p_mtc_dvd_drv->array4, 32);

		*(p_mtc_dvd_drv + 4) = 0;
		p_mtc_dvd_drv->dvd_power_on = 1;
		p_mtc_dvd_drv->_gap19[24] = 0;

		*(pp_mtc_keys_drv + 0x32) = 0;
		*(pp_dvd_probe + 124) = 24;

		gpio_direction_input(gpio_DVD_DATA);
		gpio_direction_input(gpio_DVD_STB);
		gpio_direction_input(gpio_DVD_ACK);

		arm_send_multi(0x9102, 0, 0);
		arm_send_multi(0x9100, 0, 0);
		msleep(150u);
		arm_send_multi(0x9103, 0, 0);
		msleep(150u);
		enable_irq(p_mtc_dvd_drv->dvd_irq);
		result = 0;
	} else {
		result = dvd_poweroff_constprop_10();
	}

	return result;
}

signed int
dvd_play_cmd(void)
{
	signed int result; // r0@2

	if (p_mtc_dvd_drv->dvd_flag_1) {
		result = 0x30D00;
	} else {
		result = 0x20D0F;
	}

	return result;
}

signed int
dvd_stop_cmd()
{
	signed int result; // r0@2

	if (p_mtc_dvd_drv->dvd_flag_1) {
		result = 0x30F00;
	} else {
		result = 0x20F0D;
	}
	return result;
}

/* dirty code */
int
dvd_send_command_direct(int result)
{
	int v1; // r1@1
	struct mtc_dvd_drv **p_dvd_dev; // r5@1 MAPDST
	struct mtc_dvd_drv *dvd_dev; // r3@1 MAPDST
	int dvd_flag; // r2@4
	bool v5; // r6@4
	char v6; // zf@4
	char v7; // zf@7
	int v8; // r2@10
	int v9; // r12@17
	int v10; // r4@19
	signed int v11; // r2@20
	int v12; // r6@24
	int v15; // r5@25
	struct delayed_work *dwork_1; // r1@26
	unsigned int timeout_ms; // r0@26
	struct workqueue_struct *media_wq; // r5@27
	unsigned __int32 delay_1; // r0@27
	struct delayed_work *dwork; // r1@28
	int v24; // ST04_4@29
	struct mtc_dvd_drv *v25; // r3@29
	struct delayed_work *v26; // r0@30
	struct mtc_dvd_drv *v27; // r4@31
	struct workqueue_struct *wq; // r5@31
	unsigned __int32 delay; // r0@31

	v1 = result;
	p_dvd_dev = pp_mtc_dvd_dev_10;
	dvd_dev = *pp_mtc_dvd_dev_10;
	if ( !(*pp_mtc_dvd_dev_10)->dvd_power_on )
	{
		return result;
	}
	if ( (*&dvd_dev->_gap7[6] - 1) > 1u )
	{
		return dvd_add_work(1, v1);
	}
	dvd_flag = *(pp_mtc_dvd_dev_10 + 4);
	v5 = dvd_flag == 0;
	v6 = result == 0x22220;
	if ( result == 0x22220 )
	{
		v6 = dvd_flag == 0;
	}
	if ( v6 )
	{
		goto LABEL_36;
	}
	result = 0x32200;
	v7 = v1 == 0x32200;
	if ( v1 == 0x32200 )
	{
		v7 = dvd_flag == 1;
	}
	v8 = v7 ? 1 : 0;
	if ( v7 )
	{
LABEL_36:
		dvd_dev->dvd_flag_3 = 0;
		dvd_dev->dvd_flag_4 = 0;
		dvd_dev->dvd_flag_5 = 0;
		cancel_delayed_work_sync(&dvd_dev->media_dwork1);
		result = *p_dvd_dev;
		if ( ((*p_dvd_dev)->_gap8[0] - 2) <= 1u && *(result + 286) )
		{
			cancel_delayed_work_sync((result + 452));
			dvd_dev = *pp_mtc_dvd_dev_10;
			dwork = &(*pp_mtc_dvd_dev_10)->dwork1;
			dvd_dev->_gap14[24] = 1;
			result = queue_delayed_work(dvd_dev->media_wq, dwork, 0);
		}
		return result;
	}
	v9 = v1 & 0xFF0000;
	if ( (v1 & 0xFF0000) == 0x90000 )
	{
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
	if ( v9 == 0xD0000 )
	{
		dvd_dev->dvd_flag_3 = v8;
		dvd_dev->dvd_flag_4 = v8;
		dvd_dev->dvd_flag_5 = (v1 >> 8) | 0xD0000;
		v26 = &dvd_dev->media_dwork;
		goto LABEL_31;
	}
	v10 = dvd_dev->_gap121[2] & 1;
	if ( !(dvd_dev->_gap121[2] & 1) )
	{
		result = 0x30F00;
		v11 = 0x20F0D;
		if ( !v5 )
		{
			v11 = 0x30F00;
		}
		if ( v1 == v11 )
		{
			v24 = v1;
			cancel_delayed_work_sync(&dvd_dev->media_dwork1);
			v25 = *p_dvd_dev;
			v25->dvd_flag_3 = v10;
			v25->dvd_flag_4 = v10;
			v25->dvd_flag_5 = v10;
			v25->_gap18[5] = v10;
			result = dvd_add_work(1, v24);
		}
		else
		{
			if ( v9 != 0xB0000 )
			{
				return dvd_add_work(1, v1);
			}
			v12 = v1 + 1;
			if ( v12 <= dvd_dev->media_count )
			{
				dvd_dev->media_idx = v1;
				dvd_add_work(6, 0xE800);
				dvd_dev = *p_dvd_dev;
				p_dvd_dev = pp_mtc_dvd_dev_10;
				v15 = (*p_dvd_dev)->_gap8[0];
				dvd_dev->dvd_flag_3 = v12 | 0xB0000;
				if ( v15 == 1 )
				{
					dvd_dev->dvd_length = 0;
					dvd_dev->dvd_position = 0;
					_memzero(dvd_dev->array1, 32);
					_memzero((*p_dvd_dev)->array2, 32);
					_memzero((*p_dvd_dev)->array3, 32);
					_memzero((*p_dvd_dev)->array4, 32);
					cancel_delayed_work_sync(&(*p_dvd_dev)->media_dwork1);
					dvd_dev = *p_dvd_dev;
					timeout_ms = 500;
					dvd_dev->_gap18[4] = 1;
				}
				else
				{
					cancel_delayed_work_sync(&dvd_dev->dwork1);
					dvd_dev = *p_dvd_dev;
					dwork_1 = &(*p_dvd_dev)->dwork1;
					dvd_dev->_gap14[24] = 1;
					queue_delayed_work(dvd_dev->media_wq, dwork_1, 0);
					cancel_delayed_work_sync(&(*p_dvd_dev)->media_dwork1);
					dvd_dev = *p_dvd_dev;
					timeout_ms = 1500;
					dvd_dev->_gap18[5] = 1;
				}
				media_wq = dvd_dev->media_wq;
				delay_1 = msecs_to_jiffies(timeout_ms);
				result = queue_delayed_work(media_wq, &dvd_dev->media_dwork1, delay_1);
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
int
dvd_get_folder(int result, const char *buf_1, int a3, int a4)
{
	struct list_head *folder_list; // r7@1
	size_t pos; // r0@8
	int v7; // r4@8
	size_t pos_1; // r0@8
	int v9; // r6@8
	int v10; // r7@8
	size_t v11; // r0@9
	int v12; // t1@9
	size_t v13; // r0@10
	int v14; // [sp+0h] [bp-18h]@1

	v14 = a4;
	folder_list = (*pp_mtc_dvd_dev_12)->folder_list[0];
	if ( (*pp_mtc_dvd_dev_12)->folder_list != folder_list )
	{
		result = result;
		if ( LOWORD(folder_list[1].next) == result )
		{
LABEL_7:
			if ( folder_list )
			{
				pos = strlen(buf_1);
				LOBYTE(v7) = 0;
				sprintf(&buf_1[pos], str_fmt_d_comma, LOWORD(folder_list[1].next));
				pos_1 = strlen(buf_1);
				sprintf(&buf_1[pos_1], str_fmt_d_comma, HIWORD(folder_list[1].next));
				v9 = &folder_list[1].prev;
				v10 = LOBYTE(folder_list[1].prev);
				do
				{
					v11 = strlen(buf_1);
					v7 = (v7 + 1);
					sprintf(&buf_1[v11], str_fmt_d_comma, v10);
					v12 = *(v9++ + 1);
					v10 = v12;
				}
				while ( v7 != 31 );
				v13 = strlen(buf_1);
				result = sprintf(&buf_1[v13], str_fmt_d, v10, v14);
			}
		}
		else
		{
			while ( 1 )
			{
				folder_list = folder_list->next;
				if ( (*pp_mtc_dvd_dev_12)->folder_list == folder_list )
				{
					break;
				}
				if ( LOWORD(folder_list[1].next) == result )
				{
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
	const char *v4; // r5@1
	struct list_head *media_list; // r2@1
	list_head *next; // r4@1 MAPDST
	size_t v8; // r0@8
	int v9; // r2@8
	int v10; // r6@8
	int v11; // r7@8
	int v12; // t1@8
	size_t v13; // r0@9
	int v14; // t1@9
	size_t v15; // r0@10
	int v16; // [sp+0h] [bp-18h]@1

	v16 = a4;
	v4 = a2;
	media_list = (*pp_mtc_dvd_dev_13)->media_list;
	next = media_list->next;
	if ( media_list->next != media_list )
	{
		result = result;
		if ( LOWORD(next[1].next) == result )
		{
LABEL_7:
			if ( next )
			{
				v8 = strlen(a2);
				v9 = LOWORD(next[1].next);
				LOBYTE(next) = 0;
				sprintf(&v4[v8], str_fmt_d_comma_0, v9);
				v12 = BYTE2(next[1].next);
				v10 = &next[1].next + 2;
				v11 = v12;
				do
				{
					v13 = strlen(v4);
					next = (next + 1);
					sprintf(&v4[v13], str_fmt_d_comma_0, v11);
					v14 = *(v10++ + 1);
					v11 = v14;
				}
				while ( next != 31 );
				v15 = strlen(v4);
				result = sprintf(&v4[v15], str_fmt_d_0, v11, v16);
			}
		}
		else
		{
			while ( 1 )
			{
				next = next->next;
				if ( next == media_list )
				{
					break;
				}
				if ( LOWORD(next[1].next) == result )
				{
					goto LABEL_7;
				}
			}
		}
	}
	return result;
}

/* fully decompiled */
int
dvd_get_media_cnt(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->media_count);
}

/* fully decompiled */
int
dvd_get_folder_cnt(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->folders_count);
}

/* fully decompiled */
int
dvd_get_media_idx(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->media_idx);
}

/* fully decompiled */
int
dvd_get_folder_idx(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->folder_idx);
}

/* dirty code */
int
dvd_get_media_title(const char *buf)
{
	int v1; // r3@0
	struct mtc_dvd_drv **p_dvd_dev; // r6@1
	signed int v4; // r4@1
	int v5; // r7@1
	size_t s_len; // r0@2
	int v7; // r3@2
	size_t pos; // r0@3

	p_dvd_dev = pp_mtc_dvd_dev_14;
	v4 = 1;
	v5 = (*pp_mtc_dvd_dev_14)->array1[0];
	do {
		s_len = strlen(buf);
		sprintf(&buf[s_len], str_fmt_d_comma_1, v5);
		v7 = *p_dvd_dev + v4++;
		v5 = *(v7 + 289);
	}
	while ( v4 != 128 );
	pos = strlen(buf);

	return sprintf(&buf[pos], "%d", v5);
}

/* fully decompiled */
int
dvd_get_length(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->dvd_length);
}

/* fully decompiled */
int
dvd_get_position(char *buf)
{
	return sprintf(buf, "%d", p_mtc_dvd_drv->dvd_position);
}

/* dirty code */
void
dvd_send_command(int command)
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
					dvd_send_command_direct((command & 0xFF00) | 0x570000);
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



/* dirty code */
static int
dvd_probe(struct platform_device *pdev)
{
	size_t size; // r0@1
	struct mtc_dvd_drv *dvd_dev; // r3@2 MAPDST
	const char *mutex_name; // r1@4
	struct mtc_dvd_drv **p_dvd_dev; // r2@4 MAPDST
	struct workqueue_struct *workqueue; // r0@4
	int v10; // lr@4
	int v11; // r12@4
	int (__cdecl *rev_work_cb)(); // lr@4
	int (__cdecl *stop_work_cb)(); // r12@4
	int v15; // r12@4
	struct timer_list *media_timer; // r0@4
	int (__cdecl *media_work_cb)(); // r12@4
	struct timer_list *timer1; // r12@4
	struct timer_list *timer3; // r0@4
	struct delayed_work *media_index_work; // r12@4
	const char *gpio_label; // r1@4
	struct list_head *folder_list; // r12@4
	struct list_head *media_list; // r2@4
	const char *irq_name; // lr@4
	irqreturn_t (__cdecl *isr_handler)(int, void *); // r1@4
	struct input_dev *input_dev; // r0@4 MAPDST
	char *_str_gpio_keys_input1; // r12@4
	const char *dev_name; // t1@4

	printk(log_dvd_probe);
	size = dword_C09BBE50->size1;
	if ( size )
	{
		dvd_dev = kmem_cache_alloc_trace(size, 0x80D0, 0x280u);
	}
	else
	{
		dvd_dev = 16;
	}
	p_dvd_dev = p_mtc_dvd_dev;
	mutex_name = str_dvd_dev_cmd_lock;
	p_dvd_dev = p_mtc_dvd_dev;
	*p_mtc_dvd_dev = dvd_dev;
	_mutex_init(&dvd_dev->cmd_lock, mutex_name, (p_dvd_dev + 1));
	dvd_dev = *p_dvd_dev;
	workqueue = _alloc_workqueue_key(str_dvd_rev_wq, 0xAu, 1, 0, 0);
	dvd_dev = *p_dvd_dev;
	v10 = (*p_dvd_dev)->_gap5;
	v11 = (*p_dvd_dev)->_gap13;
	dvd_dev->dvd_wq = workqueue;
	*&dvd_dev->_gap5[0] = v10;
	*&dvd_dev->_gap5[4] = v10;
	*&dvd_dev->_gap13[0] = v11;
	rev_work_cb = p_rev_work[0];
	*&dvd_dev->_gap13[4] = v11;
	stop_work_cb = p_stop_work[0];
	dvd_dev->rev_work_cb = rev_work_cb;
	dvd_dev->dvd_work = 0x500;
	dvd_dev->dwork1 = 0x500;
	dvd_dev->stop_work_cb = stop_work_cb;
	init_timer_key(&dvd_dev->stop_timer, 0, 0);
	dvd_dev = *p_dvd_dev;
	v15 = (*p_dvd_dev)->_gap15;
	media_timer = &(*p_dvd_dev)->media_timer;
	*&dvd_dev->_gap15[0] = v15;
	*&dvd_dev->_gap15[4] = v15;
	media_work_cb = p_media_work[0];
	dvd_dev->media_dwork = 0x500;
	dvd_dev->media_work_cb = media_work_cb;
	init_timer_key(media_timer, 0, 0);
	dvd_dev = *p_dvd_dev;
	timer1 = &(*p_dvd_dev)->timer1;
	timer3 = &(*p_dvd_dev)->timer3;
	dvd_dev->timer1 = timer1;
	dvd_dev->timer2 = timer1;
	media_index_work = p_media_index_work;
	dvd_dev->media_dwork1 = 0x500;
	dvd_dev->media_index_work = media_index_work;
	init_timer_key(timer3, 0, 0);
	dvd_dev = *p_dvd_dev;
	gpio_label = gpio_dvd_data_name;
	folder_list = (*p_dvd_dev)->folder_list;
	media_list = (*p_dvd_dev)->media_list;
	dvd_dev->folder_list[0] = folder_list;
	dvd_dev->folder_list[1] = folder_list;
	dvd_dev->media_list[0] = media_list;
	dvd_dev->media_list[1] = media_list;
	gpio_request(gpio_DVD_DATA, gpio_label);
	gpio_pull_updown(gpio_DVD_DATA, 0);
	gpio_direction_input(gpio_DVD_DATA);
	gpio_request(gpio_DVD_STB, gpio_dvd_stb_name);
	gpio_pull_updown(gpio_DVD_STB, 0);
	gpio_direction_input(gpio_DVD_STB);
	gpio_request(gpio_DVD_ACK, gpio_dvd_ack_name);
	gpio_pull_updown(gpio_DVD_ACK, 0);
	gpio_direction_input(gpio_DVD_ACK);
	dvd_dev = *p_dvd_dev;
	irq_name = str_mtc_dvd;
	isr_handler = dvd_isr_handler;
	dvd_dev->dvd_irq = gpio_DVD_DATA;
	request_threaded_irq(gpio_DVD_DATA, isr_handler, 0, 2u, irq_name, dvd_dev);
	disable_irq_nosync((*p_dvd_dev)->dvd_irq);
	dvd_dev = *p_dvd_dev;
	dvd_dev->media_wq = _alloc_workqueue_key(str_dvd_wq, 8u, 1, 0, 0);
	input_dev = input_allocate_device();
	_str_gpio_keys_input1 = str_gpio_keys_input1;
	(*p_dvd_dev)->mtc_wq = input_dev;
	dev_name = pdev->name;
	input_dev->phys = _str_gpio_keys_input1;
	input_dev->id.vendor = 1;
	input_dev->id.bustype = 25;
	input_dev->name = dev_name;
	input_dev->dev.power.suspend_timer.base = &pdev->dev;
	input_dev->id.version = 256;
	input_dev->id.product = 1;
	input_set_capability(input_dev, 1u, 0x10Eu);
	input_set_capability(input_dev, 1u, 0x8Fu);
	input_register_device(input_dev);
	register_early_suspend(unknown_handler_1);
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

static struct dev_pm_ops dvd_pm_ops = {
	.suspend = dvd_suspend,
	.resume = dvd_resume,
};

static struct platform_driver mtc_dvd_driver = {
	.probe = dvd_probe,
	.remove = __devexit_p(dvd_remove),
	.driver = {
		.name = "mtc-dvd",
		.pm = &dvd_pm_ops,
	},
};

module_platform_driver(mtc_dvd_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC DVD driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-dvd");
