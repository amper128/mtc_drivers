#include <asm/string.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "mtc-car.h"
#include "mtc_shared.h"

enum MTC_AV_CHANNEL {
	MTC_AV_CHANNEL_GSM_BT = 0,
	MTC_AV_CHANNEL_SYS = 1,
	MTC_AV_CHANNEL_DVD = 2,
	MTC_AV_CHANNEL_LINE = 3,
	MTC_AV_CHANNEL_FM = 4,
	MTC_AV_CHANNEL_DTV = 5,
	MTC_AV_CHANNEL_IPOD = 6,
	MTC_AV_CHANNEL_DVR = 7,
};

struct mtc_audio_struct {
	struct workqueue_struct *audio_wq;
	char gap0[4];
	u8 audio_ch;
	char gap1[6];
	char mute;
	char mute_all;
	char gap2[3];
	struct delayed_work *dwork;
	char char18;
	u16 dword1C;
	u16 dword20;
	struct timer_list timer;
	char gap3[20];
	struct mutex lock;
	char gap5[3];
	char eq1;
	char eq2;
	char eq3;
	char eq4;
	char balance1;
	char balance2;
	char gap6[7];
};

static unsigned char ChannelSel[12] = {0x80, 0x81, 0x82, 0x83, 0x8A, 0x82,
				       0x8B, 0x80, 0x80, 0,    0,    0};
static unsigned char BalanceTable[32] = {
    0, 0, 0, 0,   0,   0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 2,
    4, 6, 9, 0xC, 0xF, 0x13, 0x17, 0x1B, 0x1F, 0x23, 0x28, 0x2D, 0x32, 0, 0, 0};

static unsigned char FadeTable[32] = {0,    0,    0,    0,    0,    0,    0,    0, 0,   0,   0,
				      0,    0,    0,    0,    2,    4,    6,    9, 0xC, 0xF, 0x13,
				      0x17, 0x1B, 0x1F, 0x23, 0x28, 0x2D, 0x32, 0, 0,   0};
static unsigned char SoundEffectTable[32] = {
    0x8A, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0,    1,    2,    3,    4,    5,
    6,    7,    8,    9,    0xA,  0xB,  0xC,  0xD,  0xE,  0xF,  0x10, 0x11, 0x12, 0x13, 0x14, 0};

void audio_add_dwork(int data, unsigned int delay, int a3, struct mtc_car_status *c_status);

/* decompiled */
void
Audio_Mute(signed int mute)
{
	if (car_struct.audio->mute != mute) {
		if (mute == 1) {
			Audio_FadeOut();
			car_struct.audio->mute = mute;
			arm_send_multi(MTC_CMD_SET_MUTE, mute, &car_struct.audio->mute);
			vs_send(2, 0x96u, &car_struct.audio->mute, mute);
		} else {
			car_struct.audio->mute = 0;
			arm_send_multi(MTC_CMD_SET_MUTE, 1, &car_struct.audio->mute);
			vs_send(2, 0x96u, &car_struct.audio->mute, 1);
			Audio_FadeIn();
		}
	}
}

