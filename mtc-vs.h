#include <linux/serial_core.h>

#ifndef _MTC_VS_H
#define _MTC_VS_H

struct mtc_vs_port {
	struct uart_port uart_port;
	struct platform_device *pdev;
	int mctrl_type;
	unsigned int tx_empty;
	char gap8[12];
	int value1;
	speed_t vs_uart_speed;
	char gap9[4];
	int vs_rx;
	unsigned int port_n;
	int dwordCC;
	int dwordD0;
	struct workqueue_struct *vs_wq;
	struct work_struct vs_work;
	int shutdown;
	int port_disabled;
	int dwordF0;
	unsigned int p_line;
	struct mutex lock;
};

struct mtc_vs_portlist {
	bool ports_added;
	struct mtc_vs_port *vss_dev[4];
	bool init_ok;
};

#endif
