#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path


def load_benchmarks(path: Path) -> tuple[dict[str, float], str]:
    with path.open("r", encoding="utf-8") as file:
        data = json.load(file)

    values: dict[str, float] = {}
    units: set[str] = set()
    for benchmark in data.get("benchmarks", []):
        name = benchmark.get("name")
        cpu_time = benchmark.get("cpu_time")
        time_unit = benchmark.get("time_unit")
        if not isinstance(name, str) or not isinstance(cpu_time, (int, float)):
            continue
        values[name] = float(cpu_time)
        if isinstance(time_unit, str):
            units.add(time_unit)

    if not values:
        raise ValueError(f"No benchmark cpu_time values found in {path}")
    if len(units) > 1:
        raise ValueError(f"Multiple time units found in {path}: {sorted(units)}")

    return values, next(iter(units), "unknown")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compute statistics for two Google Benchmark JSON files."
    )
    parser.add_argument(
        "left",
        help="First benchmark JSON file.",
    )
    parser.add_argument(
        "right",
        help="Second benchmark JSON file.",
    )
    parser.add_argument("-o", "--output", default="compared_stats.txt")
    parser.add_argument("--top", type=int, default=10, help="Top improved/regressed rows to show.")
    return parser.parse_args()


def stats_line(values: list[float]) -> dict[str, float]:
    if not values:
        return {
            "count": 0,
            "mean": math.nan,
            "median": math.nan,
            "stdev": math.nan,
            "min": math.nan,
            "max": math.nan,
        }

    return {
        "count": len(values),
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
        "stdev": statistics.pstdev(values) if len(values) > 1 else 0.0,
        "min": min(values),
        "max": max(values),
    }


def geometric_mean(values: list[float]) -> float:
    if not values:
        return math.nan
    return math.exp(statistics.fmean(math.log(value) for value in values if value > 0.0))


def format_summary(name: str, values: list[float], unit: str = "") -> list[str]:
    summary = stats_line(values)
    suffix = f" {unit}" if unit else ""
    return [
        f"{name}:",
        f"  count  = {summary['count']}",
        f"  mean   = {summary['mean']:.6f}{suffix}",
        f"  median = {summary['median']:.6f}{suffix}",
        f"  stdev  = {summary['stdev']:.6f}{suffix}",
        f"  min    = {summary['min']:.6f}{suffix}",
        f"  max    = {summary['max']:.6f}{suffix}",
    ]


def benchmark_kind(name: str) -> str:
    return name.split("<", 1)[0]


def container_kind(name: str) -> str:
    match = re.search(r"(MultiMap|MultiSet|Map|Set)", name)
    return match.group(1) if match else "Unknown"


