# mtc_drivers
Researching kernel for KLD on RK3188.

It seems that the kernel assembly is based on the source code of RK3188 SDK,
in particular, there are many matches with mach-rk3188/board-rk3188-sdk.c.
Obviously, the developers have created a new device, but I think that now
it is reasonable to take these source codes as a basis.

## Reference kernel

The reference kernel (the base on top of which the driver pack in this repo is layered) is
https://github.com/omegamoon/rockchip-rk3188-generic — HEAD d2440f70 (2013-06-16), Linux 3.0.36+
(Makefile: VERSION=3 PATCHLEVEL=0 SUBLEVEL=36 EXTRAVERSION=+), Rikomagic RK3188 line; common rk3x vendor base.
Mali-400/UMP sources also come from this repo (drivers/gpu/mali/, ARM reference r3p2-01rel1, API_VERSION=20).
Caveat: the kernel actually built for vmlinux is still the SDK kernel 3.0.36+
(/home/amper/Coding/mtc_build/ref_kernel, scripts/apply_overlay.sh) — same version/line;
a full build-base switch to the omegamoon repo is a separate, not-yet-done task.
Details: REPORTS/reference_repo.md.
