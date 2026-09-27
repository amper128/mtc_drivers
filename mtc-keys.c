#include <asm-generic/gpio.h>
#include <asm/errno.h>
#include <linux/adc.h>
#include <linux/earlysuspend.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include "mtc-car.h"

static unsigned int mtc_keycodes[] = {
    KEY_NUMERIC_0,
    KEY_BACK,
    KEY_NUMERIC_2,
    KEY_NUMERIC_3,
    KEY_NUMERIC_4,
    KEY_NUMERIC_5,
    KEY_NUMERIC_6,
    KEY_NUMERIC_7,
    KEY_NUMERIC_8,
    KEY_NUMERIC_9,
    KEY_NUMERIC_STAR,
    KEY_NUMERIC_POUND,
    0x20E,
    0x20F,
    KEY_CAMERA_FOCUS,
    KEY_TOUCHPAD_ON,
    KEY_HOME,
    KEY_MENU,
    KEY_VOLUMEDOWN,
    KEY_VOLUMEUP,
    KEY_TOUCHPAD_OFF,
    KEY_CAMERA_ZOOMIN,
    KEY_CAMERA_ZOOMOUT,
    KEY_CAMERA_UP,
    KEY_CAMERA_DOWN,
};

static struct early_suspend mtc_keys_early_suspend;

/* reconstructed from code usage; layout TBD (порядок = первое использование:
 * wheel_wq :159, volkey_work :162, modemute_work :163; binaRE: keys_ws @keys_data+0x10,
 * wheel_wq@ws+0, volkey_work.func@ws+0x10 — decompiled_keys_probe) */
struct mtc_keys_work_struct {
	struct workqueue_struct *wheel_wq;
	struct work_struct volkey_work;
	struct work_struct modemute_work;
};

struct mtc_keys_data {
	struct workqueue_struct *process_wq;
	char no_adc_ch1;
	struct input_dev *p_input_dev;
	int intval1;
	struct mtc_keys_work_struct keys_ws;
	struct mtc_keys_drv *keys_dev;
	char flag1;
	char flag2;
	char flag3;
	char home_disabled;
	int modemuteval;
	int ir_val;
};

static struct mtc_keys_data *keys_data;

static struct platform_driver mtc_keys_driver;
static struct mtc_keys_input_id *mtc_keys_devices;
static int input_dev_count = 7;
static int enable_key_repeat;

struct mtc_keys_input_dev {
	char _gap0[8];
	void *callback_param;
	char _gap1[12];
	struct timer_list ir_timer;
	struct timer_list mcu_timer;
	char wheel_adc_init[16];   /* was _gap4[16]: [0]=SET_CONST 1023 (10-bit ADC max) до setup timer/hrtimer — ADC-ветки dev_type 4/5 (naming_report2, semantic-only) */
	struct adc_client *wheel_adc_client;
	struct adc_client *wheel_adc_client_ch2;
	char _gap5[4];
	unsigned int irq;
	char _gap51[16];
	struct hrtimer adc_wheel_hrtimer;
	char _gap7[68];
	struct tasklet_struct ir_tasklet;
};

struct mtc_keys_input_id {
	int dev_type;
	unsigned int keycode1;
	unsigned int keycode2;
	int gpio1;
	int gpio2;
	int trig_type;
	int adc_ch1;
	int adc_ch2;
	const char *dev_name;
	int wakeup;
};

struct mtc_keys_drv {
	int input_dev_count;
	struct input_dev *keys_input0;
	struct mtc_keys_input_dev keys_input[7];
};

static struct mtc_keys_drv *keys_dev;

