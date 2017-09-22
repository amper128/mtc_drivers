// this file contains differences between RK3188 SDK kernel sources and decompiled code

#include "mtc_shared.h"


/* kernel/sys.c */
void kernel_restart(char *cmd)
{
	/* sending cmd to MCU */
	arm_send(MTC_CMD_SHUTDOWN);

	/*
	*  debug trace
	*/
	restart_dbg("%s->%d->cmd=%s",__FUNCTION__,__LINE__,cmd);

	kernel_restart_prepare(cmd);
	disable_nonboot_cpus();
	if (!cmd)
		printk(KERN_EMERG "Restarting system.\n");
	else
		printk(KERN_EMERG "Restarting system with command '%s'.\n", cmd);
	kmsg_dump(KMSG_DUMP_RESTART);
	machine_restart(cmd);
}
EXPORT_SYMBOL_GPL(kernel_restart);


/* arch/arm/mach-rk30/? */
/* maybe, new board device? */
void rk30_pm_power_off()
{
	printk("<3>rk30_pm_power_off start...\n");
	printk("ARM_SHUTDOWN\n");
	arm_send(0x755u);
	while ( 1 )
	{
		msleep(0xAu);
	}
}
