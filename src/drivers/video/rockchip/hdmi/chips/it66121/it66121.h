#ifndef __IT66121_H__
#define __IT66121_H__
#include <linux/delay.h>
#include "../../rk_hdmi.h"
#include "hdmitx.h"
#include "hdmitx_sys.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define IT66121_I2C_RATE	100 * 1000

#define delay1ms(ms) msleep(ms)

/* t9 [fix 14]: donor cat66121_hdmi_hw.h */
enum {
	OUTPUT_DVI = 0,
	OUTPUT_HDMI
};

#ifdef CONFIG_RK_HDMI_DEBUG
#define DBG(format, ...) \
		printk(KERN_INFO "CAT66121: " format "\n", ## __VA_ARGS__)
#define HDMITX_DEBUG_PRINTF(x)  DBG x
#define HDMITX_DEBUG_PRINTF1(x)	//DBG x
#define HDMITX_DEBUG_PRINTF2(x) //DBG x
#define HDMITX_DEBUG_PRINTF3(x) //DBG x

#define HDCP_DEBUG_PRINTF(x)   //DBG x
#define HDCP_DEBUG_PRINTF1(x)  //DBG x
#define HDCP_DEBUG_PRINTF2(x)  //DBG x
#define HDCP_DEBUG_PRINTF3(x)  //DBG x
#else
#define DBG(format, ...)
#define HDMITX_DEBUG_PRINTF(x)  
#define HDMITX_DEBUG_PRINTF1(x)
#define HDMITX_DEBUG_PRINTF2(x)
#define HDMITX_DEBUG_PRINTF3(x) // printf x

#define HDCP_DEBUG_PRINTF(x)
#define HDCP_DEBUG_PRINTF1(x)
#define HDCP_DEBUG_PRINTF2(x)
#define HDCP_DEBUG_PRINTF3(x)
#endif

/* t9 [fix 13]: HDMI_SOURCE_DEFAULT — 1-в-1 с donor cat66121_hdmi.h (GPL 3.0.101) */
#if defined(CONFIG_RK_LCDC0_AS_PRIMARY)
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC0
#else // CONFIG_RK_LCDC0_AS_PRIMARY
#if defined(CONFIG_RK_LCDC1_AS_PRIMARY)
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC1
#else
#if defined(CONFIG_HDMI_SOURCE_LCDC1)
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC1
#else
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC0
#endif
#endif // CONFIG_RK_LCDC1_AS_PRIMARY
#endif

/* t9 [fix 13]: donor-структура pdata (заменяет ручной struct cat66121, отменён handoff #5) */
struct cat66121_hdmi_pdata {
	int gpio;
	struct i2c_client *client;
	struct delayed_work delay_work;
	struct workqueue_struct *workqueue;
        int plug_status;
};

extern struct cat66121_hdmi_pdata *cat66121_hdmi;

extern void hdmi_register_display_sysfs(struct hdmi *hdmi, struct device *parent);
extern void hdmi_unregister_display_sysfs(struct hdmi *hdmi);

/* it66121_hal.c (donor cat66121_hdmi_hw.c) — 8 cat66121_hdmi_sys_* */
extern int cat66121_hdmi_sys_init(void);
extern int cat66121_hdmi_sys_insert(void);
extern int cat66121_hdmi_sys_remove(void);
extern void cat66121_hdmi_sys_enalbe_output(int enable);
extern int cat66121_hdmi_sys_detect_hpd(void);
extern int cat66121_hdmi_sys_read_edid(int block, unsigned char *buff);
extern int cat66121_hdmi_sys_config_video(struct hdmi_video_para *vpara);
extern int cat66121_hdmi_sys_config_audio(struct hdmi_audio *audio);

/* it66121.c glue / it66121_hal.c: IRQ path — сигнатуры = donor (GPL 3.0.101) */
extern void cat66121_hdmi_interrupt(void);
extern int cat66121_detect_device(void);
extern void cat66121_InterruptClr(void);
#endif
