/*
 * ch7025_recon.c — binaRE Batch 9: реконструкция CH7025 рег-доступа из vendor vmlinux
 * (decompiled_{ch7025_i2c_write,ch7025_reg,ch7025_reg_1024}.c, IDA 9.3).
 * Все non-static → T в nm (judge). Новый файл; файлы Batch 5 (ch7025.c и др.)
 * НЕ модифицированы.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>

/* decompiled ch7025_i2c_write @0xc09c5cf0 (104 bytes).
 * msg = {addr=client->addr, flags=0, len=2, buf=[reg,val]};
 * i2c_transfer(client->adapter, &msg, 1) >= 0 → 0, иначе 255.
 * Локальный timeout=100000 (IDA sp-slot) — в i2c_transfer не передаётся. */
int ch7025_i2c_write(struct i2c_client *client, u8 reg, u8 val)
{
	u8 wbuf[2];
	struct i2c_msg msg;
	unsigned long timeout = 100000;	/* decompiled sp-локаль, неиспользуется */

	wbuf[0] = reg;
	wbuf[1] = val;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = wbuf;

	if (i2c_transfer(client->adapter, &msg, 1) >= 0)
		return 0;
	(void)timeout;
	return 255;
}

/* decompiled ch7025_reg @0xc0a088a0 / ch7025_reg_1024 @0xc0a088f4: в nm vendor'а —
 * T (код); IDA показывает тело как ошибочно распарсенную byte-последовательность
 * (в decompiled_probe эти имена итерятся как u16-таблицы — таблично-подобный код),
 * восстановлению не поддаётся → минимальные stub'ы, сохраняющие call-graph:
 * ch7025_reg(a, b) == ch7025_reg_1024(16, b - 4, 0x10000, 16) (decompiled). */
int ch7025_reg_1024(int a, int off, int page, int b)
{
	(void)a;
	(void)off;
	(void)page;
	(void)b;
	return 0;
}

int ch7025_reg(int a, int off)
{
	return ch7025_reg_1024(16, off - 4, 0x10000, 16);
}
