/* binaRE: MTC-драйвер — аудио/codec (WM8731) часть. НОВЫЙ файл (не существовал в дереве).
 * Транскрипция 1:1 из IDA 9.3 decompiled (/home/amper/tmp/ida-tmp/mtc_audio/src_all/)
 * + disassembly; адреса сверены с 3188_kallsyms (tr -d '\r'). */

#include <asm/string.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "mtc-car.h"
#include "mtc_shared.h"

/* binaRE: BSS @0xC168EB98, set by wm8731_probe @0xC0860FB8 (wm8731-драйвер вне дерева) */
extern struct snd_soc_component *wm8731_client;

/* binaRE: локальные прототипы (определений в дереве нет — kernel/sound, другие драйверы) */
int snd_soc_write(struct snd_soc_component *comp, unsigned int reg, int value);
/* binaRE: бинарный вызов — 5 регистрам (R0-R4 + R5): u64 ppos в R4:R5, лишний R3 — stale
 * (см. комменты в call-sites). */
int soc_codec_reg_show(void *comp, char *buf, unsigned int count, unsigned long long ppos);

/* binaRE codec_pwr @0xc086104c (IDA 9.3 decompiled) */
void
codec_pwr(void)
{
	; /* binaRE: ПУСТОЕ тело (заглушка в бинаре — size 4 байта) */
}

/* binaRE codec_deactive @0xc0861050 (IDA 9.3 decompiled) */
int
codec_deactive(void)
{
	return snd_soc_write(wm8731_client, 6, 255);
}

/* binaRE codec_active @0xc0861068 (IDA 9.3 decompiled) */
int
codec_active(void)
{
	int r;

	msleep(100); /* binaRE: IDA-артефакт — 3 extra stale-арга (a2, a3, a4) */
	r = snd_soc_write(wm8731_client, 15, 0);
	if (r < 0) {
		/* binaRE: wm8731_client+12 — struct device* внутри component (имён в дереве нет) */
		dev_err(*(struct device **)((char *)wm8731_client + 12), "Failed to issue reset: %d\n", r);
		return r;
	}
	snd_soc_write(wm8731_client, 0, 151);
	snd_soc_write(wm8731_client, 1, 151);
	snd_soc_write(wm8731_client, 2, 0);
	snd_soc_write(wm8731_client, 3, 0);
	if (car_struct.config_data.d.cfg_bt == 6 || car_struct.config_data.d.cfg_bt == 7) { /* binaRE A:0xC168AD45 = cfg_bt@5 */
		snd_soc_write(wm8731_client, 4, 20);
	} else {
		snd_soc_write(wm8731_client, 4, 21);
	}
	snd_soc_write(wm8731_client, 5, 0);
	snd_soc_write(wm8731_client, 6, 97);
	snd_soc_write(wm8731_client, 7, 2);
	snd_soc_write(wm8731_client, 8, 32);
	snd_soc_write(wm8731_client, 9, 1);
	return 0;
}

/* binaRE codec_io_init @0xc0860f94 (IDA 9.3 decompiled) */
int
codec_io_init(void)
{
	gpio_request(172, "codec_pwr"); /* binaRE: IDA-артефакт — 2 extra stale-арга */
	return gpio_direction_output(172, 0); /* binaRE: IDA-артефакт — 2 extra stale-арга */
}

/* binaRE codec_reg_show @0xc085b650 (IDA 9.3 decompiled) */
int
codec_reg_show(void *kobj, void *attr, char *buf)
{
	/* binaRE: a1+360 — указатель на component в runtime-объекте; арг a4 в бинаре stale */
	return soc_codec_reg_show(*(void **)((char *)kobj + 360), buf, 4096, 0);
	/* binaRE: бинарный вызов — 5 регистрам (лишний R3 = stale a4; R4:R5 = ppos = 0) */
}

/* binaRE codec_reg_open_file @0xc085589c (IDA 9.3 decompiled) */
int
codec_reg_open_file(void *file, void *reg_file)
{
	/* binaRE: a1+356 — источник указателя; a2+132 — целевое поле */
	*(void **)((char *)reg_file + 132) = *(void **)((char *)file + 356);
	return 0;
}

/* binaRE codec_reg_read_file @0xc085b570 (IDA 9.3 decompiled) */
int
codec_reg_read_file(void *reg_file, char __user *to, unsigned int count, loff_t *ppos)
{
	void *comp = *(void **)((char *)reg_file + 132);
	char *buf;
	int r;

	if (!count || ((*((unsigned long long *)ppos) >> 63) & 1))
		return -22; /* -EINVAL; binaRE: IDA (*(_DWORD*)(a4+4) >> 31) — sign-bit hi-u64 ppos (ppos-u64-идиом) */

	buf = kmalloc(count, 0xD0); /* binaRE: gfp = 0xD0 (208) — необычное сочетание флагов (в 3.4: ZERO|NORETRY|COMP|RECLAIMABLE); значение бинара */
	if (!buf)
		return -12; /* -ENOMEM */

	r = soc_codec_reg_show(comp, buf, count, *(unsigned long long *)ppos);
	/* binaRE: бинарный вызов — 5 регистрам (лишний R3 = stale → вне прототипа) */
	if (r >= 0) {
		/* binaRE: в бинаре перед copy_to_user — inline-проверка access_ok (__CFADD__/__CFSUB__-идиом
		 * с get_current()+8, см. IDA decompiled); в исходнике — стандартный idiom */
		if (access_ok(VERIFY_WRITE, to, r))
			r = copy_to_user(to, buf, r);
		if (r) {
			kfree(buf);
			return -14; /* -EFAULT */
		}
		*ppos += r; /* binaRE: u64-инкремент ppos */
	}
	kfree(buf);
	return r;
}

