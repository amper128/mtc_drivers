#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Реноме гэп-полей 4 структур mtc-драйвера по analyzer2/naming_report.md.
Только имена; layout/порядок/размеры НЕ меняются."""
import re, sys, io

ROOT = '/home/amper/Coding/mtc_drivers/'
stats = {}

def load(p):
    return open(ROOT + p, encoding='utf-8').read()

def save(p, s):
    open(ROOT + p, 'w', encoding='utf-8').write(s)

def count(key, n):
    stats[key] = stats.get(key, 0) + n
    if n:
        print('  %-46s %d' % (key, n))

# ============================================================ 1. mtc-car.h
h = load('mtc-car.h')

OLD_CS = """struct mtc_car_status {
\tchar _gap0[2];
\tchar rpt_power;
\tchar _gap1[1];
\tchar rpt_boot_android;
\tchar _gap2[2];
\tchar touch_type;
\tchar _gap3[3];
\tint intval1;
\tint intval2;
\tchar battery;
\tchar _gap4[3];
\tint intval3;
\tint intval4;
\tchar wipe_flag;
\tchar backlight_status;
\tchar _gap5[6];
\tchar sta_view;
\tchar ch_mode;
\tchar key_mode;
\tchar _gap7[1];
\tchar backview_vol;
\tchar sta_video_signal;
\tchar av_channel_flag1;
\tchar _gap8[2];
\tchar mcu_clk;
\tchar _gap81[6];
\tchar mcuver2[16];
\tchar mcuver1[16];
\tchar _gap9[6];
\tchar is1024screen;
\tchar _gap10[2];
\tchar ch_status;
\tchar _gap101[2];
\tchar uv_cal;
\tchar _gap11[32];
\tchar rpt_boot_appinit;
\tchar cfg_maxvolume;
\tchar _gap12[1];
\tint touch_width;
\tint touch_height;
\tchar touch_info1;
\tchar touch_info2;
\tchar mtc_customer;
\tchar boot_flags;
\tchar _gap13[4];
\tchar av_gps_switch;
\tchar av_gps_monitor;
\tchar av_gps_gain;
\tchar _gap14[5];
};"""

NEW_CS = """/* Имена гэп-полей по analyzer2/naming_report.md §1 (бинарные якоря проверены по
 * абсолютным адресам; car_status @ 0xC168AC84). Layout байт-exact: sizeof == 160.
 * reserved_N — без semantic/бинарного смысла (пометка — если бинарный доступ есть). */
