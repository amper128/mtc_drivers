#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
completeness_scan.py — S4-fix-6, S4-fix-8: COMPLETENESS-SCAN + RE-VERIFY (binaRE MTC).

S4-fix-8: WORKING-TREE режим — T9 перечисляется по РАБОЧЕМУ ДЕРЕВУ (os.walk ==
`find T9 -type f`), НЕ `git ls-files`: UNTRACKED vendor-файлы видны.
Artifact-filter точный: исключаются ТОЛЬКО build-artifacts (*.o,*.cmd,*.mod,*.ko,
*.mod.c, .tmp_*, .vmlinux*, include/generated/**, include/config/**, System.map,
vmlinux*, .scmversion, .version, defkeymap.c, version.h, asm-offsets.s, bounds.s,
.missing-syscalls.d + host-ELF). PRE-GENERATED logo *.c без source-изображения
(.ppm/.pbm/.pgm/.bmp в T9 И BASE — его kbuild НЕ регенерирует) = NECESSARY SOURCE
(пример: logo_linux_800x480_clut224.c, .ppm в skip_list).

Сравнивает ПЕРВОНАЧАЛЬНЫЙ T9 (real-source, минус артефакты сборки) с
реконструкцией BASE + patches/{vendor,local} + src/ (overlay).

Режимы:
  (по умолчанию)  — быстрый scan: для каждого real-source файла T9 проверить,
                    что он воспроизведён (BASE==T9 ИЛИ patch ИЛИ src); печать GAP.
  --full         — RE-VERIFY: чистая копия BASE → --dir (default /tmp/mtc_complete_test),
                    apply ВСЕ patches (vendor → local, `patch -p1 -N -f`),
                    overlay `cp -a src/. .`, затем diff-tree vs T9 по real-source:
                    каждый real-source файл T9 == результат (cmp, 0 различий).
                    Дополнительно: it66121/ полный (18 файлов, Kconfig+Makefile present),
                    reverse-check (файлы результата без соответствия в T9 — ожидаемо пусто
                    за исключением build-scaffolding src/config/, MANIFEST.md5, .gitkeep).

Использование: python3 scripts/completeness_scan.py [--full] [--dir DIR]
Выход 0 = GAP 0 (scan) / 0 diff (full).
"""
import argparse, filecmp, hashlib, os, shutil, subprocess, sys

BASE = "/home/amper/Coding/RK3188/kernel_src/omegamoon/rockchip-rk3188-generic"
T9   = "/home/amper/Coding/mtc_build/t9/base"
ROOT = "/home/amper/Coding/mtc_build/mtc-kernel"
SRC  = ROOT + "/src"

# ---------------------------------------------------------------- артефакты
# Исключаются из сравнения (генерация сборки T9):
def is_artifact(rel, full):
    b = os.path.basename(rel)
    if any(b.endswith(x) for x in (".o", ".cmd", ".mod", ".ko", ".mod.c", ".a")): return True  # *.o,*.cmd,*.mod,*.ko,*.mod.c, arch-libs
    if b in ("modules.order", "modules.builtin"): return True                                   # kbuild-generated
    if b.startswith(".tmp_") or b.startswith(".vmlinux"): return True
    if b in ("System.map", "Module.symvers", "defkeymap.c", "version.h", "bounds.s",
             "timeconst.h", "crc32table.c", "crc32table.h", ".scmversion", ".version",
             ".missing-syscalls.d", "asm-offsets.s"): return True
    if b.startswith("vmlinux") or b.startswith(".config") or b.endswith(".config"): return True
    if rel.startswith("usr/") or rel == "lib/lib.a": return True                                # initramfs/out
    if "include/generated/" in rel or "include/config/" in rel: return True
    # drivers/video/logo: ТΟЧНОЕ правило (S4-fix-8) — .c исключается (build-artifact)
    # ТОЛЬКО если kbuild может его регенерировать, т.е. source-изображение того же
    # stem (.ppm/.pbm/.pgm/.bmp) есть в T9 ИЛИ в BASE (правила Makefile %_mono/%_vga16/
    # %_clut224/%_gray256/%_bmp). PRE-GENERATED .c без image (image в skip_list) —
    # NECESSARY SOURCE, НЕ исключается: logo_linux_800x480_clut224.c. logo.c — handwritten.
    if rel.startswith("drivers/video/logo/") and b.endswith(".c"):
        if b == "logo.c": return False
        stem = b[:-2]
        img = ("drivers/video/logo/%s." % stem)
        for ext in ("ppm", "pbm", "pgm", "bmp"):
            if os.path.exists(os.path.join(T9, img + ext)) or \
               os.path.exists(os.path.join(BASE, img + ext)):
                return True   # регенерируется kbuild из image
        return False          # pre-generated source (image отсутствует) — necessary
    # host-ELF утилиты (gen_crc32table, gen_init_cpio, pnmtologo-build и т.п.)
    try:
        with open(full, "rb") as f:
            if f.read(4) == b"\x7fELF": return True
    except OSError: pass
    return False

# репо-шум / scratch — не source (отдельно учитываются, в diff не участвуют)
NOISE_PREFIX = (".git/", ".work/")

def real_source_tree(t9):
    out = []
    for dp, dn, fn in os.walk(t9):
        for f in fn:
            full = os.path.join(dp, f); rel = os.path.relpath(full, t9)
            if rel.startswith(NOISE_PREFIX): continue
            if not is_artifact(rel, full): out.append(rel)
    return sorted(out)

def patch_rels():
    pr = {}
    for sub in ("vendor", "local"):
        d = os.path.join(ROOT, "patches", sub)
        for p in sorted(os.listdir(d)):
            if not p.endswith(".patch"): continue
            for line in open(os.path.join(d, p), "rb"):
                if line.startswith(b"+++ "):
                    r = line[4:].strip().decode("utf-8", "replace").lstrip("/")
                    if r.startswith(("a/", "b/")): r = r[2:]
                    pr[r] = "%s/%s" % (sub, p); break
    return pr

def src_files():
    out = set()
    for dp, dn, fn in os.walk(SRC):
        for f in fn:
            full = os.path.join(dp, f)
            if os.path.isfile(full): out.add(os.path.relpath(full, SRC))
    return out

def manifest_rels():
    return [l.strip() for l in open(ROOT + "/patches/SRC_MANIFEST.txt")
            if l.strip() and not l.startswith("#")]

# ---------------------------------------------------------------- scan
def do_scan():
    t9 = real_source_tree(T9)
    pr, sf, mf = patch_rels(), src_files(), manifest_rels()
    gaps = []
    for rel in t9:
        if rel in sf: continue                      # src overlay (авторитетен)
        if rel in pr: continue                      # patch
        bp, tp = os.path.join(BASE, rel), os.path.join(T9, rel)
        if os.path.exists(bp):
            if not filecmp.cmp(tp, bp, shallow=False):
                gaps.append((rel, "DIFF-vs-BASE без patch/src"))
        else:
            gaps.append((rel, "NEW в T9 — нет в BASE/patches/src"))
    print("T9 real-source: %d | patches: %d | src: %d (manifest %d)"
          % (len(t9), len(pr), len(sf), len(mf)))
    extra = [s for s in sf if s not in mf and s not in ("MANIFEST.md5", ".gitkeep")
             and not s.startswith("config/")]
    if extra: print("WARN src-файлы вне manifest: %s" % sorted(extra))
    print("GAP: %d" % len(gaps))
    for rel, why in gaps: print("  GAP %s :: %s" % (rel, why))
    return 0 if not gaps else 1

# ---------------------------------------------------------------- full re-verify
def apply_all(test):
    for sub in ("vendor", "local"):
        d = os.path.join(ROOT, "patches", sub)
        for p in sorted(f for f in os.listdir(d) if f.endswith(".patch")):
            r = subprocess.run(["patch", "-p1", "-N", "-f"], cwd=test,
                               stdin=open(os.path.join(d, p), "rb"),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if r.returncode != 0 or b"FAILED" in r.stderr or b"rejected" in r.stderr:
                sys.exit("PATCH FAILED: %s\n%s" % (p, r.stderr.decode()[-2000:]))

def do_full(dir_):
    if os.path.exists(dir_): shutil.rmtree(dir_)
    subprocess.run(["cp", "-a", BASE, dir_], check=True)
    apply_all(dir_)
    subprocess.run(["cp", "-a", SRC + "/.", dir_ + "/"], check=True)

    t9 = real_source_tree(T9)
    missing, diff = [], []
    for rel in t9:
        tp, rp = os.path.join(T9, rel), os.path.join(dir_, rel)
        if not os.path.exists(rp): missing.append(rel); continue
        if not filecmp.cmp(tp, rp, shallow=False): diff.append(rel)
    print("RE-VERIFY: %d real-source files T9 vs %s" % (len(t9), dir_))
    print("  missing in result: %d" % len(missing))
    for r in missing[:50]: print("    MISSING " + r)
    print("  content diff:      %d" % len(diff))
    for r in diff[:50]: print("    DIFF " + r)

    # it66121: полный набор (18 файлов), Kconfig+Makefile present
    it = sorted(r for r in t9 if r.startswith("drivers/video/rockchip/hdmi/chips/it66121/"))
    ok_it = all(os.path.exists(os.path.join(dir_, r)) and
                filecmp.cmp(os.path.join(T9, r), os.path.join(dir_, r), shallow=False)
                for r in it)
    have_km = all(os.path.exists(os.path.join(dir_, "drivers/video/rockchip/hdmi/chips/it66121/" + x))
                  for x in ("Kconfig", "Makefile"))
    print("  it66121/: %d files, all==T9: %s, Kconfig+Makefile present: %s"
          % (len(it), ok_it, have_km))

    # reverse: файлы результата (real-source) без соответствия в T9
    t9set = set(t9)
    extra = []
    for dp, dn, fn in os.walk(dir_):
        for f in fn:
            full = os.path.join(dp, f); rel = os.path.relpath(full, dir_)
            if rel.startswith(NOISE_PREFIX) or is_artifact(rel, full): continue
            if rel not in t9set: extra.append(rel)
    extra = [r for r in sorted(extra) if r not in
             ("MANIFEST.md5", ".gitkeep", "config/.config", "config/mtc_defconfig")]
    print("  reverse (result-only, минус scaffolding): %d" % len(extra))
    for r in extra[:50]: print("    EXTRA " + r)

    rc = 1 if (missing or diff or not ok_it or not have_km or extra) else 0
    print("RE-VERIFY: %s" % ("PASS (0 diff)" if rc == 0 else "FAIL"))
    return rc

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--full", action="store_true")
    ap.add_argument("--dir", default="/tmp/mtc_complete_test")
    a = ap.parse_args()
    sys.exit(do_full(a.dir) if a.full else do_scan())
