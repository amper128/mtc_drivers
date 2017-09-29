#include <linux/serial_core.h>

#ifndef _MTC_VS_H
#define _MTC_VS_H

struct mtc_vs_port
{
	struct uart_port uart_port;
	struct platform_device *pdev;
	int mctrl_type;
	int tx_empty;
	char gap8[12];
	int value1;
	speed_t vs_uart_speed;
	char gap9[4];
	int vs_rx;
	int port_n;
	int dwordCC;
	int dwordD0;
	struct workqueue_struct *vs_wq;
	struct work_struct vs_work;
	int shutdown;
	int port_disabled;
	int dwordF0;
	int p_line;
	struct mutex lock;
};

#endif