/* ==========================================================================
 * binaRE recon round3 — 9 keys-функций (адреса сверены с kallsyms)
 *   key_enter_mode      @ 0xc083bebc  static (вызов из mtc-car.c — implicit decl; mtc-car.c/mtc_shared.h НЕ тронуты)
 *   key_beep            @ 0xc083bfbc  static
 *   send_ir_key         @ 0xc083c0d4  static (149L; полный декод по disasm C083C0D4-C083C44C)
 *   send_event_key      @ 0xc083ce74  static
 *   mtc_key_suspend     @ 0xc083cfb8  НЕ static (earlysuspend из mtc-car.c)
 *   mtc_key_resume      @ 0xc083d064  НЕ static (earlysuspend из mtc-car.c)
 *   mtc_getTouchKey     @ 0xc083ddbc  НЕ static
 *   mtc_getTouchKey_tab @ 0xc083de4c  НЕ static
 *   mtc_touch_work_func @ 0xc083e1a0  static
 * Таблица адресов (все MEMORY[0x...] из decompiled → offset; выверено скриптом):
 *   car_status  = 0xC168AC84: +1 power_refcnt, +3 call_active, +4 rpt_boot_android,
 *     +5 input_ready, +7 touch_type, +0x21 backlight_status, +0x22 cam_state,
 *     +0x29 ch_mode, +0x2A key_mode, +0x5B video_src_ready, +0x5C ajx_active,
 *     +0x65 (=0xC168ACE9) touch-таблица 10×{b0,b1,lo,hi} в .bss, +0x92 mtc_customer
 *   config_data = 0xC168AD40: +0x17 d.cfg_frontview, +0x1E4 (484) d.ctl_beep
 *   keys_data   = 0xC168E474: +0x08 p_input_dev, +0x10 keys_ws.wheel_wq,
 *     +0x6C entries P (kmalloc n*264+16 в keys_probe: n*0x108+0x10; P[0]=count;
 *     entry_i = P+16+264*i: id-указатель @entry+0x08, timer3 @entry+0x10,
 *     timer4 @entry+0x2C, irq @entry+0x64, cookie=entry),
 *     +0x78 backlight-переключатель (полей нет), +0x8C/+0x8D touch-результат (полей нет),
 *     +0x94 touch-гейт, +0x98 counter, +0x9C key, +0x9D code2, +0xA0 dwork (полей нет)
 * ========================================================================== */
static char key_enter_mode(char result); /* вызов из mtc-car.c (implicit decl); binary 0xc083bebc */
static int key_beep(void);
static int send_ir_key(int result);
static int send_event_key(unsigned int a1);
static void mtc_touch_work_func(void);

/* binaRE round3 externs. vs_send/audio_add_work/capture_add_work объявлены под ТИПЫ БИНАРЯ
 * (R0 используется вызывающим) — расхождение с void-определениями дерева помечено;
 * backlight_on/off, isKeyDisable, isAudioKeyEnable — определений в дереве нет. */
extern int vs_send(int port_num, unsigned char cmd, char *cmd_data, int count); /* дерево mtc-vs.c:368: void — расхождение */
extern int audio_add_work(unsigned int cmd1, int cmd2, int cmd3, int val1); /* дерево mtc-audio.c:1644: void — расхождение */
extern int capture_add_work(unsigned int cmd1, int cmd2, unsigned int delay, int flush); /* дерево mtc-backview.c:556: void — расхождение */
extern void backlight_on(void); /* определения нет в дереве; бинар: арг не нужен (IDA — stale R0) */
extern void backlight_off(void); /* то же */
extern int isKeyDisable(void); /* определения нет в дереве (call C083C174) */
extern int isAudioKeyEnable(void); /* определения нет в дереве (call C083C25C) */
extern int arm_send_multi(unsigned int cmd, int count, unsigned char *buf); /* = mtc_shared.h:251 (совместимое переобъявление) */
extern unsigned char dword_C0BCA228[]; /* бинарные данные: таблица тона key_beep */

/* key_enter_mode @ 0xc083bebc */
static char
key_enter_mode(char result)
{
	car_struct.car_status.key_mode = result; /* binaRE: STRB R0, [car_status+0x2A] = key_mode (0xC168ACAE) */
	return result;
}

/* key_beep @ 0xc083bfbc */
static int
key_beep(void)
{
	if (car_struct.config_data.d.ctl_beep) /* binaRE 0xC168AF24 = config_data+0x1E4 (484) = d.ctl_beep */
		return arm_send_multi(38176, 1, dword_C0BCA228); /* 0x9520 (asm MOVW R0,#0x9520); tail B arm_send_multi */

	return 0; /* return-нормализация: бинар BXEQ LR с РАЗНЕИНИЦИАЛИЗОВАННЫМ R0 (мусор) */
}

/* send_ir_key @ 0xc083c0d4 (149L) — полный декод по disasm C083C0D4-C083C44C;
 * result/R0 = исходный key; v1/R4 = key после ремапа ('O' → 'E'/'D') */