/* binaRE codec_reg_write_file @0xc0857ac8 (IDA 9.3 decompiled) */
int
codec_reg_write_file(void *reg_file, const char __user *userbuf, unsigned int count)
{
	void *comp = *(void **)((char *)reg_file + 132);
	char local[32]; /* binaRE: буфер IDA var20 (до 31+1 байт) */
	unsigned int len;
	char *p;
	unsigned long reg;
	unsigned long val;

	len = (count >= 31) ? 31 : count;

	if (!access_ok(VERIFY_READ, userbuf, len)) {
		/* binaRE: в бинаре access_ok инлайнится как __CFADD__/__CFSUB__-идиом с get_current()+8
		 * (см. IDA decompiled); fail → memzero + -EFAULT */
		memzero(local, len);
		return -14; /* -EFAULT */
	}
	if (copy_from_user(local, userbuf, len))
		return -14; /* -EFAULT */

	local[len] = 0;

	p = local;
	while (*p == ' ') /* binaRE: IDA do/while skip-spaces */
		p++;
	reg = simple_strtoul(p, &p, 16);
	while (*p == ' ') /* binaRE: IDA do/while skip-spaces */
		p++;
	if (kstrtoul(p, 16, &val))
		return -22; /* -EINVAL */

	add_taint(6); /* binaRE-артефакт: add_taint(6) в бинаре — сохранён как есть */
	snd_soc_write(comp, reg, val);
	return len;
}

/* binaRE codec_list_read_file @0xc08564fc — IDA-декомпиляция СЛОМАНА (нет list-цикла);
 * тело по ТОЧНОМУ disasm (disassembly_full.txt L985986-986059, блок C08564FC-C08565D4) */
int
codec_list_read_file(char __user *to, unsigned int count, loff_t *ppos)
{
	/* binaRE: LDR R3,=0xC0D160AC; LDR R0,[R3,#0x30] — runtime-указатель на kmem_cache */
	struct kmem_cache *cachep = *(struct kmem_cache **)((char *)0xC0D160AC + 0x30);
	/* binaRE: off_C0BD33A0 — RUNTIME-объект (devattr/kobject-цепочка со строками "soc-audio"/"codec_reg",
	 * .show = codec_reg_show); в текущем образе [base+0x60] = base → len = 0. Имена НЕ выдумывать. */
	void *base = (void *)0xC0BD33A0;
	void *head = *(void **)((char *)base + 0x60); /* LDR R5,[R7,#(off_C0BD3400-off_C0BD33A0)]; imm12 в бинаре = 0x60 (проверено raw-байтом kernel.elf C0856538) */
	char *node;
	int len;
	int ret;
	void *buf;

	if (!cachep) {
		buf = (void *)16; /* binaRE-артефакт: MOV R8,#0x10 fallback — "buf" = мусорное 16 (краш, если список не пуст); сохранено как в бинаре */
	} else {
		buf = kmem_cache_alloc_trace(cachep, 0xD0, 0x1000); /* binaRE: литералы аргументов (R1=0xD0 flag — как kmalloc выше; R2=0x1000 — значение бинара, сохранено) */
		if (!buf)
			return -12; /* -ENOMEM: MOV R4,#-12 */
	}

	len = 0;
	if (head != base) {
		node = (char *)head - 0x30; /* SUB R5,R5,#0x30 — узел списка */
		do {
			ret = snprintf(buf + len, 4096 - len, "%s\n", *(char **)node); /* fmt "%s\n" подтверждён в бинаре (a16s0x10x0x10xS+0x24) */
			if (ret >= 0) {
				len += ret;
				if (len > 4096) { /* CMP R4,#0x1000; BLS */
					len = 4096;
					break;
				}
			}
			{
				void *next = *(void **)((char *)node + 0x30); /* LDR R5,[R5,#0x30] */
				if (next == base) /* CMP R5,R7; BEQ */
					break;
				node = (char *)next - 0x30; /* SUB R5,R5,#0x30 */
			}
		} while (1);
	}
	ret = simple_read_from_buffer(to, count, ppos, buf, len);
	kfree(buf); /* binaRE: даже в fallback при buf == 16 (артефакт — сохранено) */
	return ret;
}
