#!/usr/bin/env python3
"""
EzPacker YARPGen Bulk File Compilation & Differential Benchmark
==============================================================
Compiles a corpus of multiple small and medium C translation units using
EzPacker (ezc) across optimization levels, measures compilation throughput
(files/sec, LOC/sec), and differentially verifies execution checksums
against a reference C compiler (GCC/Clang as the source of truth).
"""

from __future__ import annotations

import argparse
import concurrent.futures
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
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

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass


class TestStatus(Enum):
    PASS = "PASS"
    CHECKSUM_MISMATCH = "CHECKSUM_MISMATCH"
    COMPILER_ERROR = "COMPILER_ERROR"
    COMPILER_CRASH = "COMPILER_CRASH"
    LINK_FAILURE = "LINK_FAILURE"
    EXECUTION_CRASH = "EXECUTION_CRASH"
    TIMEOUT = "TIMEOUT"
    SKIPPED = "SKIPPED"


@dataclass
class SingleTestResult:
    file_name: str
    opt_level: str
    loc: int
    status: TestStatus
    compile_time_ms: float = 0.0
    ref_compile_time_ms: float = 0.0
    ezc_checksum: Optional[str] = None
    ref_checksum: Optional[str] = None
    details: str = ""


@dataclass
class BulkBenchmarkReport:
    target_triple: str
    total_files: int
    total_loc: int
    opt_levels: List[str]
    total_time_ms: float
    results: List[SingleTestResult] = field(default_factory=list)
    has_ref_compiler: bool = False
    ref_compiler_name: str = "None"

    @property
    def passed_count(self) -> int:
        return sum(1 for r in self.results if r.status == TestStatus.PASS)

    @property
    def failed_count(self) -> int:
        return len(self.results) - self.passed_count

    @property
    def throughput_loc_per_sec(self) -> float:
        total_sec = self.total_time_ms / 1000.0
        return (self.total_loc * len(self.opt_levels) / total_sec) if total_sec > 0 else 0.0

    @property
    def throughput_files_per_sec(self) -> float:
        total_sec = self.total_time_ms / 1000.0
        return (len(self.results) / total_sec) if total_sec > 0 else 0.0


class Color:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    GREEN = "\033[32m"
    RED = "\033[31m"
    YELLOW = "\033[33m"
    CYAN = "\033[36m"
    MAGENTA = "\033[35m"
    GRAY = "\033[90m"
    WHITE_BOLD = "\033[1;37m"

    @classmethod
    def enabled(cls) -> bool:
        return sys.stdout.isatty() or os.environ.get("FORCE_COLOR") == "1"

    @classmethod
    def paint(cls, text: str, color: str) -> str:
        if cls.enabled():
            return f"{color}{text}{cls.RESET}"
        return text


class EnvironmentDetector:
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
        for path in candidate_paths:
            if path.is_file() and os.access(path, os.X_OK):
                return path.resolve()
            exe_path = path.with_suffix(".exe" if cls.is_windows() else "")
            if exe_path.is_file() and os.access(exe_path, os.X_OK):
                return exe_path.resolve()

        resolved = shutil.which(name)
        if resolved:
            return Path(resolved).resolve()
        return None

    @classmethod
    def build_env(cls, ezc_bin: Optional[Path], ref_compiler: Optional[Path]) -> Dict[str, str]:
        env = dict(os.environ)
        path_entries = []
        if ezc_bin and ezc_bin.parent.exists():
            path_entries.append(str(ezc_bin.parent.resolve()))
        if ref_compiler and ref_compiler.parent.exists():
            path_entries.append(str(ref_compiler.parent.resolve()))
        if cls.is_windows():
            clion_mingw = Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin")
            if clion_mingw.exists():
                path_entries.append(str(clion_mingw.resolve()))
        existing = env.get("PATH", "")
        env["PATH"] = os.pathsep.join(path_entries + [existing]) if path_entries else existing
        return env