static int
send_ir_key(int result)
{
	int v1; /* decompiled v1 (R4): key после ремапа */
	int v4; /* decompiled v4 (R6): старое keys_data+0x78; при использовании гарантированно 0 (ненулевое → обнулено и return) */
	char v12[2]; /* decompiled v12[2] (asm: 1 байт в [SP+2]); vs_send(..., len=1) */

	v1 = result;
	if (car_struct.car_status.ch_mode) { /* binaRE 0xC168ACAD = car_status+0x29 ch_mode */
		if (result == 54 || result == 56) /* '6'/'8' (asm C083C0F0-C083C0F8) */
			return result;
	}
	switch (result) {
	case '@': /* 0x40 (asm C083C26C) */
		car_struct.car_status.video_src_ready = 1; /* binaRE 0xC168ACDF = +0x5B */
		result = key_beep();
		if (car_struct.car_status.rpt_boot_android) /* +4 */
			result = audio_add_work(22, 0, 0, 0); /* asm: R0=0x16 */
		if (car_struct.car_status.call_active) /* +3 */
			return vs_send(2, 159, (char *)0, 0); /* asm: cmd 0x9F, buf NULL, len 0 */
		return result;
	case 'A': /* 0x41 (asm C083C2BC) */
		car_struct.car_status.video_src_ready = 0; /* +0x5B */
		result = key_beep();
		if (car_struct.car_status.rpt_boot_android) { /* +4 */
			if (car_struct.car_status.ajx_active && /* binaRE 0xC168ACE0 = +0x5C */
			    car_struct.car_status.power_refcnt == 1) /* +1; asm CMP #1 — decompiled «==1» НЕ перевёрнут */
				backlight_off(); /* IDA: backlight_off(result) — артефакт (arg не используется; дерево: void) */
			result = audio_add_work(23, 0, 0, 0); /* asm: R0=0x17 */
		}
		if (car_struct.car_status.call_active) /* +3 */
			return vs_send(2, 160, (char *)0, 0); /* asm: cmd 0xA0 */
		return result;
	case 'O': /* 0x4F (asm C083C22C) */
		if ((unsigned char)(car_struct.car_status.ch_mode - 2) <= 1)
			v1 = 69; /* 'E' (asm: UXTB(ch_mode-2)<=1 → MOVLS #'E') */
		else
			v1 = 68; /* 'D' (asm MOVHI #'D') */
		if (!car_struct.car_status.input_ready) /* +5 */
			return result;
		break; /* → общий поток (asm C083C248: B C083C120) */
	default: /* asm C083C114 */
		if (!car_struct.car_status.input_ready) /* +5 */
			return result;
		break; /* → asm C083C120 */
	}

	if (v1 == 66 && /* 'B' (asm C083C120 / C083C2A8) */
	    (unsigned char)car_struct.car_status.touch_type != 129) /* binaRE 0xC168AC8B = +7; asm LDRB+CMP #0x81 — unsigned */
		return result;
	if (car_struct.car_status.mtc_customer == 18) { /* binaRE 0xC168AD16 = +0x92 */
		if (v1 != 85 || !car_struct.car_status.cam_state) /* 'U' / +0x22 */
			goto kdisable;
		key_beep(); /* LABEL_59 */
		return capture_add_work(66, 0, 0, 0); /* asm: R0=0x42 */
	}
	if (v1 == 54 || v1 == 85) { /* '6'/'U' (asm C083C134-C083C13C) */
		if (car_struct.car_status.cam_state && /* +0x22 */
		    car_struct.config_data.d.cfg_frontview) { /* binaRE 0xC168AD57 = config_data+0x17 = d.cfg_frontview */
			key_beep();
			return capture_add_work(66, 0, 0, 0);
		}
		if (v1 == 85 && car_struct.car_status.cam_state) { /* LABEL_17 (asm C083C160-C083C170) */
			key_beep();
			return capture_add_work(66, 0, 0, 0);
		}
	}
kdisable: /* LABEL_19 (asm C083C174) */
	result = isKeyDisable();
	if (!result)
		goto backlight_flow; /* LABEL_20 */
	if (v1 == 26 || v1 == 18) { /* 0x1A/0x12 (asm C083C250-C083C258) */
		if (isAudioKeyEnable())
			goto backlight_flow; /* asm C083C264: BNE C180 */
	}
	return result;

backlight_flow: /* LABEL_20 (asm C083C180) */
	if (!car_struct.car_status.backlight_status) { /* binaRE 0xC168ACA5 = +0x21 */
		backlight_on(); /* IDA: backlight_on(result) — артефакт (дерево: void; R0 stale) */
		((unsigned char *)keys_data)[0x78] = 1; /* binaRE 0xC168E4EC = keys_data+0x78 (поля в текущем дереве нет) */
		return 0; /* return-нормализация: бинар возвращает результат backlight_on() (дерево: void) */
	}
	v4 = ((unsigned char *)keys_data)[0x78]; /* binaRE 0xC168E4EC = keys_data+0x78 */
	if (((unsigned char *)keys_data)[0x78]) {
		((unsigned char *)keys_data)[0x78] = 0;
		return result;
	}
	if (v1 == 54) { /* '6' (asm C083C3D0): dev = keys_data+0x08 (binaRE 0xC168E47C = p_input_dev) */
		input_event(keys_data->p_input_dev, 1, 158, 1); /* KEY_BACK; asm R2=0x9E */
		input_event(keys_data->p_input_dev, v4, v4, v4); /* IDA честно: (dev,v4,v4,v4); v4==0 гарантированно */
		input_event(keys_data->p_input_dev, 1, 158, v4);
		input_event(keys_data->p_input_dev, v4, v4, v4);
		return key_beep(); /* IDA: key_beep(v10) — артефакт; R0 = key_beep() */
	}
	if (car_struct.car_status.key_mode == 1) /* binaRE 0xC168ACAE = +0x2A (asm C083C1B0-C083C1B8) */
		return result;
	if (v1 == 56) { /* '8' (asm C083C40C) */
		v12[0] = 56;
		vs_send(2, 0x8E, v12, 1);
		vs_send(2, 0x8F, v12, 1);
		return key_beep();
	}
	if (v1 != 55) { /* не '7' (asm C083C1CC) */
		v12[0] = v1;
		vs_send(2, 0x8E, v12, 1);
		result = vs_send(2, 0x8F, v12, 1);
		if (!(v1 == 26 || v1 == 18) ||
		    car_struct.car_status.mtc_customer == 7 ||
		    car_struct.car_status.mtc_customer == 13 ||
		    car_struct.car_status.mtc_customer == 15) /* asm C083C208-C083C220 */
			return key_beep(); /* IDA: key_beep(result) — артефакт */
		return result; /* = результат vs_send(2,0x8F) (asm → C083C224) */
	}
	input_event(keys_data->p_input_dev, 1, 139, 1); /* '7': KEY_MENU; asm R2=0x8B (C083C344) */
	input_event(keys_data->p_input_dev, v4, v4, v4); /* IDA честно: v4==0 гарантированно */
	input_event(keys_data->p_input_dev, 1, 139, v4);
	input_event(keys_data->p_input_dev, v4, v4, v4);
	return key_beep();
}

