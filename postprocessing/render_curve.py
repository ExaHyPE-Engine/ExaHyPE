# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

import matplotlib

matplotlib.use("Agg", force=True)
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.collections import PolyCollection  # noqa: E402

try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource

LABEL_FIELDS = ("Subdomain", "Subtree")
CURVE_POSITION_FIELD = "CurvePosition"


def cell_polygons(snapshot: MeshSnapshot) -> np.ndarray:
    """Returns the corner coordinates of every cell as an (Ncells, 4, 2) array.

    The first four corners of a cell form its quadrilateral; a 3D snapshot is rejected.
    """
    if snapshot.points.shape[1] != 2:
        raise ValueError("The curve renderer draws 2D snapshots only")
    cells = np.asarray(snapshot.cells)
    if cells.ndim != 2 or cells.shape[1] < 4:
        raise ValueError("Every cell needs at least four corners")
    return snapshot.points[cells[:, :4]]


def patch_centres_along_curve(
    snapshot: MeshSnapshot, polygons: np.ndarray
) -> np.ndarray:
    """Returns the centre of every patch in the order the curve visits the patches.

    Every cell of one patch carries the same ``CurvePosition``, so the distinct values enumerate
    the patches and sorting them follows the curve. Without the field every cell forms its own
    patch and the centres keep their file order.
    """
    centres = polygons.mean(axis=1)
    if CURVE_POSITION_FIELD not in snapshot.cell_data:
        return centres
    positions = np.asarray(snapshot.cell_data[CURVE_POSITION_FIELD]).reshape(-1)
    _, membership = np.unique(positions, return_inverse=True)
    cells_per_patch = np.bincount(membership)
    return np.column_stack(
        (
            np.bincount(membership, weights=centres[:, 0]) / cells_per_patch,
            np.bincount(membership, weights=centres[:, 1]) / cells_per_patch,
        )
    )


def label_colours(labels: np.ndarray) -> np.ndarray:
    """Returns one colour per cell from a categorical colour map over the label values."""
    values = labels.astype(int)
    number_of_labels = int(values.max()) + 1 if len(values) else 1
    colormap = matplotlib.colormaps["tab20" if number_of_labels > 10 else "tab10"]
    return colormap((values % colormap.N) / colormap.N)


def render_curve(
    snapshot: MeshSnapshot, title: str, output: Path, dpi: int = 300
) -> dict:
    """Draws the ranks and the tasks side by side into ``output`` and returns a summary."""
    polygons = cell_polygons(snapshot)
    centres = patch_centres_along_curve(snapshot, polygons)
    figure, panels = plt.subplots(1, 2, figsize=(18, 9))
    counts = {}
    for axes, field in zip(panels, LABEL_FIELDS):
        labels = np.asarray(snapshot.cell_data[field]).reshape(len(polygons), -1)[:, 0]
        counts[field] = len(np.unique(labels))
        axes.add_collection(
            PolyCollection(
                polygons,
                facecolors=label_colours(labels),
                edgecolors=(0, 0, 0, 0.25),
                linewidths=0.2,
            )
        )
        axes.plot(centres[:, 0], centres[:, 1], color="black", linewidth=0.8)
        axes.set_xlim(polygons[:, :, 0].min(), polygons[:, :, 0].max())
        axes.set_ylim(polygons[:, :, 1].min(), polygons[:, :, 1].max())
        axes.set_aspect("equal")
        axes.set_title(f"{counts[field]} {field}s")
    time = "" if snapshot.time is None else f", t={snapshot.time:.3g}"
    figure.suptitle(f"{title}\n{len(centres)} Patches, {len(polygons)} Cells{time}")
    figure.savefig(output, dpi=dpi, bbox_inches="tight")
    plt.close(figure)
    return {
        "output": Path(output),
        "subdomains": counts["Subdomain"],
        "subtrees": counts["Subtree"],
        "patches": len(centres),
        "cells": len(polygons),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Render the domain decomposition of a 2D snapshot along the space-filling curve."
    )
    parser.add_argument(
        "input",
        help="A solver name (discovered under ./ and its sub-folders), an index/snapshot file, "
        "or an output directory",
    )
    parser.add_argument(
        "--step", type=int, default=-1, help="Snapshot index (default: last)"
    )
    parser.add_argument("--output-dir", default=".", help="Where to write the image")
    parser.add_argument("--dpi", type=int, default=300, help="DPI for raster outputs")
    return parser


def main(argv: list[str] | None = None) -> int:
    arguments = build_parser().parse_args(argv)
    source = SolutionSource.locate(arguments.input)
    step = arguments.step if arguments.step >= 0 else source.count - 1
    snapshot = source.load(step)
    missing = [field for field in LABEL_FIELDS if field not in snapshot.cell_data]
    if missing:
        raise SystemExit(
            f"{', '.join(missing)} missing from the cell fields of {source.describe()}; "
            f"available: {', '.join(sorted(snapshot.cell_data))}"
        )
    output_dir = Path(arguments.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    output = output_dir / f"{source.name}-{step:04d}-decomposition.png"
    summary = render_curve(snapshot, source.name, output, arguments.dpi)
    print(
        f"{summary['output']}: {summary['subdomains']} subdomains, {summary['subtrees']} subtrees, "
        f"{summary['patches']} patches, {summary['cells']} cells"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
