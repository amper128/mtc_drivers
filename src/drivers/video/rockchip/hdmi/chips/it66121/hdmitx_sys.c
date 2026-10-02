///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <hdmitx_sys.c>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2012/07/05
//   @fileversion: ITE_HDMITX_SAMPLE_3.11
//******************************************/
/* t9 [fix 13]: T9.2 — ручная реконструкция ОТМЕНЕНА (handoff #6).
 * 8 функций cat66121_hdmi_sys_* + i2c-регистра + cat66121_hdmi_interrupt
 * теперь = донор GPL 3.0.101 (chips/it66121/it66121_hal.c = cat66121_hdmi_hw.c).
 * Файл оставлен в Makefile (пустой TU) для сохранения layout-порядка объектов. */
#include "it66121.h"