/* send_event_key @ 0xc083ce74 — dev = keys_data+0x08 (binaRE 0xC168E47C = p_input_dev) */
static int
send_event_key(unsigned int a1)
{
	input_event(keys_data->p_input_dev, 1u, a1, 1);
	input_event(keys_data->p_input_dev, 0, 0, 0); /* IDA: (dev,0,0,0) — SYN */
	input_event(keys_data->p_input_dev, 1u, a1, 0);
	input_event(keys_data->p_input_dev, 0, 0, 0);
	return key_beep(); /* tail (decompiled: return key_beep()) */
}

/* mtc_key_suspend @ 0xc083cfb8 — НЕ static (earlysuspend из mtc-car.c) */
int
mtc_key_suspend(void)
{
	int *base; /* P = *(keys_data+0x6C): kmalloc(n*264+16) (keys_probe: n*0x108+0x10); P[0]=count; entry_i = P+16+264*i */
	int count;
	int i;

	car_struct.car_status.key_mode = 4; /* asm: STRB #4, [car_status+0x2A] */
	base = (int *)(*(int **)((char *)keys_data + 0x6C)); /* binaRE 0xC168E4E0 = keys_data+0x6C */
	count = base[0];
	for (i = 0; i < count; i++) {
		int *e = base + 4 + 66 * i; /* entry_i: P+16+264*i (asm: ADD R5,R6,R5,LSL#3; ADD #0x10) */
		struct mtc_keys_input_id *id = (struct mtc_keys_input_id *)e[2]; /* e+0x08: указатель id (asm LDR R3,[R5,#8]) */

		switch (id->dev_type) { /* asm: LDR R3,[R3] — 1-е поле id */
		case 3:
			free_irq(e[25], e); /* e+0x64: irq (asm LDR R0,[R5,#0x64]); cookie = e (asm MOV R1,R5) */
			del_timer((struct timer_list *)&e[4]); /* e+0x10 (asm ADD R0,R5,#0x10) */
			break;
		case 4:
			del_timer((struct timer_list *)&e[11]); /* e+0x2C (asm ADD R0,R5,#0x2C) */
			break;
		}
	}
	return flush_workqueue(keys_data->keys_ws.wheel_wq); /* asm tail: LDR R0,[keys_data+0x10] = wheel_wq (0xC168E484) */
}

