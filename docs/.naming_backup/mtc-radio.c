

#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include "mtc-car.h"
#include "mtc_shared.h"

struct mtc_tef6606_drv {
	struct i2c_client *client;
	struct mutex mutex;
};

struct mtc_radio_work {
	u32 cmd;
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

struct __attribute__((aligned(4))) mtc_radio_struct {
	char _gap0[4];
	int power;
	struct workqueue_struct *radio_wq;
	char _gap1[4];
	struct mtc_tef6606_drv *tef_drv;
	struct mtc_radio_struct2 _s2;
	char mem1[244];
	char _gap2[4];
	struct wake_lock *radio_lock;
	char _gap3[4];
	char _gap4[64];
	struct workqueue_struct *ta_wq;
	struct workqueue_struct *af_wq;
	struct delayed_work dwork_signal_low;
	struct delayed_work dwork_af;
	struct delayed_work ta_work;
	struct delayed_work rdslost_dwork;
	struct delayed_work dwork_sta_valid;
	char _gap5[12];
	char psn_data[10];
	char _gap6[2];
	int radio_freq;
	char _gap7[1];
	char has_af_work;
	char _gap8[1];
	char has_ta_work;
	char _gap9[1];
	char radio_af;
	char _gap10[2];
	int cur_freq;
	char _gap11[3];
	char radio_ta;
	u16 u16_1;
	char _gap12[2];
	u16 u16_2;
	char _gap13[10];
	char _gap14[8];
	char _gap15[4];
};

static struct mtc_radio_struct radio;
static struct early_suspend tef6606_early_suspend;

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
static void
radio_add_work(int cmd, int a2, int a3)
{
	struct mtc_radio_work *work;
	unsigned int delay;

	work = kzalloc(sizeof(struct mtc_radio_work), GFP_KERNEL);

	INIT_DELAYED_WORK(&work->radio_work, radio_cmd_work);

	work->cmd = cmd;
	work->dword4 = a2;
	work->dword8 = a3;

	queue_delayed_work(radio.radio_wq, &work->radio_work, msecs_to_jiffies(0));
}

/* decompiled */
static void
rds_af_add(char *buf, int af)
{
	if (af != 0) {
		int pos = 0;

		while (buf[pos + 1] != 0) {
			pos++;

			if (af == buf[pos] || pos == 25) {
				return;
			}
		}

		pos++;

		buf[pos] = af;
		buf[0] = pos; // length?

		printk("--mtc af add %d,%d\n", pos, af);
	}
}

/* contains unknown fields */
static void
sta_invalid()
{
	cancel_delayed_work(&radio.dwork_sta_valid);

	radio._gap5[0] = 0;
	radio._gap0[0] = 0;

	schedule_delayed_work(&radio.dwork_sta_valid, msecs_to_jiffies(20u));
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

	cmd_data[0] = radio._s2.psn_data[1];
	cmd_data[2] = radio._s2._gap8[0];
	cmd_data[3] = radio._s2.mem1[156];
	cmd_data[1] = radio._s2.psn_data[0];
	cmd_data[4] = radio._s2.mem1[157];

	if (!memcmp(cmd_data, &radio._gap5[4], 4u)) {
		vs_send(2, 0x98u, cmd_data, 5);
	} else {
		memcpy(&radio._gap5[4], cmd_data, 4u);
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
	radio.psn_data[0] = radio._s2.psn_data[1];
	radio.psn_data[1] = radio._s2.psn_data[0];

	if (!memcmp(&radio.psn_data[2], &radio._s2.psn_data[2], 8u)) {
		vs_send(2, 0x99u, radio.psn_data, 10);
	} else {
		memcpy(&radio.psn_data[2], &radio._s2.psn_data[2], 8u);
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

	cmd_data[0] = radio._s2.psn_data[1];
	cmd_data[1] = radio._s2.psn_data[0];
	len = strlen(&radio._s2.mem1[162]);
	memcpy(&cmd_data[2], &radio._s2.mem1[162], len + 1);
	// зачем два раза????
	vs_len = strlen(&radio._s2.mem1[162]);
	vs_send(2, 0x9Cu, cmd_data, vs_len + 3);
}

/* decompiled */
signed int
TEF6686_signal(int v)
{
	signed int result;

	if (v & 0x80) {
		v = 0;
	}

	if (radio.radio_freq <= 10000000) {
		if (v == 1) {
			result = 255;
		} else {
			result = 0;
		}
	} else if (v == 1) {
		result = 172;
	} else if (v == 2) {
		result = 240;
	} else {
		result = 0;
	}

	return result;
}

/* decompiled */
void
rds_input(int a1)
{
	if (car_struct.config_data.cfg_rds) {
		radio_add_work(1, a1, 0);
	}
}

/* decompiled */
void
rds_input2(int a1)
{
	if (car_struct.config_data.cfg_rds) {
		radio_add_work(2, a1, 0);
	}
}

/* decompiled */
int
barray2int(char *arr)
{
	return arr[1] | (arr[0] << 8);
}

/* decompiled */
void
rds_input3A(char *rds)
{
	if (car_struct.config_data.cfg_rds) {
		radio_add_work(3, barray2int(rds), 0);
	}
}

/* decompiled */
void
rds_input3(char *rds)
{
	if (car_struct.config_data.cfg_rds) {
		radio_add_work(3, barray2int(rds), 0);
		radio_add_work(3, barray2int(&rds[2]), 1);
		radio_add_work(3, barray2int(&rds[4]), 2);
		radio_add_work(3, barray2int(&rds[6]), 3);
	}
}

/* decompiled */
void
Radio_AF(int af)
{
	if (car_struct.config_data.cfg_rds) {
		radio.radio_af = af;
	} else {
		radio.radio_af = 0;
	}
}

/* decompiled */
void
Radio_Set_Search(char arg)
{
	arm_send_multi(MTC_CMD_RADIO_SEARCH, 1, &arg);
}

/* decompiled */
void
Radio_Set_Mute(int mute)
{
	if (mute) {
		arm_send_multi(MTC_CMD_RADIO_MUTE, 0, 0);
	} else {
		arm_send_multi(MTC_CMD_RADIO_UNMUTE, 0, 0);
	}
}

/* decompiled */
void
Radio_Set_Stereo(int val)
{
	if (val) {
		arm_send_multi(MTC_CMD_FM_STEREO_ON, 0, 0);
	} else {
		arm_send_multi(MTC_CMD_FM_STEREO_OFF, 0, 0);
	}
}

/* decompiled */
int
Radio_Get_Stereo()
{
	return 0;
}

/* contains unknown fields */
static int
tef6606_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mtc_tef6606_drv *tef_drv;
	signed int result;

	if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_YZ_RM_ZT) {
		radio._gap1[0] = 0x1F;
	} else if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_MX) {
		radio._gap1[1] = 0xF7u;
	}
	pr_info("mtc_radio: v0.01: probe radio_tef6606\n");

