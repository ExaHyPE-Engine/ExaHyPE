# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import glob
from pathlib import Path
import re
from typing import NamedTuple

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import (
    FixedLocator,
    LogFormatterSciNotation,
    LogLocator,
    NullFormatter,
    NullLocator,
)

DEFAULT_DPI = 330
NUMBER = r"([0-9.]+(?:[eE][-+]?[0-9]+)?)"
NODES = re.compile(r"([0-9]+) Ranks? on ([0-9]+) Nodes?")
DIMENSIONS = re.compile(r"Dimensions\s+([0-9]+)")
CELLS = re.compile(r"Cells\s+([0-9][0-9,]*)\s")
TIME_PER_STEP = re.compile(r"Average Time Step\s+" + NUMBER + " s")
PERFORMANCE = re.compile(
    r"Average Performance\s+" + NUMBER + r" Cell/s · " + NUMBER + r" DoF/s"
)
LINE_COLOUR = "#3a76a6"
IDEAL_COLOUR = "#808080"


class Run(NamedTuple):
    nodes: int
    dimensions: int
    cells: int
    time_per_step: float
    degrees_of_freedom_per_second: float


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Plot the strong scaling and the parallel efficiency of one series of "
        "ExaHyPE runs from their logs, one log per node count."
    )
    parser.add_argument("logs", nargs="+", help="Log files or glob patterns.")
    parser.add_argument(
        "--top-axis",
        nargs=2,
        metavar=("LABEL", "PER_NODE"),
        help="Label and count per node of a top axis, for example "
        '"Number of Cores" 48.',
    )
    parser.add_argument(
        "-o",
        "--output",
        default="scaling",
        help="Output path without suffix. The plots go to <output>-strong-scaling and "
        "<output>-efficiency, each as .pdf and .png.",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=DEFAULT_DPI,
        help=f"PNG output DPI (default: {DEFAULT_DPI}).",
    )
    return parser


def _read_run(text: str) -> Run | None:
    """
    Returns the node count, the grid and the averages of a log, or None if the run ended early.
    The last cell count of the log applies to a grid that changes during the run.
    """
    nodes = NODES.search(text)
    dimensions = DIMENSIONS.search(text)
    cells = CELLS.findall(text)
    times = TIME_PER_STEP.findall(text)
    performances = PERFORMANCE.findall(text)
    if None in (nodes, dimensions) or not (cells and times and performances):
        return None
    return Run(
        int(nodes.group(2)),
        int(dimensions.group(1)),
        int(cells[-1].replace(",", "")),
        float(times[-1]),
        float(performances[-1][1]),
    )


def _read_runs(patterns: list[str]) -> list[Run]:
    """
    Returns the runs of the logs sorted by their node count. Every node count occurs once.
    """
    paths = []
    for pattern in patterns:
        paths.extend(
            sorted(glob.glob(pattern)) if glob.has_magic(pattern) else [pattern]
        )
    runs = []
    for path in paths:
        try:
            text = Path(path).read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            raise SystemExit(f"ERROR: Cannot read the log {path}: {exc}") from exc
        run = _read_run(text)
        if run is not None:
            runs.append(run)
    node_counts = [run.nodes for run in runs]
    for count in node_counts:
        if node_counts.count(count) > 1:
            raise SystemExit(
                f"ERROR: The plot takes one log per node count. Several logs run on {count} nodes."
            )
    return sorted(runs, key=lambda run: run.nodes)


def _parallel_efficiency(runs: list[Run]) -> list[float]:
    """
    Returns the parallel efficiency of every run against the run with the fewest nodes. A log
    prints the degree-of-freedom rate with five significant digits and the step time with fewer.
    """
    reference = runs[0]
    return [
        run.degrees_of_freedom_per_second
        * reference.nodes
        / (run.nodes * reference.degrees_of_freedom_per_second)
        for run in runs
    ]


def _grid_label(run: Run) -> str:
    cells_per_axis = round(run.cells ** (1.0 / run.dimensions))
    if cells_per_axis**run.dimensions == run.cells:
        return f"${cells_per_axis}^{run.dimensions}$"
    return f"{run.cells:,} cells"