/* mtc_key_resume @ 0xc083d064 — НЕ static (earlysuspend из mtc-car.c) */
void
mtc_key_resume(void)
{
	int *base; /* P = *(keys_data+0x6C); P[0]=count; entry_i = P+16+264*i */
	int count;
	const char *keys_name = "keys"; /* бинар: LDR R7,=(aMtcKeys_0+4) — строка "mtc-keys", +4 = "keys" */
	int i;

	car_struct.car_status.key_mode = 0; /* asm: STRB #0, [car_status+0x2A] */
	base = (int *)(*(int **)((char *)keys_data + 0x6C)); /* binaRE 0xC168E4E0 = keys_data+0x6C */
	count = base[0];
	for (i = 0; i < count; i++) {
		int *e = base + 4 + 66 * i; /* entry_i: P+16+264*i */
		struct mtc_keys_input_id *id = (struct mtc_keys_input_id *)e[2]; /* e+0x08: указатель id */
		int trig;

		switch (id->dev_type) {
		case 4:
			mod_timer((struct timer_list *)&e[11], /* e+0x2C (asm ADD R0,R5,#0x2C) */
				  jiffies + msecs_to_jiffies(100)); /* asm: BL msecs_to_jiffies(#0x64); LDR R1,[dword_C0B6E080]=jiffies; ADD R1,R0,R1 (decompiled «v6-30000» — артефакт) */
			break;
		case 3:
			trig = id->trig_type; /* id+0x14 (asm LDR R3,[R1,#0x14]) */
			request_threaded_irq(e[25], /* e+0x64: irq (asm LDR R0,[R5,#0x64]) */
					     ir_isr, /* asm LDR R1,=ir_isr */
					     0,
					     trig ? 2 : 1, /* asm: MOVEQ #1 / MOVNE #2 (числовые флаги, не макросы IRQF_*) */
					     id->dev_name ? id->dev_name : keys_name, /* id+0x20 (asm LDR R12,[R1,#0x20]; MOVEQ R12,R7) */
					     e); /* 6-й аргумент на стеке (asm STR R5,[SP,#4]): cookie = e — как в free_irq */
			break;
		}
	}
}

/* mtc_getTouchKey @ 0xc083ddbc — таблица 10×{b0,b1,lo,hi} (u8) в .bss:
 * binaRE 0xC168ACE9 = car_status+0x65 (asm: R1=0xC168AC84, LDRB [R1,#0x65..#0x68], шаг +4;
 * 40 байт выходят за reserved_22[32] (@101..132) до @140) */
int
mtc_getTouchKey(unsigned int a1)
{
	unsigned char *tab;
	unsigned int i;

	tab = (unsigned char *)car_struct.car_status.reserved_22; /* @101 (=0x65) */
	for (i = 0; i < 10; i++) {
		if (!tab[i * 4] && !tab[i * 4 + 1]) /* b0/b1 (asm LDRB [R1,#0x65]/[R1,#0x66]) */
			return 255; /* asm MOV R0,#0xFF */
		if ((unsigned int)tab[i * 4 + 2] <= a1 && (unsigned int)tab[i * 4 + 3] > a1) { /* lo/hi (asm [R1,#0x67]/[R1,#0x68], unsigned) */
			((unsigned char *)keys_data)[0x8C] = tab[i * 4]; /* binaRE 0xC168E500 = keys_data+0x8C (полей в дереве нет) */
			((unsigned char *)keys_data)[0x8D] = tab[i * 4 + 1]; /* binaRE 0xC168E501 = keys_data+0x8D */
			return (unsigned char)(i + 1); /* asm: R0 = UXTB(idx+1) */
		}
	}
	return 255;
}

