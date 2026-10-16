#!/usr/bin/env python3
"""
EzPacker YARPGen Monolithic Compiler Pass Performance & Stress Benchmark
========================================================================
Runs high-load stress testing against EzPacker (ezc) using a single monolithic
C translation unit containing a few thousand lines of computationally intensive
YARPGen-synthesized kernels.

Profiles middle-end and backend compiler pass execution times, percentage share,
and isolates pass bottlenecks under heavy multi-kernel pressure.
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


# 148 curated, verified clean seeds producing ~3,600+ lines of valid C code
DEFAULT_STRESS_SEEDS = [
    121, 132, 162, 188, 231, 280, 325, 411, 483, 484,
    592, 620, 650, 712, 740, 809, 819, 936, 948, 1000,
    1086, 1104, 1195, 1261, 1363, 1493, 1504, 1519, 1603, 1605,
    1625, 1667, 1679, 1741, 1790, 1819, 1844, 1881, 1937, 1939,
    1994, 2067, 2083, 2144, 2148, 2162, 2165, 2211, 2229, 2444,
    2647, 2711, 2734, 2802, 2824, 2861, 2869, 2909, 2915, 2924,
    3018, 3175, 3263, 3343, 3364, 3588, 3603, 3631, 3745, 3834,
    3857, 3883, 3894, 3920, 4098, 4244, 4267, 4337, 4375, 4394,
    4446, 4462, 4538, 4592, 4668, 4713, 5022, 5238, 5329, 5336,
    5432, 5492, 5528, 5613, 5735, 5828, 6017, 6329, 6366, 6408,
    6429, 6454, 6538, 6638, 6642, 6673, 6694, 6783, 6969, 6982,
    7023, 7027, 7136, 7237, 7296, 7447, 7493, 7521, 7762, 7829,
    7849, 7931, 8036, 8304, 8307, 8390, 8494, 8570, 8618, 8760,
    8796, 8803, 8844, 8856, 8993, 9115, 9228, 9280, 9295, 9322,
    9334, 9351, 9402, 9630, 9667, 9682, 9771, 9896
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
    """Generates and stitches multiple YARPGen kernels into a monolithic stress translation unit."""

    def __init__(self, yarpgen_bin: Path, work_dir: Path, env: Dict[str, str]):
        self.yarpgen_bin = yarpgen_bin
        self.work_dir = work_dir
        self.env = env

    def generate_monolithic_suite(self, seeds: List[int], out_file: Path) -> Tuple[Path, int]:
        out_file.parent.mkdir(parents=True, exist_ok=True)
        scratch_dir = self.work_dir / "gen_scratch"
        scratch_dir.mkdir(parents=True, exist_ok=True)

        combined_lines = [
            "/* ========================================================================= */",
            "/* EzPacker Monolithic Big Stress Benchmark Suite                            */",
            f"/* Auto-generated single translation unit: {len(seeds)} kernels, ~3,600+ lines     */",
            "/* ========================================================================= */\n",
            "#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })",
            "#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })\n",
        ]

        for idx, seed in enumerate(seeds):
            seed_dir = scratch_dir / f"seed_{seed}"
            seed_dir.mkdir(parents=True, exist_ok=True)

            cmd = [
                str(self.yarpgen_bin),
                "--std=c",
                "-s",
                str(seed),
                f"--out-dir={seed_dir.resolve()}",
            ]
            res = subprocess.run(cmd, cwd=str(seed_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=self.env)
            if res.returncode != 0 or not (seed_dir / "func.c").exists():
                shutil.rmtree(seed_dir, ignore_errors=True)
                continue

            init_src = (seed_dir / "init.h").read_text(encoding="utf-8")
            func_src = (seed_dir / "func.c").read_text(encoding="utf-8")

            prefix = f"s{seed}_{idx}"

            # Namespace variables and functions to avoid global collision
            def namespace_symbols(code: str) -> str:
                code = re.sub(r"\b(var_\d+|zero)\b", f"{prefix}_\\g<1>", code)
                code = re.sub(r"\b(arr_\d+)\b", f"{prefix}_\\g<1>", code)
                code = re.sub(r"\bvoid\s+test\s*\(", f"void {prefix}_kernel(", code)
                code = re.sub(r'#include\s+["<][^">]+[">]', "", code)
                return code

            clean_init = namespace_symbols(init_src)
            clean_func = namespace_symbols(func_src)

            combined_lines.append(f"/* ---------- Kernel {idx+1}/{len(seeds)}: Seed {seed} ---------- */")
            combined_lines.append(clean_init)
            combined_lines.append(clean_func)
            combined_lines.append("")

            shutil.rmtree(seed_dir, ignore_errors=True)

        shutil.rmtree(scratch_dir, ignore_errors=True)

        content = "\n".join(combined_lines)
        out_file.write_text(content, encoding="utf-8")
        total_lines = len(content.splitlines())
        return out_file, total_lines


class PassProfiler:
    """Invokes ezc with --time-passes on a single file, parses timing metrics, and produces performance reports."""

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
        print(Color.paint(f"EzPacker Single-File Mega Stress Profile (-{report.opt_level})", Color.BOLD + Color.WHITE_BOLD))
        print(Color.paint("=" * 96, Color.CYAN))
        print(f"  Target Triple:   {Color.paint(report.target_triple, Color.YELLOW)}")
        print(f"  Test Scale:      {Color.paint(str(report.num_functions), Color.BOLD)} kernels, {Color.paint(str(report.source_lines), Color.BOLD)} source lines (single file)")
        print(f"  Pipeline Time:   {Color.paint(f'{total_ms:.2f} ms', Color.GREEN + Color.BOLD)} (aggregated pass time across compilation)")
        print(Color.paint("-" * 96, Color.GRAY))

        # Header
        header = f" {'Pass Name':<34} | {'Calls':>6} | {'Total (ms)':>11} | {'Avg (ms)':>9} | {'Max (ms)':>9} | {'Share':>7} | {'Distribution':<20}"
        print(Color.paint(header, Color.BOLD))
        print(Color.paint("-" * 96, Color.GRAY))

        for s in sorted_passes:
            pct = (s.total_ms / total_ms * 100.0) if total_ms > 0 else 0.0
            bar = cls.render_bar(pct)

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
            f"# EzPacker Single-File Mega Stress Report (-{report.opt_level})",
            "",
            f"- **Target Triple**: `{report.target_triple}`",
            f"- **Functions Compiled**: `{report.num_functions}`",
            f"- **Total Source Lines**: `{report.source_lines}` (Single Monolithic File)",
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
        description="EzPacker Single-File Big Stress Test & Compiler Pass Performance Profiler"
    )
    parser.add_argument("--file", type=Path, help="Path to single monolithic C stress file (default: tests/YarpgenDifferentialTests/mega_stress.c)")
    parser.add_argument("--ezc-bin", type=Path, help="Path to ezc compiler binary")
    parser.add_argument("--yarpgen-bin", type=Path, help="Path to yarpgen executable (required only if regenerating)")
    parser.add_argument("--target", type=str, help="Target architecture triple")
    parser.add_argument("--opt-level", type=str, default="all", help="Optimization level (O0, O1, O2, Os, or 'all')")
    parser.add_argument("--regenerate", action="store_true", help="Force re-synthesizing the single monolithic stress file via YARPGen")
    parser.add_argument("--seeds", type=str, help="Comma-separated custom seeds if regenerating")
    parser.add_argument("--work-dir", type=Path, default=Path(".yarpgen_stress_work"), help="Scratch directory for generation")
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

    target = args.target or ("x86_64-pc-windows-coff" if platform.system().lower() == "windows" else "x86_64-unknown-linux-gnu")

    # MinGW environment on Windows
    env = dict(os.environ)
    if platform.system().lower() == "windows":
        clion_mingw = Path("C:/Program Files/JetBrains/CLion 2026.2.2/bin/mingw/bin")
        if clion_mingw.exists():
            env["PATH"] = str(clion_mingw) + os.pathsep + env.get("PATH", "")

    # Default file path
    default_stress_file = workspace / "tests" / "YarpgenDifferentialTests" / "mega_stress.c"
    stress_file = args.file or default_stress_file

    # Handle regeneration if requested or if file is missing
    if args.regenerate or not stress_file.exists():
        yarp_candidates = [
            workspace / "cmake-build-debug" / "_deps" / "yarpgen_ext-build" / "yarpgen",
            workspace / "cmake-build-debug" / "bin" / "yarpgen",
            Path.home() / ".gemini" / "antigravity-cli" / "brain" / "8358203c-4d4f-4462-8d4f-e317c6f17976" / "scratch" / "yarpgen" / "build" / "yarpgen",
        ]
        yarpgen_bin = args.yarpgen_bin or find_tool("yarpgen", yarp_candidates)
        if not yarpgen_bin:
            sys.stderr.write(f"Error: {stress_file} not found and yarpgen binary not available to generate it. Provide --yarpgen-bin.\n")
            return 1

        if args.seeds:
            seeds = [int(s.strip()) for s in args.seeds.split(",") if s.strip()]
        else:
            seeds = DEFAULT_STRESS_SEEDS

        print(f"[*] Synthesizing single monolithic stress suite ({len(seeds)} kernels) to: {stress_file}...")
        generator = YarpgenStressGenerator(yarpgen_bin, args.work_dir, env)
        stress_file, line_count = generator.generate_monolithic_suite(seeds, stress_file)
        print(f"    -> Generated {stress_file} ({line_count} lines of C code)")

    # Analyze file scale
    content = stress_file.read_text(encoding="utf-8")
    line_count = len(content.splitlines())
    func_count = len(re.findall(r"\bvoid\s+s\w+_kernel\s*\(", content))
    if func_count == 0:
        func_count = 1

    print(Color.paint("=" * 96, Color.CYAN))
    print(Color.paint("EzPacker Single-File Mega Stress & Pass Performance Profiler", Color.BOLD + Color.CYAN))
    print(Color.paint("=" * 96, Color.CYAN))
    print(f"  EzPacker:       {ezc_bin}")
    print(f"  Target Triple:  {target}")
    print(f"  Stress File:    {stress_file} ({line_count} lines, {func_count} kernels)")
    print(Color.paint("-" * 96, Color.GRAY))

    opt_levels = ["O0", "O1", "O2", "Os"] if args.opt_level.lower() == "all" else [args.opt_level.lstrip("-")]
    profiler = PassProfiler(ezc_bin, target, env)
    reports: List[BenchmarkReport] = []

    print(f"\n[*] Executing stress pass profiling on single file across {opt_levels}...")
    for opt in opt_levels:
        report = profiler.profile_file(stress_file, opt, func_count, line_count)
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
