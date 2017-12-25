#include <linux/module.h>

#include "mtc_shared.h"
#include "mtc-car.h"

/* decompiled */
signed int
is_Atv()
{
	if (car_struct.config_data.cfg_dtv > 5) {
		if (car_struct.config_data.cfg_dtv != 14) {
			return 0;
		}
	}

	return 1;
}

/* decompiled */
void
Tv_Set_Frequency(signed int freq)
{
	unsigned char buf[4];

	if (is_Atv()) {
		/* digital magic */
		if (freq > 2825) {
			if (freq > 8545) {
				buf[3] = 8;
			} else {
				buf[3] = 2;
			}
		} else {
			buf[3] = 1;
		}

		buf[0] = ((freq + 778) >> 8) & 0xFF;
		buf[1] = (freq + 10) & 0xFF;
		buf[2] = 0x80u;

		printk(KERN_ALERT "--mtc TV Freq %d %02x %02x %02x %02x\n", 5 * freq, buf[0],
		       buf[1], buf[2], buf[3]);
		arm_send_multi(MTC_CMD_TV_FREQ, 4, buf);
	}
}

/* dirty code */
void
Tv_Set_Demod(int a1)
{
	unsigned int v1; // r2@1
	unsigned char v2; // cf@1
	char v3; // zf@1
	int v4; // r3@5
	int v5; // r2@6
	char v6; // zf@15
	char v7; // zf@26
	char v8; // zf@29
	int v9; // r3@34
	int v10; // r2@35
	char v11; // zf@42
	char v12; // zf@56
	char v13; // zf@62
	char v14; // zf@68
	char v15; // zf@71
	char v16; // zf@76
	char a2[4]; // [sp+5h] [bp-Bh]@10

	if (!is_Atv()) {
		return;
	}

	if ( car_struct.config_data.cfg_dtv != 5 )
	{
		v4 = a1 - 1 + ((a1 - 1) <= 0) - (a1 - 1);
		if ( a1 == 5 )
		{
			v5 = v4 | 1;
		}
		else
		{
			v5 = a1 - 1 + ((a1 - 1) <= 0) - (a1 - 1);
		}
		if ( car_struct.config_data.cfg_dtv - 14 > 0 )
		{
			if ( v5 )
			{
				a2[0] = 214;
				v5 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
				a2[1] = 48;
			}
			else if ( (a1 - 7) <= 1u )
			{
				a2[0] = 214;
				a2[1] = 112;
			}
			else
			{
				v5 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
				v7 = a1 == 6;
				if ( a1 != 6 )
				{
					v7 = a1 == 2;
				}
				if ( !v7 )
				{
					v8 = a1 == 3;
					if ( a1 != 3 )
					{
						v8 = a1 == 4;
					}
					if ( !v8 )
					{
						return;
					}
				}
				a2[0] = 22;
				a2[1] = 120;
			}
			if ( a1 == 7 )
			{
				v5 |= 1u;
			}
			if ( v5 )
			{
				a2[2] = 9;
LABEL_82:
				printk("<1>--mtc TV Demod %02x %02x %02x\n", a2[0], a2[1], a2[2]);
				arm_send_multi(MTC_CMD_TV_DEMOD, 3, a2);
				return;
			}
			v6 = a1 == 4;
			if ( a1 != 4 )
			{
				v6 = a1 == 8;
			}
			if ( v6 )
			{
				a2[2] = 11;
				goto LABEL_82;
			}
			if ( a1 == 2 )
			{
				a2[2] = 10;
				goto LABEL_82;
			}
			goto LABEL_21;
		}
		if ( v5 )
		{
			a2[0] = -42;
			v5 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
			a2[1] = 48;
		}
		else if ( (a1 - 7) <= 1u )
		{
			a2[0] = -42;
			a2[1] = 112;
		}
		else
		{
			v5 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
			v14 = a1 == 6;
			if ( a1 != 6 )
			{
				v14 = a1 == 2;
			}
			if ( !v14 )
			{
				v15 = a1 == 3;
				if ( a1 != 3 )
				{
					v15 = a1 == 4;
				}
				if ( !v15 )
				{
					return;
				}
			}
			a2[0] = 22;
			a2[1] = 112;
		}
		if ( a1 == 7 )
		{
			v5 |= 1u;
		}
		if ( !v5 )
		{
			v12 = a1 == 4;
			if ( a1 != 4 )
			{
				v12 = a1 == 8;
			}
			if ( !v12 )
			{
				if ( a1 != 2 )
				{
LABEL_21:
					if ( (a1 - 5) <= 1u )
					{
						v4 |= 1u;
					}
					if ( !v4 )
					{
						return;
					}
					a2[2] = 8;
					goto LABEL_82;
				}
LABEL_60:
				a2[2] = 74;
				goto LABEL_82;
			}
LABEL_85:
			a2[2] = 75;
			goto LABEL_82;
		}
LABEL_84:
		a2[2] = 73;
		goto LABEL_82;
	}
	v9 = a1 - 1 + ((a1 - 1) <= 0) - (a1 - 1);
	if ( a1 == 5 )
	{
		v10 = v9 | 1;
	}
	else
	{
		v10 = a1 - 1 + ((a1 - 1) <= 0) - (a1 - 1);
	}
	if ( v10 )
	{
		a2[0] = 22;
		v10 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
		a2[1] = 48;
	}
	else
	{
		if ( (a1 - 7) > 1u )
		{
			v10 = a1 - 3 + ((a1 - 3) <= 0) - (a1 - 3);
			v13 = a1 == 6;
			if ( a1 != 6 )
			{
				v13 = a1 == 2;
			}
			if ( !v13 )
			{
				v16 = a1 == 3;
				if ( a1 != 3 )
				{
					v16 = a1 == 4;
				}
				if ( !v16 )
				{
					return;
				}
			}
		}
		a2[0] = 0xD6u;
		a2[1] = 112;
	}
	if ( a1 == 7 )
	{
		v10 |= 1u;
	}
	if ( v10 )
	{
		goto LABEL_84;
	}
	v11 = a1 == 4;
	if ( a1 != 4 )
	{
		v11 = a1 == 8;
	}
	if ( v11 )
	{
		goto LABEL_85;
	}
	if ( a1 == 2 )
	{
		goto LABEL_60;
	}
	if ( (a1 - 5) <= 1u )
	{
		v9 |= 1u;
	}
	if ( v9 )
	{
		a2[2] = 72;
		goto LABEL_82;
	}
}

/* decompiled */
int
Tv_Get_Status()
{
	int status;
	unsigned char buf[2];

	if (!is_Atv()) {
		return 0;
	} else {
		status = 1;
	}

	arm_send_multi(MTC_CMD_GET_TV_STATUS, 2, buf);
	status = buf[1] | (buf[0] << 8);
	printk("<1>--mtc TV Status %02x %02x\n", buf[0], buf[1]);

	return status;
}
