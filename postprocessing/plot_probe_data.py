# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import csv
from pathlib import Path
import shutil
from typing import Iterable

import pandas as pd

import matplotlib

matplotlib.use("Agg")

from matplotlib.backend_bases import register_backend
from matplotlib.backends.backend_pgf import FigureCanvasPgf
from matplotlib.ticker import AutoMinorLocator
import matplotlib.pyplot as plt

register_backend("pgf", FigureCanvasPgf)

DEFAULT_DPI = 300
BASE_COLUMNS_2D = ("number(0)", "number(1)", "t", "x(0)", "x(1)")
BASE_COLUMNS_3D = ("number(0)", "number(1)", "t", "x(0)", "x(1)", "x(2)")
COLOR_CYCLE = (
    "#0f4c81",
    "#e76f51",
    "#2a9d8f",
    "#7a5195",
    "#ef476f",
    "#118ab2",
    "#bc6c25",
    "#3a86ff",
    "#6a994e",
    "#ff006e",
)
PROBE_STYLES = (
    {"linestyle": "-", "marker": None},
    {"linestyle": "--", "marker": None},
    {"linestyle": "-.", "marker": None},
    {"linestyle": ":", "marker": None},
    {"linestyle": "-", "marker": "o"},
    {"linestyle": "--", "marker": "s"},
    {"linestyle": "-.", "marker": "^"},
    {"linestyle": ":", "marker": "D"},
)


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Parse a probe CSV file and create a plot over time."
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
    parser.add_argument(
        "--probe",
        nargs=2,
        type=int,
        metavar=("NUMBER0", "NUMBER1"),
        help="Optional probe identifier pair to plot.",
    )
    parser.add_argument(
        "--data-columns",
        nargs="+",
        metavar="COLUMN",
        help="Optional list of data(*) columns to plot.",
    )
    return parser


def _normalise_header(fieldnames: Iterable[str | None]) -> list[str]:
    return [field.strip() for field in fieldnames if field is not None]


def _read_header(path: Path) -> list[str]:
    with path.open(newline="") as handle:
        reader = csv.reader(handle)
        try:
            return _normalise_header(next(reader))
        except StopIteration as exc:
            raise SystemExit(f"ERROR: CSV file is empty: {path}") from exc


def _validate_base_columns(
    header: list[str], path: Path
) -> tuple[list[str], list[str]]:
    data_columns = [column for column in header if column.startswith("data(")]
    if not data_columns:
        raise SystemExit(f"ERROR: CSV file contains no data(*) columns: {path}")

    if header[: len(BASE_COLUMNS_3D)] == list(BASE_COLUMNS_3D):
        return list(BASE_COLUMNS_3D), data_columns

    if header[: len(BASE_COLUMNS_2D)] == list(BASE_COLUMNS_2D):
        return list(BASE_COLUMNS_2D), data_columns

    raise SystemExit(
        "ERROR: Unexpected CSV header in "
        f"{path}. Expected a probe header beginning with {BASE_COLUMNS_2D} "
        f"or {BASE_COLUMNS_3D}, got {tuple(header)}."
    )


def _load_csv(path: Path) -> tuple[pd.DataFrame, list[str], list[str]]:
    header = _read_header(path)
    base_columns, data_columns = _validate_base_columns(header, path)
    expected_columns = base_columns + data_columns

    try:
        frame = pd.read_csv(path, skipinitialspace=True)
    except Exception as exc:
        raise SystemExit(f"ERROR: Failed to read CSV file {path}: {exc}") from exc

    frame.columns = [column.strip() for column in frame.columns]
    if list(frame.columns) != expected_columns:
        raise SystemExit(
            "ERROR: Unexpected CSV columns in "
            f"{path}. Expected {tuple(expected_columns)}, got {tuple(frame.columns)}."
        )

    frame = frame.dropna(how="all")
    if frame.empty:
        raise SystemExit(f"ERROR: CSV file contains no data rows: {path}")

    numeric_columns = expected_columns
    try:
        frame[numeric_columns] = frame[numeric_columns].astype(float)
    except ValueError as exc:
        raise SystemExit(
            f"ERROR: CSV file contains non-numeric values: {path}"
        ) from exc

    frame["number(0)"] = frame["number(0)"].astype(int)
    frame["number(1)"] = frame["number(1)"].astype(int)
    frame = frame.sort_values(["number(0)", "number(1)", "t"], kind="mergesort")

    return frame, base_columns, data_columns


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


