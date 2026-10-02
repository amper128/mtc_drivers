#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_patches.py — S2a (binaRE MTC): генерация patches/{vendor,local} из BASE → T9/SDK.

Входы (read-only):
  BASE = /home/amper/Coding/RK3188/kernel_src/omegamoon/rockchip-rk3188-generic  (d2440f70)
  SDK  = /home/amper/Coding/RK3188/kernel_src/sdk_4.4.2                           (microntet df5ad97)
  T9   = /home/amper/Coding/mtc_build/t9/base                                     (r5-state: base+159 правок+mtc, валидирован)

Алгоритм (для каждого rel в U = union(diff -rq BASE SDK, diff -rq BASE T9),
за вычетом артефактов сборки ARTIFACT_PATTERNS):
  if rel in SRC_MANIFEST:                          → skip (src overlay, идёт в src/)
  elif exists(T9) and exists(SDK) and cmp(T9,SDK)==0: → patches/vendor/ (vendor дословно из SDK)
  elif exists(T9) and cmp(T9,BASE)!=0:             → patches/local/    (наша форма T9)
  elif exists(SDK) and not exists(T9):             → patches/skip_list.md (judge-кандидат, НЕ включать)
  else:                                            → skip

Формат: unified `diff -uN --label a/<rel> --label b/<rel>` (новый файл: a/<rel>=/dev/null);
ОДИН .patch на файл; имена NNNN-<slug>.patch (детерминированно: sort rel → 0001.., slug из rel).