/* mtc_getTouchKey_tab @ 0xc083de4c — то же, таблица = аргумент a2 (asm: LDRB [R1,R3]) */
int
mtc_getTouchKey_tab(unsigned int a1, unsigned char *a2)
{
	unsigned int i;

	for (i = 0; i < 10; i++) {
		if (!a2[i * 4] && !a2[i * 4 + 1])
			return 255;
		if ((unsigned int)a2[i * 4 + 2] <= a1 && (unsigned int)a2[i * 4 + 3] > a1) {
			((unsigned char *)keys_data)[0x8C] = a2[i * 4]; /* binaRE 0xC168E500 = keys_data+0x8C */
			((unsigned char *)keys_data)[0x8D] = a2[i * 4 + 1]; /* binaRE 0xC168E501 = keys_data+0x8D */
			return (unsigned char)(i + 1);
		}
	}
	return 255;
}

/* mtc_touch_work_func @ 0xc083e1a0 — все поля через keys_data-offsets (в дереве полей нет) */
static void
mtc_touch_work_func(void)
{
	unsigned char *kd8; /* binaRE offset-вид keys_data */

	kd8 = (unsigned char *)keys_data;
	if (kd8[0x94]) { /* binaRE 0xC168E508 = keys_data+0x94: гейт */
		unsigned char key = kd8[0x9C]; /* binaRE 0xC168E510 = keys_data+0x9C (в handoff было +0xA4 — арифметическая ошибка, пересчитано: 0xC168E510-0xC168E474=0x9C) */
		unsigned char code2;
		int cnt;

		cnt = ++(*(int *)(kd8 + 0x98)); /* binaRE 0xC168E50C = keys_data+0x98: counter (инкремент ДО ветвления) */
		if (key == 50 || key == 58) {
			car_add_work(67, key, 0);
			schedule_delayed_work((struct delayed_work *)(kd8 + 0xA0), /* binaRE 0xC168E514 = keys_data+0xA0: dwork */
					      msecs_to_jiffies(200)); /* бинар: 1 аргумент msecs_to_jiffies (decompiled «4-арг» — артефакт) */
			return;
		}
		if (cnt > 6) {
			code2 = kd8[0x9D]; /* binaRE 0xC168E511 = keys_data+0x9D */
			if (code2) {
				car_add_work(67, code2, 0);
				return; /* бинар: tail (POP+B) — БЕЗ рескедулы */
			}
		}
		schedule_delayed_work((struct delayed_work *)(kd8 + 0xA0), msecs_to_jiffies(200));
	}
}

static int
keys_suspend(struct device *dev)
{
	return 0;
}

static int
keys_resume(struct device *dev)
{
	return 0;
}

