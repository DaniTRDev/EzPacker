#!/usr/bin/env python3
"""
EzPacker YARPGen Unified Test, Benchmark & Differential Suite
============================================================
Single compact engine providing:
  - stress : Monolithic file compilation & compiler pass profiler
  - bulk   : Multi-file compilation throughput and differential verifier
  - fuzz   : Differential fuzzing against GCC/Clang with reproducer isolation
  - seeds  : Regression runner for curated known-edge seeds
"""

from __future__ import annotations

import argparse
import concurrent.futures
from dataclasses import dataclass, field
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Optional, Tuple

PASS_TIME_RE = re.compile(r"\[Time\]\s+([\w]+):\s+([\d.]+)\s+ms")


class Style:
    USE = sys.stdout.isatty() or os.environ.get("FORCE_COLOR") == "1"
    GREEN = "\033[32m" if USE else ""
    RED = "\033[31m" if USE else ""
    YELLOW = "\033[33m" if USE else ""
    CYAN = "\033[36m" if USE else ""
    BOLD = "\033[1m" if USE else ""
    RESET = "\033[0m" if USE else ""

    @classmethod
    def ok(cls, s: str) -> str: return f"{cls.GREEN}{s}{cls.RESET}"
    @classmethod
    def fail(cls, s: str) -> str: return f"{cls.RED}{s}{cls.RESET}"
    @classmethod
    def warn(cls, s: str) -> str: return f"{cls.YELLOW}{s}{cls.RESET}"
    @classmethod
    def info(cls, s: str) -> str: return f"{cls.CYAN}{s}{cls.RESET}"
    @classmethod
    def bold(cls, s: str) -> str: return f"{cls.BOLD}{s}{cls.RESET}"


def is_windows() -> bool:
    return platform.system().lower() == "windows"


def default_triple() -> str:
    return "x86_64-pc-windows-coff" if is_windows() else "x86_64-unknown-linux-gnu"


def find_binary(name: str, candidates: List[Path]) -> Optional[Path]:
    for path in candidates:
        if path.is_file() and os.access(path, os.X_OK):
            return path.resolve()
        exe = path.with_suffix(".exe" if is_windows() else "")
        if exe.is_file() and os.access(exe, os.X_OK):
            return exe.resolve()
    which = shutil.which(name)
    return Path(which).resolve() if which else None


@dataclass
class CompilerPass:
    name: str
    count: int = 0
    total_ms: float = 0.0

    @property
    def avg_ms(self) -> float:
        return self.total_ms / self.count if self.count > 0 else 0.0


@dataclass
class RunResult:
    name: str
    opt_level: str
    success: bool
    status: str
    compile_time_ms: float = 0.0
    passes: Dict[str, CompilerPass] = field(default_factory=dict)
    ref_checksum: Optional[str] = None
    ezc_checksum: Optional[str] = None
    details: str = ""


