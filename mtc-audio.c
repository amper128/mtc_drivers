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
	char gap1[3];
	char audio_ch2;
	char gap11[2];
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

// unknown table
static unsigned char ch_data[12] = {0xE, 0, 0xC, 0xA, 0x14, 0xA, 6, 0, 0xE, 0, 0, 0};

void audio_add_dwork(int data, unsigned int delay, int a3, struct mtc_car_status *c_status);

/* dirty code */
int
mtcGetSetVolume(int volume)
{
	int setvol;       // r2@1
	signed int v2;    // r3@2
	unsigned char v3; // zf@2
	char v4;	  // nf@2
	unsigned char v5; // vf@2
	int v6;		  // r2@3

	setvol = 10000 * volume;
	if (10000 * volume <= 299999) {
		v6 = (0xAAAAAAACLL * (setvol + 10000)) >> 32;
	} else {
		v2 = 599999;
		v5 = __OFSUB__(setvol, 599999);
		v3 = setvol == 599999;
		v4 = setvol - 599999 < 0;
		if (setvol <= 599999) {
			v6 = setvol - 89088;
		} else {
			v6 = 50000 * volume;
		}
		if ((v4 ^ v5) | v3) {
			v6 -= 912;
		} else {
			v2 = v6 - 950000;
		}
		if (!((v4 ^ v5) | v3)) {
			v6 = v2 >> 2;
		}
	}
	return ((((0x431BDE83LL * v6 * car_struct.car_status._gap12[0]) >> 32) >> 18) -
		(v6 * car_struct.car_status._gap12[0] >> 31));
}

/* decompiled */
signed int
inPhoneMode()
{
	return 1;
}

/* decompiled */
void
Func_set_balance(unsigned char *balance_val)
{
	arm_send_multi(MTC_CMD_SET_BALANCE, 4, balance_val);
}

/* decompiled */
void
Func_set_balance_v151(unsigned char *balance)
{
	/* for old MCU */
	arm_send_multi(MTC_CMD_SET_BALANCE, 2, balance);
}

/* decompiled */
void
Func_set_equalizer(unsigned char *eq_val)
{
	arm_send_multi(MTC_CMD_SET_EQUALIZER, 3, eq_val);
}

/* decompiled */
void
Func_set_equalizer_v151(unsigned char *eq_val)
{
	/* for old MCU */
	arm_send_multi(MTC_CMD_SET_EQUALIZER, 4, eq_val);
}

/* decompiled */
void
Func_set_channel(unsigned char *ch_val)
{
	arm_send_multi(MTC_CMD_SET_CHANNEL, 2, ch_val);
}

/* decompiled */
void
Func_set_volume(unsigned char *volume)
{
	arm_send_multi(MTC_CMD_SET_VOLUME, 2, volume);
}

/* decompiled */
int
getAudioChannel()
{
	return car_struct.audio->audio_ch;
}

/* decompiled */
bool
isAudioMute()
{
	if (car_struct.audio->mute) {
		return true;
	}

	return false;
}

/* decompiled */
void
btMicControl()
{
	if (car_struct.car_status.ch_mode != 3 || car_struct.config_data.cfg_bt == 6 ||
	    car_struct.config_data.cfg_bt == 7) {
		gpio_direction_output(172u, 0);
	} else {
		gpio_direction_output(172u, 1);
	}
}

/* decompiled */
void
SetbtMicControl(int val)
{
	gpio_direction_output(172u, val);
}

/* decompiled */
void
Audio_SoftMute()
{
	arm_send_multi(MTC_CMD_SOFT_MUTE, 0, 0);
}