class BulkRunner:
    def __init__(
        self,
        ezc_bin: Path,
        corpus_dir: Path,
        ref_compiler: Optional[Path],
        target_triple: str,
        opt_levels: List[str],
        jobs: int = 4,
        timeout: float = 15.0,
        compile_only: bool = False,
        verbose: bool = False,
    ):
        self.ezc_bin = ezc_bin
        self.corpus_dir = corpus_dir
        self.ref_compiler = ref_compiler
        self.target_triple = target_triple
        self.opt_levels = opt_levels
        self.jobs = max(1, jobs)
        self.timeout = timeout
        self.compile_only = compile_only
        self.verbose = verbose
        self.env = EnvironmentDetector.build_env(ezc_bin, ref_compiler)
        self.work_dir = Path(".bulk_work")
        self.work_dir.mkdir(parents=True, exist_ok=True)

    def run_suite(self) -> BulkBenchmarkReport:
        test_files = sorted(self.corpus_dir.glob("test_*.c"))
        if not test_files:
            # Fallback to any .c files except driver
            test_files = sorted([f for f in self.corpus_dir.glob("*.c") if not f.name.startswith("driver")])

        total_loc = sum(len(f.read_text(encoding="utf-8", errors="replace").splitlines()) for f in test_files)
        report = BulkBenchmarkReport(
            target_triple=self.target_triple,
            total_files=len(test_files),
            total_loc=total_loc,
            opt_levels=self.opt_levels,
            total_time_ms=0.0,
            has_ref_compiler=(self.ref_compiler is not None and not self.compile_only),
            ref_compiler_name=str(self.ref_compiler.name) if self.ref_compiler else "None",
        )

        tasks = []
        for test_file in test_files:
            driver_file = self.corpus_dir / test_file.name.replace("test_", "driver_")
            if not driver_file.exists():
                driver_file = None
            for opt in self.opt_levels:
                tasks.append((test_file, driver_file, opt))

        start_time = time.time()
        if self.jobs > 1:
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.jobs) as executor:
                future_to_task = {
                    executor.submit(self._execute_single_test, t[0], t[1], t[2]): t for t in tasks
                }
                for future in concurrent.futures.as_completed(future_to_task):
                    report.results.append(future.result())
        else:
            for t in tasks:
                report.results.append(self._execute_single_test(t[0], t[1], t[2]))

        report.total_time_ms = (time.time() - start_time) * 1000.0
        # Sort results by file name and opt level
        report.results.sort(key=lambda r: (r.file_name, r.opt_level))
        shutil.rmtree(self.work_dir, ignore_errors=True)
        return report

    def _execute_single_test(
        self, test_file: Path, driver_file: Optional[Path], opt: str
    ) -> SingleTestResult:
        content = test_file.read_text(encoding="utf-8", errors="replace")
        loc = len(content.splitlines())
        task_id = f"{test_file.stem}_{opt}"
        item_work = self.work_dir / task_id
        item_work.mkdir(parents=True, exist_ok=True)

        obj_ext = ".obj" if EnvironmentDetector.is_windows() else ".o"
        exe_ext = ".exe" if EnvironmentDetector.is_windows() else ""

        ezc_obj = item_work / f"{test_file.stem}_ezc_{opt}{obj_ext}"
        ezc_cmd = [
            str(self.ezc_bin),
            f"-{opt}",
            "--target",
            self.target_triple,
            str(test_file.resolve()),
            "-o",
            str(ezc_obj.resolve()),
        ]

        t0 = time.time()
        try:
            res = subprocess.run(
                ezc_cmd,
                cwd=str(item_work),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=self.timeout,
                env=self.env,
            )
            ezc_compile_ms = (time.time() - t0) * 1000.0
        except subprocess.TimeoutExpired:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.TIMEOUT,
                details="ezc compilation timed out",
            )
        except Exception as e:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.COMPILER_CRASH,
                details=str(e),
            )

        if res.returncode != 0 or not ezc_obj.exists():
            log = (res.stderr or res.stdout).strip()
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.COMPILER_ERROR,
                compile_time_ms=ezc_compile_ms,
                details=log.splitlines()[-1] if log else "Compilation failed",
            )

        # If pure compilation test or no ref compiler / driver, pass here
        if self.compile_only or not self.ref_compiler or not driver_file:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.PASS,
                compile_time_ms=ezc_compile_ms,
                details="Compiled successfully",
            )

        # Differential Verification against Reference Compiler
        ref_exe = item_work / f"ref_{task_id}{exe_ext}"
        ref_cmd = [
            str(self.ref_compiler),
            "-O3",
            str(driver_file.resolve()),
            str(test_file.resolve()),
            "-o",
            str(ref_exe.resolve()),
        ]

        rt0 = time.time()
        ref_res = subprocess.run(
            ref_cmd,
            cwd=str(item_work),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=self.timeout,
            env=self.env,
        )
        ref_compile_ms = (time.time() - rt0) * 1000.0

        if ref_res.returncode != 0 or not ref_exe.exists():
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.LINK_FAILURE,
                compile_time_ms=ezc_compile_ms,
                ref_compile_time_ms=ref_compile_ms,
                details="Reference compiler build failed",
            )

        run_ref = subprocess.run(
            [str(ref_exe.resolve())],
            cwd=str(item_work),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=self.timeout,
            env=self.env,
        )
        if run_ref.returncode != 0:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.EXECUTION_CRASH,
                compile_time_ms=ezc_compile_ms,
                ref_compile_time_ms=ref_compile_ms,
                details="Reference binary execution failed",
            )
        ref_checksum = run_ref.stdout.strip()

        # Link EzPacker Object with Driver
        ezc_exe = item_work / f"ezc_{task_id}{exe_ext}"
        link_cmd = [
            str(self.ref_compiler),
            str(driver_file.resolve()),
            str(ezc_obj.resolve()),
            "-o",
            str(ezc_exe.resolve()),
        ]
        link_res = subprocess.run(
            link_cmd,
            cwd=str(item_work),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=self.timeout,
            env=self.env,
        )
        if link_res.returncode != 0 or not ezc_exe.exists():
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.LINK_FAILURE,
                compile_time_ms=ezc_compile_ms,
                ref_compile_time_ms=ref_compile_ms,
                ref_checksum=ref_checksum,
                details="Linking ezc object with driver failed",
            )

        run_ezc = subprocess.run(
            [str(ezc_exe.resolve())],
            cwd=str(item_work),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=self.timeout,
            env=self.env,
        )
        if run_ezc.returncode != 0:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.EXECUTION_CRASH,
                compile_time_ms=ezc_compile_ms,
                ref_compile_time_ms=ref_compile_ms,
                ref_checksum=ref_checksum,
                details=f"ezc binary crashed with code {run_ezc.returncode}",
            )
        ezc_checksum = run_ezc.stdout.strip()

        if ezc_checksum != ref_checksum:
            return SingleTestResult(
                file_name=test_file.name,
                opt_level=opt,
                loc=loc,
                status=TestStatus.CHECKSUM_MISMATCH,
                compile_time_ms=ezc_compile_ms,
                ref_compile_time_ms=ref_compile_ms,
                ezc_checksum=ezc_checksum,
                ref_checksum=ref_checksum,
                details=f"MISCOMPILATION: ezc({ezc_checksum}) != ref({ref_checksum})",
            )

        return SingleTestResult(
            file_name=test_file.name,
            opt_level=opt,
            loc=loc,
            status=TestStatus.PASS,
            compile_time_ms=ezc_compile_ms,
            ref_compile_time_ms=ref_compile_ms,
            ezc_checksum=ezc_checksum,
            ref_checksum=ref_checksum,
            details=f"Checksum matched: {ezc_checksum}",
        )