Выходы (в mtc-kernel/patches/): vendor/*.patch, local/*.patch, skip_list.md,
SRC_MANIFEST.txt, MANIFEST.sha256 (md5+sha256 всех patch + SRC_MANIFEST.txt).

Использование: python3 scripts/gen_patches.py [--check]   (--check: не писать, только сводка)
"""
import hashlib
import os
import re
import subprocess
import sys

BASE = "/home/amper/Coding/RK3188/kernel_src/omegamoon/rockchip-rk3188-generic"
SDK = "/home/amper/Coding/RK3188/kernel_src/sdk_4.4.2"
T9 = "/home/amper/Coding/mtc_build/t9/base"
OUT = "/home/amper/Coding/mtc_build/mtc-kernel/patches"

# ----------------------------------------------------------------------------
# SRC_MANIFEST (финал, S2a). MTC-платформа: ИСКЛЮЧАЕТСЯ из patches, идёт в src/ (S2b).
# Все 40 путей подтверждены присутствием в T9 (проверка в main()).
# Отличие от исходного ТЗ: sound/soc/codecs/rk_hdmi_i2s.c — в BASE и SDK ФАЙЛА НЕТ;
# реальный MTC-файл находится в sound/soc/rk29/rk_hdmi_i2s.c (T9-only) — учтён здесь.
# ----------------------------------------------------------------------------
SRC_MANIFEST = [
    # drivers/misc/mtc/* (16)
    "drivers/misc/mtc/Kconfig",
    "drivers/misc/mtc/Makefile",
    "drivers/misc/mtc/car.c",
    "drivers/misc/mtc/car.h",
    "drivers/misc/mtc/keys.c",
    "drivers/misc/mtc/backview.c",
    "drivers/misc/mtc/lcd.c",
    "drivers/misc/mtc/radio_tef6606.c",
    "drivers/misc/mtc/shared.h",
    "drivers/misc/mtc/tv.c",
    "drivers/misc/mtc/dvd.c",
    "drivers/misc/mtc/vs.c",
    "drivers/misc/mtc/vs.h",
    "drivers/misc/mtc/codec.c",
    "drivers/misc/mtc/audio_card_glue.c",
    "drivers/misc/mtc/mtcGetSetVolume.c",
    # arch (5)
    "arch/arm/mach-rk3188/board-mtc.c",
    "arch/arm/mach-rk3188/Kconfig",
    "arch/arm/mach-rk3188/Makefile",
    "arch/arm/mach-rk3188/clock_data.c",
    "arch/arm/mach-rk3188/include/mach/iomux.h",
    # MTC-wiring в drivers/misc (2)
    "drivers/misc/Kconfig",
    "drivers/misc/Makefile",
    # video (9)
    "drivers/video/rockchip/rk_fb.c",
    "drivers/video/rockchip/Kconfig",
    "drivers/video/rockchip/Makefile",
    "drivers/video/rockchip/hdmi/chips/Kconfig",
    "drivers/video/rockchip/hdmi/chips/Makefile",
    "drivers/video/rockchip/hdmi/chips/it66121/it66121.c",
    "drivers/video/rockchip/hdmi/chips/it66121/it66121_hal.c",
    "drivers/video/rockchip/hdmi/chips/it66121/it66121.h",
    # sound (9)
    "sound/soc/rk29/rk29_wm8731.c",
    "sound/soc/rk29/Kconfig",
    "sound/soc/rk29/Makefile",
    "sound/soc/rk29/rk30_i2s.c",
    "sound/soc/codecs/Kconfig",
    "sound/soc/codecs/Makefile",
    "sound/soc/codecs/hdmi_i2s.c",
    "sound/soc/rk29/rk_hdmi_i2s.c",  # см. примечание выше (не sound/soc/codecs/)
    # misc (1)
    "kernel/timeconst.pl",
]

# ----------------------------------------------------------------------------
# Артефакты сборки — ИСКЛЮЧАТЬ из U. Первая группа — список из ТЗ S2a,
# вторая — документированное расширение (только build-выход/мета, не исходники).
# ----------------------------------------------------------------------------
ARTIFACT_PATTERNS = [
    r"\.o$",
    r"\.cmd$",
    r"^\.config$", r"^\.config\.old$",
    r"^System\.map$", r"^vmlinux.*",
    r"^include/generated/", r"^include/config/",
    r"^include/trace/events/",                       # tracepoints — генерация при сборке
    r"(^|/)asm-offsets\.s$",
    r"(^|/)version\.h$", r"(^|/)bounds\.s$",
    r"(^|/)timeconst\.h$",
    r"(^|/)crc32table\.(c|h)$",
    r"^drivers/video/logo/[^/]*_clut224\.c$",
    r"(^|/)logo_bmp\.c$",
    r"(^|/)defkeymap\.c$",
    # расширение (build-выход/мета, не исходники):
    r"\.mod$|\.ko$|\.mod\.c$",
    r"^\.git/",
    r"^\..*\.(cmd|o|S)$", r"^\.\.?tmp_", r"^\.version$", r"^\.vmlinux\.",
    r"^\.missing-syscalls\.d$",
    r"(^|/)Module\.symvers$", r"(^|/)modules\.order$", r"(^|/)modules\.builtin$",
    r"(^|/)\.tmp_versions$",
    r"\.a$",
    r"\.cpio$", r"(^|/)\.initramfs",
    r"^drivers/video/logo/[^/]*_(vga16|mono|bmp)\.c$",
    r"(^|/)gen_crc32table$", r"(^|/)gen_init_cpio$", r"(^|/)conmakehash$",
]
ART_RX = [re.compile(p) for p in ARTIFACT_PATTERNS]


def is_artifact(p: str) -> bool:
    return any(rx.search(p) for rx in ART_RX)


def _unquote(s: str) -> str:
    # diff -rq оборачивает имена с пробелами в кавычки
    if len(s) >= 2 and s[0] == s[-1] and s[0] in "\"'":
        return s[1:-1]
    return s


def diff_union(a: str, b: str) -> set:
    """rel-пути, отличающиеся между деревьями a и b (diff -rq)."""
    aR, bR = os.path.realpath(a), os.path.realpath(b)
    out = subprocess.run(["diff", "-rq", a, b], capture_output=True, text=True,
                         timeout=900).stdout
    res = set()
    for line in out.splitlines():
        m = re.match(r"^Only in (.*): (.+)$", line)
        if m:
            d, f = _unquote(m.group(1)), _unquote(m.group(2))
            p = os.path.join(d, f)
            if os.path.isdir(p):
                continue  # файловая деталь придёт отдельными строками
            root = aR if (os.path.realpath(d) == aR or
                          os.path.realpath(d).startswith(aR + os.sep)) else bR
            res.add(os.path.relpath(p, root))
            continue
        m = re.match(r"^Files (.+) and (.+) differ$", line)
        if m:
            res.add(os.path.relpath(_unquote(m.group(1)), aR))
    return res


def is_binary(path: str) -> bool:
    try:
        with open(path, "rb") as f:
            return b"\0" in f.read(8192)
    except OSError:
        return False


def classify():
    """-> (vendor, local, skip) — отсортированные списки rel."""
    bs = diff_union(BASE, SDK)
    ts = diff_union(BASE, T9)
    u = (bs | ts) - {p for p in (bs | ts) if is_artifact(p)}
    src_set = set(SRC_MANIFEST)
    vendor, local, skip = [], [], []
    for p in sorted(u):
        if p in src_set:
            continue  # src overlay (S2b)
        t9e, sde = os.path.exists(os.path.join(T9, p)), os.path.exists(os.path.join(SDK, p))
        t9b = os.path.join(T9, p)
        if t9e and is_binary(t9b):
            continue  # в T9 вместо исходника ELF (kbuild перекомпилировал на месте:
                      # scripts/basic/fixdep и т.п.) → артефакт сборки, не патчим
        if t9e and sde and open(t9b, "rb").read() == \
                open(os.path.join(SDK, p), "rb").read():
            vendor.append(p)          # vendor: дословно из SDK
        elif t9e and (not os.path.exists(os.path.join(BASE, p)) or
                      open(t9b, "rb").read() != open(os.path.join(BASE, p), "rb").read()):
            local.append(p)           # локальная форма T9 (T9 != BASE)
        elif sde and not t9e:         # только в SDK, нет в T9 → judge-кандидат
            skip.append(p)
        # else: T9 == BASE, а отличие лишь в SDK → тихий skip по ТЗ
    return vendor, local, skip, sorted(src_set & u), len(u)


def slug(rel: str) -> str:
    s = re.sub(r"[^A-Za-z0-9]+", "-", rel).strip("-")
    return s[:60] or "unnamed"


def make_patch(rel: str, new_file: str) -> bytes:
    """unified diff BASE→new_file (bytes — исходники могут быть cp1251/иным);
    новый файл: a/<rel> = /dev/null."""
    old = os.path.join(BASE, rel) if os.path.exists(os.path.join(BASE, rel)) else "/dev/null"
    for f in (old, new_file):
        if f != "/dev/null" and is_binary(f):
            raise RuntimeError("binary file in patch set: %s" % f)
    cmd = ["diff", "-uN", "--label", "a/" + rel, "--label", "b/" + rel, old, new_file]
    r = subprocess.run(cmd, capture_output=True)
    if r.returncode not in (0, 1):
        raise RuntimeError("diff failed for %s: %s" % (rel, r.stderr.decode("utf-8", "replace").strip()))
    if r.returncode == 0:
        raise RuntimeError("empty diff (no changes) for %s" % rel)
    return r.stdout


def main():
    check_only = "--check" in sys.argv
    vendor, local, skip, src_in_union, n_clean = classify()

    # --- проверки ---
    missing = [p for p in SRC_MANIFEST if not os.path.exists(os.path.join(T9, p))]
    if missing:
        raise SystemExit("SRC_MANIFEST missing in T9: %s" % missing)
    vset, lset = set(vendor), set(local)
    overlap = (vset & lset) | (vset & set(skip)) | (lset & set(skip)) \
        | ((vset | lset | set(skip)) & set(SRC_MANIFEST))
    if overlap:
        raise SystemExit("category overlap: %s" % sorted(overlap)[:10])

    print("U (clean, без артефактов): %d" % n_clean)
    print("SRC_MANIFEST в union (src overlay, из patches исключены): %d" % len(src_in_union))
    print("vendor: %d | local: %d | skip_list: %d" % (len(vendor), len(local), len(skip)))

    if check_only:
        return

    os.makedirs(OUT + "/vendor", exist_ok=True)
    os.makedirs(OUT + "/local", exist_ok=True)
    # чистка старых генераций (только NNNN-*.patch, чтобы не убить .gitkeep)
    for d in ("vendor", "local"):
        for f in os.listdir(OUT + "/" + d):
            if re.match(r"^\d{4}-.*\.patch$", f):
                os.remove(OUT + "/" + d + "/" + f)

    def emit(dirname, rels, source_root):
        files = []
        used = set()
        for i, rel in enumerate(rels, 1):
            name = "%04d-%s.patch" % (i, slug(rel))
            k = 2
            while name in used:  # защита от коллизий slug (детерминированно)
                name = "%04d-%s-%d.patch" % (i, slug(rel), k)
                k += 1
            used.add(name)
            patch = make_patch(rel, os.path.join(source_root, rel))
            with open(OUT + "/" + dirname + "/" + name, "wb") as f:
                f.write(patch)
            files.append((dirname + "/" + name, rel))
        return files

    vfiles = emit("vendor", vendor, SDK)   # vendor дословно из SDK (== T9 по определению)
    lfiles = emit("local", local, T9)      # локальная форма T9

    # --- skip_list.md ---
    with open(OUT + "/skip_list.md", "w", encoding="utf-8", newline="\n") as f:
        f.write("# skip_list (S2a) — judge-кандидаты\n\n")
        f.write("Файлы, существующие в SDK, но ОТСУТСТВУЮЩИЕ в T9 (не в BASE — ")
        f.write("иначе они попали бы в vendor/local). НЕ включаются в patches; ")
        f.write("решение о включении — за judge (S2+).\n\n")
        f.write("Всего: %d\n\n" % len(skip))
        f.write("| rel-путь |\n|---|\n")
        for p in skip:
            f.write("| %s |\n" % p.replace("|", "\\|"))

    # --- SRC_MANIFEST.txt (для S2b + хэш в MANIFEST.sha256) ---
    with open(OUT + "/SRC_MANIFEST.txt", "w", encoding="utf-8", newline="\n") as f:
        f.write("# SRC_MANIFEST (S2a финал): MTC-платформа, идёт в src/ (S2b), ")
        f.write("исключена из patches\n")
        for p in sorted(set(SRC_MANIFEST)):
            f.write(p + "\n")

    # --- MANIFEST.sha256 ---
    def digests(path):
        h = hashlib.sha256()
        m = hashlib.md5()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(1 << 20), b""):
                h.update(chunk)
                m.update(chunk)
        return h.hexdigest(), m.hexdigest()

    all_patches = sorted([n for n, _ in vfiles] + [n for n, _ in lfiles])
    with open(OUT + "/MANIFEST.sha256", "w", encoding="utf-8", newline="\n") as f:
        f.write("# MANIFEST.sha256 (S2a): md5/sha256 всех patch + src-manifest\n")
        f.write("# <sha256>  <rel-путь в patches/>\n")
        for name in all_patches:
            sh, _ = digests(OUT + "/" + name)
            f.write("%s  %s\n" % (sh, name))
        sh, _ = digests(OUT + "/SRC_MANIFEST.txt")
        f.write("%s  SRC_MANIFEST.txt\n" % sh)
        f.write("# md5:\n")
        for name in all_patches:
            _, md = digests(OUT + "/" + name)
            f.write("%s  %s\n" % (md, name))
        _, md = digests(OUT + "/SRC_MANIFEST.txt")
        f.write("%s  SRC_MANIFEST.txt\n" % md)

    print("written: %d vendor + %d local patches, skip_list.md (%d), "
          "SRC_MANIFEST.txt, MANIFEST.sha256 → %s"
          % (len(vfiles), len(lfiles), len(skip), OUT))


if __name__ == "__main__":
    main()
