

#include <linux/delay.h>
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
	struct mutex mutex;	 /* @4: binaRE probe: _mutex_init(v3 + 4) */
	char _gap28;		 /* @28: binaRE: аллока 32B (probe); байт не используется */
	char tef_cur_band;	 /* @29: binaRE *(u8 *)(tef_drv + 29): текущий FM-банд (1/2), 0xFF — off (Radio_Power) */
	char _gap30[2];	 /* @30..31: binaRE: хвост 32B-аллока */
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
	char rds_pti[4];              /* was _gap8[4]: RDS PTY/PI/TAC -> packet 0x98 (semantic-only) */
	char rds_psn_buf[1116];     /* was mem1[1116]: RDS PS buffer [156/157]=PI/PTY?, [162..]=PS-текст (semantic-only) */
	char mem2[5192];
};

struct __attribute__((aligned(4))) mtc_radio_struct {
	char sta_valid[4];           /* was _gap0[4]: [0]=флаг STA-валидности (semantic-only) */
	int power;
	struct workqueue_struct *radio_wq;
	char customer_rds_bits[4];   /* was _gap1[4]: per-customer RDS-константы (semantic-only) */
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
	char sta_rds_cache[12];      /* was _gap5[12]: [0]=STA flag, [4..7]=кэш RDS 0x98 (semantic-only) */
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

/* binaRE recon round3: local prototypes */
/* (ta_check_start/ta_check_back — прототипы уже в mtc_shared.h, не дублируем) */
static void radio_cmd_work(struct work_struct *work);   /* binaRE @0xc08376c8 */
int ta_back(int a1, int a2);			    /* binaRE @0xc0836e00 (определения в дереве нет) */
void rds_process(void);				    /* binaRE @0xc083722c (определения в дереве нет) */
void rds_send_sta(void);				    /* binaRE @0xc0835a4c (def: этот файл) */
int vs_send(int channel, int cmd, char *buf, int len);  /* binaRE @0xc082ced8 */
void audio_add_work(unsigned int cmd1, int cmd2, int cmd3, int val1); /* binaRE @0xc08355ec (def: mtc-audio.c) */
int Hit_signal8(unsigned int a1, unsigned int a2, unsigned int a3, int a4,
		unsigned char a5, unsigned char a6, char a7, char a8); /* binaRE @0xc0835f28 */
unsigned int Hit_signal(unsigned int a1, unsigned int a2,
			unsigned int a3, unsigned int a4); /* binaRE @0xc08360f4 */
unsigned int Tef_signal(unsigned int a1, unsigned int a2, char a3); /* binaRE @0xc0835d34 */
int SI4754_signal(int a1, unsigned int a2, unsigned int a3, unsigned int a4); /* binaRE @0xc0835df8 */
int status_check(unsigned int a1, int a2);		    /* binaRE @0xc0836998 */

/* binaRE 0xc08376c8 (radio_cmd_work): вспомогательный RDS-блок @0xC168E0C0 — бинар пишет сюда
 * вместо основного блока (audio+0x1C) при активном TA-флаге (E303) или AF (E301).
 * В кодах трогаются 3 поля: u16 @+0x00, u8 @+0x0C, u8 @+0xAC. */
struct mtc_rds_alt_block {
	u16 psn_pair;	 /* @+0x00: binaRE *(u16 *)dest = значение 0xC168E314 */
	char _pad_2[10];
	u8 rds_pti;	 /* @+0x0C: binaRE *(u8 *)(dest + 12) = (word >> 5) & 0x1F */
	char _pad_d[159];
	u8 rds_ps_bit; /* @+0xAC: binaRE *(u8 *)(dest + 172) = (word & 0x400) != 0 */
};
static struct mtc_rds_alt_block rds_alt_block;

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

	radio.sta_rds_cache[0] = 0;
	radio.sta_valid[0] = 0;

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
	cmd_data[2] = radio._s2.rds_pti[0];
	cmd_data[3] = radio._s2.rds_psn_buf[156];
	cmd_data[1] = radio._s2.psn_data[0];
	cmd_data[4] = radio._s2.rds_psn_buf[157];

