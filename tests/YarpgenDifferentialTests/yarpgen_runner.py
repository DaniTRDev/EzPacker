#!/usr/bin/env python3
"""
EzPacker YARPGen Differential Testing & Fuzzing Harness
=======================================================
Generates random, valid C programs using Intel YARPGen (https://github.com/intel/yarpgen),
compiles them to machine code using EzPacker (ezc) across optimization levels, links and
runs the resulting binaries, and compares output checksums against GCC/Clang reference builds.

Automatically isolates reproducible test bundles on failures, crashes, or miscompilations.
"""

from __future__ import annotations

import argparse
import concurrent.futures
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import json
import os
from pathlib import Path
import platform
import random
import re
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Optional, Tuple


class TestStatus(Enum):
    PASS = "PASS"
    CHECKSUM_MISMATCH = "CHECKSUM_MISMATCH"
    COMPILER_CRASH = "COMPILER_CRASH"
    SELECTION_FAILURE = "SELECTION_FAILURE"
    FRONTEND_FAILURE = "FRONTEND_FAILURE"
    EMISSION_FAILURE = "EMISSION_FAILURE"
    COMPILER_ERROR = "COMPILER_ERROR"
    LINK_FAILURE = "LINK_FAILURE"
    EXECUTION_CRASH = "EXECUTION_CRASH"
    EXECUTION_TIMEOUT = "EXECUTION_TIMEOUT"
    YARPGEN_ERROR = "YARPGEN_ERROR"
    REF_COMPILER_ERROR = "REF_COMPILER_ERROR"


@dataclass
class TestResult:
    seed: int
    opt_level: str
    status: TestStatus
    details: str = ""
    reproducer_dir: Optional[Path] = None
    ref_checksum: Optional[str] = None
    ezc_checksum: Optional[str] = None
    duration_sec: float = 0.0


class Color:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    GREEN = "\033[32m"
    RED = "\033[31m"
    YELLOW = "\033[33m"
    CYAN = "\033[36m"
    MAGENTA = "\033[35m"
    GRAY = "\033[90m"

    @classmethod
    def enabled(cls) -> bool:
        return sys.stdout.isatty() or os.environ.get("FORCE_COLOR") == "1"

    @classmethod
    def paint(cls, text: str, color: str) -> str:
        if cls.enabled():
            return f"{color}{text}{cls.RESET}"
        return text


class EnvironmentDetector:
    """Detects available compilers, generators, and tools on host system."""

    @staticmethod
    def is_windows() -> bool:
        return platform.system().lower() == "windows"

    @classmethod
    def default_target_triple(cls) -> str:
        if cls.is_windows():
            return "x86_64-pc-windows-coff"
        return "x86_64-unknown-linux-gnu"

    @classmethod
    def find_executable(cls, name: str, candidate_paths: List[Path]) -> Optional[Path]:
        # 1. Search candidate paths
        for path in candidate_paths:
            if path.is_file() and os.access(path, os.X_OK):
                return path.resolve()
            exe_path = path.with_suffix(".exe" if cls.is_windows() else "")
            if exe_path.is_file() and os.access(exe_path, os.X_OK):
                return exe_path.resolve()

        # 2. Search PATH
        found = shutil.which(name)
        if found:
            return Path(found).resolve()

        return None

    @classmethod
    def find_ezc(cls, workspace_root: Path) -> Optional[Path]:
        candidates = [
            workspace_root / "cmake-build-debug" / "bin" / "ezc",
            workspace_root / "build" / "bin" / "ezc",
            workspace_root / "bin" / "ezc",
            workspace_root / "cmake-build-release" / "bin" / "ezc",
        ]
        return cls.find_executable("ezc", candidates)

    @classmethod
    def find_yarpgen(cls, workspace_root: Path) -> Optional[Path]:
        candidates = [
            workspace_root / "cmake-build-debug" / "_deps" / "yarpgen_ext-build" / "yarpgen",
            workspace_root / "cmake-build-debug" / "bin" / "yarpgen",
            workspace_root / "build" / "_deps" / "yarpgen_ext-build" / "yarpgen",
            workspace_root / "build" / "bin" / "yarpgen",
            workspace_root / "yarpgen" / "build" / "yarpgen",
            Path.home() / ".gemini" / "antigravity-cli" / "brain" / "8358203c-4d4f-4462-8d4f-e317c6f17976" / "scratch" / "yarpgen" / "build" / "yarpgen",
        ]
        return cls.find_executable("yarpgen", candidates)

    @classmethod
    def find_reference_compiler(cls) -> Optional[Path]:
        candidates: List[Path] = []
        if cls.is_windows():
            candidates.extend([
                Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin/gcc.exe"),
                Path("C:/msys64/ucrt64/bin/gcc.exe"),
                Path("C:/msys64/mingw64/bin/gcc.exe"),
                Path("C:/Program Files/LLVM/bin/clang.exe"),
            ])
        for name in ["gcc", "clang"]:
            found = cls.find_executable(name, candidates)
            if found:
                return found
        return None

    @classmethod
    def build_env(cls, ezc_bin: Optional[Path], ref_compiler: Optional[Path]) -> Dict[str, str]:
        env = dict(os.environ)
        path_entries = []
        if ezc_bin:
            path_entries.append(str(ezc_bin.parent))
        if ref_compiler:
            path_entries.append(str(ref_compiler.parent))
        if cls.is_windows():
            # Add CLion MinGW if available
            clion_mingw = Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin")
            if clion_mingw.exists():
                path_entries.append(str(clion_mingw))

        existing_path = env.get("PATH", "")
        env["PATH"] = os.pathsep.join(path_entries + [existing_path]) if path_entries else existing_path
        return env