static int
keys_probe(struct platform_device *pdev)
{
	int v3;
	int v4;

	struct input_dev *gpio_keys_input0;
	struct mtc_keys_input_id *keys_ids;
	struct mtc_keys_input_id *cur_id;
	int v26;

	struct adc_client *wheel_adc_client;
	void (*adc_wheel_callback)(struct adc_client *, void *, int);
	struct adc_client *wheel_adc_client_ch2;
	struct adc_client *adc_client;
	const char *gpio_label;

	unsigned int irq;

	int trig_type;
	irq_handler_t isr_callback;
	unsigned int irq_flags;
	signed int error;
	int keycode2;

	int register_result;
	signed int wakeup = 0;

	printk("--mtc keys_probe\n");

	if (car_struct.car_status.mtc_customer == 12) {
		LOBYTE(keys_data->intval1) = 1;
	} else if (car_struct.car_status.mtc_customer == 8 &&
		   car_struct.config_data.d.arr[1] != 3) {
		if (car_struct.config_data.d.arr[1] == 1) {
			BYTE1(keys_data->intval1) = 1;
		}
		keys_data->no_adc_ch1 = 1;
	}

	keys_data->keys_ws.wheel_wq = create_workqueue("wheel_wq");
	keys_data->process_wq = create_workqueue("process_wq");

	INIT_WORK(keys_data->keys_ws.volkey_work, volkey_work);
	INIT_WORK(keys_data->keys_ws.modemute_work, modemute_work);

	keys_data->keys_dev = kzalloc(sizeof(struct mtc_keys_drv), GFP_KERNEL); // 0x748 bytes
	if (!keys_data->keys_dev) {
		register_result = -ENOMEM;
		goto dev_create_failed;
	}

	gpio_keys_input0 = input_allocate_device();
	if (!gpio_keys_input0) {
		register_result = -ENOMEM;
		goto dev_create_failed;
	}

	dev_set_drvdata(&pdev->dev, keys_dev);

	gpio_keys_input0->id.vendor = 1;
	gpio_keys_input0->id.product = 1;
	gpio_keys_input0->name = pdev->name;
	gpio_keys_input0->id.version = 256;
	gpio_keys_input0->phys = "gpio-keys/input0";
	gpio_keys_input0->id.bustype = BUS_HOST;
	gpio_keys_input0->dev.power.suspend_timer.function = &pdev->dev; // ???
	keys_dev->keys_input0 = gpio_keys_input0;
	keys_dev->input_dev_count = input_dev_count;

	if (enable_key_repeat) {
		gpio_keys_input0->evbit[0] |= BIT_MASK(EV_REP);
	}

	for (int cur_dev = 0; cur_dev < input_dev_count; cur_dev++) {
		keys_ids = mtc_keys_devices;
		cur_id = &keys_ids[cur_dev];

		// unknown struct
		v26 = &keys_dev->keys_input[cur_dev].callback_param;
		*(v26 + 12) = gpio_keys_input0;
		*(v26 + 8) = &keys_ids[cur_dev];

		if (keys_ids[cur_dev].dev_type == 1) {
			goto skip_input_setup;
		}

		if (keys_ids[cur_dev].dev_type == 4) {
			*&keys_dev->keys_input[cur_dev].wheel_adc_init[0] = 1023;
			setup_timer(&keys_dev->keys_input[cur_dev].mcu_timer, adc_mcu_timer,
				    &keys_dev->keys_input[cur_dev].callback_param);
			mod_timer(&keys_dev->keys_input[cur_dev].mcu_timer,
				  msecs_to_jiffies(100u) - jiffies_64);

			goto skip_input_setup;
		}

		if (keys_ids[cur_dev].dev_type == 5) {
			if (keys_data->no_adc_ch1) {
				if (cur_id->adc_ch1) {
					goto skip_input_setup;
				}

				wheel_adc_client =
				    adc_register(0, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				keys_dev->keys_input[cur_dev].wheel_adc_client = wheel_adc_client;
				wheel_adc_client_ch2 =
				    adc_register(cur_id->adc_ch2, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				wheel_adc_client = keys_dev->keys_input[cur_dev].wheel_adc_client;
				keys_dev->keys_input[cur_dev].wheel_adc_client_ch2 =
				    wheel_adc_client_ch2;

				if (!wheel_adc_client) {
					register_result = -ENOMEM;
					goto gpio_keys_failed;
				}
			} else {
				adc_client =
				    adc_register(cur_id->adc_ch1, adc_wheel_callback,
						 &keys_dev->keys_input[cur_dev].callback_param);
				keys_dev->keys_input[cur_dev].wheel_adc_client = adc_client;

				if (!adc_client) {
					register_result = -ENOMEM;
					goto gpio_keys_failed;
				}
			}

			*&keys_dev->keys_input[cur_dev].wheel_adc_init[0] = 1023;
			hrtimer_init(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer,
				     HRTIMER_MODE_REL,
				     2000000); // ?
			keys_dev->keys_input[cur_dev].adc_wheel_hrtimer.function = adc_wheel_timer;
			hrtimer_start(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer, 10000000LL,
				      HRTIMER_MODE_REL);
		} else {
			if (keys_ids[cur_dev].dev_type == 2) {
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				gpio_request((unsigned int)cur_id->gpio1, gpio_label);
				gpio_pull_updown((unsigned int)cur_id->gpio1, 1u);
				gpio_direction_input((unsigned int)cur_id->gpio1);
				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				gpio_request((unsigned int)cur_id->gpio2, gpio_label);
				gpio_pull_updown((unsigned int)cur_id->gpio2, 1u);
				gpio_direction_input((unsigned int)cur_id->gpio2);
				hrtimer_init(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer, 1,
					     2000000);
				keys_dev->keys_input[cur_dev].adc_wheel_hrtimer.function =
				    wheel_timer;
				hrtimer_start(&keys_dev->keys_input[cur_dev].adc_wheel_hrtimer,
					      10000000LL, HRTIMER_MODE_REL);
			} else if (cur_id->gpio1 != -1) {
				int res;

				gpio_label = cur_id->dev_name;
				if (!gpio_label) {
					gpio_label = "keys";
				}

				res = gpio_request((unsigned int)cur_id->gpio1, gpio_label);
				if (res < 0) {
					register_result = res;
					pr_err("gpio-keys: failed to request GPIO %d, error %d\n",
					       cur_id->gpio1, res);

					goto gpio_keys_failed;
				}

				gpio_pull_updown((unsigned int)cur_id->gpio1,
						 cur_id->dev_type == 3);
				res = gpio_direction_input((unsigned int)cur_id->gpio1);
				if (res < 0) {
					register_result = res;
					pr_err("gpio-keys: failed to configure input direction for "
					       "GPIO %d, error %d\n",
					       cur_id->gpio1, res);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}

				keys_dev->keys_input[cur_dev].irq = (unsigned int)cur_id->gpio1;
				if (cur_id->gpio1 < 0) {
					register_result = cur_id->gpio1;
					pr_err("gpio-keys: Unable to get irq number for GPIO %d, "
					       "error %d\n",
					       cur_id->gpio1, cur_id->gpio1);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}

				if (cur_id->dev_type == 3) {
					tasklet_init(&keys_dev->keys_input[cur_dev].ir_tasklet,
						     ir_task,
						     &keys_dev->keys_input[cur_dev].callback_param);

					setup_timer(keys_dev->keys_input[cur_dev].ir_timer,
						    ir_timer,
						    &keys_dev->keys_input[cur_dev].callback_param);

					irq = keys_dev->keys_input[cur_dev].irq;
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					isr_callback = ir_isr;
				} else {
					trig_type = cur_id->trig_type;
					gpio_label = cur_id->dev_name;
					isr_callback = keys_isr;
				}

				if (trig_type) {
					irq_flags = IRQF_TRIGGER_FALLING;
				} else {
					irq_flags = IRQF_TRIGGER_RISING;
				}

				if (!gpio_label) {
					gpio_label = "keys";
				}

				error = request_threaded_irq(
				    irq, isr_callback, 0, irq_flags, gpio_label,
				    &keys_dev->keys_input[cur_dev].callback_param);

				if (error) {
					register_result = error;
					pr_err("gpio-keys: Unable to claim irq %d; error %d\n",
					       keys_dev->keys_input[cur_dev].irq, error);
					gpio_free((unsigned int)cur_id->gpio1);

					goto gpio_keys_failed;
				}
			}
		}

	skip_input_setup:
		input_set_capability(gpio_keys_input0, 1u, cur_id->keycode1);
		if (keycode2) {
			input_set_capability(gpio_keys_input0, 1u, cur_id->keycode2);
		}

		if (cur_id->wakeup) {
			wakeup = 1;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(mtc_keycodes); i++) {
		input_set_capability(gpio_keys_input0, 1u, mtc_keycodes[i]);
	}

	input_set_capability(gpio_keys_input0, 1u, KEY_ENTER);
	input_set_capability(gpio_keys_input0, 1u, KEY_WAKEUP);
	input_set_capability(gpio_keys_input0, 1u, KEY_BACK);
	input_set_capability(gpio_keys_input0, 1u, KEY_WPS_BUTTON);
	input_set_capability(gpio_keys_input0, 1u, 0x20Cu); // unknown keycode

	register_result = input_register_device(gpio_keys_input0);
	if (!register_result) {
		device_init_wakeup(&pdev->dev, wakeup);
		register_early_suspend(&mtc_keys_early_suspend);
		keys_data->p_input_dev = gpio_keys_input0;
		car_struct.car_status.input_ready = 1;

		return register_result;
	}

	pr_err("gpio-keys: Unable to register input device, error: %d\n", register_result);

gpio_keys_failed:
	dev_set_drvdata(&pdev->dev, 0);

dev_create_failed:
	input_free_device(gpio_keys_input0);
	kzfree(keys_dev);

	return register_result;
}

/* decompiled */
static int __devexit
keys_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_get_drvdata(dev);
	keys_data->p_input_dev = NULL;
	device_init_wakeup(dev, 0);

	return 0;
}

/* recovered structures */

static struct dev_pm_ops keys_pm_ops = {
    .suspend = keys_suspend, .resume = keys_resume,
};

static struct early_suspend mtc_keys_early_suspend = {
	.level = 0x2f,
	.suspend = keys_early_suspend,
	.resume = keys_later_resume,

};

static struct platform_driver mtc_keys_driver = {
    .probe = keys_probe,
    .remove = __devexit_p(keys_remove),
    .driver =
	{
	    .name = "mtc-keys", .pm = &keys_pm_ops,
	},
};

module_platform_driver(mtc_keys_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC keys driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-keys");
