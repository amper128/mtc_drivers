#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "mtc-vs.h"

/* forward declarations */
static struct uart_ops vs_uart_ops;
static struct uart_driver vs_uart_driver;
static struct platform_driver mtc_vs_driver;

static DEFINE_MUTEX(mtc_vs_mutex);

static struct mtc_vs_portlist vs_portlist;

/*
 * ===========================================
 *	MTC Virtual Serial Port Driver
 * ===========================================
 */

/* fully decompiled */
static void
vs_enable_ms(struct uart_port *port)
{
	(void)port;
}

/* fully decompiled */
static void
vs_stop_rx(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;

	vs_port->vs_rx = 0;
}

/* fully decompiled */
static unsigned int
vs_get_mctrl(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;
	unsigned int result;

	result = TIOCM_DSR | TIOCM_CAR;

	if (vs_port->mctrl_type) {
		result |= TIOCM_CTS;
	}

	return result;
}

/* fully decompiled */
static void
vs_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	(void)port;
	(void)mctrl;
}

/* fully decompiled */
static const char *
vs_type(struct uart_port *port)
{
	if (port->type == 86) {
		return "MAX3100";
	}

	return NULL;
}

/* fully decompiled */
static void
vs_release_port(struct uart_port *port)
{
	(void)port;
}

/* fully decompiled */
static void
vs_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = 86;
	}
}

/* fully decompiled */
static int
vs_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	(void)port;

	if (ser->type == 0 || ser->type == 86) {
		return 0;
	}

	return -EINVAL;
}

/* fully decompiled */
static void
vs_stop_tx(struct uart_port *port)
{
	(void)port;
}

/* fully decompiled */
static int
vs_request_port(struct uart_port *port)
{
	(void)port;

	return 0;
}

/* fully decompiled */
static void
vs_break_ctl(struct uart_port *port, int break_state)
{
	(void)port;
	(void)break_state;
}

/* fully decompiled */
static int
vs_resume(struct platform_device *pdev)
{
	dev_get_drvdata(&pdev->dev);

	return 0;
}

/* fully decompiled */
static int
vs_suspend(struct platform_device *pdev, pm_message_t state)
{
	(void)state;
	dev_get_drvdata(&pdev->dev);

	return 0;
}

/* fully decompiled */
static void
vs_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;
	struct tty_struct *tty = port->state->port.tty;

	(void)old;

	if (tty) {
		speed_t port_speed;

		port_speed = tty_get_baud_rate(tty);
		tty_encode_baud_rate(tty, port_speed, port_speed);
		vs_port->vs_uart_speed = port_speed;

		termios->c_cflag &= ~CMSPAR;
		vs_port->uart_port.ignore_status_mask = 0;
		tty->low_latency = 1;

		uart_update_timeout(&vs_port->uart_port, termios->c_cflag, port_speed);
	}
}

/* dirty code */
static void
vs_work(struct work_struct *work)
{
	struct mtc_vs_port *vss = container_of(work, mtc_vs_port, vs_work);
	struct tty_struct *tty = vss->uart_port.state->port.tty;
	struct task_info *task;
	// int tx;
	void *pd;
	void *pdev;

	task = get_current();
	do {
		// if ( CONTAINING_RECORD(work, mtc_vs_port, vs_work)->uart_port.x_char )
		if (vss->uart_port.icount.tx) {
			vss->uart_port.icount.tx++;
			vss->uart_port.x_char = 0;

			pd = vss->uart_port.private_data;
			pdev = vss->pdev;
		LABEL_4:
			if (((pd - pdev) & 0xFFFu) > 0xFF) {
				goto LABEL_5;
			}
			goto LABEL_13;
		}

		// field magic???
		pd = vss->uart_port.private_data;
		pdev = vss->pdev;
		if (pd == pdev) {
			goto LABEL_13;
		}
		if (tty->stopped || tty->hw_stopped) {
			goto LABEL_4;
		}

		vss->pdev = ((pdev + 1) & 0xFFF);
		vss->uart_port.icount.tx++;
		if (((vss->uart_port.private_data - vss->pdev) & 0xFFFu) > 0xFF) {
		LABEL_5:
			if (vss->shutdown) {
				return;
			}
			continue;
		}
	LABEL_13:
		uart_write_wakeup(&vss->uart_port);
		if (vss->shutdown) {
			return;
		}
	} while (!(*task->stack & 0x80000) && vss->uart_port.private_data != vss->pdev &&
		 !(tty->stopped || tty->hw_stopped)); // what is it???
}