class YarpgenRunner:
    """Executes code generation, compilation, execution, and differential analysis."""

    def __init__(
        self,
        ezc_bin: Path,
        yarpgen_bin: Path,
        ref_compiler: Path,
        target_triple: str,
        opt_levels: List[str],
        mode: str,
        work_dir: Path,
        reproducers_dir: Path,
        timeout_sec: float = 10.0,
        keep_passed: bool = False,
        verbose: bool = False,
    ):
        self.ezc_bin = ezc_bin
        self.yarpgen_bin = yarpgen_bin
        self.ref_compiler = ref_compiler
        self.target_triple = target_triple
        self.opt_levels = opt_levels
        self.mode = mode
        self.work_dir = work_dir
        self.reproducers_dir = reproducers_dir
        self.timeout_sec = timeout_sec
        self.keep_passed = keep_passed
        self.verbose = verbose
        self.env = EnvironmentDetector.build_env(ezc_bin, ref_compiler)

        self.work_dir.mkdir(parents=True, exist_ok=True)
        self.reproducers_dir.mkdir(parents=True, exist_ok=True)

    def _run_cmd(
        self, cmd: List[str], cwd: Path, timeout: Optional[float] = None
    ) -> Tuple[int, str, str]:
        timeout = timeout or self.timeout_sec
        try:
            res = subprocess.run(
                cmd,
                cwd=str(cwd),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout,
                env=self.env,
            )
            return res.returncode, res.stdout, res.stderr
        except subprocess.TimeoutExpired:
            return -999, "", f"Command timed out after {timeout} seconds: {' '.join(cmd)}"
        except Exception as ex:
            return -1, "", f"Failed to execute command: {ex}"

    def run_seed(self, seed: int) -> List[TestResult]:
        seed_dir = self.work_dir / f"seed_{seed}"
        if seed_dir.exists():
            shutil.rmtree(seed_dir, ignore_errors=True)
        seed_dir.mkdir(parents=True, exist_ok=True)

        results: List[TestResult] = []
        t0 = time.time()

        # 1. Generate C code using YARPGen
        yarp_cmd = [
            str(self.yarpgen_bin),
            "--std=c",
            "-s",
            str(seed),
            f"--out-dir={seed_dir.resolve()}",
        ]
        ret, stdout, stderr = self._run_cmd(yarp_cmd, seed_dir)
        func_c = seed_dir / "func.c"
        driver_c = seed_dir / "driver.c"
        init_h = seed_dir / "init.h"

        if ret != 0 or not func_c.exists() or not driver_c.exists():
            results.append(
                TestResult(
                    seed=seed,
                    opt_level="ALL",
                    status=TestStatus.YARPGEN_ERROR,
                    details=f"YARPGen generation failed: {stderr or stdout}",
                    duration_sec=time.time() - t0,
                )
            )
            return results

        # 2. Compile reference binary using GCC / Clang (-O3)
        ref_exe = seed_dir / ("ref_test.exe" if EnvironmentDetector.is_windows() else "ref_test")
        ref_compile_cmd = [
            str(self.ref_compiler),
            "-O3",
            "-I",
            str(seed_dir.resolve()),
            str(driver_c.resolve()),
            str(func_c.resolve()),
            "-o",
            str(ref_exe.resolve()),
        ]
        ret, stdout, stderr = self._run_cmd(ref_compile_cmd, seed_dir)
        if ret != 0 or not ref_exe.exists():
            results.append(
                TestResult(
                    seed=seed,
                    opt_level="ALL",
                    status=TestStatus.REF_COMPILER_ERROR,
                    details=f"Reference compiler failed to build: {stderr}",
                    duration_sec=time.time() - t0,
                )
            )
            return results

        # 3. Execute reference binary to obtain baseline checksum
        ret, ref_stdout, ref_stderr = self._run_cmd([str(ref_exe.resolve())], seed_dir)
        if ret != 0:
            results.append(
                TestResult(
                    seed=seed,
                    opt_level="ALL",
                    status=TestStatus.REF_COMPILER_ERROR,
                    details=f"Reference binary exited with code {ret}: {ref_stderr}",
                    duration_sec=time.time() - t0,
                )
            )
            return results

        ref_checksum = ref_stdout.strip()

        # 4. For each optimization level, compile with EzPacker and compare
        all_passed = True
        for opt in self.opt_levels:
            opt_t0 = time.time()
            res = self._test_ezc_variant(
                seed=seed,
                seed_dir=seed_dir,
                opt=opt,
                ref_checksum=ref_checksum,
            )
            res.duration_sec = time.time() - opt_t0
            results.append(res)
            if res.status != TestStatus.PASS:
                all_passed = False

        if all_passed and not self.keep_passed:
            shutil.rmtree(seed_dir, ignore_errors=True)

        return results

    def _test_ezc_variant(
        self,
        seed: int,
        seed_dir: Path,
        opt: str,
        ref_checksum: str,
    ) -> TestResult:
        func_c = seed_dir / "func.c"
        driver_c = seed_dir / "driver.c"
        obj_suffix = ".obj" if EnvironmentDetector.is_windows() else ".o"
        exe_suffix = ".exe" if EnvironmentDetector.is_windows() else ""

        func_obj = seed_dir / f"func_ezc_{opt}{obj_suffix}"
        ezc_exe = seed_dir / f"ezc_test_{opt}{exe_suffix}"

        # Step 4a: Compile with ezc
        ezc_cmd = [
            str(self.ezc_bin),
            f"-{opt}",
            "--target",
            self.target_triple,
            "-I",
            str(seed_dir.resolve()),
            str(func_c.resolve()),
            "-o",
            str(func_obj.resolve()),
        ]

        ret, stdout, stderr = self._run_cmd(ezc_cmd, seed_dir)
        full_ezc_log = f"{stdout}\n{stderr}".strip()

        if ret != 0 or not func_obj.exists():
            status = self._classify_compiler_error(ret, full_ezc_log)
            repro_dir = self._create_reproducer(
                seed=seed,
                opt=opt,
                category=status.value,
                seed_dir=seed_dir,
                error_log=full_ezc_log,
                ezc_cmd=ezc_cmd,
            )
            return TestResult(
                seed=seed,
                opt_level=opt,
                status=status,
                details=self._extract_key_error(full_ezc_log),
                reproducer_dir=repro_dir,
                ref_checksum=ref_checksum,
            )

        # Step 4b: Link using reference linker / host compiler
        link_cmd = [
            str(self.ref_compiler),
            str(driver_c.resolve()),
            str(func_obj.resolve()),
            "-o",
            str(ezc_exe.resolve()),
        ]
        ret, stdout, stderr = self._run_cmd(link_cmd, seed_dir)
        if ret != 0 or not ezc_exe.exists():
            repro_dir = self._create_reproducer(
                seed=seed,
                opt=opt,
                category="LINK_FAILURE",
                seed_dir=seed_dir,
                error_log=stderr or stdout,
                ezc_cmd=ezc_cmd,
                link_cmd=link_cmd,
            )
            return TestResult(
                seed=seed,
                opt_level=opt,
                status=TestStatus.LINK_FAILURE,
                details=f"Linker failed: {stderr}",
                reproducer_dir=repro_dir,
                ref_checksum=ref_checksum,
            )

        # Step 4c: Execute ezc binary
        ret, ezc_stdout, ezc_stderr = self._run_cmd([str(ezc_exe.resolve())], seed_dir)

        if ret == -999:
            repro_dir = self._create_reproducer(
                seed=seed,
                opt=opt,
                category="EXECUTION_TIMEOUT",
                seed_dir=seed_dir,
                error_log=ezc_stderr,
                ezc_cmd=ezc_cmd,
            )
            return TestResult(
                seed=seed,
                opt_level=opt,
                status=TestStatus.EXECUTION_TIMEOUT,
                details="Program timed out during execution",
                reproducer_dir=repro_dir,
                ref_checksum=ref_checksum,
            )

        if ret != 0:
            repro_dir = self._create_reproducer(
                seed=seed,
                opt=opt,
                category="EXECUTION_CRASH",
                seed_dir=seed_dir,
                error_log=f"Exit code: {ret}\n{ezc_stderr}",
                ezc_cmd=ezc_cmd,
            )
            return TestResult(
                seed=seed,
                opt_level=opt,
                status=TestStatus.EXECUTION_CRASH,
                details=f"Program crashed with exit code {hex(ret) if ret < 0 else ret}",
                reproducer_dir=repro_dir,
                ref_checksum=ref_checksum,
            )

        # Step 4d: Compare output checksums
        ezc_checksum = ezc_stdout.strip()
        if ezc_checksum != ref_checksum:
            mismatch_log = (
                f"CHECKSUM MISMATCH!\n"
                f"Reference Checksum: {ref_checksum}\n"
                f"EzPacker Checksum:  {ezc_checksum}\n"
            )
            repro_dir = self._create_reproducer(
                seed=seed,
                opt=opt,
                category="CHECKSUM_MISMATCH",
                seed_dir=seed_dir,
                error_log=mismatch_log,
                ezc_cmd=ezc_cmd,
                ref_checksum=ref_checksum,
                ezc_checksum=ezc_checksum,
            )
            return TestResult(
                seed=seed,
                opt_level=opt,
                status=TestStatus.CHECKSUM_MISMATCH,
                details=f"Mismatch: ref={ref_checksum} != ezc={ezc_checksum}",
                reproducer_dir=repro_dir,
                ref_checksum=ref_checksum,
                ezc_checksum=ezc_checksum,
            )

        return TestResult(
            seed=seed,
            opt_level=opt,
            status=TestStatus.PASS,
            details=f"Checksum matched: {ref_checksum}",
            ref_checksum=ref_checksum,
            ezc_checksum=ezc_checksum,
        )

    def _classify_compiler_error(self, return_code: int, log: str) -> TestStatus:
        if "Instruction selection failed" in log or "Could not select instruction" in log:
            return TestStatus.SELECTION_FAILURE
        if "[CFrontend]" in log or "parse" in log.lower() or "syntax error" in log.lower():
            return TestStatus.FRONTEND_FAILURE
        if "Emission" in log or "Relocation" in log or "Emitter" in log:
            return TestStatus.EMISSION_FAILURE
        if "Fatal Exception" in log or return_code in (2, -1073741819, 139):
            return TestStatus.COMPILER_CRASH
        return TestStatus.COMPILER_ERROR

    def _extract_key_error(self, log: str) -> str:
        for line in log.splitlines():
            line_str = line.strip()
            if any(k in line_str for k in ["Error:", "Fatal Exception", "failed", "Could not"]):
                return line_str
        lines = [l.strip() for l in log.splitlines() if l.strip()]
        return lines[-1] if lines else "Unknown error"

    def _create_reproducer(
        self,
        seed: int,
        opt: str,
        category: str,
        seed_dir: Path,
        error_log: str,
        ezc_cmd: List[str],
        link_cmd: Optional[List[str]] = None,
        ref_checksum: Optional[str] = None,
        ezc_checksum: Optional[str] = None,
    ) -> Path:
        repro_dir = self.reproducers_dir / f"{category}_seed_{seed}_{opt}"
        repro_dir.mkdir(parents=True, exist_ok=True)

        for fname in ["func.c", "driver.c", "init.h"]:
            src = seed_dir / fname
            if src.exists():
                shutil.copy2(src, repro_dir / fname)

        # Write metadata.json
        meta = {
            "seed": seed,
            "opt_level": opt,
            "category": category,
            "target_triple": self.target_triple,
            "ezc_bin": str(self.ezc_bin.resolve()),
            "ref_compiler": str(self.ref_compiler.resolve()),
            "timestamp": datetime.now().isoformat(),
            "ref_checksum": ref_checksum,
            "ezc_checksum": ezc_checksum,
            "ezc_command": ezc_cmd,
            "link_command": link_cmd,
        }
        with open(repro_dir / "metadata.json", "w", encoding="utf-8") as f:
            json.dump(meta, f, indent=2)

        with open(repro_dir / "error.log", "w", encoding="utf-8") as f:
            f.write(error_log)

        # Generate one-click repro script
        ezc_str = f'"{self.ezc_bin.resolve()}" -{opt} --target {self.target_triple} -I . func.c -o func.obj -v'
        mir_dump_str = f'"{self.ezc_bin.resolve()}" -{opt} --target {self.target_triple} -I . func.c --emit-mir > dump.mir'
        asm_dump_str = f'"{self.ezc_bin.resolve()}" -{opt} --target {self.target_triple} -I . func.c -S > dump.s'

        if EnvironmentDetector.is_windows():
            bat_content = f"""@echo off
echo ========================================================
echo Reproducing EzPacker Failure: {category} (Seed {seed}, -{opt})
echo ========================================================
echo Running: {ezc_str}
{ezc_str}
echo.
echo Emitting MIR dump to dump.mir...
{mir_dump_str}
echo Emitting assembly to dump.s...
{asm_dump_str}
"""
            with open(repro_dir / "repro.bat", "w", encoding="utf-8") as f:
                f.write(bat_content)
        else:
            sh_content = f"""#!/usr/bin/env bash
echo "========================================================"
echo "Reproducing EzPacker Failure: {category} (Seed {seed}, -{opt})"
echo "========================================================"
echo "Running: {ezc_str}"
{ezc_str}
echo "Emitting MIR dump to dump.mir..."
{mir_dump_str}
echo "Emitting assembly to dump.s..."
{asm_dump_str}
"""
            sh_path = repro_dir / "repro.sh"
            with open(sh_path, "w", encoding="utf-8") as f:
                f.write(sh_content)
            sh_path.chmod(0o755)

        return repro_dir