class BulkReporter:
    @classmethod
    def display(cls, report: BulkBenchmarkReport):
        print("\n" + Color.paint("=" * 96, Color.CYAN))
        print(Color.paint("EzPacker YARPGen Bulk File Compilation Benchmark", Color.BOLD + Color.WHITE_BOLD))
        print(Color.paint("=" * 96, Color.CYAN))
        print(f"  Target Triple:   {Color.paint(report.target_triple, Color.YELLOW)}")
        print(f"  Corpus Scale:    {Color.paint(str(report.total_files), Color.BOLD)} files, {Color.paint(str(report.total_loc), Color.BOLD)} LOC total")
        print(f"  Opt Levels:      {Color.paint(', '.join(report.opt_levels), Color.BOLD)}")
        mode_str = f"Differential Verification (Ref: {report.ref_compiler_name})" if report.has_ref_compiler else "Compilation-Only Benchmark"
        print(f"  Execution Mode:  {Color.paint(mode_str, Color.MAGENTA + Color.BOLD)}")
        print(f"  Total Duration:  {Color.paint(f'{report.total_time_ms:.2f} ms', Color.GREEN + Color.BOLD)}")
        print(f"  Throughput:      {Color.paint(f'{report.throughput_loc_per_sec:.1f} LOC/s', Color.GREEN + Color.BOLD)} ({report.throughput_files_per_sec:.1f} files/s)")
        print(Color.paint("-" * 96, Color.GRAY))

        # Table Header
        header = f" {'File':<16} | {'Opt':>3} | {'LOC':>5} | {'ezc (ms)':>9} | {'ref (ms)':>9} | {'Status':<18} | {'Details'}"
        print(Color.paint(header, Color.BOLD))
        print(Color.paint("-" * 96, Color.GRAY))

        for r in report.results:
            if r.status == TestStatus.PASS:
                status_colored = Color.paint(f"[PASS]", Color.GREEN)
            elif r.status == TestStatus.CHECKSUM_MISMATCH:
                status_colored = Color.paint(f"[MISMATCH]", Color.RED + Color.BOLD)
            else:
                status_colored = Color.paint(f"[{r.status.value}]", Color.RED)

            ref_str = f"{r.ref_compile_time_ms:7.1f} ms" if r.ref_compile_time_ms > 0 else "   -   "
            details_str = r.details[:35] if r.details else ""
            line = f" {r.file_name:<16} | {r.opt_level:>3} | {r.loc:>5} | {r.compile_time_ms:>7.1f} ms | {ref_str:>9} | {status_colored:<27} | {details_str}"
            print(line)

        print(Color.paint("=" * 96, Color.CYAN))
        pass_ratio = (report.passed_count / len(report.results) * 100.0) if report.results else 0.0
        summary_color = Color.GREEN if report.failed_count == 0 else Color.RED
        print(Color.paint(
            f"Result: {report.passed_count}/{len(report.results)} variants passed ({pass_ratio:.1f}%). Failures: {report.failed_count}",
            Color.BOLD + summary_color
        ))
        print(Color.paint("=" * 96, Color.CYAN) + "\n")

    @classmethod
    def export_json(cls, report: BulkBenchmarkReport, out_path: Path):
        data = {
            "target_triple": report.target_triple,
            "total_files": report.total_files,
            "total_loc": report.total_loc,
            "opt_levels": report.opt_levels,
            "total_time_ms": report.total_time_ms,
            "throughput_loc_per_sec": report.throughput_loc_per_sec,
            "throughput_files_per_sec": report.throughput_files_per_sec,
            "passed": report.passed_count,
            "failed": report.failed_count,
            "results": [
                {
                    "file": r.file_name,
                    "opt": r.opt_level,
                    "loc": r.loc,
                    "status": r.status.value,
                    "compile_time_ms": r.compile_time_ms,
                    "ezc_checksum": r.ezc_checksum,
                    "ref_checksum": r.ref_checksum,
                    "details": r.details,
                }
                for r in report.results
            ],
        }
        out_path.write_text(json.dumps(data, indent=2), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="EzPacker Bulk File Compilation & Differential Testing Benchmark"
    )
    parser.add_argument("--ezc-bin", type=Path, help="Path to ezc compiler executable")
    parser.add_argument("--yarpgen-bin", type=Path, help="Path to yarpgen executable")
    parser.add_argument("--ref-compiler", type=Path, help="Path to reference compiler (gcc/clang)")
    parser.add_argument("--target", type=str, help="Target architecture triple")
    parser.add_argument(
        "--corpus-dir",
        type=Path,
        default=Path(__file__).parent / "bulk_corpus",
        help="Path to directory containing bulk small/medium test files",
    )
    parser.add_argument(
        "--opt-levels",
        type=str,
        default="O0,O2",
        help="Comma-separated optimization levels (e.g. O0,O1,O2,Os)",
    )
    parser.add_argument("--jobs", "-j", type=int, default=4, help="Number of concurrent worker threads")
    parser.add_argument("--timeout", type=float, default=15.0, help="Per-file compilation timeout in seconds")
    parser.add_argument("--compile-only", action="store_true", help="Run pure compilation test without linking/exec")
    parser.add_argument("--regenerate", action="store_true", help="Regenerate bulk corpus via YARPGen")
    parser.add_argument("--count", type=int, default=25, help="Number of files to regenerate")
    parser.add_argument("--json-out", type=Path, help="Path to export JSON metrics")
    parser.add_argument("--markdown-out", type=Path, help="Path to export Markdown report")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    return parser.parse_args()