class SuiteHarness:
    def __init__(self, ezc_bin: Optional[Path], yarpgen_bin: Optional[Path], ref_compiler: Optional[Path], triple: str):
        repo_root = Path(__file__).resolve().parent.parent.parent
        self.triple = triple or default_triple()
        self.ezc = ezc_bin or find_binary("ezc", [
            repo_root / "build" / "bin" / "ezc",
            repo_root / "bin" / "ezc",
        ])
        self.yarpgen = yarpgen_bin or find_binary("yarpgen", [
            repo_root / "build" / "bin" / "yarpgen",
            repo_root / "build" / "yarpgen",
            repo_root / "build" / "_deps" / "yarpgen-build" / "src" / "yarpgen",
            repo_root / "bin" / "yarpgen",
        ])
        self.ref_cc = ref_compiler or (find_binary("gcc", []) or find_binary("clang", []))
        if self.ref_cc:
            ref_dir = str(self.ref_cc.parent)
            if ref_dir not in os.environ.get("PATH", ""):
                os.environ["PATH"] = f"{ref_dir}{os.pathsep}{os.environ.get('PATH', '')}"
        if self.ezc:
            ezc_dir = str(self.ezc.parent)
            if ezc_dir not in os.environ.get("PATH", ""):
                os.environ["PATH"] = f"{ezc_dir}{os.pathsep}{os.environ.get('PATH', '')}" 

    def compile_ezc(self, src: Path, obj_out: Path, opt: str, time_passes: bool = False, timeout_sec: int = 60) -> Tuple[bool, float, Dict[str, CompilerPass], str]:
        if not self.ezc:
            return False, 0.0, {}, "ezc executable not located"
        cmd = [str(self.ezc), f"-{opt}", "--target", self.triple, str(src.resolve()), "-o", str(obj_out.resolve())]
        if time_passes:
            cmd.append("--time-passes")

        t0 = time.time()
        try:
            res = subprocess.run(cmd, cwd=str(src.parent), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_sec)
            elapsed_ms = (time.time() - t0) * 1000.0
        except subprocess.TimeoutExpired:
            return False, timeout_sec * 1000.0, {}, f"Compilation timed out after {timeout_sec}s"

        passes: Dict[str, CompilerPass] = {}
        for line in res.stdout.splitlines():
            m = PASS_TIME_RE.search(line)
            if m:
                pname, ptime = m.group(1), float(m.group(2))
                stats = passes.setdefault(pname, CompilerPass(name=pname))
                stats.count += 1
                stats.total_ms += ptime

        ok = (res.returncode == 0 and obj_out.exists())
        err = res.stderr if not ok else ""
        return ok, elapsed_ms, passes, err

    def run_differential(self, src: Path, obj: Path, driver: Path, opt: str, timeout_sec: int = 30) -> Tuple[bool, str, Optional[str], Optional[str]]:
        if not self.ref_cc:
            return True, "SKIPPED_REF", None, None

        work_dir = src.parent
        ext = ".exe" if is_windows() else ""
        ref_exe = work_dir / f"ref_tmp_{opt}{ext}"
        ezc_exe = work_dir / f"ezc_tmp_{opt}{ext}"

        # 1. Compile & run reference
        ref_cmd = [str(self.ref_cc), "-O3", "-fcommon", "-Wl,--allow-multiple-definition", str(src.resolve()), str(driver.resolve()), "-o", str(ref_exe.resolve())]
        try:
            res = subprocess.run(ref_cmd, cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_sec)
            if res.returncode != 0 or not ref_exe.exists():
                return False, f"Ref compile failed: {res.stderr[:200]}", None, None
            run_ref = subprocess.run([str(ref_exe.resolve())], cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_sec)
            ref_hash = run_ref.stdout.strip()
        except Exception as e:
            return False, f"Ref execution error: {e}", None, None
        finally:
            ref_exe.unlink(missing_ok=True)

        # 2. Link & run ezc object
        link_cmd = [str(self.ref_cc), "-fcommon", "-Wl,--allow-multiple-definition", str(driver.resolve()), str(obj.resolve()), "-o", str(ezc_exe.resolve())]
        try:
            res = subprocess.run(link_cmd, cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_sec)
            if res.returncode != 0 or not ezc_exe.exists():
                return False, f"Linking ezc object failed: {res.stderr[:200]}", ref_hash, None
            run_ezc = subprocess.run([str(ezc_exe.resolve())], cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_sec)
            ezc_hash = run_ezc.stdout.strip()
        except Exception as e:
            return False, f"Ezc execution error: {e}", ref_hash, None
        finally:
            ezc_exe.unlink(missing_ok=True)

        matched = (ref_hash == ezc_hash)
        detail = "PASS" if matched else f"MISCOMPILATION: ezc({ezc_hash}) != ref({ref_hash})"
        return matched, detail, ref_hash, ezc_hash


# ---------------------------------------------------------------------------
# Subcommand: Stress Benchmark & Pass Profiler
# ---------------------------------------------------------------------------
def cmd_stress(args, suite: SuiteHarness) -> int:
    src = Path(args.file).resolve()
    driver = Path(args.driver).resolve() if args.driver else src.parent / "mega_stress_driver.c"
    if not src.exists():
        print(Style.fail(f"Error: Stress file not found: {src}"))
        return 1

    opt_levels = ["O0", "O1", "O2", "Os"] if args.opt_level == "all" else [args.opt_level]
    print(Style.bold(f"\n[Stress] Running Monolithic Benchmark on {src.name} (~{len(src.read_text(encoding='utf-8').splitlines())} LOC)"))
    print(f"Target: {suite.triple} | Ref Compiler: {suite.ref_cc or 'None (compile-only)'}")

    total_failed = 0
    for opt in opt_levels:
        obj = src.with_suffix(f".{opt}.obj" if is_windows() else f".{opt}.o")
        ok, compile_ms, passes, err = suite.compile_ezc(src, obj, opt, time_passes=True)
        if not ok:
            print(f"  [{Style.fail('FAIL')}] -{opt}: Compilation failed ({compile_ms:.1f}ms): {err[:150]}")
            total_failed += 1
            continue

        verif_str = ""
        if not args.compile_only and suite.ref_cc and driver.exists():
            matched, vdetail, rhash, ehash = suite.run_differential(src, obj, driver, opt)
            if not matched:
                verif_str = f" | {Style.fail(vdetail)}"
                total_failed += 1
            else:
                verif_str = f" | {Style.ok('Checksum: ' + str(ehash))}"

        print(f"  [{Style.ok('PASS')}] -{opt}: {compile_ms:.1f}ms pipeline time{verif_str}")
        if passes and args.verbose:
            total_p = sum(p.total_ms for p in passes.values())
            for p in sorted(passes.values(), key=lambda x: x.total_ms, reverse=True)[:5]:
                pct = (p.total_ms / total_p * 100) if total_p > 0 else 0
                print(f"      - {p.name:<25}: {p.total_ms:>6.2f} ms ({pct:>5.1f}%)")
        obj.unlink(missing_ok=True)

    return 1 if total_failed > 0 else 0