def print_banner(ezc_bin: Path, yarpgen_bin: Path, ref_compiler: Path, target: str):
    print(Color.paint("=" * 78, Color.CYAN))
    print(Color.paint("EzPacker YARPGen Differential Testing Suite", Color.BOLD + Color.CYAN))
    print(Color.paint("=" * 78, Color.CYAN))
    print(f"  {Color.paint('EzPacker (ezc):', Color.BOLD)}   {ezc_bin}")
    print(f"  {Color.paint('YARPGen Binary:', Color.BOLD)}   {yarpgen_bin}")
    print(f"  {Color.paint('Ref Compiler:', Color.BOLD)}     {ref_compiler}")
    print(f"  {Color.paint('Target Triple:', Color.BOLD)}    {target}")
    print(Color.paint("-" * 78, Color.GRAY))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="EzPacker YARPGen Differential Fuzzer & Verification Harness"
    )
    parser.add_argument("--ezc-bin", type=Path, help="Path to ezc compiler binary")
    parser.add_argument("--yarpgen-bin", type=Path, help="Path to yarpgen executable")
    parser.add_argument("--ref-compiler", type=Path, help="Path to GCC or Clang reference compiler")
    parser.add_argument("--target", type=str, help="Target architecture triple")
    parser.add_argument("--opt-levels", type=str, default="O0,O2", help="Comma-separated optimization levels (e.g. O0,O1,O2,Os)")
    parser.add_argument("--mode", choices=["func-only", "full"], default="func-only", help="Compilation mode (default: func-only)")
    parser.add_argument("--iterations", "-n", type=int, default=1, help="Number of random programs to test")
    parser.add_argument("--seed", "-s", type=int, help="Test a single specific seed")
    parser.add_argument("--seeds-file", type=Path, help="Path to a file containing regression seeds")
    parser.add_argument("--work-dir", type=Path, default=Path(".yarpgen_work"), help="Temporary scratch directory")
    parser.add_argument("--reproducers-dir", type=Path, default=Path("yarpgen_reproducers"), help="Directory to save failed reproducers")
    parser.add_argument("--jobs", "-j", type=int, default=1, help="Number of parallel worker jobs")
    parser.add_argument("--timeout", type=float, default=10.0, help="Per-command execution timeout in seconds")
    parser.add_argument("--keep-passed", action="store_true", help="Retain files for passing tests")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print verbose output")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    workspace_root = Path.cwd()

    # 1. Discover binaries
    ezc_bin = args.ezc_bin or EnvironmentDetector.find_ezc(workspace_root)
    if not ezc_bin or not ezc_bin.exists():
        sys.stderr.write(f"Error: Could not locate ezc binary. Build it or specify --ezc-bin.\n")
        return 1

    yarpgen_bin = args.yarpgen_bin or EnvironmentDetector.find_yarpgen(workspace_root)
    if not yarpgen_bin or not yarpgen_bin.exists():
        sys.stderr.write(f"Error: Could not locate yarpgen binary. Specify --yarpgen-bin.\n")
        return 1

    ref_compiler = args.ref_compiler or EnvironmentDetector.find_reference_compiler()
    if not ref_compiler or not ref_compiler.exists():
        sys.stderr.write(f"Error: Could not locate reference compiler (gcc/clang). Specify --ref-compiler.\n")
        return 1

    target = args.target or EnvironmentDetector.default_target_triple()
    opt_levels = [opt.strip().lstrip("-") for opt in args.opt_levels.split(",") if opt.strip()]

    print_banner(ezc_bin, yarpgen_bin, ref_compiler, target)

    # 2. Determine seeds to test
    seeds: List[int] = []
    if args.seed is not None:
        seeds = [args.seed]
    elif args.seeds_file and args.seeds_file.exists():
        with open(args.seeds_file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    try:
                        seeds.append(int(line.split()[0]))
                    except ValueError:
                        pass
    else:
        # Generate N random 32-bit seeds
        seeds = [random.randint(10000, 2147483647) for _ in range(args.iterations)]

    runner = YarpgenRunner(
        ezc_bin=ezc_bin,
        yarpgen_bin=yarpgen_bin,
        ref_compiler=ref_compiler,
        target_triple=target,
        opt_levels=opt_levels,
        mode=args.mode,
        work_dir=args.work_dir,
        reproducers_dir=args.reproducers_dir,
        timeout_sec=args.timeout,
        keep_passed=args.keep_passed,
        verbose=args.verbose,
    )

    total_runs = len(seeds) * len(opt_levels)
    passed_count = 0
    failure_counts: Dict[TestStatus, int] = {}
    start_time = time.time()

    def handle_result(res: TestResult):
        nonlocal passed_count
        color = Color.GREEN if res.status == TestStatus.PASS else Color.RED
        status_label = f"[{res.status.value}]"
        opt_label = f"(-{res.opt_level})"
        seed_label = f"Seed {res.seed}"

        print(f"  {Color.paint(status_label, color):<24} {seed_label:<16} {opt_label:<8} {res.details}")
        if res.reproducer_dir:
            print(f"      -> {Color.paint('Reproducer saved to:', Color.YELLOW)} {res.reproducer_dir}")

        if res.status == TestStatus.PASS:
            passed_count += 1
        else:
            failure_counts[res.status] = failure_counts.get(res.status, 0) + 1

    print(f"Testing {len(seeds)} seed(s) across {len(opt_levels)} optimization level(s) (Total variant checks: {total_runs})...\n")

    if args.jobs > 1 and len(seeds) > 1:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
            future_to_seed = {executor.submit(runner.run_seed, s): s for s in seeds}
            for future in concurrent.futures.as_completed(future_to_seed):
                for res in future.result():
                    handle_result(res)
    else:
        for s in seeds:
            for res in runner.run_seed(s):
                handle_result(res)

    total_time = time.time() - start_time
    total_failures = sum(failure_counts.values())

    print("\n" + Color.paint("=" * 78, Color.CYAN))
    print(Color.paint("Differential Testing Summary", Color.BOLD + Color.CYAN))
    print(Color.paint("=" * 78, Color.CYAN))
    print(f"  Total Variant Tests:  {total_runs}")
    print(f"  {Color.paint('Passed:', Color.GREEN)}               {passed_count} / {total_runs} ({(passed_count / total_runs * 100):.1f}%)")
    print(f"  {Color.paint('Failures / Anomalies:', Color.RED if total_failures > 0 else Color.GREEN)} {total_failures}")

    if failure_counts:
        print("\n  Failure Breakdown:")
        for status, count in failure_counts.items():
            print(f"    - {status.value}: {count}")

    print(f"\n  Total Time Elapsed:   {total_time:.2f}s")
    if total_failures > 0:
        print(f"  Reproduction Bundles: {args.reproducers_dir.resolve()}")
    print(Color.paint("=" * 78, Color.CYAN))

    return 0 if total_failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