/* fully decompiled */
static void
vs_shutdown(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;

	if (!vs_port->port_disabled) {
		vs_port->shutdown = 1;

		if (vs_port->vs_wq) {
			flush_workqueue(vs_port->vs_wq);
			destroy_workqueue(vs_port->vs_wq);
			vs_port->vs_wq = 0;
		}
	}
}

/* fully decompiled */
static int
vs_startup(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;
	char buf[8];

	vs_port->vs_uart_speed = 115200;

	if (vs_port->port_disabled) {
		return 0;
	}

	sprintf(buf, "vs-%d", vs_port->port_n);

	vs_port->shutdown = 0;
	vs_port->value1 = 0;

	vs_port->vs_wq = create_workqueue(buf);
	if (!vs_port->vs_wq) {
		dev_warn(&vs_port->pdev->dev, "cannot create workqueue\n");
		return -ENOMEM;
	}

	INIT_WORK(&vs_port->vs_work, vs_work);

	vs_port->vs_rx = 1;

	return 0;
}

/* dirty code */
static void
vs_dowork(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;

	if (!vs_port->shutdown && !(vs_port->vs_work.data.counter & 1) &&
	    !(**(get_current()->flags + 4) & 0x80000) && !vs_port->port_disabled) {
		queue_work(vs_port->vs_wq, &vs_port->vs_work);
	}
}

/* fully decompiled */
static void
vs_start_tx(struct uart_port *port)
{
	vs_dowork(port);
}

/* fully decompiled */
static unsigned int
vs_tx_empty(struct uart_port *port)
{
	struct mtc_vs_port *vs_port = (struct mtc_vs_port *)port;

	vs_dowork(port);

	return vs_port->tx_empty;
}

/* dirty code */
void
vs_send_raw(int port_num, unsigned char *data, int count)
{
	signed int flip_buf;	 // r11@0
	struct mtc_vs_port *vs_port; // r6@1
	int i;			     // r10@5
	struct tty_struct *tty;      // r0@11
	struct tty_buffer *tail;     // r3@11
	int v12;		     // r2@12
	int v13;		     // r2@13
	char flags;		     // [sp+6h] [bp-2Ah]@11
	unsigned char byte;	  // [sp+7h] [bp-29h]@11

	vs_port = vs_portlist.vss_dev[port_num];

	if (dword_C168AC7C && car_status._gap1[0] && vs_port->vs_rx) {
		mutex_lock(&vs_port->lock);

		printk("vs_send raw ");

		for (i = 0; i < count; i++) {
			vs_port->uart_port.icount.rx++;
			if (!vs_port->vs_rx) {
				goto LABEL_8;
			}

			printk("%x ", data[i]);
			tty = vs_port->uart_port.state->port.tty;
			byte = data[i];
			flags = 0;

			tail = tty->buf.tail; // bytes magic?
			if (tail && (v12 = *(tail + 3), v12 < *(tail + 4))) {
				*(*(tail + 2) + v12) = 0;
				v13 = *(tail + 3);
				*(*(tail + 1) + v13) = chars;
				*(tail + 3) = v13 + 1;
			} else {
				tty_insert_flip_string_flags(tty, &byte, &flags, 1u);
			}

			flip_buf = 0;
			if ((i & 0xF) == 15) {
				flip_buf = 1;
				tty_flip_buffer_push(vs_port->uart_port.state->port.tty);
			}
		}

		printk("\n");
		if (!flip_buf) {
			tty_flip_buffer_push(vs_port->uart_port.state->port.tty);
		}

	LABEL_8:
		mutex_unlock(&vs_port->lock);
	}
}