/* dirty code */
void
Audio_Balance()
{
	unsigned int b2;	// r2@2
	int ob1;		// r12@6
	int ob2;		// r3@6
	char v3;		// r0@6
	int v4;			// r1@6
	char v5;		// r4@6
	int v6;			// r2@6
	int v7;			// r12@6
	char v8;		// lr@6
	int v9;			// r3@6
	unsigned __int8 v10;    // cf@6
	char v11;		// zf@6
	unsigned int v12;       // r0@6
	unsigned int v13;       // r1@6
	signed int v14;		// r12@8
	signed int v15;		// r3@10
	unsigned __int64 v16;   // kr00_8@10
	unsigned int v17;       // r2@10
	unsigned __int64 div;   // kr08_8@10
	signed int v19;		// r5@12
	unsigned __int8 v20;    // cf@14
	char v21;		// zf@14
	signed int v22;		// r12@14
	signed int v23;		// r4@14
	signed int v24;		// r3@14
	unsigned __int8 buf[2]; // [sp+6h] [bp-1Ah]@18
	char balance[2];	// [sp+8h] [bp-18h]@2
	char oldmcu_balance[4]; // [sp+Ch] [bp-14h]@6

	if (strcmp(car_struct.car_status.mcuver2, "V1.51") < 0) // "V1.51"
	{
		/* тут происходит адская числовая магия, которую я так и не понял, но результат тупо
		 * (x+1)/2. */
		ob1 = car_struct.audio->balance1;
		ob2 = car_struct.audio->balance2;
		v3 = ChannelSel[-ob1 + 72];
		v4 = ChannelSel[-ob2 + 40];
		v5 = ChannelSel[ob2 + 12];
		v6 = ChannelSel[ob1 + 44];
		v7 = 7 * (ob1 + 1);
		v8 = v4 + v3 + -128;
		v9 = 7 * (ob2 + 1);
		v10 = v8 >= 0xB1u;
		v11 = v8 == 177;
		v12 = (v3 + v5 + -128);
		oldmcu_balance[0] = v8;
		v13 = v4 + v6 - 128;
		if (v8 > 0xB1u) {
			v8 = -1;
		}
		v14 = 2 * v7;
		if (!v11 & v10) {
			oldmcu_balance[0] = v8;
		}
		v15 = 2 * v9;
		v13 = v13;
		v16 = -1840700269LL * v14;
		LOBYTE(v17) = v5 + v6 + -128;
		div = -1840700269LL * v15;
		oldmcu_balance[1] = v12;
		if (v12 > 177) {
			oldmcu_balance[1] = -1;
		}
		v17 = v17;
		oldmcu_balance[2] = v13;
		v19 = v14 >> 31;
		if (v13 > 177) {
			oldmcu_balance[2] = -1;
		}
		v20 = v17 >= 177;
		v21 = v17 == 177;
		v22 = HIDWORD(v16) + v14;
		v23 = v15 >> 31;
		v24 = HIDWORD(div) + v15;
		oldmcu_balance[3] = v17;
		if (v17 > 177) {
			LOBYTE(v17) = -1;
		}
		if (!v21 & v20) {
			oldmcu_balance[3] = v17;
		}
		buf[0] = (v22 >> 4) - v19;
		buf[1] = (v24 >> 4) - v23;
		arm_send_multi(0x9010u, 2, buf);
		Func_set_balance(oldmcu_balance);
	} else {
		b2 = car_struct.audio->balance2;
		balance[0] = car_struct.audio->balance1;
		balance[1] = b2;
		if (balance[0] <= 28u && b2 <= 28) {
			Func_set_balance_v151(balance);
		}
	}
}

/* decompiled */
void
Audio_Eq()
{
	unsigned char eq_settings[4] = {0, 0, 0, 0};

	if (strcmp(car_struct.car_status.mcuver2, "V1.51") < 0) {
		if (car_struct.audio->eq2 <= 20) {
			if (car_struct.audio->eq1) {
				eq_settings[0] = SoundEffectTable[car_struct.audio->eq2 + 10];
			} else {
				eq_settings[0] = SoundEffectTable[car_struct.audio->eq2];
			}
		}

		if (car_struct.audio->eq3 <= 20) {
			eq_settings[1] = SoundEffectTable[car_struct.audio->eq3];
		}
		if (car_struct.audio->eq4 <= 20) {
			if (car_struct.audio->eq1) {
				eq_settings[2] = SoundEffectTable[car_struct.audio->eq4 + 5];
			} else {
				eq_settings[2] = SoundEffectTable[car_struct.audio->eq4];
			}
		}
		Func_set_equalizer(eq_settings);
	} else {
		eq_settings[0] = car_struct.audio->eq2;
		eq_settings[1] = car_struct.audio->eq3;
		eq_settings[2] = car_struct.audio->eq4;
		eq_settings[3] = car_struct.audio->eq1;
		if (car_struct.audio->eq2 <= 20 && car_struct.audio->eq3 <= 20 &&
		    car_struct.audio->eq4 <= 20) {
			Func_set_equalizer_v151(eq_settings);
		}
	}
}

