

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "mtc_shared.h"

struct mtc_radio_work {
	u32 dword0;
	u32 dword4;
	u32 dword8;
	char gapC[12];
	struct delayed_work radio_work;
};

struct mtc_radio_struct2 {
	char psn_data[10];
	char _gap0[2];
	char _gap8[4];
	char mem1[1116];
	char mem2[5192];
};

struct mtc_radio_struct {
	char _gap0[4];
	int power;
	struct workqueue_struct *wq;
	char _gap3[4];
	char _gap4[4];
	struct mtc_radio_struct2 s2;
	char _gap5[128];
	char _gap6[128];
	char _gap7[68];
	struct workqueue_struct wq2;
	char _gap8[92];
	struct delayed_work dwork;
	char _gap9[12];
	char psn_data[10];
	char _gap10[2];
	int radio_freq;
	char _gap14[32];
	char _gap15[8];
	char _gap16[4];
};

static struct mtc_radio_struct radio;

/* decompiled */
static int
tef6606_i2c_suspend()
{
	return 0;
}

/* decompiled */
static int
tef6606_i2c_resume()
{
	return 0;
}

/* decompiled */
static void
tef6606_s_suspend()
{
	radio.power = 0;
}

/* decompiled */
static void
tef6606_s_resume()
{
	radio.power = 0;
}

/* decompiled */
static int
radio_add_work(int a1, int a2, int a3)
{
	struct mtc_radio_work *work;
	unsigned int delay;

	work = kzalloc(sizeof(struct mtc_radio_work), GFP_KERNEL);

	INIT_DELAYED_WORK(&work->radio_work, radio_cmd_work);

	work->dword8 = a3;
	work->dword0 = a1;
	work->dword4 = a2;

	return queue_delayed_work(radio.wq, &work->radio_work, msecs_to_jiffies(0));
}

/* decompiled */
static void
rds_af_add(char *buf, int a2)
{
	if (a2 != 0) {
		int pos = 0;

		while (buf[pos + 1] != 0) {
			pos++;

			if (a2 == buf[pos] || pos == 25) {
				return;
			}
		}

		pos++;

		buf[pos] = a2;
		buf[0] = pos; // length?

		printk("--mtc af add %d,%d\n", pos, a2);
	}
}

/* contains unknown fields */
static void
sta_invalid()
{
	cancel_delayed_work(&radio.dwork);

	radio._gap9[0] = 0;
	radio._gap0[0] = 0;

	schedule_delayed_work(&radio.dwork, msecs_to_jiffies(20u));
}

/* decompiled */
void
radio_send_sta(char *cmd, char d1, char d2)
{
	char data[6];

	data[0] = cmd[3];
	data[1] = cmd[2];
	data[2] = cmd[1];
	data[3] = cmd[0];
	data[4] = d1;
	data[5] = d2;

	vs_send(2, 0x97u, data, 6);
}
EXPORT_SYMBOL_GPL(radio_send_sta)

/* contains unknown fields */
void
rds_send_sta()
{
	char cmd_data[5];

	cmd_data[0] = radio.s2.psn_data[1];
	cmd_data[2] = radio.s2._gap8[0];
	cmd_data[3] = radio.s2.mem1[156];
	cmd_data[1] = radio.s2.psn_data[0];
	cmd_data[4] = radio.s2.mem1[157];

	if (!memcmp(cmd_data, &radio._gap9[4], 4u)) {
		vs_send(2, 0x98u, cmd_data, 5);
	} else {
		memcpy(&radio._gap9[4], cmd_data, 4u);
	}
}
EXPORT_SYMBOL_GPL(rds_send_sta)

/* decompiled */
void
rds_send_sta_empty()
{
	char cmd_data[5] = {0, 0, 0, 0, 0};

	vs_send(2, 0x98u, cmd_data, 5);
}
EXPORT_SYMBOL_GPL(rds_send_sta_empty)

/* dirty code */
static void
Radio_Set_Frequency_com(signed int freq, int a2)
{
	unsigned int cmd;  // r0@1
	int v5;		   // r4@2
	unsigned char *v6; // r2@3
	int count;	 // r1@3
	char buf[2];       // [sp+6h] [bp-12h]@5

	sta_invalid();

	if ((freq - 144000) > 1576000) {
		if ((freq - 76000000) > 32000000) {
			if ((freq - 2940000) <= 15195000) {
				v5 = freq / 5000 | 0x4000;
			} else {
				if ((freq - 65000000) > 9000000) {
					return;
				}

				v5 = freq / 10000 | 0x6000;
			}
		} else {
			v5 = freq / 50000 | 0x2000;
		}
	} else {
		v5 = freq / 1000;
	}

	v6 = NULL;
	count = v5 >> 8;
	if (a2) {
		cmd = 0x9208;
	}

	buf[0] = (v5 >> 8) & 0xFF;
	radio._gap9[9] = 0;

	if (a2) {
		count = 2;
	}

	radio._gap9[10] = 0;

	if (a2) {
		v6 = buf;
	} else {
		cmd = 0x9200;
		v6 = buf;
		count = 2;
	}

	buf[1] = v5;
	arm_send_multi(cmd, count, v6);
	rds_send_sta_empty();

	car_status._gap9[1] = 0;
	car_status._gap9[2] = 0;
}

/* decompiled */
void
rds_send_psn()
{
	radio.psn_data[0] = radio.s2.psn_data[1];
	radio.psn_data[1] = radio.s2.psn_data[0];

	if (!memcmp(&radio.psn_data[2], &radio.s2.psn_data[2], 8u)) {
		vs_send(2, 0x99u, radio.psn_data, 10);
	} else {
		memcpy(&radio.psn_data[2], &radio.s2.psn_data[2], 8u);
	}
}
EXPORT_SYMBOL_GPL(rds_send_psn)

/* dirty */
void
rds_send_rt()
{
	size_t len;	// r0@1
	size_t vs_len;     // r0@1
	char cmd_data[16]; // [sp+5h] [bp-4Bh]@1

	cmd_data[0] = radio.s2.psn_data[1];
	cmd_data[1] = radio.s2.psn_data[0];
	len = strlen(&radio.s2.mem1[162]);
	memcpy(&cmd_data[2], &radio.s2.mem1[162], len + 1);
	// зачем два раза????
	vs_len = strlen(&radio.s2.mem1[162]);
	vs_send(2, 0x9Cu, cmd_data, vs_len + 3);
}

static struct i2c_driver mtc_wm8731_i2c_driver = {
    .driver =
	{
	    .name = "wm8731", .owner = THIS_MODULE,
	},
    .probe = wm8731_i2c_probe,
    .remove = __devexit_p(wm8731_i2c_remove),
    .id_table = wm8731_i2c_id,
};

/* decompiled */
static int __init
tef6606_init()
{
	radio.power = 1;
	return i2c_register_driver(0, &tef6606_i2c_driver);
}
module_init(tef6606_init);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC Radio driver");
MODULE_LICENSE("GPL");
