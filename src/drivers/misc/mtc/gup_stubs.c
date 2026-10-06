/*
 * G5 (binaRE): gup_* update-mode stubs, restored 1:1 from target 3188 vmlinux.
 *
 * Target kallsyms:
 *   c083dda8 T gup_enter_update_mode  (8 bytes: mov r0,#0; bx lr)
 *   c083ddb0 T gup_leave_update_mode  (4 bytes: bx lr)
 *   c083ddb4 T gup_update_proc        (8 bytes: mov r0,#0; bx lr)
 *
 * The full implementations live in ref_kernel/drivers/input/touchscreen/gt9xx_update.c,
 * but CONFIG_TOUCHSCREEN_GT9110_BQ is off in this build, so the target vmlinux carries
 * only empty stubs for these globals (referenced by goodix9xx_tool.c / gt9xx_update.c).
 * Signatures match the extern declarations in goodix9xx_tool.c.
 */

#include <linux/types.h>
#include <linux/i2c.h>

s32 gup_enter_update_mode(struct i2c_client *client)
{
	return 0;
}

void gup_leave_update_mode(void)
{
}

s32 gup_update_proc(void *dir)
{
	return 0;
}
