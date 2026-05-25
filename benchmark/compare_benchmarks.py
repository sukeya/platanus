#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

import plotly.graph_objects as go
from plotly.subplots import make_subplots


def load_benchmarks(path: Path) -> tuple[dict[str, float], str]:
    with path.open("r", encoding="utf-8") as file:
        data = json.load(file)

    benchmarks = data.get("benchmarks", [])
    values: dict[str, float] = {}
    units: set[str] = set()

    for benchmark in benchmarks:
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


def build_order(left: dict[str, float], right: dict[str, float]) -> list[str]:
    names = list(left)
    names.extend(name for name in right if name not in left)
    return names


BENCHMARK_PATTERN = re.compile(
    r"^(?P<benchmark>BM_[^<]+)"
    r"<(?P<impl>STL|Absl|BTree)"
    r"(?P<container>MultiMap|MultiSet|Map|Set)"
    r"<(?P<data_type>std::(?:int32_t|int64_t|string))"
    r"(?:, (?P<node_size>\d+|platanus::kAutoSize))?"
    r">>$"
)


def parse_benchmark_name(name: str) -> dict[str, str] | None:
    match = BENCHMARK_PATTERN.match(name)
    if match is None:
        return None

    parsed = match.groupdict()
    impl = parsed["impl"]
    node_size = parsed["node_size"]
    if impl == "STL":
        parsed["label"] = "STL"
        return parsed
    if impl == "Absl":
        parsed["label"] = "absl"
        return parsed
    if impl == "BTree" and node_size is not None:
        if node_size == "platanus::kAutoSize":
            parsed["label"] = "platanus(auto)"
        else:
            parsed["label"] = f"platanus({node_size})"
        return parsed
    return None


def subplot_title(benchmark: str, data_type: str, container: str) -> str:
    benchmark_label = benchmark.removeprefix("BM_")
    return f"{benchmark_label} / {data_type} / {container}"


def implementation_sort_key(label: str) -> tuple[int, int]:
    if label == "STL":
        return (0, 0)
    if label == "absl":
        return (1, 0)
    if label == "platanus(auto)":
        return (2, 0)
    match = re.fullmatch(r"platanus\((\d+)\)", label)
    if match is not None:
        return (3, int(match.group(1)))
    return (4, 0)


IMPLEMENTATION_PALETTE = [
    "#1f77b4",
    "#ff7f0e",
    "#2ca02c",
    "#d62728",
    "#9467bd",
    "#8c564b",
    "#e377c2",
    "#7f7f7f",
    "#bcbd22",
    "#17becf",
]


def implementation_colors(labels: list[str]) -> list[str]:
    color_map: dict[str, str] = {}
    for index, label in enumerate(labels):
        color_map[label] = IMPLEMENTATION_PALETTE[index % len(IMPLEMENTATION_PALETTE)]
    return [color_map[label] for label in labels]


def collect_subplot_groups(
    left: dict[str, float], right: dict[str, float]
) -> tuple[list[tuple[str, str]], list[str], dict[tuple[str, str], dict[str, dict[str, str]]]]:
    row_order: list[tuple[str, str]] = []
    row_seen: set[tuple[str, str]] = set()
    container_order = ["Set", "MultiSet", "Map", "MultiMap"]
    groups: dict[tuple[str, str], dict[str, dict[str, str]]] = {}

    for name in build_order(left, right):
        parsed = parse_benchmark_name(name)
        if parsed is None:
            continue

        row_key = (parsed["benchmark"], parsed["data_type"])
        container = parsed["container"]
        label = parsed["label"]

        if row_key not in row_seen:
            row_seen.add(row_key)
            row_order.append(row_key)

        cell = groups.setdefault(row_key, {}).setdefault(container, {})
        cell[label] = name

    active_containers = [
        container
        for container in container_order
        if any(container in row_groups for row_groups in groups.values())
    ]
    return row_order, active_containers, groups


def make_trace(
    x_values: list[str],
    benchmark_names: list[str | None],
    values: dict[str, float],
    label: str,
    color: str | list[str],
) -> go.Bar:
    y_values = [values.get(name) if name is not None else None for name in benchmark_names]
    hover_text = [
        f"{name}<br>{label}: {value:.3f}" if name is not None and value is not None else f"{x_label}<br>{label}: n/a"
        for x_label, name, value in zip(x_values, benchmark_names, y_values)
    ]
    return go.Bar(
        name=label,
        x=x_values,
        y=y_values,
        marker=dict(color=color),
        hovertext=hover_text,
        hovertemplate="%{hovertext}<extra></extra>",
    )


