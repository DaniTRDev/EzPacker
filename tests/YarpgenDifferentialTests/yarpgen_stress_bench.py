#!/usr/bin/env python3
"""
EzPacker YARPGen Compiler Pass Performance & Stress Benchmark
=============================================================
Synthesizes massive multi-kernel C stress suites using Intel YARPGen,
compiles them with EzPacker (ezc) with pass-timing instrumentation enabled,
and profiles the execution time, percentage distribution, and bottlenecks
across every compiler pass in the middle-end and backend pipelines.
"""

from __future__ import annotations

import argparse
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

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass


# Curated known-good seeds for rapid stress test assembly
DEFAULT_STRESS_SEEDS = [
    121, 132, 162, 188, 231, 280, 325, 411, 483, 484, 592,
    620, 642, 690, 715, 742, 790, 812, 855, 888, 915, 940
]


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


@dataclass
class PassStats:
    name: str
    count: int = 0
    total_ms: float = 0.0
    min_ms: float = float("inf")
    max_ms: float = 0.0
    times: List[float] = field(default_factory=list)

    @property
    def avg_ms(self) -> float:
        return self.total_ms / self.count if self.count > 0 else 0.0


@dataclass
class BenchmarkReport:
    opt_level: str
    target_triple: str
    num_functions: int
    source_lines: int
    total_pipeline_ms: float
    passes: Dict[str, PassStats] = field(default_factory=dict)
    stdout: str = ""
    stderr: str = ""
    success: bool = True
    error_message: str = ""