EXPORT_SYMBOL_GPL(vs_send_raw)

/* dirty code */
void
vs_send(int port_num, unsigned char cmd, char *cmd_data, signed int count)
{
	signed int flip_buf;	 // r7@0
	int _port_num;		     // r9@1
	struct mtc_vs_port *vs_port; // r4@1
	size_t size;		     // r11@5
	char *data;		     // r0@5 MAPDST
	signed int _count;	   // r3@5
	char checksum;		     // r1@5
	char *cmd_p;		     // r2@7
	char *data_pos;		     // r0@8
	char cmd_byte;		     // t1@9
	int vs_rx;		     // r2@12 MAPDST
	int i;			     // r5@13
	int v18;		     // r3@15
	struct tty_struct *tty;      // r0@18 MAPDST
	struct tty_buffer *tail;     // r12@18
	int v23;		     // lr@19
	char flags;		     // [sp+Eh] [bp-2Ah]@18
	unsigned char chars;	 // [sp+Fh] [bp-29h]@15

	_port_num = port_num;
	vs_port = vs_portlist.vss_dev[port_num];
	if (dword_C168AC7C[0] && car_status._gap1[0] && vs_port->vs_rx) {
		size = count + 4;
		mutex_lock(&vs_port->lock);
		data = _kmalloc(size, __GFP_ZERO | __GFP_FS | __GFP_IO | __GFP_WAIT);
		_count = count;
		checksum = count + cmd; // really checksum?
		data[1] = cmd;
		*data = 46; // data[0] = 46;
		if (!count) {
			_count = 3;
		}
		data[2] = count;
		cmd_p = cmd_data;
		if (count) {
			data_pos = data + 2;
			do {
				(data_pos++)[1] = *cmd_p;
				cmd_byte = *cmd_p++;
				checksum += cmd_byte;
			} while (cmd_p != &cmd_data[_count]);
			_count += 3;
		}
		data[_count] = ~checksum;
		if (size) {
			vs_rx = vs_port->vs_rx;
			++vs_port->uart_port.icount.rx;
			if (vs_rx) {
				i = 0;
				do {
					tty = vs_port->uart_port.state->port.tty;
					chars = data[i];
					flags = 0;
					tail = tty->buf.tail;
					if (tail && (v23 = *(tail + 3), v23 < *(tail + 4))) {
						flip_buf = 0;
						*(*(tail + 2) + v23) = 0;
						v18 = *(tail + 3);
						*(*(tail + 1) + v18) = chars;
						*(tail + 3) = v18 + 1;
						if ((i & 0xF) == 15) {
							goto LABEL_21;
						}
					} else {
						tty_insert_flip_string_flags(tty, &chars, &flags,
									     1u);
						flip_buf = 0;
						if ((i & 0xF) == 15) {
						LABEL_21:
							flip_buf = 1;
							tty_flip_buffer_push(
							    vs_port->uart_port.state->port.tty);
							goto LABEL_16;
						}
					}
				LABEL_16:
					if (size <= ++i) {
						goto LABEL_22;
					}
					vs_rx = vs_port->vs_rx;
					++vs_port->uart_port.icount.rx;
				} while (vs_rx);
			}
		} else {
		LABEL_22:
			if (!flip_buf) {
				tty_flip_buffer_push(vs_port->uart_port.state->port.tty);
			}
		}
		mutex_unlock(&vs_portlist.vss_dev[_port_num]->lock);
		kfree(data);
	}
}