/* dirty code */
void
Audio_AJXChannel(int volume)
{
	int ch;				      // r0@2
	char *v2;			      // r3@3
	struct mtc_audio_struct *p_mtc_audio; // r3@10

	if (volume) {
		Audio_SetVolume(60u);
		msleep(20u);
		p_mtc_audio = car_struct.audio;
		car_struct.audio->gap1[5] = 0;
		p_mtc_audio->mute = 0;
		arm_send_multi(MTC_CMD_SET_MUTE, 1, &p_mtc_audio->mute);
		vs_send(2, 0x96u, &car_struct.audio->mute, 1);
		Audio_Channel(car_struct.config_data._gap3[0]);
		arm_send_multi(0x9030u, 0, 0);
		car_struct.car_status._gap9[4] = 1;
		msleep(20u);
		Audio_SetVolume(0);
	} else {
		Audio_SetVolume(60u);
		msleep(20u);
		if (car_struct.car_status._gap6[0]) {
			Audio_Channel(8);
		} else {
			v2 = car_struct.car_status.fm_volume;
			if (car_struct.car_status.fm_volume) {
				ch = MTC_AV_CHANNEL_FM;
			} else {
				v2 = &car_struct.audio;
			}
			if (!car_struct.car_status.fm_volume) {
				ch = *(*v2 + 8); // wut?
			}
			Audio_Channel(ch);
		}
		arm_send_multi(0x9031u, 0, 0);
		car_struct.car_status._gap9[4] = 0;
		msleep(20u);
		Audio_SetVolume(0);
	}
}

/* dirty code */
void
Audio_PhoneChannel(unsigned int a1)
{
	char v1;				 // r6@1
	int phone_mute;				 // r5@1
	int v3;					 // r1@3
	unsigned int cmd;			 // r0@6
	int channel;				 // r0@10
	struct mtc_audio_struct **p_p_mtc_audio; // r5@11
	struct mtc_audio_struct *p_mtc_audio;    // r3@17

	v1 = a1;
	phone_mute = car_struct.car_status._gap9[4];
	if (car_struct.car_status._gap9[4]) {
		car_struct.car_status._gap6[0] = a1;
		if (a1) {
			v3 = 0;
		} else {
			v3 = a1;
		}
		if (a1) {
			cmd = 0x9020;
		} else {
			cmd = 0x9021;
		}
		arm_send_multi(cmd, v3, v3);
		btMicControl();
	} else if (a1) {
		Audio_SetVolume(60u);
		msleep(20u);
		p_mtc_audio = car_struct.audio;
		car_struct.audio->gap1[5] = phone_mute;
		p_mtc_audio->mute = phone_mute;
		arm_send_multi(0x9024u, 1, &p_mtc_audio->mute);
		vs_send(2, 0x96u, &car_struct.audio->mute, 1);
		Audio_Channel(8);
		car_struct.car_status._gap6[0] = v1;
		btMicControl();
		arm_send_multi(0x9020u, phone_mute, phone_mute);
		msleep(20u);
		msleep(1500u);
		Audio_SetVolume(phone_mute);
	} else {
		Audio_SetVolume(60u);
		msleep(20u);
		if (car_struct.car_status.fm_volume) {
			channel = 4;
			p_p_mtc_audio = &car_struct.audio;
		} else {
			p_p_mtc_audio = &car_struct.audio;
		}
		if (!car_struct.car_status.fm_volume) {
			channel = (*p_p_mtc_audio)->audio_ch;
		}
		Audio_Channel(channel);
		car_struct.car_status._gap6[0] = 0;
		btMicControl();
		arm_send_multi(0x9021u, 0, 0);
		msleep(20u);
		msleep(1000u);
		if (!(*p_p_mtc_audio)->mute) {
			Audio_SetVolume(0);
		}
	}
}

void
Audio_EnterChannel(int set_ch, int a2)
{
	// too many code
}

void
audio_active()
{
	// too many code
}