# ---------------------------------------------------------------------------
# Subcommand: Bulk Compilation & Differential Benchmark
# ---------------------------------------------------------------------------
def cmd_bulk(args, suite: SuiteHarness) -> int:
    corpus_dir = Path(args.corpus_dir).resolve()
    if not corpus_dir.exists():
        print(Style.fail(f"Corpus dir not found: {corpus_dir}"))
        return 1

    test_files = sorted(corpus_dir.glob("test_*.c"))
    if not test_files:
        print(Style.fail(f"No test_*.c files found in {corpus_dir}"))
        return 1

    opt_levels = [o.strip() for o in args.opt_levels.split(",") if o.strip()]
    print(Style.bold(f"\n[Bulk] Running Corpus Verification on {len(test_files)} files across {opt_levels}"))

    t0 = time.time()
    passed, failed = 0, 0
    for test in test_files:
        driver = corpus_dir / test.name.replace("test_", "driver_")
        for opt in opt_levels:
            obj = test.with_suffix(f".{opt}.obj" if is_windows() else f".{opt}.o")
            ok, ms, _, err = suite.compile_ezc(test, obj, opt)
            if not ok:
                failed += 1
                print(f"  [{Style.fail('FAIL')}] {test.name} -{opt}: Compile error ({err[:100]})")
                continue

            if not args.compile_only and suite.ref_cc and driver.exists():
                matched, vdetail, _, _ = suite.run_differential(test, obj, driver, opt)
                if not matched:
                    failed += 1
                    print(f"  [{Style.fail('DIFF')}] {test.name} -{opt}: {vdetail}")
                    obj.unlink(missing_ok=True)
                    continue

            passed += 1
            obj.unlink(missing_ok=True)

    total_sec = time.time() - t0
    status = Style.ok("SUCCESS") if failed == 0 else Style.fail(f"{failed} FAILED")
    print(f"\n[Bulk] Finished in {total_sec:.2f}s | Passed: {passed} | Failed: {failed} [{status}]")
    return 1 if failed > 0 else 0


