# mtc-kernel

Overlay-repo для binaRE: ядро RK3188 (omegamoon rockchip-rk3188-generic, base 3.0.36+) + MTC-платформа.

## Структура

- `ref_kernel/` — git submodule, base kernel (omegamoon/rockchip-rk3188-generic, зафиксирован на d2440f70)
- `patches/vendor/` — патчи на base-kernel (vendor)
- `patches/local/` — патчи на base-kernel (local)
- `src/` — MTC-платформа (overlay)
- `scripts/` — сборка

## Сборка

    git clone --recurse-submodules <this-repo> mtc-kernel
    cd mtc-kernel
    scripts/build.sh