struct mtc_car_status {
\tchar car_ready;\t     /* @0  was _gap0[0]: boot-инит-флаг (wait while(!); SET 1 boot/recovery) */
\tchar power_refcnt;\t  /* @1  was _gap0[1]: refcount power-on сессий (++/-- cmd 29/30; SET 1/0 cmd 43) */
\tchar rpt_power;
\tchar call_active;\t   /* @3  was _gap1[0]: входящий звонок (SET 1 + arm 0x202; гейт vs_uart) */
\tchar rpt_boot_android;
\tchar input_ready;\t   /* @5  was _gap2[0]: input-устройства зарегистрированы (mtc-keys) */
\tchar audio_ready;\t   /* @6  was _gap2[1]: audio-подсистема готова (гейт audio-ворков) */
\tchar touch_type;
\tchar sta_bits;\t      /* @8  was _gap3[0]: ioctl-статус-биты: 0x08=ill, 0x10=driving, 0x20=iPod */
\tchar reserved_3;\t    /* @9  was _gap3[1]: unused */
\tchar reserved_4;\t    /* @10 was _gap3[2]: unused */
\tint intval1;
\tint intval2;
\tchar battery;
\tchar reserved_5;\t    /* @21 was _gap4[0]: unused */
\tchar reserved_6;\t    /* @22 was _gap4[1]: unused */
\tchar reserved_7;\t    /* @23 was _gap4[2]: unused */
\tint intval3;
\tint intval4;
\tchar wipe_flag;
\tchar backlight_status;
\tchar cam_state;\t     /* @34 was _gap5[0]: состояние видеоисточника/камеры (бинар R/W; код — только READ) */
\tchar reserved_8;\t    /* @35 was _gap5[1]: binaRE-active (capture_work), роль ? */
\tchar reserved_9;\t    /* @36 was _gap5[2]: binaRE-active (T132B_Page_Write), роль ? */
\tchar cam_signal;\t    /* @37 was _gap5[3]: видеосигнал/камера подключена */
\tchar reserved_10;\t   /* @38 was _gap5[4]: binaRE-active (camera), роль ? */
\tchar reserved_11;\t   /* @39 was _gap5[5]: binaRE-active (video_Channel), роль ? */
\tchar sta_view;
\tchar ch_mode;
\tchar key_mode;
\tchar reserved_12;\t   /* @43 was _gap7[0]: binaRE-active (ioctl+camera+I2C), роль ? */
\tchar backview_vol;
\tchar sta_video_signal;
\tchar av_channel_flag1;
\tchar video_mode;\t    /* @47 was _gap8[0]: режим/источник видео (SET 0xFF при boot, wipe_flag&8) */
\tchar reserved_13;\t   /* @48 was _gap8[1]: binaRE-active (capture_work), роль ? */
\tchar mcu_clk;
\tchar decoder_state;\t /* @50 was _gap81[0]: состояние декодера (dvd/ADV7181D); OOB-чтение кодом как _gap8[3] (mtc-car.c:1908) */
\tchar reserved_14;\t   /* @51 was _gap81[1]: unused */
\tchar radar_val;\t     /* @52 was _gap81[2]: значение радара (ctl_radar); OOB-чтение кодом как _gap8[5] (mtc-car.c:2338) */
\tchar reserved_15;\t   /* @53 was _gap81[3]: unused */
\tchar reserved_16;\t   /* @54 was _gap81[4]: unused */
\tchar reserved_17;\t   /* @55 was _gap81[5]: unused */
\tchar mcuver2[16];
\tchar mcuver1[16];
\tchar power_on;\t      /* @88 was _gap9[0]: флаг «включено» (зеркало power_refcnt; SET 1 при boot) */
\tchar radio_rds_flag;\t /* @89 was _gap9[1]: RDS/radio-флаг (SET 0 в radio-init) */
\tchar reserved_18;\t   /* @90 was _gap9[2]: unused (SET 0 в radio-init) */
\tchar video_src_ready; /* @91 was _gap9[3]: видео-источник доступен (пары SET 1/0 при смене источника) */
\tchar ajx_active;\t    /* @92 was _gap9[4]: внешний/AJX-канал аудиопровода активен (binaRE +92 ✓) */
\tchar reserved_19;\t   /* @93 was _gap9[5]: binaRE-active (camera/tv), роль ? */
\tchar is1024screen;
\tchar reserved_20;\t   /* @95 was _gap10[0]: binaRE-active (gtp_init_panel), роль ? */
\tchar reserved_21;\t   /* @96 was _gap10[1]: binaRE-active (gtp_init_panel), роль ? */
\tchar ch_status;
\tchar u_value;\t       /* @98 was _gap101[0]: U-значение T132B (binaRE 0xCE6 ✓ = T132B_UV_Set) */
\tchar v_value;\t       /* @99 was _gap101[1]: V-значение T132B (binaRE 0xCE7 ✓) */
\tchar uv_cal;
\tchar reserved_22[32]; /* @101..132 was _gap11[32]: unused (OOB-чтение бывшего _gap9[21] @109 = [8]) */
\tchar rpt_boot_appinit;
\tchar cfg_maxvolume;
\tchar reserved_54;\t   /* @135 was _gap12[0]: binaRE-active (touch init), роль ? */
\tint touch_width;
\tint touch_height;
\tchar touch_info1;
\tchar touch_info2;
\tchar mtc_customer;
\tchar boot_flags;
\tchar reserved_55[4];  /* @148..151 was _gap13[4]: binaRE-active (ioctl/audio), роль ? */
\tchar av_gps_switch;
\tchar av_gps_monitor;
\tchar av_gps_gain;
\tchar power2_flag;\t   /* @155 was _gap14[0]: флаг «запрошено выключение питания» (ctl power2, arm 0x9529) */
\tchar mcu_cmd_state;\t /* @156 was _gap14[1]: состояние MCU-команд (SET 5/6 при boot) */
\tchar reserved_59[3];  /* @157..159 was _gap14[2..4]: unused */
};"""

assert h.count(OLD_CS) == 1, 'car_status block not found'
h = h.replace(OLD_CS, NEW_CS)

OLD_CARS = """struct mtc_car_struct {
\tstruct mtc_car_drv *car_dev;
\tstruct mtc_car_status car_status;
\tstruct mtc_car_comm *car_comm;
\tint rev_bytes_count;
\tunsigned int arm_rev_cmd;
\tchar _gap0[16];
\tunion mtc_config_data config_data;
\tchar _gap1[16];
\tstruct workqueue_struct *car_wq;
\tstruct mutex car_io_lock;
\tstruct mutex car_cmd_lock;
\tstruct timeval tv;
\tstruct delayed_work wipecheckclear_work;
\tchar mcu_version[16];
\tunsigned char mcu_date[16];
\tunsigned char mcu_time[16];
\tchar _gap2[4];
\tunsigned char ioctl_buf1[3072];
\tunsigned char buffer2[3072];
\tchar _gap3[4];
\tstruct mtc_audio_struct *audio;
};"""

NEW_CARS = """/* Layout-офсеты бинарные якоря: car_status @ +4, config_data @ +0xC0 (base 0xC168AC80).
 * reserved_<N> — N = байт-офсет от начала структуры (вычислен при binaRE-проверенных
 * размерах: mutex=24B, timer_list=32B, delayed_work=44B, timeval=8B — RK3188 32-bit). */
