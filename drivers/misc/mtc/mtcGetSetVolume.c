/* binaRE mtcGetSetVolume @0xc0833960 (132B): карта громкости по частоте (10000*a1).
 * Транскрипция 1:1 из src_all/decompiled_mtcGetSetVolume.c (константы точные). */

#include "shared.h"
#include "car.h"

int mtcGetSetVolume(int a1)
{
	int v1 = 10000 * a1;
	int v4;

	if (v1 <= 299999) {
		/* binaRE: (u64)(2863311532LL * (v1 + 10000)) >> 32 */
		v4 = (int)(((unsigned long long)2863311532ULL * (unsigned long long)(v1 + 10000)) >> 32);
	} else if (v1 <= 599999) {
		v4 = v1 - 89088;
		v4 = (v4 & ~0xFF) | ((v4 + 112) & 0xFF); /* binaRE: LOBYTE(v4) = v4 + 112 */
	} else {
		v4 = ((50000 * a1) - 950000) >> 2;
	}
	return (unsigned char)((char)(v4 * car_struct.car_status.cfg_maxvolume) / 64); /* binaRE 0xC168AD0A = car_status.cfg_maxvolume (+134) */
}
