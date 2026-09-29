/*
 * binaRE rk29_wm8731 @0xc0862494, rk29_hw_params @0xc08624ac,
 * audio_card_init @0xc0421ec4 (IDA 9.3 decompiled)
 *
 * T3: RK3188 + Wolfson WM8731 machine driver, recovered from MTC binary.
 * initcall __initcall_audio_card_init7 => level 7 = late_initcall
 * (SDK include/linux/init.h L209).
 *
 * dai_link fields exactly per ELF data dump c0bd4158; card layout inferred
 * from SDK template sound/soc/rk29/rk29_wm8900.c (>= 2.6.37:
 * {.name,.dai_link,.num_links}); card data block off c0bd4004 (drvdata).
 *
 * NOT duplicated here (separate TUs / out of this overlay):
 *  - HDMI card "RK-HDMI-I2S": audio_card_init level 6 @0xc0421de8.
 *  - WM8731 codec glue: wm8731_probe @0xc0860fb8, wm8731 i2c_driver modinit
 *    @0xc0421d88 live in drivers/misc/mtc/codec.c; the i2c_driver itself is
 *    NOT part of this overlay. Codec driver is from SDK: sound/soc/codecs/
 *    wm8731.c contains wm8731_probe (grep-verified).
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>

/* binaRE rk29_wm8731_init @0xc0862494 (IDA 9.3 decompiled, 24 bytes) */
static int rk29_wm8731_init(struct snd_soc_codec *codec)
{
	printk("<1>--mtc %s(%d)\n", "rk29_wm8731_init", 101);
	return 0;
}

/*
 * binaRE rk29_hw_params @0xc08624ac (IDA 9.3 decompiled, 152 bytes)
 *
 * asm: rtd = substream->private_data; DAI @ rtd+0x170 = codec_dai,
 * DAI @ rtd+0x174 = cpu_dai (offsets verified against SDK soc.h).
 * EQVOL branch calls codec_dai->driver(+0x10)->ops(+0x1c)->hw_params(+0x28)
 * (substream, params, codec_dai). HW_PARAMS_FLAG_EQVOL_ON/OFF = 0x21/0x22
 * per SDK rk29_wm8900.c L52-54 (same function family, "by Vincent").
 */
#define HW_PARAMS_FLAG_EQVOL_ON		0x21
#define HW_PARAMS_FLAG_EQVOL_OFF	0x22

static int rk29_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	/* decompiled: *(u32 *)params (first word of hw_params) */
	u32 flag = *(u32 *)params;
	int ret;

	printk("<1>--mtc %s(%d) %x\n", "rk29_hw_params", 43, flag);

	if ((flag == HW_PARAMS_FLAG_EQVOL_ON) ||
	    (flag == HW_PARAMS_FLAG_EQVOL_OFF)) {
		rtd->codec_dai->driver->ops->hw_params(substream, params,
						       rtd->codec_dai);
		printk("<1>--mtc %s(%d)\n", "rk29_hw_params", 52);
		return 0;
	}

	/* binaRE: binary set_fmt 1-arg, no dai->fmt write; fmt per SDK rk29 convention */
	ret = snd_soc_dai_set_fmt(rtd->codec_dai, SND_SOC_DAIFMT_CBM_CFM); /* asm: rtd+0x170 first */
	if (ret >= 0)
		ret = snd_soc_dai_set_fmt(rtd->cpu_dai, SND_SOC_DAIFMT_CBM_CFM); /* asm: rtd+0x174 2nd */
	return ret; /* decompiled "v8 & (v8>>31)" = sign pass-through (IDA noise) */
}

static const struct snd_soc_dai_ops rk29_wm8731_ops = {
	.hw_params = rk29_hw_params,
};

/* dai_link exactly per ELF data dump c0bd4158 */
static struct snd_soc_dai_link rk29_wm8731_dai = {
	.name = "wm8731",
	.stream_name = "wm8731 PCM",
	.platform_name = "rockchip-audio",
	.cpu_dai_name = "rk29_i2s.1",
	.codec_name = "wm8731.0-001c",
	.codec_dai_name = "wm8731-hifi",
	.init = rk29_wm8731_init,
	.ops = &rk29_wm8731_ops,
};

/* card layout inferred from SDK template (rk29_wm8900.c); card data @off c0bd4004 */
static struct snd_soc_card rk29_wm8731_card = {
	.name = "RK29_WM8731",
	.dai_link = &rk29_wm8731_dai,
	.num_links = 1,
};

/* binaRE audio_card_init @0xc0421ec4 (IDA 9.3 decompiled) */
static struct platform_device *soc_audio_pdev; /* BSS @c168ee38 */

static int __init audio_card_init(void)
{
	int ret;

	printk("<1>--mtc %s(%d)\n", "audio_card_init", 164);

	soc_audio_pdev = platform_device_alloc("soc-audio", -1);
	if (!soc_audio_pdev) {
		ret = -ENOMEM;
		printk("<1>--mtc platform device allocation failed\n\n");
		return ret;
	}

	dev_set_drvdata(&soc_audio_pdev->dev, &rk29_wm8731_card);

	ret = platform_device_add(soc_audio_pdev);
	if (ret) {
		printk("<1>--mtc platform device add failed\n\n");
		platform_device_put(soc_audio_pdev);
	}

	return ret;
}

late_initcall(audio_card_init); /* __initcall_audio_card_init7 (level 7) */