	if (!memcmp(cmd_data, &radio.sta_rds_cache[4], 4u)) {
		vs_send(2, 0x98u, cmd_data, 5);
	} else {
		memcpy(&radio.sta_rds_cache[4], cmd_data, 4u);
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
	radio._gap11[1] = 0; /* was _gap9[9] — OOB: реальный байт +9 = _gap11[1] (naming_report2 OOB-①) */

	if (a2) {
		count = 2;
	}

	radio._gap11[2] = 0; /* was _gap9[10] — OOB: реальный байт +10 = _gap11[2] (naming_report2 OOB-①) */

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

	car_status.radio_rds_flag = 0;
	car_status.reserved_18 = 0;
}

/* binaRE ta_check_start @0xc08366cc (IDA 9.3 decompiled) */
void
ta_check_start(void)
{
	char buf[5];

	buf[0] = 1;
	if (radio.has_ta_work) /* binaRE 0xC168E303: TA-флаг */
		vs_send(2, MTC_VS_CMD_TA_CHECK, buf, 1);
}

/* binaRE ta_check_back @0xc0836708 (IDA 9.3 decompiled) */
void
ta_check_back(void)
{
	char buf[5];

	buf[0] = 0;
	if (radio.has_ta_work) /* binaRE 0xC168E303: TA-флаг */
		vs_send(2, MTC_VS_CMD_TA_CHECK, buf, 1);
}

/* binaRE Radio_TA @0xc0836f9c (IDA 9.3 decompiled) */
int
Radio_TA(int enable, int a2)
{
	if (car_struct.config_data.d.cfg_rds) { /* binaRE 0xC168AD54: RDS-enable */
		radio._gap11[2] = enable;	    /* binaRE 0xC168E30F: TA-состояние */
		if (!enable && radio.has_ta_work) /* binaRE 0xC168E303 */
			return ta_back(0, a2);
	} else {
		radio._gap11[2] = 0;
	}

	return enable;
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
	len = strlen(&radio._s2.rds_psn_buf[162]);
	memcpy(&cmd_data[2], &radio._s2.rds_psn_buf[162], len + 1);
	// зачем два раза????
	vs_len = strlen(&radio._s2.rds_psn_buf[162]);
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

/* binaRE Radio_Set_Frequency @0xc0837028 (IDA 9.3 decompiled) */
int
Radio_Set_Frequency(int freq, int with_data) /* a1 — частота; a2 — флаг данных для Radio_Set_Frequency_com (0x9208 vs 0x9200) */
{
	char ta_buf[5];

	if (car_struct.config_data.d.cfg_rds) { /* binaRE 0xC168AD54: RDS-enable */
		int ta_was;

		radio.has_af_work = 0; /* binaRE 0xC168E301: сброс AF-индекса при смене частоты */
		if (del_timer_sync(&radio.dwork_af.timer)) /* binaRE 0xC168E244 */
			clear_bit(0, (unsigned long *)&radio.dwork_signal_low.timer + 4); /* binaRE clear_bit(0, 0xC168E234) = &timer+28 */
		flush_workqueue(radio.af_wq); /* binaRE 0xC168E204 = af_wq (probe) */
		ta_was = radio.has_ta_work; /* binaRE 0xC168E303: вычитывается до сброса */
		ta_buf[0] = 0;
		radio.has_ta_work = 0;
		if (ta_was == 1) {
			if (del_timer_sync(&radio.dwork_ta.timer)) /* binaRE 0xC168E270 */
				clear_bit(0, (unsigned long *)&radio.dwork_af.timer + 4); /* binaRE clear_bit(0, 0xC168E260) = &timer+28 */
			vs_send(2, MTC_VS_CMD_TA_CHECK, ta_buf, 1);
		} else if (del_timer_sync(&radio.dwork_ta.timer)) {
			clear_bit(0, (unsigned long *)&radio.dwork_af.timer + 4); /* binaRE clear_bit(0, 0xC168E260) */
		}
	}

	radio.radio_freq = freq; /* binaRE 0xC168E2FC */
	radio._gap7[1] = 0;      /* binaRE 0xC168E300 */
	radio._gap12[0] = 0;     /* binaRE 0xC168E312 */
	printk("--mtc freq set %d\n", freq);
	{
		unsigned char band = radio.tef_drv->tef_cur_band; /* binaRE *(u8 *)(tef_drv + 29) */
		unsigned char new_band = (freq > 10000000) ? 2 : 1;

		if (band == new_band) {
			Radio_Set_Frequency_com(freq, with_data);
			memset(&radio._s2, 0, sizeof(radio._s2)); /* binaRE _memzero(0xC168C80C, 6324) */
			return 0; /* decompiled: return _memzero(...) — IDA-артефакт (заполнитель возвращает указатель) */
		}

		if (band != 255 || car_struct.config_data.d.cfg_radio == 2 || /* binaRE 0xC168AD46: radio-vendor */
		    car_struct.config_data.d.cfg_radio == 4 ||
		    car_struct.config_data.d.cfg_radio == 5) {
			audio_add_work(19, 1, 0, 0);
			msleep(car_struct.config_data.d.cfg_radio == 5 ? 100 : 25);
			Radio_Set_Mute(1);
			Radio_Set_Frequency_com(freq, with_data);
			memset(&radio._s2, 0, sizeof(radio._s2));
			if (car_struct.config_data.d.cfg_radio == 2 ||
			    car_struct.config_data.d.cfg_radio == 4) {
				msleep(250);
			} else {
				msleep(car_struct.config_data.d.cfg_radio == 5 ? 500 : 50);
			}
		} else {
			Radio_Set_Frequency_com(freq, with_data);
			memset(&radio._s2, 0, sizeof(radio._s2));
		}
		radio.tef_drv->tef_cur_band = new_band; /* binaRE *(u8 *)(tef_drv + 29) = v5 */
	}

	return 0;
}

/* binaRE Radio_Power @0xc0837f78 (IDA 9.3 decompiled) */
int
Radio_Power(int power)
{
	if (power == radio.power) /* binaRE 0xC168C7FC */
		return -1;

	radio._gap15[0] = -96; /* binaRE 0xC168E328: константа 0xA0 */

	if (power) {
		radio.tef_drv->tef_cur_band = -1; /* binaRE *(u8 *)(tef_drv + 29) = 0xFF */
		return arm_send_multi(37382, 0, 0); /* 0x9226: radio on */
	}

	arm_send_multi(37383, 0, 0); /* 0x9227: radio off */
	radio.power = power;	    /* binaRE 0xC168C7FC */

	return power;
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

/* binaRE Radio_Get_Signal @0xc0837de0 (IDA 9.3 decompiled) */
int
Radio_Get_Signal(void)
{
	unsigned char buf[8];

	if (car_struct.config_data.d.cfg_radio == 2) { /* binaRE 0xC168AD46: radio-vendor */
		int level;

		arm_send_multi(4622, 8, buf); /* 0x1216 */
		level = Hit_signal8(buf[0], buf[1], buf[2], buf[3],
				    buf[4], buf[5], buf[6], buf[7]);
		/* порядок аргументов printk — 1-в-1 из decompiled (v11,v4,v8,v9,v5,v6,v7,v10) */
		printk("--mtc status chk sk:%02x, sm:%02x, st:%02x, qty:%02x, dt:%02x, aj:%02x, mp:%02x, dv:%02x  (%d)\n",
		       buf[7], buf[0], buf[4], buf[5], buf[1], buf[2], buf[3], buf[6], level);
		if (!level)
			return 0;
		return level;
	}

	arm_send_multi(4611, 5, buf); /* 0x1203 */

	if (car_struct.config_data.d.cfg_radio == 3) {
		/* decompiled-ветка (v3 = v4; if (v4 & 0x80) v3 = 0; freq-логика) 1-в-1 = тело TEF6686_signal() */
		return TEF6686_signal(buf[0]);
	}
	if (car_struct.config_data.d.cfg_radio == 2 || car_struct.config_data.d.cfg_radio == 4) {
		return Hit_signal(buf[0], buf[1], buf[2], buf[3]);
	}
	if (car_struct.config_data.d.cfg_radio == 5) {
		return SI4754_signal(buf[0], buf[1], buf[2], buf[3]);
	}

	return Tef_signal(buf[1], buf[2], buf[3]);
}

/* binaRE radio_cmd_work @0xc08376c8 (IDA 9.3 decompiled): общий проход по RDS-блоку
 * (LABEL_91/54/57/96/98 декомпиляции) — вызывается и из цикла case 1, и после CRC-расчёта. */
static void
radio_rds_pass(unsigned char *v12p, unsigned int v1, unsigned int bword)
{
	bool above = v1 > 0x3CC;

	if (v1 == 972) { /* LABEL_91: блок 0x3CC */
		if (radio.sta_rds_cache[0] == 2) { /* binaRE 0xC168E2E4: RDS-состояние */
			radio.sta_rds_cache[0] = 3;
			*(u16 *)&radio._gap13[2] = (u16)bword; /* binaRE 0xC168E318 */
			goto pass96;
		}
		goto pass98;
	}

	if (above) { /* LABEL_54: блоки > 0x3CC */
		if (v1 == 980) { /* 0x3D4 */
			u8 ps_bit;

			if (radio.sta_rds_cache[0] != 1)
				goto pass98;
			*(u16 *)&radio._gap13[0] = (u16)bword; /* binaRE 0xC168E316 */
			radio.sta_rds_cache[0] = 2;
			*(u16 *)&radio._s2.psn_data[0] = (u16)radio.u16_2; /* binaRE 0xC168C80C := 0xC168E314 (rds_psn, B:0x1C/0x1D) */
			ps_bit = ((u16)bword >> 10) & 1;
			radio._s2.rds_psn_buf[156] = ((u16)bword & 0x400) != 0; /* binaRE 0xC168C8B8 */
			radio._s2.rds_pti[0] = ((u16)bword >> 5) & 0x1F;	 /* binaRE 0xC168C818 */
			if (radio.has_ta_work != 1 || /* binaRE 0xC168E303 */
			    (*(u16 *)&radio._s2.psn_data[0] == (u16)radio.u16_1) && ps_bit) { /* binaRE 0xC168E310 */
				rds_send_sta();
			} else {
				ta_back(*(u16 *)&radio._s2.psn_data[0], ps_bit);
			}
			/* decompiled: LOBYTE(v12) = MEMORY[E31D] — синхронизация no-op (E31D == *v12p) */
		} else if (v1 != 984) { /* 0x3D8 */
			goto pass57;
		} else {
			radio.u16_2 = (u16)bword; /* binaRE 0xC168E314 */
			radio.sta_rds_cache[0] = 1;
		}
		goto pass96;
	}

	if (v1 == 600) { /* 0x258 */
		if (radio.sta_rds_cache[0] != 3)
			goto pass98;
		*(u16 *)&radio._gap13[4] = (u16)bword; /* binaRE 0xC168E31A */
		radio.sta_rds_cache[0] = 0;
		rds_process();
		goto pass96;
	}
	if (v1 == 604) { /* 0x25C */
		if (radio.sta_rds_cache[0] == 2) {
			radio.sta_rds_cache[0] = 3;
			*(u16 *)&radio._gap13[2] = (u16)bword; /* binaRE 0xC168E318 */
			goto pass96;
		}
	}

pass57: /* LABEL_57 */
pass98: /* LABEL_98 */
	radio.sta_rds_cache[0] = 0;
	*v12p = (u8)(*v12p - 1);
	radio._gap13[7] = *v12p;		 /* binaRE 0xC168E31D */
	*(u64 *)&radio._gap14[0] *= 2;	 /* binaRE 0xC168E320 (u64-аккумулятор) */
	return;

pass96: /* LABEL_96 */
	*v12p = (u8)(*v12p - 26);
	radio._gap13[7] = *v12p;		 /* binaRE 0xC168E31D */
	*(u64 *)&radio._gap14[0] <<= 26; /* binaRE 0xC168E320 */
}

/* binaRE radio_cmd_work @0xc08376c8 (IDA 9.3 decompiled) */
static void
radio_cmd_work(struct work_struct *work)
{
	struct mtc_radio_work *rw = container_of(from_delayed_work(work),
						  struct mtc_radio_work, radio_work);
	unsigned int data = rw->dword4; /* binaRE *(u32 *)(a1 - 20) */
	unsigned int sel = rw->dword8;	 /* binaRE *(u32 *)(a1 - 16) */

	switch (rw->cmd) {
	case 1: /* RDS-поток: customer 2/4 — побайтовый RRB-парсер, иначе CRC-проход */
	{
		bool cust24;
		unsigned char v12;

		if (!radio.sta_valid[0]) /* binaRE 0xC168C7F8: STA-valid (work_sta_valid/fn_sta_invalid) */
			break;

		cust24 = (car_struct.config_data.d.cfg_radio == 2 ||
			  car_struct.config_data.d.cfg_radio == 4); /* binaRE 0xC168AD46 */

		{
			/* binaRE: v11 = u64-rol(data, 32 - shift); при shift==0: v11 = (u64)data << 32 */
			u8 shift = radio._gap13[7]; /* binaRE 0xC168E31D */
			u64 bits;

			if (shift)
				bits = ((u64)(data >> shift) << 32) | (u32)(data << (32 - shift));
			else
				bits = (u64)data << 32;
			v12 = (u8)(shift + 32);
			radio._gap13[7] = v12; /* binaRE E31D += 32 */
			*(u64 *)&radio._gap14[0] |= bits; /* binaRE 0xC168E320 = v11 | E320 */
			if (cust24) {
				v12 = 26;
				radio._gap13[7] = v12; /* binaRE: if (cust24) E31D = 26 */
			}
			/* decompiled: off_C0BCA0AC = v10 — артефакт «write access to const memory»
			 * (v10 = адрес основного/вспомогательного RDS-блока; далее в коде не используется) */
		}

		for (;;) {
			unsigned int v1, bword;

			if (v12 <= 0x19) {
				kfree(rw); /* decompiled: return kzfree(v2) */
				return;
			}

			if (cust24) {
				unsigned char grp = (data >> 16) & 0xFF; /* binaRE v24 = BYTE2(v9) */

				if (grp) {
					/* decompiled: switch без default — IDA-потеря ветки
					 * (в asm jumptable C0837278 есть default-путь) — 1-в-1 декомпиляции */
					switch (grp) {
					case 1: v1 = 980; break; /* 0x3D4 */
					case 2: v1 = 604; break; /* 0x25C */
					case 3: v1 = 600; break; /* 0x258 */
					}
				} else {
					v1 = 984; /* 0x3D8 */
				}
				bword = (u16)data; /* binaRE v17 = HIWORD(v9 << 16) */
			} else {
				/* binaRE: CRC-расчёт — 16 XOR по битам 0xC168E324 */
				unsigned int crc_src = *(u32 *)&radio._gap14[4]; /* binaRE 0xC168E324 */

				v1 = crc_src >> 22;
				if (crc_src & 0x40)	v1 ^= 0x31B;
				if (crc_src & 0x80)	v1 ^= 0x38F;
				if (crc_src & 0x100)	v1 ^= 0x2A7;
				if (crc_src & 0x200)	v1 ^= 0xF7;
				if (crc_src & 0x400)	v1 ^= 0x1EE;
				if (crc_src & 0x800)	v1 ^= 0x3DC;
				if (crc_src & 0x1000)	v1 ^= 0x201;
				if (crc_src & 0x2000)	v1 ^= 0x1BB;
				if (crc_src & 0x4000)	v1 ^= 0x376;
				if (crc_src & 0x8000)	v1 ^= 0x355;
				if (crc_src & 0x10000)	v1 ^= 0x313;
				if (crc_src & 0x20000)	v1 ^= 0x39F;
				if (crc_src & 0x40000)	v1 ^= 0x287;
				if (crc_src & 0x80000)	v1 ^= 0xB7;
				if (crc_src & 0x100000)	v1 ^= 0x16E;
				if (crc_src & 0x200000)	v1 ^= 0x2DC;
				bword = radio._gap14[6]; /* binaRE v17 = 0xC168E326 (8 бит в этом проходе) */
			}

			radio_rds_pass(&v12, v1, bword); /* LABEL_91/54/57/96/98 */
		}
	}
	break;
	case 2: /* RDS-группа одним словом (rds_input2) */
	{
		u16 word = (u16)data; /* binaRE v7 = (u16)v9 */
		u8 grp = (data >> 16) & 0xFF; /* binaRE BYTE2(v7) */
		char *dest = (radio.has_ta_work || radio.has_af_work) ? /* binaRE E303/E301 */
			(char *)&rds_alt_block : /* 0xC168E0C0 */
			(char *)&radio._s2;	 /* 0xC168C80C (основной RDS-блок) */

		if (radio.sta_valid[0]) { /* binaRE 0xC168C7F8 */
			if (grp) {
				switch (grp) {
				case 1:
					if (radio.sta_rds_cache[0] != 1)
						goto case2_reset;
					{
						u16 psn_old = radio.u16_2; /* binaRE v20 = E314 (до записи) */
						u8 ps_bit = ((u16)word >> 10) & 1;

						*(u16 *)&radio._gap13[0] = word; /* binaRE 0xC168E316 */
						*(u16 *)dest = radio.u16_2;	 /* binaRE *(u16 *)v8 = E314 */
						radio.sta_rds_cache[0] = 2;
						*((u8 *)dest + 12) = ((u16)word >> 5) & 0x1F;
						*((u8 *)dest + 172) = ((u16)word & 0x400) != 0;
						if (radio.has_ta_work != 1 || /* binaRE E303 == 1 */
						    (radio.u16_1 == psn_old) && ps_bit) { /* binaRE E310 == v20 */
							rds_send_sta();
						} else {
							/* asm C0837C3C: LDRH R0 (PSN-слово из dest); decompiled не разрешил
							 * аргумент — значение = только что записанный PSN-слово */
							ta_back(radio.u16_2, ps_bit);
						}
					}
					break;
				case 2:
					if (radio.sta_rds_cache[0] != 2)
						goto case2_reset;
					radio.sta_rds_cache[0] = 3;
					*(u16 *)&radio._gap13[2] = word; /* binaRE 0xC168E318 */
					break;
				case 3:
					if (radio.sta_rds_cache[0] != 3)
						goto case2_reset;
					radio.sta_rds_cache[0] = 0;
					*(u16 *)&radio._gap13[4] = word; /* binaRE 0xC168E31A */
					rds_process();
					break;
				default:
					radio.sta_rds_cache[0] = 0; /* binaRE LABEL_15: E2E4 = 0 */
					break;
				}
			} else {
				radio.sta_rds_cache[0] = 1;
				radio.u16_2 = word; /* binaRE 0xC168E314 */
			}
		}

	case2_reset: /* binaRE LABEL_26: E2E4 = 0; (дальше общий кfree) */
		radio.sta_rds_cache[0] = 0;
		break;
	}
	case 3: /* RDS-группа с селектором (rds_input3A/rds_input3) */
	{
		u16 word = (u16)data; /* binaRE v4 = (u16)v9 */
		u8 s = sel & 0xFF;    /* binaRE v5: 0..3 */
		char *dest = (radio.has_ta_work || radio.has_af_work) ? /* binaRE E303/E301 */
			(char *)&rds_alt_block : /* 0xC168E0C0 */
			(char *)&radio._s2;	 /* 0xC168C80C */

		if (radio.sta_valid[0]) { /* binaRE 0xC168C7F8 */
			if (s) {
				if (s != 1) {
					if (s != 2) {
						if (s != 3)
							goto case3_reset; /* binaRE LABEL_15 */
						if (radio.sta_rds_cache[0] == 3) {
							radio.sta_rds_cache[0] = 0;
							*(u16 *)&radio._gap13[4] = word; /* binaRE 0xC168E31A */
							rds_process();
							break;
						}
						goto case3_reset;
					}
					if (radio.sta_rds_cache[0] == 2) {
						radio.sta_rds_cache[0] = 3;
						*(u16 *)&radio._gap13[2] = word; /* binaRE 0xC168E318 */
						break;
					}
					goto case3_reset; /* binaRE LABEL_26 */
				}
				if (radio.sta_rds_cache[0] != 1)
					goto case3_reset;
				{
					u16 psn_old = radio.u16_2; /* binaRE v22 = E314 (до записи) */
					u8 ps_bit = ((u16)word >> 10) & 1; /* binaRE v7 */

					*(u16 *)&radio._gap13[0] = word; /* binaRE 0xC168E316 */
					*(u16 *)dest = radio.u16_2;	 /* binaRE *(u16 *)v6 = E314 */
					radio.sta_rds_cache[0] = 2;
					*((u8 *)dest + 12) = ((u16)word >> 5) & 0x1F;
					*((u8 *)dest + 172) = ((u16)word & 0x400) != 0;
					if (radio.has_ta_work != 1 || /* binaRE E303 == 1 */
					    (radio.u16_1 == psn_old) && ps_bit) { /* binaRE E310 == v22 */
						rds_send_sta();
					} else {
						/* decompiled: v21 = -1242 — неразрешённый регистр (артефакт);
						 * по аналогии с case 1/2 аргумент = только что записанный PSN-слово */
						ta_back(radio.u16_2, ps_bit);
					}
				}
				break;
			} else {
				radio.sta_rds_cache[0] = 1;
				radio.u16_2 = word; /* binaRE 0xC168E314 */
			}
		}

	case3_reset: /* binaRE LABEL_26/LABEL_15: E2E4 = 0 */
		radio.sta_rds_cache[0] = 0;
		break;
	}
	case 4: /* status-запрос (rds_input4/ctl) */
		if (radio.sta_valid[0]) /* binaRE 0xC168C7F8 */
			status_check(data & 0xFF, sel & 0xFF); /* binaRE *(u8 *)(a1 - 20), *(u8 *)(a1 - 16) */
		break;
	default:
		break;
	}

	kfree(rw); /* decompiled: kzfree(v2) на всех путях */
}

/* contains unknown fields */
static int
tef6606_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mtc_tef6606_drv *tef_drv;
	signed int result;

	if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_YZ_RM_ZT) {
		radio.customer_rds_bits[0] = 0x1F;
	} else if (car_struct.car_status.mtc_customer == MTC_CUSTOMER_MX) {
		radio.customer_rds_bits[1] = 0xF7u;
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
