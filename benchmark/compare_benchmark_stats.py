#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path

IMPLEMENTATION_ORDER = ("STL", "absl", "platanus(auto)")

BENCHMARK_PATTERN = re.compile(
    r"^(?P<benchmark>BM_[^<]+)"
    r"<(?P<impl>STL|Absl|BTree)"
    r"(?P<container>MultiMap|MultiSet|Map|Set)"
    r"<(?P<data_type>std::(?:int32_t|int64_t|string))"
    r"(?:, (?P<node_size>\d+|platanus::kAutoSize))?"
    r">>$"
)


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
        description=(
            "Compute statistics for either two Google Benchmark JSON files or "
            "one JSON file containing multiple implementations such as STL, absl, and platanus."
        )
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help=(
            "One benchmark JSON file for implementation comparison, or two JSON files "
            "for file-to-file comparison."
        ),
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


def parse_benchmark_name(name: str) -> dict[str, str] | None:
    match = BENCHMARK_PATTERN.match(name)
    if match is None:
        return None

    parsed = match.groupdict()
    impl = parsed["impl"]
    node_size = parsed["node_size"]

    if impl == "STL":
        parsed["label"] = "STL"
    elif impl == "Absl":
        parsed["label"] = "absl"
    elif impl == "BTree" and node_size == "platanus::kAutoSize":
        parsed["label"] = "platanus(auto)"
    else:
        return None

    return parsed


def build_slot_label(row: dict[str, str]) -> str:
    return f"{row['benchmark']} / {row['data_type']} / {row['container']}"


def collect_implementation_rows(
    values: dict[str, float],
) -> tuple[
    dict[tuple[str, str, str], dict[str, dict[str, str | float]]],
    list[str],
]:
    rows: dict[tuple[str, str, str], dict[str, dict[str, str | float]]] = defaultdict(dict)
    skipped: list[str] = []

    for name, cpu_time in values.items():
        parsed = parse_benchmark_name(name)
        if parsed is None:
            skipped.append(name)
            continue

        key = (parsed["benchmark"], parsed["data_type"], parsed["container"])
        rows[key][parsed["label"]] = {
            "name": name,
            "label": build_slot_label(parsed),
            "benchmark": parsed["benchmark"],
            "data_type": parsed["data_type"],
            "container": parsed["container"],
            "value": cpu_time,
        }

    return rows, skipped


def compare_row(
    slot: dict[str, dict[str, str | float]],
    left_label: str,
    right_label: str,
) -> dict[str, float | str] | None:
    left_entry = slot.get(left_label)
    right_entry = slot.get(right_label)
    if left_entry is None or right_entry is None:
        return None

    left_value = float(left_entry["value"])
    right_value = float(right_entry["value"])
    delta = right_value - left_value
    percent = (delta / left_value) * 100.0
    speedup = left_value / right_value if right_value != 0.0 else math.inf

    return {
        "name": str(left_entry["label"]),
        "benchmark": str(left_entry["benchmark"]),
        "container": str(left_entry["container"]),
        "left": left_value,
        "right": right_value,
        "delta": delta,
        "percent": percent,
        "speedup": speedup,
    }


def build_pairwise_report(
    title: str,
    left_label: str,
    right_label: str,
    rows: list[dict[str, float | str]],
    time_unit: str,
    top_n: int,
) -> list[str]:
    improved = 0
    regressed = 0
    unchanged = 0

    grouped_rows: dict[str, list[dict[str, float | str]]] = defaultdict(list)
    container_rows: dict[str, list[dict[str, float | str]]] = defaultdict(list)

    for row in rows:
        left = float(row["left"])
        right = float(row["right"])
        if math.isclose(left, right, rel_tol=1e-12, abs_tol=1e-12):
            unchanged += 1
        elif right < left:
            improved += 1
        else:
            regressed += 1

        grouped_rows[str(row["benchmark"])].append(row)
        container_rows[str(row["container"])].append(row)

    delta_values = [float(row["delta"]) for row in rows]
    percent_values = [float(row["percent"]) for row in rows]
    speedup_values = [float(row["speedup"]) for row in rows if float(row["speedup"]) > 0.0]

    report: list[str] = []
    report.append(title)
    report.append(f"  compared rows = {len(rows)}")
    report.append(f"  lower cpu_time is better")
    report.append(f"  delta% = ({right_label} - {left_label}) / {left_label} * 100")
    report.append(f"  speedup = {left_label} / {right_label} (> 1 means {right_label} is faster)")
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
        kind_percents = [float(row["percent"]) for row in kind_rows]
        kind_speedups = [float(row["speedup"]) for row in kind_rows if float(row["speedup"]) > 0.0]
        kind_improved = sum(1 for row in kind_rows if float(row["right"]) < float(row["left"]))
        kind_regressed = sum(1 for row in kind_rows if float(row["right"]) > float(row["left"]))
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
        kind_percents = [float(row["percent"]) for row in kind_rows]
        kind_speedups = [float(row["speedup"]) for row in kind_rows if float(row["speedup"]) > 0.0]
        kind_improved = sum(1 for row in kind_rows if float(row["right"]) < float(row["left"]))
        kind_regressed = sum(1 for row in kind_rows if float(row["right"]) > float(row["left"]))
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

    improved_rows = sorted(rows, key=lambda row: float(row["percent"]))[:top_n]
    regressed_rows = sorted(rows, key=lambda row: float(row["percent"]), reverse=True)[:top_n]

    report.append(f"Top {top_n} improvements:")
    for row in improved_rows:
        report.append(
            "  "
            f"{float(row['percent']):+8.3f}%  speedup={float(row['speedup']):.6f}  "
            f"{row['name']}"
        )
    report.append("")

    report.append(f"Top {top_n} regressions:")
    for row in regressed_rows:
        report.append(
            "  "
            f"{float(row['percent']):+8.3f}%  speedup={float(row['speedup']):.6f}  "
            f"{row['name']}"
        )
    report.append("")
    return report