def _create_axis(node_counts: list[int], top_axis: tuple[str, float] | None):
    plt.rcParams.update(
        {
            "font.size": 12,
            "axes.labelsize": 13,
            "xtick.labelsize": 12,
            "ytick.labelsize": 12,
            "legend.fontsize": 12,
            "mathtext.fontset": "dejavusans",
        }
    )
    figure, axis = plt.subplots(figsize=(8.0, 5.2), layout="constrained")
    axis.set_xscale("log", base=2)
    axis.xaxis.set_major_locator(FixedLocator(node_counts))
    axis.xaxis.set_minor_locator(NullLocator())
    axis.xaxis.set_major_formatter(lambda value, _position: f"{value:g}")
    axis.set_xlabel("Number of Nodes")
    axis.grid(True, which="major", color="#d4d4d4", linewidth=0.8)
    axis.grid(True, which="minor", color="#e2e2e2", linewidth=0.6, linestyle=":")
    axis.set_axisbelow(True)
    if top_axis is None:
        return figure, axis
    label, per_node = top_axis
    secondary_axis = axis.secondary_xaxis(
        "top",
        functions=(lambda nodes: nodes * per_node, lambda count: count / per_node),
    )
    secondary_axis.xaxis.set_major_locator(
        FixedLocator([count * per_node for count in node_counts])
    )
    secondary_axis.xaxis.set_minor_locator(NullLocator())
    secondary_axis.xaxis.set_major_formatter(lambda value, _position: f"{value:g}")
    secondary_axis.set_xlabel(label)
    return figure, axis


def _plot_line(axis, nodes, values, label) -> None:
    axis.plot(
        nodes,
        values,
        color=LINE_COLOUR,
        marker="o",
        linewidth=2.5,
        markersize=8,
        label=label,
    )


def _create_strong_scaling_plot(runs: list[Run], top_axis: tuple[str, float] | None):
    """
    Returns a figure with the degree-of-freedom updates per second of every run and the ideal
    scaling from the run with the fewest nodes.
    """
    nodes = [run.nodes for run in runs]
    throughput = [run.degrees_of_freedom_per_second for run in runs]
    figure, axis = _create_axis(nodes, top_axis)
    _plot_line(axis, nodes, throughput, _grid_label(runs[0]))
    axis.plot(
        nodes,
        [throughput[0] * count / nodes[0] for count in nodes],
        color=IDEAL_COLOUR,
        linestyle="--",
        linewidth=2.0,
        label="Ideal",
    )
    axis.set_yscale("log")
    axis.yaxis.set_major_locator(LogLocator(subs=(1.0, 2.0, 5.0)))
    axis.yaxis.set_major_formatter(
        LogFormatterSciNotation(labelOnlyBase=False, minor_thresholds=(100, 100))
    )
    axis.yaxis.set_minor_locator(LogLocator(subs=(3.0, 4.0, 6.0, 7.0, 8.0, 9.0)))
    axis.yaxis.set_minor_formatter(NullFormatter())
    axis.set_ylabel("Number of DoF Updates per Second")
    axis.legend(loc="upper left")
    return figure


def _create_efficiency_plot(runs: list[Run], top_axis: tuple[str, float] | None):
    """
    Returns a figure with the parallel efficiency of every run against the run with the fewest
    nodes.
    """
    nodes = [run.nodes for run in runs]
    efficiency = _parallel_efficiency(runs)
    figure, axis = _create_axis(nodes, top_axis)
    _plot_line(axis, nodes, efficiency, _grid_label(runs[0]))
    axis.axhline(
        1.0, color=IDEAL_COLOUR, linestyle="--", linewidth=2.0, label="Ideal", zorder=1
    )
    axis.set_ylim(0.0, max(1.1, 1.05 * max(efficiency)))
    axis.set_ylabel("Parallel Efficiency")
    axis.legend(loc="lower left")
    return figure


def _save(figure, output: Path, dpi: int) -> None:
    figure.savefig(output.with_suffix(".pdf"))
    figure.savefig(output.with_suffix(".png"), dpi=dpi)
    print(f"Wrote {output.with_suffix('.pdf')} and {output.with_suffix('.png')}.")


def main() -> None:
    args = _build_argument_parser().parse_args()
    runs = _read_runs(args.logs)
    if not runs:
        raise SystemExit("ERROR: No log reports a time per step and a performance.")
    for run, efficiency in zip(runs, _parallel_efficiency(runs)):
        print(
            f"{run.nodes:>5} nodes {run.time_per_step:10.4f} s per step "
            f"efficiency {efficiency:5.2f}"
        )
    top_axis = None
    if args.top_axis is not None:
        label, per_node = args.top_axis
        try:
            top_axis = (label, float(per_node))
        except ValueError as exc:
            raise SystemExit(
                f"ERROR: The count per node of the top axis is no number: {per_node}"
            ) from exc
    output = Path(args.output)
    _save(
        _create_strong_scaling_plot(runs, top_axis),
        output.with_name(output.name + "-strong-scaling"),
        args.dpi,
    )
    _save(
        _create_efficiency_plot(runs, top_axis),
        output.with_name(output.name + "-efficiency"),
        args.dpi,
    )


if __name__ == "__main__":
    main()