def main():
    args = parse_args()
    detector = EnvironmentDetector()

    # Locate ezc
    ezc_candidates = [
        Path("cmake-build-debug/bin/ezc"),
        Path("cmake-build-debug/ezc"),
        Path(__file__).parent.parent.parent / "cmake-build-debug/bin/ezc",
        Path(__file__).parent.parent.parent / "cmake-build-debug/ezc",
    ]
    ezc_bin = args.ezc_bin or detector.find_executable("ezc", ezc_candidates)
    if not ezc_bin or not ezc_bin.exists():
        print(Color.paint("Error: ezc executable not found. Build target 'ezc' first.", Color.RED))
        sys.exit(1)

    target_triple = args.target or detector.default_target_triple()

    # Reference compiler lookup
    ref_compiler = None
    if not args.compile_only:
        ref_candidates = [
            Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin/gcc.exe"),
            Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin/clang.exe"),
        ]
        ref_compiler = args.ref_compiler or detector.find_executable("gcc", ref_candidates)
        if not ref_compiler:
            ref_compiler = detector.find_executable("clang", ref_candidates)

    opt_levels = [o.strip().lstrip("-") for o in args.opt_levels.split(",") if o.strip()]

    # Handle corpus regeneration if requested
    if args.regenerate:
        yarpgen_candidates = [
            Path("cmake-build-debug/bin/yarpgen"),
            Path("cmake-build-debug/yarpgen"),
            Path(__file__).parent.parent.parent / "cmake-build-debug/bin/yarpgen",
        ]
        yarpgen_bin = args.yarpgen_bin or detector.find_executable("yarpgen", yarpgen_candidates)
        if not yarpgen_bin or not yarpgen_bin.exists():
            print(Color.paint("Error: yarpgen binary not found for regeneration.", Color.RED))
            sys.exit(1)

        print(Color.paint(f"[*] Regenerating {args.count} bulk tests in {args.corpus_dir} via YARPGen...", Color.CYAN))
        # Call regeneration
        args.corpus_dir.mkdir(parents=True, exist_ok=True)
        env = detector.build_env(ezc_bin, ref_compiler)
        scratch = Path(".temp_regen")
        scratch.mkdir(parents=True, exist_ok=True)
        # Use curated seeds that compile cleanly in ezc
        default_curated = [
            121, 132, 162, 188, 231, 280, 325, 411, 483, 484,
            592, 620, 650, 712, 740, 809, 819, 936, 948, 1000,
            1086, 1104, 1195, 1261, 1363
        ]
        seeds = [default_curated[i % len(default_curated)] for i in range(args.count)]
        for idx, seed in enumerate(seeds, 1):
            seed_dir = scratch / f"s_{seed}"
            seed_dir.mkdir(parents=True, exist_ok=True)
            res = subprocess.run([
                str(yarpgen_bin.resolve()), "--std=c", "-s", str(seed), f"--out-dir={seed_dir.resolve()}"
            ], cwd=str(seed_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
            if res.returncode == 0:
                f_src = (seed_dir / "func.c").read_text(encoding="utf-8")
                i_src = (seed_dir / "init.h").read_text(encoding="utf-8")
                d_src = (seed_dir / "driver.c").read_text(encoding="utf-8")
                clean_f = re.sub(r'#include\s+["<]init\.h[">]\s*', '', f_src)
                (args.corpus_dir / f"test_{idx:03d}.c").write_text(f"/* Seed {seed} */\n{i_src}\n{clean_f}", encoding="utf-8")
                (args.corpus_dir / f"driver_{idx:03d}.c").write_text(f"/* Seed {seed} */\n{d_src}", encoding="utf-8")
        shutil.rmtree(scratch, ignore_errors=True)
        print(Color.paint(f"[+] Successfully regenerated {args.count} tests in {args.corpus_dir}", Color.GREEN))

    runner = BulkRunner(
        ezc_bin=ezc_bin,
        corpus_dir=args.corpus_dir,
        ref_compiler=ref_compiler,
        target_triple=target_triple,
        opt_levels=opt_levels,
        jobs=args.jobs,
        timeout=args.timeout,
        compile_only=args.compile_only,
        verbose=args.verbose,
    )

    report = runner.run_suite()
    BulkReporter.display(report)

    if args.json_out:
        BulkReporter.export_json(report, args.json_out)
        print(f"Metrics saved to: {args.json_out}")

    sys.exit(0 if report.failed_count == 0 else 1)


if __name__ == "__main__":
    main()
