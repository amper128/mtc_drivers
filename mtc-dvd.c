typedef u32 uint32_t;

struct {
	u8 _pad[104];

	unsigned int irq_104;		// +104
	u32 __pad[1];			// +108
	struct workqueue_struct *wq;	// +112
	struct work_struct *work;	// +116
} vC168AC48;

struct {
	u32* id_446; // +446?
} vC082A73C;



void
dvd_s_resume(void)
{
	// empty function
}

int
dvd_suspend(void)
{
	return 0;
}

int
dvd_resume(void)
{
	return 0;
}

signed int
dvd_isr(int irq)
{
	disable_irq_nosync(irq);
	/* (struct workqueue_struct *wq, struct work_struct *work) */
	queue_work(vC168AC48.wq, vC168AC48.work);

	return 1; // IRQ_HANDLED ?
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

struct list_head *
free_folder(struct list_head *folder_list)
{
	struct list_head *v1; // r6@1
	list_head *v2; // r3@1
	list_head *v3; // r4@2
	list_head *i; // r5@2
	list_head **v5; // r2@4

	v1 = folder_list;
	v2 = folder_list->next;
	if ( folder_list != folder_list->next )
	{
		v3 = v2->next;
		for ( i = v2->next; ; i = v3 )
		{
			v5 = &v2->prev->next;
			v3->prev = v5;
			*v5 = v3;
			v2->next = LIST_POISON1;
			v2->prev = LIST_POISON2;
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

struct list_head *
free_media(struct list_head *media_list)
{
	struct list_head *v1; // r6@1
	list_head *v2; // r3@1
	list_head *v3; // r4@2
	list_head *i; // r5@2
	list_head **v5; // r2@4

	v1 = media_list;
	v2 = media_list->next;
	if ( media_list != media_list->next )
	{
		v3 = v2->next;
		for ( i = v2->next; ; i = v3 )
		{
			v5 = &v2->prev->next;
			v3->prev = v5;
			*v5 = v3;
			v2->next = LIST_POISON1;
			v2->prev = LIST_POISON2;
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

int
input_event_func(u16 a1)
{
	char v2;
	char v3;

	v2 = (char)(a1 >> 8);
	v3 = (a1 & 0xFF);

	return vs_send(2, 137, &v2, 2);
}

int
cmd_surface(int result)
{
	if (*(u16*)(*(u32*)vC082A73C.id_446) != result) {
		*(u16*)(*(u32*)vC082A73C.id_446 = result;
		if (result) {
			result = input_event_func(0x9702);
		} else {
			result = input_event_func(0x9701);
		}
	}

	return result;
}

bool
CheckTimeOut_constprop_9(unsigned int timeout)
{
	unsigned int v1;
	int usec;
	struct timeval tv;

	v1 = timeout;
	do_gettimeofday(&tv);
	usec = tv.tv_usec;
	if (tv.tv_usec < v1) {
		usec = tv.tv_usec + 1000000;
	}

	return usec - v1 > 49999;
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

void
rev_work()
{
	int v0; // r4@1

	v0 = dword_C082AA08;
	if ( *(_DWORD *)(*(_DWORD *)dword_C082AA08 + 100) )
	{
		dvd_rev_part_2();
		if ( *(_DWORD *)(*(_DWORD *)v0 + 100) )
		{
			enable_irq(vC168AC48.irq_104);
			if ( *(_DWORD *)(*(_DWORD *)v0 + 100) )
			{
				dvd_rev_part_2();
			}
		}
	}
}





int
dvd_power(int pwr)
{
	int v1;
	signed int result;
	int v3;
	int v4;
	int v5;
	u32* v6;
	int v7;

	v1 = pwr;
	printk("--mtc dvd %d\n", pwr);

	if (v1) {

	} else {
		result = dvd_poweroff_constprop_10();
	}

	return result;
}