	tef_drv = kmalloc(sizeof(struct mtc_tef6606_drv), __GFP_HIGH);

	radio.tef_drv = tef_drv;

	if (!tef_drv) {
		return -ENOMEM;
	};

	mutex_init(&tef_drv->mutex); // &radio->mutex

	radio.tef_drv->client = client;

	memset(&radio._s2, 0, sizeof(radio._s2));
	memset(radio.mem1, 0, sizeof(radio.mem1));

	wake_lock_init(radio.radio_lock, 0, "RadioLock");
	dev_set_drvdata(&client->dev, radio.tef_drv);
	radio.power = 0;
	register_early_suspend(&tef6606_early_suspend);

	radio.ta_wq = create_singlethread_workqueue("ta_wq");
	radio.af_wq = create_singlethread_workqueue("af_wq");
	radio.radio_wq = create_singlethread_workqueue("radio_wq");

	INIT_DELAYED_WORK(&radio.dwork_signal_low, work_signal_low);
	INIT_DELAYED_WORK(&radio.dwork_af, work_af);
	INIT_DELAYED_WORK(&radio.ta_work, work_ta);
	INIT_DELAYED_WORK(&radio.rdslost_dwork, rdslost_work);
	INIT_DELAYED_WORK(&radio.dwork_sta_valid, ork_sta_valid);

	pr_info("mtc_radio: v0.01: registered.\n");

	return 0;
}

/* decompiled */
static int
tef6606_i2c_remove(struct i2c_client *client)
{
	struct mtc_tef6606_drv *drv;

	drv = dev_get_drvdata(&client->dev);
	pr_info("mtc_radio: v0.01: remove\n");
	if (drv) {
		kfree(drv);
	}

	return 0;
}

static struct early_suspend tef6606_early_suspend = {
    .level = 0x46, .suspend = tef6606_s_suspend, .resume = tef6606_s_resume,
};

static const struct i2c_device_id tef6606_id[] = {{"radio-tef6606", 0}, {}};
MODULE_DEVICE_TABLE(i2c, tef6606_id);

static struct i2c_driver tef6606_i2c_driver = {
    .driver =
	{
	    .name = "radio-tef6606", .owner = THIS_MODULE,
	},
    .probe = tef6606_i2c_probe,
    .remove = __devexit_p(tef6606_i2c_remove),
    .suspend = tef6606_i2c_suspend,
    .resume = tef6606_i2c_resume,
    .id_table = tef6606_id,
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