/* contains unknown fields */
void
audio_deactive()
{
	if (car_struct.audio->audio_active) {
		car_struct.audio->audio_active = 0;
		car_struct.car_status._gap6[0] = 0;
		btMicControl();
		arm_send_multi(0x9006u, 0, 0);
		car_struct.audio->mute_all = 1;
		codec_deactive();

		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_JY) {
			gpio_direction_output(gpio_FCAM_PWR, 0);
		}
	}
}

static void
audio_work(struct work_struct *work)
{
	// too many code
}

/* decompiled */
void
audio_add_work(unsigned int cmd1, int cmd2, int cmd3, int val1)
{
	if (car_struct.car_status._gap2[1]) {
		struct mtc_work *a_work;

		a_work = kmalloc(sizeof(struct mtc_work), __GFP_IO);

		INIT_WORK(&a_work->dwork.work, audio_work);

		a_work->cmd1 = cmd1;
		a_work->cmd2 = cmd2;
		*(int *)&a_work->cmd3 = cmd3;
		a_work->val1 = val1;

		queue_work(car_struct.audio->audio_wq, &a_work->dwork.work);
	}
}

/* decompiled */
void
audio_add_work_delay(int data, unsigned int delay, int a3)
{
	audio_add_dwork(data, delay, a3, &car_struct.car_status);
}

/* contains unknown fields */
void
audio_add_dwork(int data, unsigned int delay, int a3, struct mtc_car_status *c_status)
{
	if (c_status->_gap2[1]) {
		struct mtc_work *work_data;

		work_data = kmalloc(sizeof(struct mtc_work), __GFP_IO);
		INIT_DELAYED_WORK(&work_data->dwork, audio_work);
		work_data->cmd1 = data;
		queue_delayed_work(car_struct.audio->audio_wq, &work_data->dwork,
				   msecs_to_jiffies(delay));
	}
}

/* contains unknown fields */
void
audio_channel_switch_unmute()
{
	if (car_struct.audio->gap1[4] == 1) {
		int audio_ch = car_struct.audio->audio_ch;
		if (audio_ch == MTC_AV_CHANNEL_DVD) {
			audio_add_work_delay(26, 3000u, 1);
		} else if (audio_ch == MTC_AV_CHANNEL_DTV) {
			audio_add_work_delay(26, 1000u, 1);
		} else {
			audio_add_work_delay(26, 0, 1);
		}
	}
}
EXPORT_SYMBOL_GPL(audio_channel_switch_unmute)

/* decompiled */
void
audio_flush_work()
{
	flush_workqueue(car_struct.audio->audio_wq);
}
EXPORT_SYMBOL_GPL(audio_flush_work)

/* contains unknown fields */
void
audio_init()
{
	struct mtc_audio_struct *audio;

	audio = kmalloc(sizeof(struct mtc_audio_struct),
			__GFP_IO | GFP_ATOMIC | __GFP_WAIT | __GFP_MOVABLE | __GFP_DMA32);

	car_struct.audio = audio;
	memset(audio, 0, sizeof(struct mtc_audio_struct));

	mutex_init(&car_struct.audio->lock);

	car_struct.audio->audio_wq = create_singlethread_workqueue("audio_wq");

	car_struct.car_status._gap2[1] = 1; // ?
	audio->eq1 = 0;
	audio->eq2 = 10;
	audio->eq3 = 10;
	audio->eq4 = 10;
	audio->balance1 = 14;
	audio->balance2 = 14;
	audio->gap1[1] = 0; // ?
	audio->gap6[2] = 0; // ?
}
EXPORT_SYMBOL_GPL(audio_init)

/* recovered structures */

static struct early_suspend audio_early_suspend = {
    .suspend = audio_s_suspend, .resume = audio_s_resume,
};

static struct dev_pm_ops audio_pm_ops = {
    .suspend = audio_suspend, .resume = audio_resume,
};

static struct platform_driver mtc_audio_driver = {
    .probe = audio_probe,
    .remove = __devexit_p(audio_remove),
    .driver =
	{
	    .name = "mtc-audio", .pm = &audio_pm_ops,
	},
};

module_platform_driver(mtc_audio_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC AUDIO driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-audio");