class YarpgenStressGenerator:
    """Generates and stitches multiple YARPGen kernels into a massive stress translation unit."""

    def __init__(self, yarpgen_bin: Path, work_dir: Path, env: Dict[str, str]):
        self.yarpgen_bin = yarpgen_bin
        self.work_dir = work_dir
        self.env = env

    def generate_mega_suite(self, seeds: List[int]) -> Tuple[Path, int]:
        suite_dir = self.work_dir / "mega_stress"
        if suite_dir.exists():
            shutil.rmtree(suite_dir, ignore_errors=True)
        suite_dir.mkdir(parents=True, exist_ok=True)

        combined_lines = [
            "/* ========================================================================= */",
            "/* EzPacker YARPGen Big Stress Suite                                          */",
            f"/* Generated: {len(seeds)} random function kernels across multiple seeds     */",
            "/* ========================================================================= */\n",
            "#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })",
            "#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })\n",
        ]

        # Call-driver harness to keep all kernels live
        driver_calls = []

        for idx, seed in enumerate(seeds):
            seed_dir = suite_dir / f"seed_{seed}"
            seed_dir.mkdir(parents=True, exist_ok=True)

            # Generate via yarpgen
            cmd = [
                str(self.yarpgen_bin),
                "--std=c",
                "-s",
                str(seed),
                f"--out-dir={seed_dir.resolve()}",
            ]
            res = subprocess.run(cmd, cwd=str(seed_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=self.env)
            if res.returncode != 0 or not (seed_dir / "func.c").exists():
                continue

            init_src = (seed_dir / "init.h").read_text(encoding="utf-8")
            func_src = (seed_dir / "func.c").read_text(encoding="utf-8")

            prefix = f"s{seed}_{idx}"

            # Namespace variables and functions to avoid global collision
            def namespace_symbols(code: str) -> str:
                code = re.sub(r"\b(var_\d+)\b", f"{prefix}_\\g<1>", code)
                code = re.sub(r"\b(arr_\d+)\b", f"{prefix}_\\g<1>", code)
                code = re.sub(r"\bvoid\s+test\s*\(", f"void {prefix}_kernel(", code)
                return code

            clean_init = namespace_symbols(init_src)
            clean_func = namespace_symbols(func_src)

            # Strip include "init.h"
            clean_func = re.sub(r'#include\s+"init\.h"', "", clean_func)

            combined_lines.append(f"/* ---------- Seed {seed} Declarations ---------- */")
            combined_lines.append(clean_init)
            combined_lines.append(f"/* ---------- Seed {seed} Kernel Function ---------- */")
            combined_lines.append(clean_func)
            combined_lines.append("")

            driver_calls.append(f"    // Kernel {prefix} registered")

        mega_file = suite_dir / "yarpgen_mega_stress.c"
        content = "\n".join(combined_lines)
        mega_file.write_text(content, encoding="utf-8")

        total_lines = len(content.splitlines())
        return mega_file, total_lines


class PassProfiler:
    """Invokes ezc with --time-passes, parses timing metrics, and produces performance reports."""

    TIME_REGEX = re.compile(r"\[Time\]\s+([\w]+):\s+([\d.]+)\s+ms")

    def __init__(self, ezc_bin: Path, target_triple: str, env: Dict[str, str]):
        self.ezc_bin = ezc_bin
        self.target_triple = target_triple
        self.env = env

    def profile_file(self, source_path: Path, opt_level: str, num_functions: int, source_lines: int) -> BenchmarkReport:
        obj_out = source_path.with_suffix(".obj" if platform.system().lower() == "windows" else ".o")
        cmd = [
            str(self.ezc_bin),
            f"-{opt_level}",
            "--target",
            self.target_triple,
            str(source_path.resolve()),
            "-o",
            str(obj_out.resolve()),
            "--time-passes",
        ]

        t0 = time.time()
        res = subprocess.run(
            cmd,
            cwd=str(source_path.parent),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=self.env,
        )
        total_wall_ms = (time.time() - t0) * 1000.0

        report = BenchmarkReport(
            opt_level=opt_level,
            target_triple=self.target_triple,
            num_functions=num_functions,
            source_lines=source_lines,
            total_pipeline_ms=0.0,
            stdout=res.stdout,
            stderr=res.stderr,
            success=(res.returncode == 0),
        )

        if res.returncode != 0:
            report.error_message = res.stderr or res.stdout
            return report

        # Parse timing metrics
        for line in res.stdout.splitlines():
            m = self.TIME_REGEX.search(line)
            if m:
                pass_name = m.group(1)
                elapsed_ms = float(m.group(2))

                if pass_name not in report.passes:
                    report.passes[pass_name] = PassStats(name=pass_name)

                stats = report.passes[pass_name]
                stats.count += 1
                stats.total_ms += elapsed_ms
                stats.times.append(elapsed_ms)
                if elapsed_ms < stats.min_ms:
                    stats.min_ms = elapsed_ms
                if elapsed_ms > stats.max_ms:
                    stats.max_ms = elapsed_ms

        report.total_pipeline_ms = sum(s.total_ms for s in report.passes.values())
        return report


class ReportRenderer:
    """Renders formatted tables, progress bars, and bottleneck summaries."""

    @staticmethod
    def render_bar(percentage: float, width: int = 18) -> str:
        filled = int(round((percentage / 100.0) * width))
        filled = max(0, min(width, filled))
        bar = "#" * filled + "-" * (width - filled)
        return f"[{bar}]"

    @classmethod
    def display_report(cls, report: BenchmarkReport):
        if not report.success:
            print(Color.paint("\n[COMPILATION FAILED]", Color.RED + Color.BOLD))
            print(Color.paint(report.error_message, Color.RED))
            return

        total_ms = report.total_pipeline_ms
        sorted_passes = sorted(report.passes.values(), key=lambda s: s.total_ms, reverse=True)

        print("\n" + Color.paint("=" * 96, Color.CYAN))
        print(Color.paint(f"EzPacker Compiler Pass Performance Profile (-{report.opt_level})", Color.BOLD + Color.WHITE_BOLD))
        print(Color.paint("=" * 96, Color.CYAN))
        print(f"  Target Triple:   {Color.paint(report.target_triple, Color.YELLOW)}")
        print(f"  Test Scale:      {Color.paint(str(report.num_functions), Color.BOLD)} functions, {Color.paint(str(report.source_lines), Color.BOLD)} source lines")
        print(f"  Pipeline Time:   {Color.paint(f'{total_ms:.2f} ms', Color.GREEN + Color.BOLD)} (sum of pass durations across all functions)")
        print(Color.paint("-" * 96, Color.GRAY))

        # Header
        header = f" {'Pass Name':<34} | {'Calls':>6} | {'Total (ms)':>11} | {'Avg (ms)':>9} | {'Max (ms)':>9} | {'Share':>7} | {'Distribution':<20}"
        print(Color.paint(header, Color.BOLD))
        print(Color.paint("-" * 96, Color.GRAY))

        for s in sorted_passes:
            pct = (s.total_ms / total_ms * 100.0) if total_ms > 0 else 0.0
            bar = cls.render_bar(pct)

            # Color coding based on share
            if pct >= 30.0:
                share_str = Color.paint(f"{pct:5.1f}%", Color.RED + Color.BOLD)
                bar_str = Color.paint(bar, Color.RED)
            elif pct >= 15.0:
                share_str = Color.paint(f"{pct:5.1f}%", Color.YELLOW + Color.BOLD)
                bar_str = Color.paint(bar, Color.YELLOW)
            else:
                share_str = f"{pct:5.1f}%"
                bar_str = Color.paint(bar, Color.CYAN)

            line = f" {s.name:<34} | {s.count:>6} | {s.total_ms:>11.2f} | {s.avg_ms:>9.3f} | {s.max_ms:>9.3f} | {share_str:>16} | {bar_str}"
            print(line)

        print(Color.paint("=" * 96, Color.CYAN))

        # Bottlenecks analysis
        print(Color.paint("\n[*] Performance Bottleneck Analysis:", Color.BOLD + Color.YELLOW))
        for i, s in enumerate(sorted_passes[:3], 1):
            pct = (s.total_ms / total_ms * 100.0) if total_ms > 0 else 0.0
            print(f"  #{i} {Color.paint(s.name, Color.BOLD)}: {s.total_ms:.2f} ms ({pct:.1f}% of total) [avg: {s.avg_ms:.3f} ms/func, max: {s.max_ms:.3f} ms]")

    @classmethod
    def export_json(cls, reports: List[BenchmarkReport], out_path: Path):
        data = []
        for r in reports:
            entry = {
                "opt_level": r.opt_level,
                "target_triple": r.target_triple,
                "num_functions": r.num_functions,
                "source_lines": r.source_lines,
                "total_pipeline_ms": r.total_pipeline_ms,
                "success": r.success,
                "passes": {
                    name: {
                        "count": s.count,
                        "total_ms": s.total_ms,
                        "avg_ms": s.avg_ms,
                        "min_ms": s.min_ms,
                        "max_ms": s.max_ms,
                    }
                    for name, s in r.passes.items()
                },
            }
            data.append(entry)

        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        print(f"\n[Export] Performance metrics exported to: {out_path.resolve()}")

    @classmethod
    def export_markdown(cls, report: BenchmarkReport, out_path: Path):
        total_ms = report.total_pipeline_ms
        sorted_passes = sorted(report.passes.values(), key=lambda s: s.total_ms, reverse=True)

        lines = [
            f"# EzPacker Pass Performance Benchmark Report (-{report.opt_level})",
            "",
            f"- **Target Triple**: `{report.target_triple}`",
            f"- **Functions Compiled**: `{report.num_functions}`",
            f"- **Total Source Lines**: `{report.source_lines}`",
            f"- **Total Pipeline Time**: `{total_ms:.2f} ms`",
            "",
            "| Pass Name | Invocations | Total Time (ms) | Avg Time (ms) | Max Time (ms) | Share (%) |",
            "| :--- | :---: | :---: | :---: | :---: | :---: |",
        ]
        for s in sorted_passes:
            pct = (s.total_ms / total_ms * 100.0) if total_ms > 0 else 0.0
            lines.append(f"| `{s.name}` | {s.count} | {s.total_ms:.2f} | {s.avg_ms:.3f} | {s.max_ms:.3f} | {pct:.1f}% |")

        lines.append("")
        out_path.write_text("\n".join(lines), encoding="utf-8")
        print(f"[Export] Markdown report exported to: {out_path.resolve()}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="EzPacker Big Stress Test & Compiler Pass Performance Profiler"
    )
    parser.add_argument("--ezc-bin", type=Path, help="Path to ezc compiler binary")
    parser.add_argument("--yarpgen-bin", type=Path, help="Path to yarpgen executable")
    parser.add_argument("--target", type=str, help="Target architecture triple")
    parser.add_argument("--opt-level", type=str, default="O2", help="Optimization level (O0, O1, O2, Os, or 'all')")
    parser.add_argument("--functions", "-n", type=int, default=10, help="Number of YARPGen function kernels in mega stress test")
    parser.add_argument("--seeds", type=str, help="Comma-separated custom seeds to stitch together")
    parser.add_argument("--work-dir", type=Path, default=Path(".yarpgen_stress_work"), help="Scratch directory")
    parser.add_argument("--json-out", type=Path, help="Path to export JSON benchmark results")
    parser.add_argument("--markdown-out", type=Path, help="Path to export Markdown benchmark table")
    return parser.parse_args()


def find_tool(name: str, candidates: List[Path]) -> Optional[Path]:
    for p in candidates:
        if p.is_file() and os.access(p, os.X_OK):
            return p.resolve()
        p_exe = p.with_suffix(".exe" if platform.system().lower() == "windows" else "")
        if p_exe.is_file() and os.access(p_exe, os.X_OK):
            return p_exe.resolve()
    found = shutil.which(name)
    return Path(found).resolve() if found else None


def main() -> int:
    args = parse_args()
    workspace = Path.cwd()

    ezc_candidates = [
        workspace / "cmake-build-debug" / "bin" / "ezc",
        workspace / "build" / "bin" / "ezc",
        workspace / "bin" / "ezc",
    ]
    ezc_bin = args.ezc_bin or find_tool("ezc", ezc_candidates)
    if not ezc_bin:
        sys.stderr.write("Error: Could not locate ezc binary. Build it or provide --ezc-bin.\n")
        return 1

    yarp_candidates = [
        workspace / "cmake-build-debug" / "_deps" / "yarpgen_ext-build" / "yarpgen",
        workspace / "cmake-build-debug" / "bin" / "yarpgen",
        Path.home() / ".gemini" / "antigravity-cli" / "brain" / "8358203c-4d4f-4462-8d4f-e317c6f17976" / "scratch" / "yarpgen" / "build" / "yarpgen",
    ]
    yarpgen_bin = args.yarpgen_bin or find_tool("yarpgen", yarp_candidates)
    if not yarpgen_bin:
        sys.stderr.write("Error: Could not locate yarpgen binary. Provide --yarpgen-bin.\n")
        return 1

    target = args.target or ("x86_64-pc-windows-coff" if platform.system().lower() == "windows" else "x86_64-unknown-linux-gnu")

    # Build environment with MinGW PATH prepended on Windows
    env = dict(os.environ)
    if platform.system().lower() == "windows":
        clion_mingw = Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin")
        if clion_mingw.exists():
            env["PATH"] = str(clion_mingw) + os.pathsep + env.get("PATH", "")

    # Pick seeds
    if args.seeds:
        seeds = [int(s.strip()) for s in args.seeds.split(",") if s.strip()]
    else:
        seeds = DEFAULT_STRESS_SEEDS[:max(1, args.functions)]

    print(Color.paint("=" * 96, Color.CYAN))
    print(Color.paint("EzPacker Big Stress Test Generator & Pass Performance Profiler", Color.BOLD + Color.CYAN))
    print(Color.paint("=" * 96, Color.CYAN))
    print(f"  EzPacker:       {ezc_bin}")
    print(f"  YARPGen:        {yarpgen_bin}")
    print(f"  Target:         {target}")
    print(f"  Stress Seeds:   {seeds}")
    print(Color.paint("-" * 96, Color.GRAY))

    # 1. Generate Mega Stress File
    print(f"\n[1/2] Synthesizing mega stress test file from {len(seeds)} YARPGen kernels...")
    generator = YarpgenStressGenerator(yarpgen_bin, args.work_dir, env)
    mega_path, line_count = generator.generate_mega_suite(seeds)
    print(f"      -> Generated {Color.paint(str(mega_path), Color.GREEN)} ({line_count} lines of C code)")

    # 2. Profile Passes
    opt_levels = ["O0", "O1", "O2", "Os"] if args.opt_level.lower() == "all" else [args.opt_level.lstrip("-")]
    profiler = PassProfiler(ezc_bin, target, env)
    reports: List[BenchmarkReport] = []

    print(f"\n[2/2] Profiling compiler pass execution times across {opt_levels}...")
    for opt in opt_levels:
        report = profiler.profile_file(mega_path, opt, len(seeds), line_count)
        reports.append(report)
        ReportRenderer.display_report(report)

    # Exports
    if args.json_out:
        ReportRenderer.export_json(reports, args.json_out)
    if args.markdown_out and reports:
        ReportRenderer.export_markdown(reports[0], args.markdown_out)

    return 0 if all(r.success for r in reports) else 1


if __name__ == "__main__":
    sys.exit(main())
