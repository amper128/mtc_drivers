#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
verify_patches.py — S2a верификация (обязательный шаг):

1. Копия BASE → /tmp/mtc_patch_test (cp -a).
2. Per-patch: `patch -p1 -N -f --dry-run` — ожидаем 0 rejects (порядок: vendor/*, local/*).
3. Реальный apply ВСЕХ patches/vendor/*.patch, потом patches/local/*.patch
   через `patch -p1 -N -f` — ожидаем 0 rejects (никаких *.rej / FAILED / hunk ... rejected).
4. Assert: vendor ∩ local ∩ SRC_MANIFEST = ∅ (нет rel в 2+ категориях).
5. Сравнение apply-результата vs T9: для каждого rel из vendor∪local — `cmp`
   (байт-в-байт). Отдельно выводим список расхождений (ожидание: пусто).

Выход 0 = успех; печать сводки в stdout.
"""
import hashlib
import os
import re
import subprocess
import sys

ROOT = "/home/amper/Coding/mtc_build/mtc-kernel"
T9 = "/home/amper/Coding/mtc_build/t9/base"
B = "/home/amper/Coding/RK3188/kernel_src/omegamoon/rockchip-rk3188-generic"
TEST = "/tmp/mtc_patch_test"
PATCHDIR = ROOT + "/patches"

failures = []


def sh(cmd, cwd=None, inp=None):
    return subprocess.run(cmd, cwd=cwd, input=inp, capture_output=True)


def patch_rel(p):
    """rel из +++ строки patch-файла."""
    for line in open(p, "rb"):
        if line.startswith(b"+++ "):
            rel = line[4:].strip().decode("utf-8", "replace").lstrip("/")
            return rel[2:] if rel.startswith(("a/", "b/")) else rel
    raise RuntimeError("no +++ header in %s" % p)


def main():
    print("== 0) prep: cp -a BASE → %s" % TEST)
    subprocess.run(["rm", "-rf", TEST], check=True)
    r = sh(["cp", "-a", B, TEST])
    if r.returncode != 0:
        sys.exit("cp -a failed: %s" % r.stderr.decode())

    vendor = sorted(PATCHDIR + "/vendor/" + f for f in os.listdir(PATCHDIR + "/vendor")
                    if f.endswith(".patch"))
    local = sorted(PATCHDIR + "/local/" + f for f in os.listdir(PATCHDIR + "/local")
                   if f.endswith(".patch"))
    allp = vendor + local
    print("patches: vendor=%d local=%d total=%d" % (len(vendor), len(local), len(allp)))

    # 4) Assert: vendor ∩ local ∩ SRC_MANIFEST = ∅
    vrel, lrel = set(), set()
    for p in vendor: vrel.add(patch_rel(p))
    for p in local: lrel.add(patch_rel(p))
    srcman = set()
    with open(PATCHDIR + "/SRC_MANIFEST.txt", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                srcman.add(line)
    overlap = (vrel & lrel) | (vrel & srcman) | (lrel & srcman)
    if overlap:
        failures.append("overlap vendor∩local∩SRC_MANIFEST: %s" % sorted(overlap)[:10])
    else:
        print("== 4) ASSERT vendor∩local∩SRC_MANIFEST = ∅  — OK (%d+%d rel)"
              % (len(vrel), len(lrel)))

    # 2) per-patch dry-run (порядок vendor→local, без применения)
    print("== 2) per-patch dry-run (patch -p1 -N -f --dry-run)")
    dry_bad = []
    for i, p in enumerate(allp, 1):
        r = sh(["patch", "-p1", "-N", "-f", "--dry-run"], cwd=TEST, inp=open(p, "rb").read())
        out = (r.stdout + r.stderr).decode("utf-8", "replace")
        if r.returncode != 0 or "rejected" in out or "FAILED" in out \
                or "previously applied" in out:
            dry_bad.append((p, out.strip().splitlines()[-3:]))
    if dry_bad:
        for p, tail in dry_bad[:10]:
            print("  DRY-RUN FAIL:", p)
            for l in tail: print("    ", l)
        failures.append("dry-run rejects: %d/%d" % (len(dry_bad), len(allp)))
    else:
        print("   dry-run: %d/%d patch, 0 rejects — OK" % (len(allp), len(allp)))

    # 3) реальный apply (vendor → local)
    print("== 3) apply: patch -p1 -N -f (vendor → local)")
    apply_bad = []
    for p in allp:
        r = sh(["patch", "-p1", "-N", "-f"], cwd=TEST, inp=open(p, "rb").read())
        out = (r.stdout + r.stderr).decode("utf-8", "replace")
        if r.returncode != 0 or "rejected" in out or "FAILED" in out:
            apply_bad.append((p, out.strip().splitlines()[-3:]))
    rej = subprocess.run(["bash", "-c", "find %s -name '*.rej' | wc -l" % TEST],
                         capture_output=True, text=True).stdout.strip()
    print("   *.rej files in test tree: %s" % rej)
    if apply_bad:
        for p, tail in apply_bad[:10]:
            print("  APPLY FAIL:", p)
            for l in tail: print("    ", l)
        failures.append("apply rejects: %d/%d; *.rej=%s" % (len(apply_bad), len(allp), rej))
    elif rej != "0":
        failures.append("unexpected *.rej=%s" % rej)
    else:
        print("   apply: %d/%d patch, 0 rejects, 0 *.rej — OK" % (len(allp), len(allp)))

    # 5) cmp apply-результат vs T9 по всем rel vendor∪local
    print("== 5) cmp TEST vs T9 по vendor∪local (%d rel)" % len(vrel | lrel))
    mism = []
    for rel in sorted(vrel | lrel):
        a, b = TEST + "/" + rel, T9 + "/" + rel
        if not os.path.exists(a):
            mism.append((rel, "missing in test tree"))
            continue
        if open(a, "rb").read() != open(b, "rb").read():
            mism.append((rel, "content differs"))
    if mism:
        for rel, why in mism[:20]:
            print("  MISMATCH:", rel, why)
        failures.append("apply vs T9 mismatch: %d" % len(mism))
    else:
        print("   все файлы vendor∪local совпадают с T9 (cmp) — OK")

    # MANIFEST.sha256: пересчёт md5/sha256 всех записей и сверка
    m = {}
    for line in open(PATCHDIR + "/MANIFEST.sha256", encoding="utf-8"):
        line = line.rstrip("\n")
        if not line or line.startswith("#") or "md5" in line.split()[0]:
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        m.setdefault(parts[1], []).append(parts[0])
    bad = []
    for name, hashes in m.items():
        f = PATCHDIR + "/" + name
        data = open(f, "rb").read()
        exp_sha = hashlib.sha256(data).hexdigest()
        exp_md5 = hashlib.md5(data).hexdigest()
        if set(hashes) != {exp_sha, exp_md5}:
            bad.append(name)
    if bad:
        failures.append("MANIFEST.sha256 mismatch: %s" % bad[:10])
    else:
        print("== 6) MANIFEST.sha256: пересчёт md5/sha256 всех записей — OK (%d файлов)"
              % len(m))

    print("\n== ИТОГ")
    if failures:
        for f in failures:
            print("  FAIL:", f)
        sys.exit(1)
    print("  ВЕРИФИКАЦИЯ ПРОЙДЕНА: dry-run 0 rejects; apply 0 rejects;")
    print("  apply-результат == T9 по всем %d rel (vendor∪local); MANIFEST свёрстан."
          % len(vrel | lrel))


if __name__ == "__main__":
    main()