def create_figure(
    left_name: str | None,
    left_values: dict[str, float],
    right_name: str | None,
    right_values: dict[str, float],
    time_unit: str,
) -> go.Figure:
    row_order, active_containers, groups = collect_subplot_groups(left_values, right_values)
    row_count = len(row_order)
    col_count = len(active_containers)

    if row_count == 0 or col_count == 0:
        raise ValueError("No STL/BTree benchmark pairs found to plot.")

    vertical_spacing = 0.04 if row_count <= 1 else min(0.04, 0.9 / (row_count - 1))

    figure = make_subplots(
        rows=row_count,
        cols=col_count,
        shared_xaxes=False,
        subplot_titles=[
            subplot_title(benchmark, data_type, container) if container in groups[row_key] else ""
            for row_key in row_order
            for benchmark, data_type in [row_key]
            for container in active_containers
        ],
        vertical_spacing=vertical_spacing,
        horizontal_spacing=0.05,
    )

    traces_to_render = []
    if left_name is not None:
        traces_to_render.append((left_name, left_values, "#1f77b4"))
    if right_name is not None:
        traces_to_render.append((right_name, right_values, "#ff7f0e"))
    single_series = len(traces_to_render) == 1

    shown_legends: set[str] = set()
    for row_index, row_key in enumerate(row_order, start=1):
        row_groups = groups[row_key]
        for col_index, container in enumerate(active_containers, start=1):
            cell = row_groups.get(container)
            if not cell:
                continue

            x_values = sorted(cell, key=implementation_sort_key)
            benchmark_names = [cell.get(x_value) for x_value in x_values]
            for trace_name, trace_values, trace_color in traces_to_render:
                marker_color: str | list[str]
                if single_series:
                    marker_color = implementation_colors(x_values)
                else:
                    marker_color = trace_color
                trace = make_trace(x_values, benchmark_names, trace_values, trace_name, marker_color)
                trace.showlegend = trace_name not in shown_legends
                shown_legends.add(trace_name)
                figure.add_trace(trace, row=row_index, col=col_index)
            figure.update_xaxes(title_text="Implementation", tickangle=0, row=row_index, col=col_index)
            figure.update_yaxes(title_text=f"CPU Time ({time_unit})", row=row_index, col=col_index)

    figure.update_layout(
        title="Benchmark CPU Time Comparison",
        barmode="group",
        template="plotly_white",
        legend_title="Source",
        height=max(280 * row_count, 900),
        width=max(360 * col_count, 1200),
        margin=dict(l=80, r=40, t=100, b=160),
    )
    return figure


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare cpu_time values from two Google Benchmark JSON files."
    )
    parser.add_argument(
        "left",
        nargs="?",
        help="First benchmark JSON file.",
    )
    parser.add_argument(
        "right",
        nargs="?",
        help="Second benchmark JSON file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="compared.html",
        help="Output HTML file.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    output_path = Path(args.output)
    if args.left is None and args.right is None:
        raise ValueError("At least one of left or right benchmark JSON files must be provided.")

    left_name: str | None = None
    right_name: str | None = None
    left_values: dict[str, float] = {}
    right_values: dict[str, float] = {}
    time_unit: str | None = None

    if args.left is not None:
        left_path = Path(args.left)
        left_values, left_unit = load_benchmarks(left_path)
        left_name = left_path.stem
        time_unit = left_unit

    if args.right is not None:
        right_path = Path(args.right)
        right_values, right_unit = load_benchmarks(right_path)
        right_name = right_path.stem
        if time_unit is None:
            time_unit = right_unit
        elif time_unit != right_unit:
            raise ValueError(
                f"Time units do not match: left={time_unit}, right={right_unit}"
            )

    figure = create_figure(
        left_name,
        left_values,
        right_name,
        right_values,
        time_unit if time_unit is not None else "unknown",
    )
    figure.write_html(output_path, include_plotlyjs='cdn')
    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()