def build_single_file_report(
    input_path: Path,
    values: dict[str, float],
    time_unit: str,
    top_n: int,
) -> str:
    slots, skipped = collect_implementation_rows(values)
    label_counts: dict[str, int] = defaultdict(int)
    for slot in slots.values():
        for label in slot:
            label_counts[label] += 1

    full_triplets = [
        slot
        for slot in slots.values()
        if all(label in slot for label in IMPLEMENTATION_ORDER)
    ]
    if not full_triplets:
        raise ValueError(
            "No benchmark groups found with STL, absl, and platanus(auto) all present."
        )

    report: list[str] = []
    report.append("Benchmark Implementation Comparison Statistics")
    report.append("")
    report.append(f"input = {input_path}")
    report.append(f"parsed benchmark rows = {len(values)}")
    report.append(f"recognized grouped rows = {len(slots)}")
    report.append(f"complete STL/absl/platanus(auto) groups = {len(full_triplets)}")
    report.append(f"skipped unrecognized rows = {len(skipped)}")
    report.append("")
    report.append("Implementation coverage:")
    for label in IMPLEMENTATION_ORDER:
        report.append(f"  {label}: {label_counts.get(label, 0)}")
    report.append("")

    winner_counts: dict[str, int] = defaultdict(int)
    tie_count = 0
    for slot in full_triplets:
        values_by_label = {label: float(slot[label]["value"]) for label in IMPLEMENTATION_ORDER}
        best_value = min(values_by_label.values())
        winners = [
            label
            for label in IMPLEMENTATION_ORDER
            if math.isclose(values_by_label[label], best_value, rel_tol=1e-12, abs_tol=1e-12)
        ]
        if len(winners) == 1:
            winner_counts[winners[0]] += 1
        else:
            tie_count += 1

    report.append("Fastest implementation counts:")
    for label in IMPLEMENTATION_ORDER:
        report.append(f"  {label}: {winner_counts.get(label, 0)}")
    report.append(f"  ties: {tie_count}")
    report.append("")

    pair_reports = [
        ("STL vs platanus(auto)", "STL", "platanus(auto)"),
        ("absl vs platanus(auto)", "absl", "platanus(auto)"),
    ]
    for title, left_label, right_label in pair_reports:
        pair_rows = [
            row
            for slot in full_triplets
            if (row := compare_row(slot, left_label, right_label)) is not None
        ]
        report.extend(
            build_pairwise_report(
                title,
                left_label,
                right_label,
                pair_rows,
                time_unit,
                top_n,
            )
        )

    return "\n".join(report) + "\n"


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
    if not 1 <= len(args.inputs) <= 2:
        raise ValueError("Provide either one input JSON file or two input JSON files.")

    output_path = Path(args.output)

    if len(args.inputs) == 1:
        input_path = Path(args.inputs[0])
        values, unit = load_benchmarks(input_path)
        if args.output == "compared_stats.txt":
            output_path = input_path.with_name(f"{input_path.stem}_stats.txt")
        report = build_single_file_report(input_path, values, unit, args.top)
    else:
        left_path = Path(args.inputs[0])
        right_path = Path(args.inputs[1])
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