def _select_probe(frame: pd.DataFrame, probe: tuple[int, int] | None) -> pd.DataFrame:
    if probe is None:
        return frame

    selected = frame[
        (frame["number(0)"] == probe[0]) & (frame["number(1)"] == probe[1])
    ]
    if selected.empty:
        raise SystemExit(
            f"ERROR: Probe ({probe[0]}, {probe[1]}) does not exist in the CSV file."
        )
    return selected


def _select_data_columns(
    available_columns: list[str], requested_columns: list[str] | None
) -> list[str]:
    if requested_columns is None:
        return available_columns

    invalid_columns = [
        column for column in requested_columns if column not in available_columns
    ]
    if invalid_columns:
        raise SystemExit(
            "ERROR: Unknown data column(s): "
            + ", ".join(invalid_columns)
            + ". Available columns: "
            + ", ".join(available_columns)
        )

    return requested_columns


def _format_probe_label(group: pd.DataFrame) -> str:
    probe = group.iloc[0]
    return f"Probe ({int(probe['number(0)'])}, {int(probe['number(1)'])})"


def _create_plot(
    frame: pd.DataFrame,
    data_columns: list[str],
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

    figure, axis = plt.subplots(figsize=(13.5, 6.6), constrained_layout=True)
    figure.patch.set_facecolor("white")
    axis.set_facecolor("#fbfbfd")

    grouped = frame.groupby(["number(0)", "number(1)"], sort=True)

    for probe_index, (_, group) in enumerate(grouped):
        probe_label = _format_probe_label(group)
        style = PROBE_STYLES[probe_index % len(PROBE_STYLES)]
        for data_index, data_column in enumerate(data_columns):
            color = COLOR_CYCLE[data_index % len(COLOR_CYCLE)]
            axis.plot(
                group["t"].to_numpy(),
                group[data_column].to_numpy(),
                label=f"{probe_label} {data_column}",
                color=color,
                linewidth=2.1,
                alpha=0.95,
                solid_capstyle="round",
                linestyle=style["linestyle"],
                marker=style["marker"],
                markersize=4.2 if style["marker"] is not None else None,
                markevery=(
                    max(len(group) // 14, 1) if style["marker"] is not None else None
                ),
            )

    axis.set_xlabel("Time t")
    axis.set_ylabel("Probe value")
    axis.margins(x=0.01)
    axis.grid(True, which="major", color="#c9d2e3", linewidth=0.9, alpha=0.85)
    axis.grid(True, which="minor", color="#e5ebf5", linewidth=0.6, alpha=0.8)
    axis.xaxis.set_minor_locator(AutoMinorLocator(2))
    axis.yaxis.set_minor_locator(AutoMinorLocator(2))
    axis.tick_params(axis="both", which="major", direction="out", length=6, width=1.1)
    axis.tick_params(axis="both", which="minor", direction="out", length=3, width=0.8)
    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)
    axis.spines["left"].set_linewidth(1.1)
    axis.spines["bottom"].set_linewidth(1.1)
    axis.legend(
        loc="center left",
        bbox_to_anchor=(1.01, 0.5),
        frameon=False,
        borderaxespad=0.0,
        ncol=1,
    )

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
        "ERROR: PGF export requested, but no supported TeX engine was found. "
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
            "ERROR: Failed to write PGF output. The Matplotlib PGF backend needs a TeX "
            f"installation. Original error: {exc}"
        ) from exc

    print(f"Wrote PGF: {pgf_path.resolve()}")


def main():
    args = _build_argument_parser().parse_args()

    csv_path = Path(args.csv_path)
    if not csv_path.is_file():
        raise SystemExit(f"ERROR: Input CSV file not found: {csv_path}")

    if args.dpi <= 0:
        raise SystemExit("ERROR: --dpi must be a positive integer.")

    frame, _base_columns, available_data_columns = _load_csv(csv_path)
    probe = tuple(args.probe) if args.probe is not None else None
    frame = _select_probe(frame, probe)
    data_columns = _select_data_columns(available_data_columns, args.data_columns)

    png_path = _resolve_output_path(csv_path, args.output)
    title = _resolve_title(csv_path, args)

    figure = _create_plot(frame, data_columns, title, args.xlim, args.ylim)
    try:
        _write_outputs(figure, png_path, args.dpi, args.pgf)
    finally:
        plt.close(figure)


if __name__ == "__main__":
    main()