/* dirty code */
void
Audio_SetVolume(unsigned int volume)
{
	int v1;		   // r12@2
	int v2;		   // r1@4
	unsigned int v3;   // r12@5
	int v4;		   // r3@8
	char v5;	   // r2@9
	unsigned int v6;   // r3@14
	char v7;	   // zf@23
	int v8;		   // r4@31
	unsigned int v9;   // r2@34
	int v10;	   // r6@43
	int v11;	   // r5@43
	unsigned int v12;  // r4@43
	int v13;	   // r2@58
	unsigned char v14; // [sp+5h] [bp-1Bh]@7
	char v15;	  // [sp+6h] [bp-1Ah]@11
	char v16;	  // [sp+7h] [bp-19h]@8

	if (car_struct.audio->gap6[2] != 4 || (v1 = car_struct.audio->gap1[3], v1 == 255)) {
		v1 = car_struct.audio->audio_ch;
	}
	if (volume > 60) {
		return;
	}
	v2 = car_struct.car_status._gap9[4];
	if (car_struct.car_status._gap9[4]) {
		v2 = 0;
		v3 = 60 * (car_struct.audio->gap1[1] + 1) / 0x64 & 0xFF;
		goto LABEL_6;
	}
	if (!car_struct.car_status._gap6[0]) {
		v2 = car_struct.car_status.ch_status;
		if (!car_struct.car_status.ch_status) {
			if (v1 == 1) {
				goto LABEL_91;
			}
			if (!v1) {
				v7 = car_struct.config_data.cfg_bt == 2;
				if (car_struct.config_data.cfg_bt != 2) {
					v7 = car_struct.config_data.cfg_bt == 6;
				}
				if (v7 || car_struct.config_data.cfg_bt == 7) {
				LABEL_91:
					v2 = car_struct.audio->gap1[1];
					if (car_struct.audio->char18 == 3) {
						if (car_struct.audio->gap1[1]) {
							v8 = v2 + 1;
							v2 = 0;
							v3 = (60 * v8 / 0x64u & 0xFF) + 8;
						} else {
							v3 = 0;
						}
					} else {
						v2 = 0;
						v3 = 0;
					}
				LABEL_55:
					if (car_struct.car_status._gap5[0]) {
						if (car_struct.car_status.backview_vol > 0xAu) {
							v2 = 0;
							goto LABEL_41;
						}
						v13 = 11 - car_struct.car_status.backview_vol;
						v2 = v13 * v2 / 0xBu;
						v3 = v13 * v3 / 0xB;
					}
					goto LABEL_6;
				}
			}
		}
		v10 = car_struct.audio->gap1[1];
		v11 = car_struct.audio->gap1[5];
		v12 = 60 * (v10 + 1) / 0x64u & 0xFF;
		if (v11 == 1 || car_struct.audio->gap1[4] == 1 ||
		    (v3 = car_struct.audio->gap5[2]) != 0) {
			v3 = 0;
		} else if (car_struct.car_status.av_gps_monitor) {
			if (car_struct.audio->gap5[0]) {
				if (!car_struct.car_status.av_gps_switch) {
					if (v12 <= 0xB) {
						v3 = car_struct.car_status.av_gps_switch;
					} else {
						v3 = v12 - 12;
					}
				}
			} else {
				v3 = 60 * (v10 + 1) / 0x64u & 0xFF;
			}
		} else if (car_struct.audio->gap5[1]) {
			v3 = 0;
		} else {
			v3 = 60 * (v10 + 1) / 0x64u & 0xFF;
		}
		if (car_struct.car_status.ch_status) {
			if (v11 == 1) {
				v3 = 0;
			} else if (car_struct.audio->gap1[4] == 1) {
				v3 = 0;
			} else {
				v3 = 60 * (v10 + 1) / 0x64u & 0xFF;
			}
			if (car_struct.audio->char18 != 3) {
				goto LABEL_54;
			}
			if (car_struct.audio->gap1[1]) {
				if (v12 > 3) {
					v2 = v12 - 4;
					goto LABEL_55;
				}
			LABEL_54:
				v2 = 0;
				goto LABEL_55;
			}
		} else {
			if (car_struct.audio->char18 != 3) {
				goto LABEL_55;
			}
			if (car_struct.audio->gap1[1]) {
				v2 = v12 + 8;
				goto LABEL_55;
			}
		}
		v2 = car_struct.audio->gap1[1];
		goto LABEL_55;
	}
	v9 = 60 * (car_struct.audio->gap1[2] + 1) / 0x64 & 0xFF;
	if (car_struct.config_data.cfg_bt != 2) {
		if ((car_struct.config_data.cfg_bt - 6) <= 1u) {
			if (car_struct.audio->char18 == 3) {
				if (car_struct.audio->gap1[2]) {
					v2 = v9 + 8;
				} else {
					v2 = 0;
				}
			}
			goto LABEL_41;
		}
		v3 = 60 * (car_struct.audio->gap1[2] + 1) / 0x64 & 0xFF;
		goto LABEL_6;
	}
	if (car_struct.car_status._gap6[0] == 1) {
		v3 = car_struct.car_status._gap9[4];
	} else {
		v3 = 60 * (car_struct.audio->gap1[2] + 1) / 0x64 & 0xFF;
		if ((car_struct.car_status._gap6[0] - 2) <= 1u) {
			goto LABEL_6;
		}
	}
	if (car_struct.audio->char18 == 3) {
		if (car_struct.audio->gap1[2]) {
			v2 = v9 + 8;
		} else {
			v2 = 0;
		}
	} else {
		v2 = 0;
	}
LABEL_6:
	if (v3) {
		v14 = -67 - v3;
		goto LABEL_8;
	}
LABEL_41:
	v14 = -1;
LABEL_8:
	v4 = car_struct.audio->mute;
	v16 = volume;
	if (v2) {
		v5 = -67 - v2;
	} else {
		v5 = -1;
	}
	v15 = v5;
	if (v4 != 1) {
		if (volume == 60) {
			v14 = -1;
		} else {
			v6 = v14 + volume;
			if (v6 > 0xCF) {
				LOBYTE(v6) = -1;
			}
			v14 = v6;
			if (!volume) {
				Func_set_volume(&v14);
				return;
			}
		}
		v15 = -1;
		Func_set_volume(&v14);
	}
}

