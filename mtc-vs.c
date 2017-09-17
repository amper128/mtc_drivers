#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>

void vs_enable_ms(void) { ; }

void vs_stop_rx(struct mtc_vs_drv *vs_dev) { vs_dev->vs_rx = 0; }

/* dirty code */
signed int vs_get_mctrl(struct mtc_vs_drv *vs_dev)
{
	signed int result; // r0@2

	if (vs_dev->mctrl_type) // ?
	{
		result = 0x160;
	} else {
		result = 0x140;
	}
	return result;
}

void vs_set_mctrl(void) { ; }

unsigned int vs_type(struct mtc_vs_drv *vs_dev)
{
	unsigned int result; // r0@2

	if ( vs_dev->vs_type == 86 )            // ?
	{
		result = 0xC0B02104;
	}
	else
	{
		result = 0;
	}
	return result;
}

void vs_release_port(void)
{
	;
}

/* dirty code */
void vs_config_port(struct mtc_vs_drv *vs_dev, char a2)
{
	if ( a2 & 1 )
	{
		vs_dev->vs_type = 86;
	}
}

/* dirty code */
unsigned int vs_verify_port(int a1, _DWORD *a2)
{
	char v2; // zf@1
	unsigned int result; // r0@4

	v2 = *a2 == 0;
	if ( *a2 )
	{
		v2 = *a2 == 86;
	}
	if ( v2 )
	{
		result = 0;
	}
	else
	{
		result = 0xFFFFFFEA;
	}
	return result;
}

void vs_stop_tx(void)
{
	;
}

int vs_request_port(void)
{
	return 0;
}

void vs_break_ctl(void)
{
	;
}

int vs_resume(struct mtc_vs_drv *vs_dev)
{
	dev_get_drvdata(&vs_dev->dev); // WTF??
	return 0;
}

int vs_suspend(struct mtc_vs_drv *vs_dev)
{
	dev_get_drvdata(&vs_dev->dev);          // WTF?
	return 0;
}

/* dirty code */
void vs_set_termios(struct uart_port *p_port, int arg2)
{
	struct uart_port *port; // r4@1
	struct uart_state *port_state; // r3@1
	struct tty_struct *tty; // r8@1
	int v6; // r7@2
	speed_t port_speed; // r5@2
	struct uart_state *uart_state; // r3@2

	port = p_port;
	port_state = p_port->state;
	tty = port_state->port.tty;
	if ( port_state->port.tty )
	{
		v6 = *(arg2 + 8);
		port_speed = tty_get_baud_rate(port_state->port.tty);
		tty_encode_baud_rate(tty, port_speed, port_speed);
		port[1].pm = port_speed;
		*(arg2 + 8) = v6 & 0xBFFFFFFF;
		uart_state = port->state;
		port->ignore_status_mask = 0;
		LOBYTE(uart_state->port.tty->link) |= 0x10u;
		uart_update_timeout(port, *(arg2 + 8), port_speed);
	}
}

void vs_work(void)
{
	int v0; // r0@0
	int v1; // r5@1
	struct uart_port *port; // r7@1
	struct task_struct *task; // r6@1
	int v4; // r4@1
	int v5; // r3@3
	int v6; // r2@3
	int v7; // r3@3

	// container_of?
	v1 = *(v0 - 156);
	port = (v0 - 216);
	task = get_current();
	v4 = v0;
	do
	{
		if ( *(v4 - 168) )
		{
			v5 = *(v4 - 132);
			*(v4 - 168) = 0;
			*(v4 - 132) = v5 + 1;
			v6 = *(v1 + 156);
			v7 = *(v1 + 160);
LABEL_4:
			if ( ((v6 - v7) & 0xFFFu) > 0xFF )
			{
				goto LABEL_5;
			}
			goto LABEL_13;
		}
		v6 = *(v1 + 156);
		v7 = *(v1 + 160);
		if ( v6 == v7 )
		{
			goto LABEL_13;
		}
		if ( *(**(v4 - 156) + 184) & 3 )
		{
			goto LABEL_4;
		}
		*(v1 + 160) = (v7 + 1) & 0xFFF;
		++*(v4 - 132);
		if ( ((*(v1 + 156) - *(v1 + 160)) & 0xFFFu) > 0xFF )
		{
LABEL_5:
			if ( *(v4 + 16) )
			{
				return;
			}
			continue;
		}
LABEL_13:
		uart_write_wakeup(port);
		if ( *(v4 + 16) )
		{
			return;
		}
	}
	while ( !(**(*(task + 3) + 4) & 0x80000) && *(v1 + 156) != *(v1 + 160) && !(*(**(v4 - 156) + 184) & 3) );
}

static int vs_probe(struct platform_device *pdev)
{
	(void)pdev;
	return 0;
}

static int __devexit vs_remove(struct platform_device *pdev)
{
	int v1;		 // r5@1
	const void **v2; // r4@1
	const void *v3;  // t1@2

	v1 = 0;
	v2 = dev_get_drvdata(&pdev->dev);
	mutex_lock(off_C09BDD9C);
	do {
		++v1;
		uart_remove_one_port(off_C09BDDA0, *v2);
		v3 = *v2;
		++v2;
		kfree(v3);
		*(v2 - 1) = 0;
	} while (v1 != 4);
	uart_unregister_driver(off_C09BDDA0);
	mutex_unlock(off_C09BDD9C);
	return 0;
}

/* recovered structures */

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

module_platform_driver(mtc_vs_driver);

MODULE_AUTHOR("Alexey Hohlov <root@amper.me>");
MODULE_DESCRIPTION("Decompiled MTC VS driver");
MODULE_LICENSE("BSD");
MODULE_ALIAS("platform:mtc-vs");
