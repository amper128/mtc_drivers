# T9 fix deviations from donor 1:1 (binaRE MTC, RK3188)

## C3 rk30_i2s — hdmi-API adaptation (fix 28a-b)
ref_kernel `sound/soc/rk29/rk30_i2s.c` was ported but the base tree's
`drivers/video/rockchip/hdmi/rk_hdmi.h` is an OLDER revision that lacks
`hdmi_config_audio()` (it lives in the new hdmi-core, not present in base).
To keep the carefully-built MTC it66121/cat66121 HDMI stack intact, rk30_i2s.c
was adapted (NOT a full hdmi-core port):
- `include/sound/pcm_params.h`: added `#define HW_PARAMS_FLAG_NLPCM 1`.
- `rk_hdmi.h`: `enum hdmi_audio_type` += `HDMI_AUDIO_NLPCM=0, HDMI_AUDIO_LPCM=1` (matches ref:131).
- `rk30_i2s.c`: call `hdmi_config_audio(..)` -> `rk30_hdmi_config_audio(..)` (extern in base chips/rk30/rk30_hdmi_hw.h). struct hdmi_audio identical in both headers.
- `rk29_i2s.h`: added I2S_CLR block (md5 == ref 1:1).
Result: md5 rk30_i2s.c 638d6947(ref) -> 3b95f267 (deviation, documented). Build RC=0, i2s symbols present.

## gtp/touch calibration cascade (t6b) — TENTATIVE bodies
`TouchPanelSetCalibration`/`LargeNum*`/`ComputeMatrix33*`/`ErrorAnalysis*`
transcribed as GLOBAL with correct judge names, but the deep math cascade is a
no-op stub (return 0 / known constants) — byte-1:1 of those bodies deferred.
Symbol presence closed; exact bodies need a dedicated calibration port.

## rk29_i2s.h / iomux.h — OMEGAMOON preserved
iomux.h NOT cp'd 1:1 (would break OMEGAMOON block + fix-12 HDMI macros); only
I2S0_MCLK/I2S0_LRCKRX aliases appended.