/* decompiled */
void
Audio_FadeIn()
{
	int volume;

	if (isAudioEnable()) {
		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_AM) {
			Audio_SetVolume(0);
		} else {
			volume = 60;
			do {
				Audio_SetVolume(volume);
				volume -= 5;

				msleep(15u);
			} while (volume != -5);
		}
	}
}

/* decompiled */
void
Audio_FadeOut()
{
	unsigned char volume;

	if (car_struct.audio->mute_all != 1) {
		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_AM) {
			Audio_SetVolume(60u);
		} else {
			volume = 0;
			do {
				Audio_SetVolume(volume);
				volume += 5;
				msleep(15u);
			} while (volume != 65);
		}
	}
}

/* decompiled */
void
Audio_Channel(int ch)
{
	char bt;
	unsigned char channel[2];

	channel[0] = ChannelSel[ch];
	channel[1] = ch_data[ch];

	if (ch == 0) {
		bt = car_struct.config_data.cfg_bt;

		if (bt == 2 || bt == 6 || bt == 7) {
			channel[0] = ChannelSel[1];
			channel[1] = ch_data[1];
		}
	}

	Func_set_channel(channel);
}

/* decompiled */
void
Audio_Mute(signed int mute)
{
	struct mtc_audio_struct *audio = car_struct.audio;

	if (audio->mute != mute) {
		if (mute == 1) {
			Audio_FadeOut();
			audio->mute = mute;
			arm_send_multi(MTC_CMD_SET_MUTE, mute, &audio->mute);
			vs_send(2, 0x96u, &audio->mute, mute);
		} else {
			audio->mute = 0;
			arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
			vs_send(2, 0x96u, &audio->mute, 1);
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
	struct mtc_audio_struct *audio = car_struct.audio;
	unsigned char eq_settings[4] = {0, 0, 0, 0};

	if (strcmp(car_struct.car_status.mcuver2, "V1.51") < 0) {
		if (car_struct.audio->eq2 <= 20) {
			if (car_struct.audio->eq1) {
				eq_settings[0] = SoundEffectTable[(int)audio->eq2 + 10];
			} else {
				eq_settings[0] = SoundEffectTable[(int)audio->eq2];
			}
		}

		if (car_struct.audio->eq3 <= 20) {
			eq_settings[1] = SoundEffectTable[(int)audio->eq3];
		}

		if (car_struct.audio->eq4 <= 20) {
			if (car_struct.audio->eq1) {
				eq_settings[2] = SoundEffectTable[(int)audio->eq4 + 5];
			} else {
				eq_settings[2] = SoundEffectTable[(int)audio->eq4];
			}
		}

		Func_set_equalizer(eq_settings);
	} else {
		eq_settings[0] = audio->eq2;
		eq_settings[1] = audio->eq3;
		eq_settings[2] = audio->eq4;
		eq_settings[3] = audio->eq1;

		if (car_struct.audio->eq2 <= 20 && car_struct.audio->eq3 <= 20 &&
		    car_struct.audio->eq4 <= 20) {
			Func_set_equalizer_v151(eq_settings);
		}
	}
}

/* contains unknown fields */
void
Audio_AJXChannel(int mode)
{
	struct mtc_audio_struct *audio = car_struct.audio;

	if (mode) {
		Audio_SetVolume(60u);
		msleep(20u);
		audio->gap11[1] = 0;
		audio->mute = 0;
		arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
		vs_send(2, 0x96u, &audio->mute, 1);
		Audio_Channel(car_struct.config_data._gap3[0]);
		arm_send_multi(0x9030u, 0, 0);
		car_struct.car_status._gap9[4] = 1;
		msleep(20u);
		Audio_SetVolume(0);
	} else {
		Audio_SetVolume(60u);
		msleep(20u);

		if (car_struct.car_status.ch_mode) {
			Audio_Channel(8);
		} else {
			if (car_struct.car_status.ch_status) {
				Audio_Channel(MTC_AV_CHANNEL_FM);
			} else {
				Audio_Channel(audio->audio_ch);
			}
		}

		arm_send_multi(0x9031u, 0, 0);
		car_struct.car_status._gap9[4] = 0;
		msleep(20u);
		Audio_SetVolume(0);
	}
}

/* contains unknown fields */
void
Audio_TAChannel(unsigned int mode)
{
	struct mtc_audio_struct *audio = car_struct.audio;

	if (car_struct.car_status._gap9[4] || car_struct.car_status.ch_mode > 0) {
		car_struct.car_status.ch_status = mode;
		return;
	}

	if (mode) {
		if (audio->audio_ch == MTC_AV_CHANNEL_FM) {
			car_struct.car_status.ch_status = mode;
			Audio_SetVolume(car_struct.car_status.ch_mode);
		} else {
			Audio_SetVolume(60u);
			msleep(20u);
			car_struct.audio->gap11[1] = 0;
			audio->mute = 0;
			arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
			vs_send(2, 0x96u, &audio->mute, 1);
			Audio_Channel(MTC_AV_CHANNEL_FM);
			car_struct.car_status.ch_status = mode;
			msleep(20u);
			Audio_SetVolume(0);
		}
	} else {
		if (audio->audio_ch == MTC_AV_CHANNEL_FM) {
			car_struct.car_status.ch_status = 0;
			Audio_SetVolume(0);
		} else {
			Audio_SetVolume(60u);
			msleep(20u);
			car_struct.car_status.ch_status = 0;
			Audio_Channel(car_struct.audio->audio_ch);
			msleep(20u);
			Audio_SetVolume(0);
		}
	}
}

/* contains unknown fields */
void
Audio_PhoneChannel(unsigned int mode)
{
	struct mtc_audio_struct *audio = car_struct.audio;

	if (car_struct.car_status._gap9[4]) {
		unsigned int cmd;

		car_struct.car_status.ch_mode = mode;
		if (mode) {
			cmd = 0x9020;
		} else {
			cmd = 0x9021;
		}
		arm_send_multi(cmd, 0, 0);
		btMicControl();

		return;
	}

	if (mode) {
		Audio_SetVolume(60u);
		msleep(20u);
		audio->gap11[1] = 0;
		audio->mute = 0;
		arm_send_multi(0x9024u, 1, &audio->mute);
		vs_send(2, 0x96u, &audio->mute, 1);
		Audio_Channel(8);
		car_struct.car_status.ch_mode = mode;
		btMicControl();
		arm_send_multi(0x9020u, 0, 0);
		msleep(20u);
		msleep(1500u);
		Audio_SetVolume(0);
	} else {
		int channel;

		Audio_SetVolume(60u);
		msleep(20u);

		if (car_struct.car_status.ch_status) {
			channel = 4;
		} else {
			channel = audio->audio_ch;
		}

		Audio_Channel(channel);
		car_struct.car_status.ch_mode = 0;
		btMicControl();
		arm_send_multi(0x9021u, 0, 0);
		msleep(20u);
		msleep(1000u);

		if (audio->mute) {
			Audio_SetVolume(0);
		}
	}
}

/* dirty code */
void
Audio_EnterChannel(int set_ch, int mode)
{
	struct mtc_audio_struct *audio = car_struct.audio;
	struct mtc_car_status *car_status = &car_struct.car_status;
	int audio_ch;
	char bt;

	signed int v8;
	signed int v9;
	signed int v10;
	char v11;
	unsigned int v12;
	unsigned char v13;
	char v14;
	char v16;

	printk("enter channel %d:%d:%d\n", set_ch, car_struct.audio->audio_ch, mode);

	if (car_struct.audio->gap6[2] == 4) {
		if (set_ch == 0) {

			Audio_FadeOut();
			msleep(20u);
			car_struct.audio->mute = 0;
			arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
			vs_send(2, 0x96u, &car_struct.audio->mute, 1);

			if (mode) {
				audio->audio_ch2 = 0;
			} else {
				audio->audio_ch2 = audio->audio_ch;
			}

			Audio_Channel(car_struct.audio->audio_ch2);
			msleep(20u);
			Audio_SetVolume(0);

			return;
		}

		audio_ch = car_struct.audio->audio_ch;
		if (!mode) {
			if (audio_ch != set_ch) {
				return;
			}

			switch (audio_ch) {
			case MTC_AV_CHANNEL_FM:
				Radio_Power(0);
				break;

			case MTC_AV_CHANNEL_DTV:
				arm_send_multi(MTC_CMD_TV_OFF, 0, 0);
				break;

			case MTC_AV_CHANNEL_DVD:
				dvd_power(0);
				return;
			}

			audio->audio_ch = MTC_AV_CHANNEL_SYS;

			return;
		}

		if (audio_ch == set_ch) {
			return;
		}

		switch (set_ch) {
		case MTC_AV_CHANNEL_FM:
			Radio_Power(1);
			audio_ch = car_struct.audio->audio_ch;
			break;

		case MTC_AV_CHANNEL_DTV:
			arm_send_multi(MTC_CMD_TV_ON, 0, 0);
			audio_ch = audio->audio_ch;
			break;

		case MTC_AV_CHANNEL_DVD:
			dvd_power(1);
			return;
		}

		if (audio_ch == MTC_AV_CHANNEL_FM) {
			Radio_Power(0);
			audio->audio_ch = set_ch;
			return;
		}

		if (audio_ch == MTC_AV_CHANNEL_DTV) {
			arm_send_multi(MTC_CMD_TV_OFF, 0, 0);
			audio->audio_ch = set_ch;
			return;
		}

		if (audio_ch != MTC_AV_CHANNEL_DVD) {
			audio->audio_ch = set_ch;
			return;
		}

		dvd_power(0);
		return;
	}

	audio_ch = audio->audio_ch;

	if (mode) {
		if (audio_ch == set_ch) {
			int c1;
			int tmp_ch;
			int c2;

			if (audio_ch == MTC_AV_CHANNEL_SYS) {
				if (audio->mute != 1) {
					return;
				}

				Audio_Mute(0);
				c1 = 1;
				tmp_ch = car_struct.audio->audio_ch;
				c2 = tmp_ch - 1 + ((tmp_ch - 1) <= 0) - (tmp_ch - 1);
			} else {
				c1 = audio_ch - 3;
				tmp_ch = car_struct.audio->audio_ch;
				c2 = 1;

				if (audio_ch != MTC_AV_CHANNEL_LINE) {
					c1 = 1;
				}
			}

			if (c1 & c2) {
				return;
			}

			set_ch = audio_ch;
			audio_ch = tmp_ch;
		}

		if (car_status->ch_mode) {
			goto LABEL_45;
		}
	} else {
		if ((audio_ch != set_ch) || (audio_ch == MTC_AV_CHANNEL_SYS)) {
			return;
		}

		set_ch = MTC_AV_CHANNEL_SYS;

		if (car_status->ch_mode) {
			goto LABEL_45;
		}
	}

	if (car_status->_gap9[4] || car_status->ch_status) {
	LABEL_45:
		if (audio_ch == MTC_AV_CHANNEL_FM) {
			Radio_Power(0);
			audio->audio_ch = set_ch;
			return;
		}

		if (audio_ch == MTC_AV_CHANNEL_DTV) {
			arm_send_multi(MTC_CMD_TV_OFF, 0, 0);
			audio->audio_ch = set_ch;
			return;
		}

		if (audio_ch != MTC_AV_CHANNEL_DVD) {
			audio->audio_ch = set_ch;
			return;
		}

		msleep(500u);
		dvd_power(0);

		return;
	}

	bt = car_struct.config_data.cfg_bt;
	if (bt == 2 || bt = 6 || bt == 7) {
		if (set_ch == MTC_AV_CHANNEL_SYS) {
			if (audio_ch) {
				goto LABEL_11;
			}
		} else if (set_ch || (audio_ch != MTC_AV_CHANNEL_SYS)) {
			goto LABEL_11;
		}

		audio->audio_ch = set_ch;
		audio->gap11[1] = 0;
		audio->mute = 0;
		audio->gap11[0] = 0;
		arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
		vs_send(2, 0x96u, &car_struct.audio->mute, 1);
		Audio_SetVolume(0);

		return;
	}

LABEL_11:
	if (audio_ch == MTC_AV_CHANNEL_DVD) {
		dvd_send_command(0x30003);
	}

	Audio_FadeOut();
	if (set_ch != MTC_AV_CHANNEL_DTV &&
	    (set_ch != MTC_AV_CHANNEL_FM || car_struct.config_data.cfg_radio == 2 ||
	     car_struct.config_data.cfg_radio == 4)) {
		if (audio_ch != MTC_AV_CHANNEL_FM || car_struct.config_data.cfg_radio == 2 ||
		    car_struct.config_data.cfg_radio == 4) {
			v8 = 0;
			v9 = 0;
			v10 = 0;

			goto LABEL_17;
		}

		v9 = 1;
		v10 = 0;
	} else {
		v9 = 0;
		v10 = 1;
	}

	v8 = 1;
	Audio_SetMutePin(1);
	msleep(200u);
LABEL_17:
	switch (set_ch) {
	case MTC_AV_CHANNEL_FM:
		Radio_Power(1);
		break;

	case MTC_AV_CHANNEL_DTV:
		arm_send_multi(MTC_CMD_TV_ON, 0, 0);
		break;

	case MTC_AV_CHANNEL_DVD:
		dvd_power(1);
		break;
	}

	msleep(20u);
	if (mode) {
		audio->mute = 0;
		arm_send_multi(MTC_CMD_SET_MUTE, 1, &audio->mute);
		vs_send(2, 0x96u, &audio->mute, 1);
	}

	audio->audio_ch = set_ch;
	Audio_Channel(set_ch);
	capture_add_work(0x2Eu, audio->audio_ch, 0, 0);
	msleep(20u);

	v12 = (set_ch - 4);
	v13 = set_ch >= MTC_AV_CHANNEL_DVD;
	v14 = set_ch == MTC_AV_CHANNEL_DVD;
	if (set_ch != MTC_AV_CHANNEL_DVD) {
		v13 = v12 >= 1;
		v14 = v12 == 1;
	}

	if (!v14 & v13) {
		v16 = 0;
	} else {
		v16 = 1;
		v11 = 1;
	}

	if (!v14 & v13) {
		audio->gap1[5] = v16;
		audio->gap1[4] = v16;
	} else {
		audio->gap1[5] = v11;
		audio->gap1[4] = v11;
	}

	if (!audio->mute) {
		Audio_SetVolume(0);
	}

	if (audio_ch == MTC_AV_CHANNEL_FM) {
		Radio_Power(0);
	LABEL_36:
		if (!v8) {
			return;
		}

		if (v9) {
			msleep(100u);
		}

		Audio_SetMutePin(0);

		return;
	}

	if (audio_ch == MTC_AV_CHANNEL_DTV) {
		arm_send_multi(MTC_CMD_TV_OFF, 0, 0);
		goto LABEL_36;
	}

	if (audio_ch != MTC_AV_CHANNEL_DVD) {
		goto LABEL_36;
	}

	if (v10) {
		Audio_SetMutePin(0);
	}

	msleep(500u);
	dvd_power(0);
}

/* dirty code */
void
audio_active()
{
	struct mtc_audio_struct *p_mtc_audio; // lr@5 MAPDST
	char *mute;			      // r2@5
	unsigned char v2;		      // r10@5
	unsigned char v3;		      // ST08_1@5
	unsigned char v4;		      // ST0C_1@5
	unsigned char v5;		      // r7@5
	unsigned int v7;		      // r2@9
	int v8;				      // r1@9
	int v9;				      // r0@9
	int v10;			      // r4@9
	int v11;			      // r12@9
	char v12;			      // zf@12
	int v13;			      // r2@12
	char v14;			      // [sp+17h] [bp-29h]@5

	if (!car_struct.audio->audio_active) {
		printk("--mtc audio active\n"); // "--mtc audio active\n"
		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_JY) {
			gpio_direction_output(gpio_FCAM_PWR, 1);
		}
		codec_active();
		p_mtc_audio = car_struct.audio;
		mute = &car_struct.audio->mute;
		v2 = car_struct.config_data._gap1[0];
		ch_data[5] = car_struct.config_data._gap1[5];
		v3 = car_struct.config_data._gap1[3];
		ch_data[8] = car_struct.config_data._gap1[0];
		ch_data[7] = 0;
		v4 = car_struct.config_data._gap1[4];
		ch_data[6] = car_struct.config_data._gap1[6];
		v5 = car_struct.config_data._gap1[1];
		ch_data[2] = car_struct.config_data._gap1[2];
		car_struct.audio->char18 = 0;
		p_mtc_audio->gap5[2] = 0;
		p_mtc_audio->gap5[0] = 0;
		p_mtc_audio->gap5[1] = 0;
		p_mtc_audio->mute_all = 1;
		p_mtc_audio->mute = 0;
		p_mtc_audio->gap1[4] = 0;
		ch_data[0] = v2;
		ch_data[1] = v5;
		ch_data[3] = v3;
		ch_data[4] = v4;
		vs_send(2, 0x96u, mute, 1);
		car_struct.audio->audio_ch = 1; // MTC_AV_CHANNEL_SYS
		arm_send_multi(0x9005u, 0, 0);
		msleep(100u);
		v14 = mtcGetSetVolume(car_struct.audio->gap1[1]);
		arm_send_multi(0x9023u, 1, &v14);
		car_struct.audio->audio_active = 1;
		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_KLD) {
			iomux_set(0x1A51u);
		}
		Audio_Balance();
		Audio_Eq();
		Audio_Channel(car_struct.audio->audio_ch);
		if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_KLD) {
			msleep(2000u);
		}
		Audio_SetMutePin(0);
		p_mtc_audio = car_struct.audio;
		v7 = car_struct.audio->gap6[1];
		v8 = v7 & 3;
		v9 = (v7 >> 1) & 1;
		v10 = (v7 >> 2) & 1;
		v11 = (v7 >> 3) & 1;
		if (v7 & 3) {
			v8 = 3;
		}
		if (car_struct.audio->gap5[0] != v9 || v8 != car_struct.audio->char18 ||
		    car_struct.audio->gap5[2] != v11 || car_struct.audio->gap5[1] != v10) {
			v12 = (v7 & 0x80) == 0;
			v13 = car_struct.audio->audio_active;
			if (!v12) {
				car_struct.audio->gap5[1] = v10;
			}
			p_mtc_audio->gap5[2] = v11;
			p_mtc_audio->gap5[0] = v9;
			p_mtc_audio->char18 = v8;
			if (v13) {
				Audio_SetVolume(0);
			}
		}
	}
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