# ---------------------------------------------------------------------------
# Subcommand: Fuzzing Loop & Deterministic Seeds
# ---------------------------------------------------------------------------
def cmd_fuzz(args, suite: SuiteHarness) -> int:
    if not suite.yarpgen:
        print(Style.fail("yarpgen binary not found; differential fuzzing requires yarpgen"))
        return 1
    if not suite.ref_cc:
        print(Style.fail("Reference compiler (GCC/Clang) required for differential fuzzing"))
        return 1

    work_dir = Path(args.work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    repro_dir = Path(args.reproducers_dir).resolve() if args.reproducers_dir else work_dir / "reproducers"

    iterations = args.iterations
    print(Style.bold(f"\n[Fuzz] Executing {iterations} differential fuzzing iterations against {suite.ref_cc.name}"))

    failures = 0
    for i in range(1, iterations + 1):
        seed = args.seed if args.seed is not None else int(time.time() * 1000) % 1000000000
        seed_dir = work_dir / f"fuzz_{seed}"
        seed_dir.mkdir(parents=True, exist_ok=True)

        res = subprocess.run([str(suite.yarpgen), "--std=c", "-s", str(seed), f"--out-dir={seed_dir}"],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        src = seed_dir / "func.c"
        driver = seed_dir / "driver.c"
        if not src.exists() or not driver.exists():
            shutil.rmtree(seed_dir, ignore_errors=True)
            continue

        obj = seed_dir / "func.obj"
        ok, _, _, err = suite.compile_ezc(src, obj, "O0")
        if not ok:
            print(f"  [{Style.fail('CRASH')}] Seed {seed}: ezc compilation failed: {err[:120]}")
            failures += 1
            continue

        matched, vdetail, rhash, ehash = suite.run_differential(src, obj, driver, "O0")
        if not matched:
            print(f"  [{Style.fail('MISMATCH')}] Seed {seed}: {vdetail}")
            failures += 1
            repro_dir.mkdir(parents=True, exist_ok=True)
            shutil.copytree(seed_dir, repro_dir / f"repro_{seed}", dirs_exist_ok=True)
        else:
            if args.verbose or (i % 5 == 0):
                print(f"  [{Style.ok('PASS')}] Iteration {i}/{iterations}: Seed {seed} checksum {ehash}")

        shutil.rmtree(seed_dir, ignore_errors=True)

    print(f"\n[Fuzz] Completed {iterations} iterations: {iterations - failures} passed, {failures} failures")
    return 1 if failures > 0 else 0


def cmd_seeds(args, suite: SuiteHarness) -> int:
    seeds_file = Path(args.seeds_file).resolve()
    if not seeds_file.exists():
        print(Style.fail(f"Seeds file not found: {seeds_file}"))
        return 1

    seeds = []
    for line in seeds_file.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            m = re.match(r"^(\d+)", line)
            if m:
                seeds.append(int(m.group(1)))

    print(Style.bold(f"\n[Seeds] Verifying {len(seeds)} curated regression seeds"))
    failed = 0
    for seed in seeds:
        args.seed = seed
        args.iterations = 1
        if cmd_fuzz(args, suite) != 0:
            failed += 1
    return 1 if failed > 0 else 0


def main() -> int:
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--ezc-bin", type=Path, help="Path to ezc compiler binary")
    common.add_argument("--yarpgen-bin", type=Path, help="Path to yarpgen binary")
    common.add_argument("--ref-compiler", type=Path, help="Path to reference C compiler (gcc/clang)")
    common.add_argument("--target", type=str, default="", help="Target triple")
    common.add_argument("-v", "--verbose", action="store_true", help="Verbose progress and pass timings")

    parser = argparse.ArgumentParser(description="EzPacker YARPGen Unified Test & Benchmark Suite", parents=[common])
    sub = parser.add_subparsers(dest="cmd", required=True)

    # stress
    p_stress = sub.add_parser("stress", parents=[common], help="Single big file stress benchmark and pass profiler")
    p_stress.add_argument("--file", type=Path, default="mega_stress.c", help="Monolithic stress C file")
    p_stress.add_argument("--driver", type=Path, default=None, help="Companion verification driver C file")
    p_stress.add_argument("--opt-level", type=str, default="all", choices=["O0", "O1", "O2", "Os", "all"])
    p_stress.add_argument("--compile-only", action="store_true", help="Skip differential verification")

    # bulk
    p_bulk = sub.add_parser("bulk", parents=[common], help="Bulk multi-file compilation and throughput benchmark")
    p_bulk.add_argument("--corpus-dir", type=Path, default="bulk_corpus", help="Directory of C test files")
    p_bulk.add_argument("--opt-levels", type=str, default="O0,O2", help="Comma-separated optimization levels")
    p_bulk.add_argument("--compile-only", action="store_true", help="Skip differential verification")

    # fuzz
    p_fuzz = sub.add_parser("fuzz", parents=[common], help="Differential fuzzing campaign against GCC/Clang")
    p_fuzz.add_argument("--iterations", type=int, default=25, help="Number of fuzzing rounds")
    p_fuzz.add_argument("--seed", type=int, default=None, help="Specific seed to run")
    p_fuzz.add_argument("--work-dir", type=Path, default="fuzz_work", help="Ephemeral working directory")
    p_fuzz.add_argument("--reproducers-dir", type=Path, default="reproducers", help="Target dir for reproducers")

    # seeds
    p_seeds = sub.add_parser("seeds", parents=[common], help="Run curated seeds regression list")
    p_seeds.add_argument("--seeds-file", type=Path, default="regression_seeds.txt", help="Seed list file")
    p_seeds.add_argument("--work-dir", type=Path, default="seeds_work", help="Ephemeral working directory")
    p_seeds.add_argument("--reproducers-dir", type=Path, default="reproducers", help="Target dir for reproducers")

    args = parser.parse_args()
    suite = SuiteHarness(args.ezc_bin, args.yarpgen_bin, args.ref_compiler, args.target)

    if args.cmd == "stress":
        return cmd_stress(args, suite)
    elif args.cmd == "bulk":
        return cmd_bulk(args, suite)
    elif args.cmd == "fuzz":
        return cmd_fuzz(args, suite)
    elif args.cmd == "seeds":
        return cmd_seeds(args, suite)
    return 0


if __name__ == "__main__":
    sys.exit(main())