struct mtc_car_struct {
\tstruct mtc_car_drv *car_dev;
\tstruct mtc_car_status car_status;
\tstruct mtc_car_comm *car_comm;
\tint rev_bytes_count;
\tunsigned int arm_rev_cmd;
\tchar reserved_164[16];\t /* @0xA4 (164) was _gap0[16]: между arm_rev_cmd и config_data (binaRE: factory_test W) */
\tunion mtc_config_data config_data;
\tchar reserved_704[16];\t /* @704 was _gap1[16]: после config_data, перед car_wq */
\tstruct workqueue_struct *car_wq;
\tstruct mutex car_io_lock;
\tstruct mutex car_cmd_lock;
\tstruct timeval tv;
\tstruct delayed_work wipecheckclear_work;
\tchar mcu_version[16];
\tunsigned char mcu_date[16];
\tunsigned char mcu_time[16];
\t/* was _gap2[4] @872..875 (0x368..0x36B): первый байт — wifi-флаг (binaRE A:0x368: car_probe W /
\t * rk29sdk_wifi_power R; в коде обращался как _gap4[0], которого не было в header).
\t * Offset-вычислен из текущего layout (B9-размеры выше); байт лежит ровно в гэпе, layout не ломается. */
\tchar wifi_capable;\t   /* @0x368 (872): wifi-поддержка/GPIO (KLD, customer==4; mtc-car.c:2377/4011) */
\tchar reserved_873[3];\t /* @873..875: остаток бывшего _gap2[4] */
\tunsigned char ioctl_buf1[3072];
\tunsigned char buffer2[3072];
\tchar reserved_7020[4];\t /* was _gap3[4]: перед audio* (офсет зависит от размеров kernel-структур) */
\tstruct mtc_audio_struct *audio;
};"""

assert h.count(OLD_CARS) == 1, 'car_struct block not found'
h = h.replace(OLD_CARS, NEW_CARS)
save('mtc-car.h', h)
print('mtc-car.h: car_status + car_struct blocks replaced')

# ============================================================ 2. mtc-audio.c (struct)
a = load('mtc-audio.c')

OLD_AUD = """struct mtc_audio_struct {
\tstruct workqueue_struct *audio_wq;
\tchar gap0[4];
\tu8 audio_ch;
\tchar gap1[3];
\tchar audio_ch2;
\tchar gap11[2];
\tchar mute;
\tchar mute_all;
\tchar gap2[3];
\tstruct delayed_work *dwork;
\tchar char18;
\tu16 dword1C;
\tu16 dword20;
\tstruct timer_list timer;
\tchar gap3[20];
\tstruct mutex lock;
\tchar gap5[3];"""

NEW_AUD = """/* Имена гэп-полей по analyzer2/naming_report.md §3 (B9 byte-exact: sizeof 124, timer@32,
 * mutex@84, audio_active@117). reserved_<N> — N = байт-офсет от начала структуры. */
struct mtc_audio_struct {
\tstruct workqueue_struct *audio_wq;
\tchar reserved_4[4];\t  /* @4..7 was gap0[4]: unused */
\tu8 audio_ch;
\tchar reserved_9;\t    /* @9 was gap1[0]: unused */
\tchar vol_main;\t      /* @10 was gap1[1]: громкость основного канала 0..99 (binaRE +10 ✓) */
\tchar vol_aux;\t       /* @11 was gap1[2]: громкость второго канала (телефон/внешний) (binaRE +11 ✓) */
\tchar audio_ch2;
\tchar gap11[2];\t      /* @13..14: используются кодом (pending-unmute / AJX-направление); вне naming_report — имя сохранено */
\tchar mute;
\tchar mute_all;
\tchar reserved_17[3];\t /* @17..19 was gap2[3]: unused */
\tstruct delayed_work *dwork;
\tchar audio_src;\t     /* @24 was char18: источник аудио 0..3 (MTC_AV_CHANNEL; binaRE +24, signed <= 3) */
\tu16 reserved_26;\t    /* @26..27 was dword1C (u16): unused (байт @25 — имплицитное C-выравнивание) */
\tu16 rds_psn;\t        /* @28..29 was dword20 (u16): 16-битное RDS/PSN-значение (B:0x1C/0x1D) */
\tstruct timer_list timer;
\tchar reserved_64[20]; /* @64..83 was gap3[20]: unused (между timer и mutex) */
\tstruct mutex lock;
\tchar act_bit1;\t      /* @108 was gap5[0]: бит1 «active»-команды (декод cmd 0xE) */
\tchar act_bit3;\t      /* @109 was gap5[1]: бит3 */
\tchar act_bit2;\t      /* @110 was gap5[2]: бит2 */"""

assert a.count(OLD_AUD) == 1, 'audio block part not found'
a = a.replace(OLD_AUD, NEW_AUD.rstrip())

OLD_AUD2 = """\tchar eq1;
\tchar eq2;
\tchar eq3;
\tchar eq4;
\tchar balance1;
\tchar balance2;
\t/* binaRE B9: audio_active flag @+117 (dec audio_active/audio_deactive; asm audio_work LDRB +0x75) */
\tchar audio_active;
\t/* binaRE B9: gap6 @+118..123 (total sizeof = 124 = __memzero 0x7C в бинаре) */
\tchar gap6[6];
};"""

NEW_AUD2 = """\tchar eq1;
\tchar eq2;
\tchar eq3;
\tchar eq4;
\tchar balance1;
\tchar balance2;
\t/* binaRE B9: audio_active flag @+117 (dec audio_active/audio_deactive; asm audio_work LDRB +0x75) */
\tchar audio_active;
\t/* binaRE B9: бывшее gap6 @+118..123 (total sizeof = 124 = __memzero 0x7C в бинаре) */
\tchar mute_pin_latch;\t /* @118 was gap6[0]: latch mute-пина (разовый флаг; binaRE asm «latch mute-pin») */
\tchar active_cmd;\t    /* @119 was gap6[1]: сырое «active»-командное слово (cmd 0xE; binaRE +0x77 ✓) */
\tchar ch_sel_mode;\t   /* @120 was gap6[2]: режим выбора канала (4 → audio_ch2; binaRE dec EnterChannel) */
\tchar reserved_121[3]; /* @121..123 was gap6[3..5]: конец структуры (sizeof 124) */
};"""

assert a.count(OLD_AUD2) == 1, 'audio block part2 not found'
a = a.replace(OLD_AUD2, NEW_AUD2)
save('mtc-audio.c', a)
print('mtc-audio.c: audio struct block replaced')

# ============================================================ 3. mtc_shared.h (config union)
s = load('mtc_shared.h')

repl_cfg = [
 ('\t\tchar _gap1[7];',
  '\t\tchar ch_attr[7];\t   /* @13..19 was _gap1[7]: атрибуты каналов для Audio_ChInit (pack в 12B ch-команду) */'),
 ('\t\tchar _gap[2];',
  '\t\tchar reserved_21[2];  /* @21..22 was _gap[2]: unused */'),
 ('\t\tchar _gap3[2];',
  '\t\tchar default_ajx_ch;  /* @25 was _gap3[0]: канал после AJX-unmute (код, mtc-audio.c:745). РАСХОЖДЕНИЕ: бинарный\n\t\t * Audio_AJXChannel читает @28 (reserved_28) — layout-shift, naming_report §6.5; чинить здесь нельзя */\n\t\tchar reserved_26;\t   /* @26 was _gap3[1]: unused */'),
 ('\t\tchar _gap4[1];',
  '\t\tchar reserved_28;\t   /* @28 was _gap4[1]: бинар — Audio_AJXChannel читает AJX-канал по умолчанию СЮДА (кандидат\n\t\t * на default_ajx_ch); в ручном коде не используется */'),
 ('\t\tchar _gap5[75];',
  '\t\tchar reserved_128[75]; /* @128..202 was _gap5[75]: unused (возможная IR-область — см. ir_assign_tab) */'),
 ('\t\tchar _gap6[1];',
  '\t\tchar reserved_204;\t   /* @204 was _gap6[1]: binaRE-доступен (ADV7181D_Init), роль ? */'),
 ('\t\tchar _gap7[1];',
  '\t\tchar reserved_206;\t   /* @206 was _gap7[1]: РАСХОЖДЕНИЕ #5 — cfg_ir_assign пишет IR-таблицу отсюда\n\t\t * (mtc-car.c:2468-2479), layout блоба в бинаре другой */'),
 ('\t\tchar _gap8[120];',
  '\t\tchar ir_assign_tab[120]; /* @208..327 was _gap8[120]: ГИПОТЕЗА — таблица IR-привязок 60×16-бит (naming_report §4, low;\n\t\t * байты @288..295 бинарно доступны car_ioctl/key_beep/rk29sdk_wifi_power) */'),
 ('\t\tchar _gap9[2];',
  '\t\tchar reserved_478[2]; /* @478..479 was _gap9[2]: unused */'),
 ('\t\tchar _gap10[1];',
  '\t\tchar reserved_488;\t   /* @488 was _gap10[1]: unused */'),
 ('\t\tchar _gap11[1];',
  '\t\tchar reserved_490;\t   /* @490 was _gap11[0]: РАСХОЖДЕНИЕ #6 — код OOB-читает [0]/[1] здесь как u/v-cfg\n\t\t * (mtc-backview.c:366-367); реальные u/v в бинаре @493/494 = uv_off_u/v */'),
 ('\t\tchar _gap12[2];',
  '\t\tchar uv_off_u;\t     /* @493 was _gap12[0]: источник u_value (binaRE 0x2AD ✓ T132B_UV_Set) */\n\t\tchar uv_off_v;\t     /* @494 was _gap12[1]: источник v_value (binaRE 0x2AE ✓) */'),
 ('\t\tchar _gap13[16];',
  '\t\tchar reserved_496[16]; /* @496..511 was _gap13[16]: хвост блоба, unused */'),
]
for old, new in repl_cfg:
    n = s.count(old)
    assert n == 1, 'config line not unique/found: %r (%d)' % (old, n)
    s = s.replace(old, new)
save('mtc_shared.h', s)
print('mtc_shared.h: config union fields renamed (%d)' % len(repl_cfg))

# ============================================================ 4. per-квалификатор замены в .c
CS_QUAL = r'(?P<q>car_status->|car_struct\.car_status\.|car_struct->car_status\.|mtc_car_struct->car_status\.|c_status->|cs->)'
CS_MAP = {
 ('_gap0',0):'car_ready', ('_gap0',1):'power_refcnt',
 ('_gap1',0):'call_active',
 ('_gap2',0):'input_ready', ('_gap2',1):'audio_ready',
 ('_gap3',0):'sta_bits',
 ('_gap5',0):'cam_state', ('_gap5',3):'cam_signal',
 ('_gap8',0):'video_mode', ('_gap8',3):'decoder_state', ('_gap8',5):'radar_val',
 ('_gap81',0):'decoder_state',
 ('_gap9',0):'power_on', ('_gap9',1):'radio_rds_flag', ('_gap9',2):'reserved_18',
 ('_gap9',3):'video_src_ready', ('_gap9',4):'ajx_active', ('_gap9',21):'reserved_22[8]',
 ('_gap14',0):'power2_flag', ('_gap14',1):'mcu_cmd_state',
}
A_QUAL = r'(?P<q>car_struct\.audio->|p_mtc_audio->|audio->)'
A_MAP = {
 ('gap1',1):'vol_main', ('gap1',2):'vol_aux',
 ('gap6',0):'mute_pin_latch', ('gap6',1):'active_cmd', ('gap6',2):'ch_sel_mode',
 ('gap5',0):'act_bit1', ('gap5',1):'act_bit3', ('gap5',2):'act_bit2',
}

def sub_cs(text, label):
    def f(m):
        key = (m.group('f'), int(m.group('i')))
        if key in CS_MAP:
            count(label + ':' + m.group('f') + '[' + m.group('i') + ']', 1)
            return m.group('q') + CS_MAP[key]
        return m.group(0)
    return re.sub(CS_QUAL + r'(?P<f>_gap\d*)\[(?P<i>\d+)\]', f, text)

def sub_a(text, label):
    def f(m):
        key = (m.group('f'), int(m.group('i')))
        if key in A_MAP:
            count(label + ':' + m.group('f') + '[' + m.group('i') + ']', 1)
            return m.group('q') + A_MAP[key]
        return m.group(0)
    t = re.sub(A_QUAL + r'(?P<f>gap\d*)\[(?P<i>\d+)\]', f, text)
    n = len(re.findall(A_QUAL + r'char18\b', t))
    if n:
        t = re.sub(A_QUAL + r'char18\b', lambda m: m.group('q') + 'audio_src', t)
        count(label + ':char18', n)
    return t

def sub_cfg(text, label):
    pats = [
        (r'(car_struct\.config_data\.)_gap1\[(\d)\]', lambda m: m.group(1) + 'ch_attr[' + m.group(2) + ']'),
        (r'(car_struct\.config_data\.)_gap3\[0\]', lambda m: m.group(1) + 'default_ajx_ch'),
        (r'(car_struct\.config_data\.)_gap11\[0\]', lambda m: m.group(1) + 'reserved_490'),
        (r'(car_struct\.config_data\.)_gap11\[1\]', lambda m: m.group(1) + 'reserved_490[1]'),
        (r'(v156->)_gap7\[0\]', lambda m: m.group(1) + 'reserved_206'),
        (r'(v156->)_gap7\[1\]', lambda m: m.group(1) + 'reserved_206[1]'),
    ]
    for p, fn in pats:
        t2, n = re.subn(p, fn, text)
        if n:
            count(label + ':' + p.split('.')[0][:22] + p.split('[')[-1], n)
            text = t2
    return text

def sub_cars(text, label):
    t2, n1 = re.subn(r'(car_struct->)_gap4\[0\]', r'\g<1>wifi_capable', text)
    if n1: count(label + ':car_struct->_gap4[0]', n1)
    t3, n2 = re.subn(r'(mtc_car_struct\.)_gap4\[0\]', r'\g<1>wifi_capable', t2)
    if n2: count(label + ':mtc_car_struct._gap4[0]', n2)
    return t3

CFILES = ['mtc-car.c', 'mtc-audio.c', 'mtc-radio.c', 'mtc-dvd.c',
          'mtc-vs.c', 'mtc-tv.c', 'mtc-backview.c', 'mtc-keys.c',
          'mtc-lcd.c', 'mtc-camera.c']

for cf in CFILES:
    t = load(cf)
    orig = t
    t = sub_cs(t, cf)
    t = sub_a(t, cf)
    t = sub_cfg(t, cf)
    t = sub_cars(t, cf)
    if t != orig:
        save(cf, t)
        print('%s: saved' % cf)
    else:
        print('%s: no changes' % cf)

# ============================================================ 5. многострочные цепочки (mtc-car.c)
t = load('mtc-car.c')
orig = t
t, n = re.subn(r'^(\s*)\._gap14\[0\] = 1;', r'\g<1>.power2_flag = 1;', t, flags=re.M)
if n: count('mtc-car.c:linechain _gap14[0]', n)
t, n = re.subn(r'^(\s*)\._gap0\[0\] = v219;', r'\g<1>.car_ready = v219;', t, flags=re.M)
if n: count('mtc-car.c:linechain _gap0[0]', n)
assert t.count('power2_flag') >= 1
if t != orig: save('mtc-car.c', t)

# ============================================================ 6. mtc-backview.c: typo + OOB-комментарии
t = load('mtc-backview.c')
orig = t
n = t.count('car_structcar_status.v_value')
assert n == 1, 'typo count %d' % n
t = t.replace('car_structcar_status.v_value', 'car_struct.car_status.v_value')
count('mtc-backview.c:typo car_structcar_status fix', 1)
# OOB-комментарии (расхождение #6) на строках u/v-cfg
n = t.count('car_struct.config_data.reserved_490 + 112;')
assert n == 1
t = t.replace('car_struct.config_data.reserved_490 + 112;',
              'car_struct.config_data.reserved_490 + 112; /* OOB-расхождение #6: бинарный T132B_UV_Set читает @493 = uv_off_u */')
n = t.count('car_struct.config_data.reserved_490[1] - 124;')
assert n == 1
t = t.replace('car_struct.config_data.reserved_490[1] - 124;',
              'car_struct.config_data.reserved_490[1] - 124; /* OOB @491; бинарный источник: @494 = uv_off_v (расхождение #6) */')
if t != orig: save('mtc-backview.c', t)

# ============================================================ 7. комментарий mtc-audio.c про ветку _gap9[4]
t = load('mtc-audio.c')
orig = t
n = t.count('в ветке _gap9[4]==0')
if n == 1:
    t = t.replace('в ветке _gap9[4]==0', 'в ветке ajx_active==0')
    count('mtc-audio.c:comment ajx_active', 1)
if t != orig: save('mtc-audio.c', t)

print()
print('TOTAL substitutions:')
tot = 0
for k in sorted(stats):
    print('  %-46s %d' % (k, stats[k]))
    tot += stats[k]
print('  TOTAL', tot)