/* dirty code */
static int
vs_probe(struct platform_device *pdev)
{
	char *port_list;	     // r5@2
	unsigned int port;	   // r4@2 MAPDST
	int uartreg_ok;		     // r4@3
	struct mtc_vs_port *vss;     // r3@6
	struct uart_port *uart_port; // t1@10
	int port_added;		     // r3@10

	printk("vs_probe\n");
	mutex_lock(&mtc_vs_mutex);
	if (*&vs_portlist._gap0[0] || (*&vs_portlist._gap0[0] = 1,
				       (uartreg_ok = uart_register_driver(&vs_uart_driver)) == 0)) {
		port_list = vs_portlist._gap0;
		port = 0;
		do {
			if (size) {
				vss = kmem_cache_alloc_trace(size, 0x80D0, __GFP_COLD | __GFP_WAIT);
			} else {
				vss = ZERO_SIZE_PTR;
			}
			*(port_list + 1) = vss; // vs_drv->vss_dev[port] = vss;
			if (!vss) {
				uartreg_ok = -12;
				dev_warn(&pdev->dev, "kmalloc for vs structure %d failed!\n", port);
				mutex_unlock(&mtc_vs_mutex);
				return uartreg_ok;
			}
			vss->dwordD0 = 1;
			vss->dwordF0 = 1;
			vss->uart_port.flags = 0x10000040;
			vss->port_n = port; // ?
			vss->uart_port.line = port;
			vss->p_line = port; // ?
			vss->pdev = pdev;
			vss->dwordCC = 0;
			vss->uart_port.uartclk = 1843200;
			vss->uart_port.fifosize = 16;
			vss->uart_port.ops = &vs_uart_ops;
			vss->uart_port.type = 86;
			vss->uart_port.dev = &pdev->dev;
			__mutex_init(&vss->lock, "&vss[i]->lock", dword_C168AC7C);
			uart_port = *(port_list + 1);
			port_list += 4;
			port_added = uart_add_one_port(&vs_uart_driver, uart_port);
			if (port_added < 0) {
				dev_warn(&pdev->dev,
					 "uart_add_one_port failed for line %d with error %d\n",
					 port, port_added);
			}
			++port;
			mutex_unlock(&mtc_vs_mutex);
		} while (port != 4);
		uartreg_ok = 0;
		dev_set_drvdata(&pdev->dev, vs_portlist.vss_dev);
		dword_C168AC7C[0] = 1;
	} else {
		printk("<3>Couldn't register vs uart driver\n");
		mutex_unlock(&mtc_vs_mutex);
	}
	return uartreg_ok;
}

/* fully decompiled */
static int __devexit
vs_remove(struct platform_device *pdev)
{
	int port;
	struct uart_port **port_list;

	port_list = dev_get_drvdata(&pdev->dev);
	mutex_lock(&mtc_vs_mutex);

	for (port = 0; port < 4; port++) {
		uart_remove_one_port(&vs_uart_driver, port_list[port]);
		kfree(port_list[port]);
		port_list[port] = NULL;
	}

	uart_unregister_driver(&vs_uart_driver);
	mutex_unlock(&mtc_vs_mutex);
	return 0;
}

/* recovered structures */

static struct uart_ops vs_uart_ops = {
    .tx_empty = vs_tx_empty,
    .set_mctrl = vs_set_mctrl,
    .get_mctrl = vs_get_mctrl,
    .stop_tx = vs_stop_tx,
    .start_tx = vs_start_tx,
    .stop_rx = vs_stop_rx,
    .enable_ms = vs_enable_ms,
    .break_ctl = vs_break_ctl,
    .startup = vs_startup,
    .shutdown = vs_shutdown,
    .set_termios = vs_set_termios,
    .type = vs_type,
    .release_port = vs_release_port,
    .request_port = vs_request_port,
    .config_port = vs_config_port,
    .verify_port = vs_verify_port,
};

static struct uart_driver vs_uart_driver = {
    .driver_name = "ttyV", .dev_name = "ttyV", .major = 204, .minor = 209, .nr = 4,
};

static struct platform_driver mtc_vs_driver = {
    .probe = vs_probe,
    .remove = __devexit_p(vs_remove),
    .suspend = vs_suspend,
    .resume = vs_resume,
    .driver =
	{
	    .name = "mtc-vs",
	},
};

static int
vs_init()
{
	return platform_driver_register(&mtc_vs_driver);
}

static void
vs_exit()
{
	platform_driver_unregister(&mtc_vs_driver);
}

module_init(vs_init);
module_exit(vs_exit);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC VS driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-vs");