def build_report(
    left_path: Path,
    right_path: Path,
    left_values: dict[str, float],
    right_values: dict[str, float],
    time_unit: str,
    top_n: int,
) -> str:
    matched_names = sorted(set(left_values) & set(right_values))
    left_only = sorted(set(left_values) - set(right_values))
    right_only = sorted(set(right_values) - set(left_values))

    if not matched_names:
        raise ValueError("No common benchmark names found.")

    rows = []
    improved = 0
    regressed = 0
    unchanged = 0

    grouped_rows: dict[str, list[dict[str, float | str]]] = defaultdict(list)
    container_rows: dict[str, list[dict[str, float | str]]] = defaultdict(list)

    for name in matched_names:
        left = left_values[name]
        right = right_values[name]
        delta = right - left
        percent = (delta / left) * 100.0
        speedup = left / right if right != 0.0 else math.inf

        if math.isclose(left, right, rel_tol=1e-12, abs_tol=1e-12):
            unchanged += 1
        elif right < left:
            improved += 1
        else:
            regressed += 1

        row = {
            "name": name,
            "left": left,
            "right": right,
            "delta": delta,
            "percent": percent,
            "speedup": speedup,
        }
        rows.append(row)
        grouped_rows[benchmark_kind(name)].append(row)
        container_rows[container_kind(name)].append(row)

    delta_values = [row["delta"] for row in rows]
    percent_values = [row["percent"] for row in rows]
    speedup_values = [row["speedup"] for row in rows if row["speedup"] > 0.0]

    report: list[str] = []
    report.append("Benchmark CPU Time Comparison Statistics")
    report.append("")
    report.append(f"left  = {left_path}")
    report.append(f"right = {right_path}")
    report.append(f"matched benchmarks = {len(matched_names)}")
    report.append(f"left-only benchmarks  = {len(left_only)}")
    report.append(f"right-only benchmarks = {len(right_only)}")
    report.append("")
    report.append("Interpretation:")
    report.append("  Lower cpu_time is better.")
    report.append("  delta% = (right - left) / left * 100")
    report.append("  speedup = left / right (> 1 means right is faster)")
    report.append("")
    report.append("Outcome counts:")
    report.append(f"  improved  = {improved}")
    report.append(f"  regressed = {regressed}")
    report.append(f"  unchanged = {unchanged}")
    report.append("")

    report.extend(format_summary("cpu_time delta", delta_values, time_unit))
    report.append("")
    report.extend(format_summary("cpu_time delta percent", percent_values, "%"))
    report.append("")
    report.extend(format_summary("speedup", speedup_values))
    report.append(f"  geometric_mean = {geometric_mean(speedup_values):.6f}")
    report.append("")

    report.append("Per benchmark kind:")
    for kind in sorted(grouped_rows):
        kind_rows = grouped_rows[kind]
        kind_percents = [row["percent"] for row in kind_rows]
        kind_speedups = [row["speedup"] for row in kind_rows if row["speedup"] > 0.0]
        kind_improved = sum(1 for row in kind_rows if row["right"] < row["left"])
        kind_regressed = sum(1 for row in kind_rows if row["right"] > row["left"])
        kind_unchanged = len(kind_rows) - kind_improved - kind_regressed
        report.append(
            "  "
            f"{kind}: count={len(kind_rows)}, improved={kind_improved}, "
            f"regressed={kind_regressed}, unchanged={kind_unchanged}, "
            f"mean_delta%={statistics.fmean(kind_percents):.3f}, "
            f"median_delta%={statistics.median(kind_percents):.3f}, "
            f"geomean_speedup={geometric_mean(kind_speedups):.6f}"
        )
    report.append("")

    report.append("Per container kind:")
    for kind in ("Set", "MultiSet", "Map", "MultiMap", "Unknown"):
        if kind not in container_rows:
            continue
        kind_rows = container_rows[kind]
        kind_percents = [row["percent"] for row in kind_rows]
        kind_speedups = [row["speedup"] for row in kind_rows if row["speedup"] > 0.0]
        kind_improved = sum(1 for row in kind_rows if row["right"] < row["left"])
        kind_regressed = sum(1 for row in kind_rows if row["right"] > row["left"])
        kind_unchanged = len(kind_rows) - kind_improved - kind_regressed
        report.append(
            "  "
            f"{kind}: count={len(kind_rows)}, improved={kind_improved}, "
            f"regressed={kind_regressed}, unchanged={kind_unchanged}, "
            f"mean_delta%={statistics.fmean(kind_percents):.3f}, "
            f"median_delta%={statistics.median(kind_percents):.3f}, "
            f"geomean_speedup={geometric_mean(kind_speedups):.6f}"
        )
    report.append("")

    improved_rows = sorted(rows, key=lambda row: row["percent"])[:top_n]
    regressed_rows = sorted(rows, key=lambda row: row["percent"], reverse=True)[:top_n]

    report.append(f"Top {top_n} improvements:")
    for row in improved_rows:
        report.append(
            "  "
            f"{row['percent']:+8.3f}%  speedup={row['speedup']:.6f}  "
            f"{row['name']}"
        )
    report.append("")

    report.append(f"Top {top_n} regressions:")
    for row in regressed_rows:
        report.append(
            "  "
            f"{row['percent']:+8.3f}%  speedup={row['speedup']:.6f}  "
            f"{row['name']}"
        )

    return "\n".join(report) + "\n"


def main() -> None:
    args = parse_args()
    left_path = Path(args.left)
    right_path = Path(args.right)
    output_path = Path(args.output)

    left_values, left_unit = load_benchmarks(left_path)
    right_values, right_unit = load_benchmarks(right_path)
    if left_unit != right_unit:
        raise ValueError(
            f"Time units do not match: {left_path}={left_unit}, {right_path}={right_unit}"
        )

    report = build_report(
        left_path,
        right_path,
        left_values,
        right_values,
        left_unit,
        args.top,
    )
    output_path.write_text(report, encoding="utf-8")
    print(report, end="")
    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()