/* dirty code */
static void
audio_work(struct work_struct *work)
{
	mtc_work *mtc_work;		      // r6@1
	signed int v3;			      // r0@4
	int v4;				      // r7@5
	int v5;				      // r1@5
	int v6;				      // r2@3
	unsigned int v8;		      // r2@16
	int v9;				      // r1@16
	int v10;			      // r0@16
	int v11;			      // r5@16
	int v12;			      // r12@16
	char v13;			      // zf@19
	int v14;			      // r2@19
	int v16;			      // r2@26
	unsigned int v17;		      // r0@27
	bool v18;			      // r2@27
	int v19;			      // r2@31
	int v21;			      // r2@43
	int v22;			      // r3@45
	int v24;			      // r3@48
	int v25;			      // r0@54
	signed int v26;			      // r0@56
	int v27;			      // r0@58
	int v28;			      // r3@62
	struct mtc_audio_struct *p_mtc_audio; // r2@64 MAPDST
	int v30;			      // r1@64
	int v31;			      // r2@71
	char v33;			      // [sp+7h] [bp-19h]@54

	mtc_work = container_of(work, mtc_work, dwork);
	mutex_lock(&car_struct.audio->lock);
	if (car_struct.audio->audio_active) {
		if (car_struct.audio->gap6[0]) {
			car_struct.audio->gap6[0] = 0;
			Audio_SetMutePin(0);
			msleep(20u);
			v6 = car_struct.audio->audio_active;
		}
	}
	v3 = printk("--mtc audio %02d(%d)\n", container_of(work, mtc_work, dwork)->cmd1);
	switch (container_of(work, mtc_work, dwork)->cmd1) {
	case 0:
		if (car_struct.audio->audio_active) {
			Audio_EnterChannel(LOBYTE(container_of(work, mtc_work, dwork)->cmd2), 1);
		}
		goto free_work;
	case 1:
		if (car_struct.audio->audio_active) {
			Audio_EnterChannel(LOBYTE(container_of(work, mtc_work, dwork)->cmd2), 0);
		}
		goto free_work;
	case 2:
		if (car_struct.audio->audio_active) {
			Audio_Mute(container_of(work, mtc_work, dwork)->cmd2);
		}
		goto free_work;
	case 3:
		v27 = LOBYTE(container_of(work, mtc_work, dwork)->cmd2);
		car_struct.audio->gap1[1] = v27;
		v33 = mtcGetSetVolume(v27);
		arm_send_multi(0x9023u, 1, &v33);
		if (car_struct.audio->audio_active) {
			v26 = car_struct.car_status._gap6[0];
			if (!car_struct.car_status._gap6[0]) {
				if (car_struct.audio->mute != 1) {
					goto LABEL_22;
				}
				goto LABEL_57;
			}
		}
		goto free_work;
	case 4:
		v25 = LOBYTE(container_of(work, mtc_work, dwork)->cmd2);
		car_struct.audio->gap1[2] = v25;
		v33 = mtcGetSetVolume(v25);
		arm_send_multi(0x9022u, 1, &v33);
		if (car_struct.car_status._gap6[0]) {
			if (car_struct.audio->mute == 1) {
				v26 = 0;
			LABEL_57:
				Audio_Mute(v26);
			}
			goto LABEL_22;
		}
		goto free_work;
	default:
		goto free_work;
	case 6:
		v31 = car_struct.audio->audio_active;
		car_struct.audio->gap5[1] = container_of(work, mtc_work, dwork)->cmd2;
		if (v31) {
			v17 = car_struct.car_status.av_gps_monitor;
			if (!car_struct.car_status.av_gps_monitor) {
				goto LABEL_42;
			}
		}
		goto free_work;
	case 7:
		if (car_struct.audio->audio_active) {
			goto LABEL_22;
		}
		goto free_work;
	case 8:
		if (car_struct.audio->audio_active && car_struct.car_status.av_gps_monitor) {
			goto LABEL_22;
		}
		goto free_work;
	case 0xA:
		v28 = container_of(work, mtc_work, dwork)->cmd2;
		if (v28 <= 28 && *&container_of(work, mtc_work, dwork)->cmd3 <= 28) {
			p_mtc_audio = car_struct.audio;
			v30 = car_struct.audio->audio_active;
			car_struct.audio->balance1 = v28;
			p_mtc_audio->balance2 = *&container_of(work, mtc_work, dwork)->cmd3;
			if (v30) {
				Audio_Balance();
			}
		}
		goto free_work;
	case 0xB:
		v22 = container_of(work, mtc_work, dwork)->cmd2;
		if (v22 <= 20 && *&container_of(work, mtc_work, dwork)->cmd3 <= 20 &&
		    container_of(work, mtc_work, dwork)->val1 <= 20) {
			p_mtc_audio = car_struct.audio;
			car_struct.audio->eq2 = v22;
			v24 = p_mtc_audio->audio_active;
			p_mtc_audio->eq3 = *&container_of(work, mtc_work, dwork)->cmd3;
			p_mtc_audio->eq4 = container_of(work, mtc_work, dwork)->val1;
			if (v24) {
				goto LABEL_44;
			}
		}
		goto free_work;
	case 0xC:
		v21 = car_struct.audio->audio_active;
		car_struct.audio->eq1 = container_of(work, mtc_work, dwork)->cmd2;
		if (v21) {
		LABEL_44:
			Audio_Eq();
		}
		goto free_work;
	case 0xD:
		v4 = container_of(work, mtc_work, dwork)->cmd2;
		v5 = car_struct.car_status._gap6[0];
		if (v4 == car_struct.car_status._gap6[0]) {
			goto free_work;
		}
		if (v4) {
			switch (v4) {
			case 1:
				printk("Phone In\n");
				v5 = car_struct.car_status._gap6[0];
				v4 = container_of(work, mtc_work, dwork)->cmd2;
				break;
			case 2:
				printk("Phone Out\n");
				v5 = car_struct.car_status._gap6[0];
				v4 = container_of(work, mtc_work, dwork)->cmd2;
				break;
			case 3:
				printk("Phone Answer\n");
				v5 = car_struct.car_status._gap6[0];
				v4 = container_of(work, mtc_work, dwork)->cmd2;
				break;
			}
		} else {
			printk("Phone Hangup\n");
			v5 = car_struct.car_status._gap6[0];
			v4 = container_of(work, mtc_work, dwork)->cmd2;
		}
		if (v5) {
			if (v4) {
				goto LABEL_13;
			}
			Audio_PhoneChannel(0);
			car_add_work(30, 0, 1);
		} else {
			if (!v4) {
			LABEL_13:
				car_struct.car_status._gap6[0] = v4;
				Audio_SetVolume(0);
				btMicControl();
				goto free_work;
			}
			car_add_work(29, 0, 1);
			Audio_PhoneChannel(LOBYTE(container_of(work, mtc_work, dwork)->cmd2));
		}
	free_work:
		kzfree(mtc_work);
		mutex_unlock(&car_struct.audio->lock);
		return;
	case 0xE:
		car_struct.audio->gap6[1] = container_of(work, mtc_work, dwork)->cmd2;
		goto LABEL_16;
	case 0xF:
	LABEL_16:
		printk("--mtc active1 %02x\n", container_of(work, mtc_work, dwork)->cmd2);
		p_mtc_audio = car_struct.audio;
		v8 = car_struct.audio->gap6[1];
		v9 = v8 & 3;
		v10 = (v8 >> 1) & 1;
		v11 = (v8 >> 2) & 1;
		v12 = (v8 >> 3) & 1;
		if (v8 & 3) {
			v9 = 3;
		}
		if (car_struct.audio->gap5[0] != v10 || v9 != car_struct.audio->char18 ||
		    car_struct.audio->gap5[2] != v12 || car_struct.audio->gap5[1] != v11) {
			v13 = (v8 & 0x80) == 0;
			v14 = car_struct.audio->audio_active;
			if (!v13) {
				car_struct.audio->gap5[1] = v11;
			}
			p_mtc_audio->gap5[2] = v12;
			p_mtc_audio->gap5[0] = v10;
			p_mtc_audio->char18 = v9;
			if (v14) {
			LABEL_22:
				Audio_SetVolume(0);
			}
		}
		goto free_work;
	case 0x11:
		if (car_struct.audio->audio_active) {
			if (car_struct.car_status._gap5[0]) {
				ta_check_back();
			} else {
				ta_check_start();
			}
			v17 = car_struct.audio->mute;
			if (!car_struct.audio->mute) {
				goto LABEL_42;
			}
		}
		goto free_work;
	case 0x12:
		p_mtc_audio = car_struct.audio;
		v16 = container_of(work, mtc_work, dwork)->cmd2;
		if (v16 == car_struct.audio->gap1[5]) {
			goto free_work;
		}
		goto LABEL_27;
	case 0x13:
		p_mtc_audio = car_struct.audio;
		if (car_struct.audio->audio_ch != MTC_AV_CHANNEL_FM) {
			goto free_work;
		}
		v16 = container_of(work, mtc_work, dwork)->cmd2;
		if (v16 == car_struct.audio->gap1[5]) {
			goto free_work;
		}
	LABEL_27:
		v17 = p_mtc_audio->mute;
		v18 = v16 != 0;
		p_mtc_audio->gap1[5] = v18;
		if (v17 || !p_mtc_audio->audio_active) {
			goto free_work;
		}
		if (v18 == 1) {
			goto LABEL_42;
		}
		Audio_FadeIn();
		goto free_work;
	case 0x14:
		audio_active();
		goto free_work;
	case 0x15:
		audio_deactive();
		goto free_work;
	case 0x16:
		if (!car_struct.car_status._gap9[4]) {
			car_add_work(29, car_struct.car_status._gap9[4], 1);
			Audio_AJXChannel(1);
		}
		goto free_work;
	case 0x17:
		if (car_struct.car_status._gap9[4]) {
			Audio_AJXChannel(0);
			car_add_work(30, 0, 1);
		}
		goto free_work;
	case 0x18:
		if (!car_struct.car_status.ch_status) {
			Audio_TAChannel(1u);
		}
		goto free_work;
	case 0x19:
		if (car_struct.car_status.ch_status) {
			Audio_TAChannel(0);
		}
		goto free_work;
	case 0x1A:
		p_mtc_audio = car_struct.audio;
		if (car_struct.audio->gap1[4]) {
			v17 = car_struct.audio->mute;
			car_struct.audio->gap1[4] = 0;
			p_mtc_audio->gap1[5] = 0;
			if (!v17) {
				if (p_mtc_audio->audio_active) {
				LABEL_42:
					Audio_SetVolume(v17);
				}
			}
		}
		goto free_work;
	case 0x1B:
		v19 = LOBYTE(container_of(work, mtc_work, dwork)->cmd2);
		p_mtc_audio = car_struct.audio;
		car_struct.audio->gap6[2] = v19;
		if (v19 == 4) {
			v3 = 1;
		}
		if (v19 != 4) {
			v3 = 0;
		}
		p_mtc_audio->gap1[3] = -1;
		SetbtMicControl(v3);
		goto free_work;
	}
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
