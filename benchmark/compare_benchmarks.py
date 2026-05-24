#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

import plotly.graph_objects as go


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


def make_trace(names: list[str], values: dict[str, float], label: str, color: str) -> go.Bar:
    y_values = [values.get(name) for name in names]
    hover_text = [
        f"{name}<br>{label}: {value:.3f}" if value is not None else f"{name}<br>{label}: n/a"
        for name, value in zip(names, y_values)
    ]
    return go.Bar(
        name=label,
        x=names,
        y=y_values,
        marker_color=color,
        hovertext=hover_text,
        hovertemplate="%{hovertext}<extra></extra>",
    )


def create_figure(
    left_name: str,
    left_values: dict[str, float],
    right_name: str,
    right_values: dict[str, float],
    time_unit: str,
) -> go.Figure:
    names = build_order(left_values, right_values)

    figure = go.Figure(
        data=[
            make_trace(names, left_values, left_name, "#1f77b4"),
            make_trace(names, right_values, right_name, "#ff7f0e"),
        ]
    )

    figure.update_layout(
        title="Benchmark CPU Time Comparison",
        barmode="group",
        template="plotly_white",
        xaxis_title="Benchmark",
        yaxis_title=f"CPU Time ({time_unit})",
        legend_title="Source",
        height=800,
        margin=dict(l=80, r=40, t=80, b=220),
    )
    figure.update_xaxes(tickangle=-35)
    return figure


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare cpu_time values from two Google Benchmark JSON files."
    )
    parser.add_argument(
        "left",
        nargs="?",
        required=True,
        help="First benchmark JSON file.",
    )
    parser.add_argument(
        "right",
        nargs="?",
        required=True,
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
    left_path = Path(args.left)
    right_path = Path(args.right)
    output_path = Path(args.output)

    left_values, left_unit = load_benchmarks(left_path)
    right_values, right_unit = load_benchmarks(right_path)

    if left_unit != right_unit:
        raise ValueError(
            f"Time units do not match: {left_path}={left_unit}, {right_path}={right_unit}"
        )

    figure = create_figure(
        left_path.stem,
        left_values,
        right_path.stem,
        right_values,
        left_unit,
    )
    figure.write_html(output_path, include_plotlyjs=True)
    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()
