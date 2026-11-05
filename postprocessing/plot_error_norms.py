# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
from pathlib import Path
import shutil

import numpy as np

import matplotlib

matplotlib.use("Agg")

from matplotlib.backend_bases import register_backend
from matplotlib.backends.backend_pgf import FigureCanvasPgf
from matplotlib.ticker import AutoMinorLocator, LogLocator, NullFormatter
import matplotlib.pyplot as plt

register_backend("pgf", FigureCanvasPgf)

try:
    from postprocessing.check_error_norms import COLUMNS, read_error_norms
except ImportError:
    from check_error_norms import COLUMNS, read_error_norms

DEFAULT_DPI = 300
SERIES_STYLES = (
    ("l1-error", "L1", "#e64b35"),
    ("l2-error", "L2", "#4dbbd5"),
    ("linf-error", "L_inf", "#00a087"),
)


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Parse a CSV file with error norms and create a plot."
    )
    parser.add_argument("csv_path", help="Input CSV file to plot.")
    parser.add_argument(
        "-o",
        "--output",
        help="Output PNG file path. Defaults to the input path with a .png suffix.",
    )
    parser.add_argument(
        "--pgf",
        action="store_true",
        help="Also export a matching PGF file next to the PNG output.",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=DEFAULT_DPI,
        help=f"PNG output DPI (default: {DEFAULT_DPI}).",
    )
    parser.add_argument(
        "--title",
        help="Override the default plot title. Defaults to the input file stem.",
    )
    parser.add_argument(
        "--no-title",
        action="store_true",
        help="Suppress the plot title.",
    )
    parser.add_argument(
        "--xlim",
        nargs=2,
        type=float,
        metavar=("XMIN", "XMAX"),
        help="Optional x-axis limits.",
    )
    parser.add_argument(
        "--ylim",
        nargs=2,
        type=float,
        metavar=("YMIN", "YMAX"),
        help="Optional y-axis limits.",
    )
    return parser


def _load_csv(path: Path) -> dict[str, np.ndarray]:
    try:
        header, rows = read_error_norms(str(path))
    except OSError as exc:
        raise SystemExit(f"ERROR: Failed to read CSV file {path}: {exc}") from exc
    if tuple(header) != COLUMNS:
        raise SystemExit(
            "ERROR: Unexpected CSV header in "
            f"{path}. Expected {COLUMNS}, got {tuple(header)}."
        )

    measurements = [row for row in rows if any(field.strip() for field in row)]
    if not measurements:
        raise SystemExit(f"ERROR: The CSV file {path} needs at least one data row.")
    if any(len(row) != len(COLUMNS) for row in measurements):
        raise SystemExit(f"ERROR: CSV file contains malformed rows: {path}")

    try:
        values = np.asarray(measurements, dtype=float)
    except ValueError as exc:
        raise SystemExit(
            f"ERROR: CSV file contains non-numeric values: {path}"
        ) from exc

    order = np.argsort(values[:, 0], kind="stable")
    return {column: values[order, index] for index, column in enumerate(COLUMNS)}


def _resolve_output_path(csv_path: Path, output: str | None) -> Path:
    if output is None:
        return csv_path.with_suffix(".png")

    output_path = Path(output)
    if output_path.suffix.lower() != ".png":
        output_path = output_path.with_suffix(".png")
    return output_path


def _resolve_title(csv_path: Path, args: argparse.Namespace) -> str | None:
    if args.no_title:
        return None
    if args.title:
        return args.title
    return csv_path.stem


def _positive_series(values: np.ndarray, label: str) -> np.ndarray:
    positive_values = values[values > 0.0]
    if positive_values.size == 0:
        raise SystemExit(
            f"ERROR: Cannot plot {label} on a logarithmic axis because all values are "
            "non-positive."
        )

    floor = max(np.finfo(float).tiny, positive_values.min() * 0.5)
    return np.where(values > 0.0, values, floor)


def _create_plot(
    data: dict[str, np.ndarray],
    title: str | None,
    xlim: list[float] | None,
    ylim: list[float] | None,
):
    plt.rcParams.update(
        {
            "font.size": 13,
            "axes.labelsize": 15,
            "axes.titlesize": 16,
            "xtick.labelsize": 12,
            "ytick.labelsize": 12,
            "legend.fontsize": 12,
        }
    )
    figure, axis = plt.subplots(figsize=(10.5, 6.3), constrained_layout=True)
    figure.patch.set_facecolor("white")
    axis.set_facecolor("#fbfbfd")

    for column, label, color in SERIES_STYLES:
        axis.plot(
            data["t"],
            _positive_series(data[column], column),
            label=label,
            color=color,
            linewidth=2.6,
            alpha=0.98,
            solid_capstyle="round",
        )

    axis.set_xlabel("Time t")
    axis.set_ylabel("Error norm")
    axis.set_yscale("log")
    axis.margins(x=0.01)
    axis.grid(True, which="major", color="#c9d2e3", linewidth=0.9, alpha=0.8)
    axis.grid(True, which="minor", color="#e5ebf5", linewidth=0.6, alpha=0.8)
    axis.xaxis.set_minor_locator(AutoMinorLocator(2))
    axis.yaxis.set_minor_locator(LogLocator(base=10.0, subs=np.arange(2, 10) * 0.1))
    axis.yaxis.set_minor_formatter(NullFormatter())
    axis.tick_params(axis="both", which="major", direction="out", length=6, width=1.1)
    axis.tick_params(axis="both", which="minor", direction="out", length=3, width=0.8)
    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)
    axis.spines["left"].set_linewidth(1.1)
    axis.spines["bottom"].set_linewidth(1.1)
    axis.legend(loc="best", frameon=False, handlelength=2.8)

    if title is not None:
        axis.set_title(title, pad=12)
    if xlim is not None:
        axis.set_xlim(xlim)
    if ylim is not None:
        axis.set_ylim(ylim)

    return figure


def _configure_pgf():
    for tex_system in ("xelatex", "lualatex", "pdflatex"):
        if shutil.which(tex_system):
            matplotlib.rcParams["pgf.texsystem"] = tex_system
            return

    raise SystemExit(
        "ERROR: The PGF export needs a supported TeX engine. "
        "Install one of xelatex, lualatex, or pdflatex."
    )


def _write_outputs(
    figure,
    png_path: Path,
    dpi: int,
    write_pgf: bool,
):
    png_path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(png_path, dpi=dpi, bbox_inches="tight")
    print(f"Wrote PNG: {png_path.resolve()}")

    if not write_pgf:
        return

    _configure_pgf()
    pgf_path = png_path.with_suffix(".pgf")
    try:
        figure.savefig(pgf_path, bbox_inches="tight")
    except Exception as exc:
        raise SystemExit(
            "ERROR: Failed to write PGF output. A TeX-compatible Matplotlib PGF "
            f"setup has to be available. Original error: {exc}"
        ) from exc

    print(f"Wrote PGF: {pgf_path.resolve()}")


def main():
    args = _build_argument_parser().parse_args()

    csv_path = Path(args.csv_path)
    if not csv_path.is_file():
        raise SystemExit(f"ERROR: Input CSV file not found: {csv_path}")

    if args.dpi <= 0:
        raise SystemExit("ERROR: --dpi must be a positive integer.")

    data = _load_csv(csv_path)
    png_path = _resolve_output_path(csv_path, args.output)
    title = _resolve_title(csv_path, args)

    figure = _create_plot(data, title, args.xlim, args.ylim)
    try:
        _write_outputs(figure, png_path, args.dpi, args.pgf)
    finally:
        plt.close(figure)


if __name__ == "__main__":
    main()
