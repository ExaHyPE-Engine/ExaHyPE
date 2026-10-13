"""Build and run the oscillating lake with ADER-DG and every subcell limiter, and with
ADER-WENO as a finite-volume solver, on comparable meshes, and plot the error of every run.

Every ADER-WENO run uses a patch of as many volumes per axis as the DG solver has nodes, on the
same cell grid, so all runs carry the same degrees of freedom.
"""

import argparse
import csv
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
APPLICATION = HERE / "oscillating-lake.py"
OUTPUT = HERE / "limiter-comparison"

CASES = {
    "dg-godunov": dict(solver="aderdg", limiter="godunov", label="ADER-DG + Godunov"),
    "dg-muscl": dict(solver="aderdg", limiter="muscl-hancock", label="ADER-DG + MUSCL-Hancock"),
    "dg-weno": dict(solver="aderdg", limiter="weno", scaling=False, fallback=False,
                    label="ADER-DG + ADER-WENO"),
    "dg-weno-scaling": dict(solver="aderdg", limiter="weno", scaling=True, fallback=False,
                            label="ADER-DG + ADER-WENO, scaling"),
    "dg-weno-fallback": dict(solver="aderdg", limiter="weno", scaling=False, fallback=True,
                             label="ADER-DG + ADER-WENO, fallback"),
    "dg-weno-scaling-fallback": dict(solver="aderdg", limiter="weno", scaling=True, fallback=True,
                                     label="ADER-DG + ADER-WENO, scaling + fallback"),
    "weno": dict(solver="ader-weno", scaling=False, fallback=False, label="ADER-WENO"),
    "weno-scaling": dict(solver="ader-weno", scaling=True, fallback=False,
                         label="ADER-WENO, scaling"),
    "weno-fallback": dict(solver="ader-weno", scaling=False, fallback=True,
                          label="ADER-WENO, fallback"),
    "weno-scaling-fallback": dict(solver="ader-weno", scaling=True, fallback=True,
                                  label="ADER-WENO, scaling + fallback"),
    "weno-fallback-volumes": dict(solver="ader-weno", scaling=False, fallback=True,
                                  granularity="volumes", label="ADER-WENO, volume fallback"),
    "weno-fallback-neighbours": dict(solver="ader-weno", scaling=False, fallback=True,
                                     granularity="volumes_and_neighbours",
                                     label="ADER-WENO, volume + neighbour fallback"),
    "weno-scaling-fallback-volumes": dict(solver="ader-weno", scaling=True, fallback=True,
                                          granularity="volumes",
                                          label="ADER-WENO, scaling + volume fallback"),
    "weno-scaling-fallback-neighbours": dict(solver="ader-weno", scaling=True, fallback=True,
                                             granularity="volumes_and_neighbours",
                                             label="ADER-WENO, scaling + volume + neighbour fallback"),
    "dg-weno-scaling-fallback-volumes": dict(solver="aderdg", limiter="weno", scaling=True,
                                             fallback=True, granularity="volumes",
                                             label="ADER-DG + ADER-WENO, scaling + volume fallback"),
}


def environment(args, directory):
    env = os.environ.copy()
    env.update(
        PATH="/usr/lib64/openmpi/bin" + os.pathsep + env.get("PATH", ""),
        PEANO_BUILD_DIR=str(args.peano_build.resolve()),
        PYTHONPATH=str(ROOT / "api") + os.pathsep + env.get("PYTHONPATH", ""),
        OMP_NUM_THREADS=str(args.threads),
        MPLCONFIGDIR=str(directory / "matplotlib-cache"),
    )
    if args.cuda_home is not None:
        cuda = args.cuda_home.resolve()
        env.update(
            CUDA_HOME=str(cuda), CUDA_PATH=str(cuda), CUDAToolkit_ROOT=str(cuda),
            CUDACXX=str(cuda / "bin" / "nvcc"),
            PATH=str(cuda / "bin") + os.pathsep + env["PATH"],
            LD_LIBRARY_PATH=str(cuda / "lib64") + os.pathsep + env.get("LD_LIBRARY_PATH", ""),
        )
    return env


def build_command(case, args):
    command = [
        sys.executable, str(APPLICATION),
        "--solver", case["solver"],
        "--degrees-of-freedom", str(2 * args.dg_order + 1),
        "--patch-size", str(args.dg_order + 1),
        "--weno-order", str(args.weno_order),
        "--end-time", str(args.end_time),
        "--cfl", str(args.cfl),
        "--dry-threshold", str(args.dry_threshold),
        "--grid-levels", str(args.grid_levels),
        "--build-mode", "Release", "--jobs", str(args.build_jobs),
    ]
    if case["solver"] == "aderdg":
        command += ["--limiter", case["limiter"]]
    if case.get("scaling"):
        command.append("--polynomial-scaling")
    if case.get("fallback") is False:
        command.append("--no-fallback")
    if case.get("granularity"):
        command += ["--fallback-granularity", case["granularity"]]
    if case["solver"] == "aderdg" and args.galerkin_flux_coupling == "off":
        command.append("--no-galerkin-flux-coupling")
    command += ["--error-samples", str(args.error_samples)]
    return command


def experiment(name, args):
    case = CASES[name]
    directory = args.output / name
    metadata_path = directory / "run.json"
    if not args.force and metadata_path.exists():
        print(f"{name}: reuse finished run", flush=True)
        return json.loads(metadata_path.read_text())
    if directory.exists():
        shutil.rmtree(directory)
    directory.mkdir(parents=True)
    for implementation in ("ADERDGSolver.cpph", "FVSolver.cpph"):
        shutil.copy(HERE / implementation, directory / implementation)
    env = environment(args, directory)
    build = build_command(case, args)
    print(f"{name}: build", flush=True)
    with (directory / "build.log").open("w") as log:
        subprocess.run(build, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    run = [
        "./ExaSkred-Tsunami", "--endTime", str(args.end_time),
        "--gridLevels", str(args.grid_levels),
        "--numberOfSnapshots", str(args.snapshots), "--outputPath", "solutions",
        "--timeStepLogInterval", "10",
    ]
    print(f"{name}: run to t = {args.end_time}", flush=True)
    start = time.monotonic()
    with (directory / "console.log").open("w") as log:
        process = subprocess.run(run, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT)
    console = (directory / "console.log").read_text(errors="replace")
    metadata = {
        "case": name, **case, "dg_order": args.dg_order, "weno_order": args.weno_order,
        "patch_size": args.dg_order + 1, "grid_levels": args.grid_levels, "cfl": args.cfl,
        "dry_threshold": args.dry_threshold,
        "galerkin_flux_coupling": args.galerkin_flux_coupling, "error_samples": args.error_samples,
        "end_time": args.end_time, "build_command": build, "run_command": run,
        "peano_build": str(args.peano_build.resolve()),
        "git_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "source_tree_dirty": bool(subprocess.check_output(
            ["git", "status", "--porcelain", "--", "src", "api"], cwd=ROOT, text=True).strip()),
        "seconds": time.monotonic() - start, "returncode": process.returncode,
        "completed": "Run terminated gracefully after" not in console and process.returncode == 0,
    }
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")
    status = "complete" if metadata["completed"] else f"stopped early (exit {process.returncode})"
    print(f"{name}: {status} in {metadata['seconds']:.1f} s", flush=True)
    return metadata


# Categorical slots of the validated default palette, assigned by variant so a variant keeps its
# hue in every figure. The line styles repeat the identity for print and colour-vision deficiency.
VARIANT_STYLE = {
    "dg-godunov": ("#2a78d6", "-"),
    "dg-muscl": ("#eb6834", "--"),
    "dg-weno": ("#1baf7a", "-."),
    "weno": ("#1baf7a", "-."),
    "dg-weno-scaling": ("#eda100", ":"),
    "weno-scaling": ("#eda100", ":"),
    "dg-weno-fallback": ("#e87ba4", (0, (5, 1, 1, 1))),
    "weno-fallback": ("#e87ba4", (0, (5, 1, 1, 1))),
    "dg-weno-scaling-fallback": ("#008300", (0, (1, 1))),
    "weno-scaling-fallback": ("#008300", (0, (1, 1))),
    "weno-fallback-volumes": ("#4a3aa6", "-"),
    "weno-fallback-neighbours": ("#8a8a00", "--"),
    "weno-scaling-fallback-volumes": ("#4a3aa6", (0, (1, 1.5))),
    "weno-scaling-fallback-neighbours": ("#8a8a00", (0, (3, 1, 1, 1, 1, 1))),
    "dg-weno-scaling-fallback-volumes": ("#4a3aa6", "-"),
}
NORM_STYLE = {"L1": ("#2a78d6", "-"), "L2": ("#eb6834", "--"), "L\u221e": ("#1baf7a", "-.")}
# The VolumeStatus values of exahype2::fv::ader::Fallback after Accepted, in their order.
FALLBACK_REASONS = [
    ("not finite", "#2a78d6"),
    ("inadmissible", "#eb6834"),
    ("maximum principle", "#1baf7a"),
    ("neighbour", "#eda100"),
    ("rest of patch", "#e87ba4"),
]
ACCEPTED_FILL = "#f1f0ec"
TEXT, MUTED, GRID = "#1f1f1e", "#6b6a65", "#e4e3df"


def read_errors(directory):
    path = directory / "solutions" / "Error.csv"
    if not path.exists():
        return None
    rows = [[float(value) for value in row] for row in csv.reader(path.open()) if row and row[0][0].isdigit()]
    times, l1, l2, linf = (list(column) for column in zip(*rows)) if rows else ([], [], [], [])
    return {"t": times, "L1": l1, "L2": l2, "L\u221e": linf}


def style_axes(axes, end_time):
    axes.set_yscale("log")
    axes.set_xlim(0.0, end_time)
    axes.set_xlabel("t", color=MUTED)
    axes.grid(True, which="major", color=GRID, linewidth=0.8)
    axes.tick_params(colors=MUTED)
    for spine in ("top", "right"):
        axes.spines[spine].set_visible(False)
    for spine in ("left", "bottom"):
        axes.spines[spine].set_color(GRID)


def label_ends(axes, ends):
    """Labels the end of every line, with the labels pushed at least 11 points apart."""
    axes.figure.canvas.draw()
    to_display = axes.transData.transform
    placed = sorted(((to_display((x, y))[1], x, y, text, color) for x, y, text, color in ends), key=lambda e: e[0])
    offsets, previous = [], None
    for display_y, *_ in placed:
        target = display_y if previous is None else max(display_y, previous + 11 * axes.figure.dpi / 72)
        offsets.append(target - display_y)
        previous = target
    for (display_y, x, y, text, color), offset in zip(placed, offsets):
        axes.plot([x], [y], marker="o", markersize=4, color=color, markeredgecolor="white",
                  markeredgewidth=1.5, zorder=4)
        axes.annotate(text, (x, y), xytext=(5, offset * 72 / axes.figure.dpi), textcoords="offset points",
                      va="center", fontsize=8, color=TEXT)


def plot(args):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    runs = {}
    for name in CASES:
        directory = args.output / name
        errors = read_errors(directory)
        if errors is None or not errors["t"]:
            continue
        metadata_path = directory / "run.json"
        metadata = json.loads(metadata_path.read_text()) if metadata_path.exists() else {}
        runs[name] = (errors, metadata)
    figures = args.output / "figures"
    figures.mkdir(exist_ok=True)

    for name, (errors, metadata) in runs.items():
        figure, axes = plt.subplots(figsize=(6.4, 3.8), dpi=150)
        for norm, (color, line) in NORM_STYLE.items():
            axes.plot(errors["t"], errors[norm], color=color, linestyle=line, linewidth=2, label=norm)
        style_axes(axes, args.end_time)
        label_ends(axes, [(errors["t"][-1], errors[norm][-1], norm, color) for norm, (color, _) in NORM_STYLE.items()])
        axes.set_ylabel("error", color=MUTED)
        stopped = not metadata.get("completed", False)
        if stopped:
            axes.axvline(errors["t"][-1], color=MUTED, linewidth=1, linestyle=":")
            axes.text(errors["t"][-1], axes.get_ylim()[1], f" stopped at t = {errors['t'][-1]:.3g}",
                      fontsize=8, color=MUTED, va="top")
        axes.set_title(CASES[name]["label"], loc="left", fontsize=11, color=TEXT)
        axes.legend(frameon=False, fontsize=8, loc="lower right", labelcolor=TEXT)
        figure.tight_layout()
        figure.savefig(figures / f"error-{name}.png")
        plt.close(figure)

    panels = [
        ("ADER-DG, subcell limiters", [n for n in CASES if n.startswith("dg-")], None),
        ("ADER-WENO solver (dashed: ADER-DG + Godunov)", [n for n in CASES if not n.startswith("dg-")], "dg-godunov"),
    ]
    figure, axes_pair = plt.subplots(1, 2, figsize=(12.8, 4.4), dpi=150, sharey=True)
    for axes, (title, names, reference) in zip(axes_pair, panels):
        if reference in runs:
            errors = runs[reference][0]
            axes.plot(errors["t"], errors["L2"], color=VARIANT_STYLE[reference][0], linewidth=1.2,
                      linestyle=(0, (4, 3)), alpha=0.7)
        for name in names:
            if name not in runs:
                continue
            errors, metadata = runs[name]
            color, line = VARIANT_STYLE[name]
            label = CASES[name]["label"].split(" + ", 1)[-1] if name.startswith("dg-") else CASES[name]["label"]
            if not metadata.get("completed", False):
                label += f" (stopped t = {errors['t'][-1]:.3g})"
            axes.plot(errors["t"], errors["L2"], color=color, linestyle=line, linewidth=2, label=label)
            axes.plot([errors["t"][-1]], [errors["L2"][-1]], marker="o", markersize=4, color=color,
                      markeredgecolor="white", markeredgewidth=1.5)
        style_axes(axes, args.end_time)
        axes.set_title(title, loc="left", fontsize=11, color=TEXT)
        axes.legend(frameon=False, fontsize=8, loc="lower right", labelcolor=TEXT)
    axes_pair[0].set_ylabel("L2 error", color=MUTED)
    figure.tight_layout()
    figure.savefig(figures / "error-overlay-l2.png")
    plt.close(figure)

    for name in runs:
        plot_fallback(name, args, figures, plt)

    with (figures / "summary.md").open("w") as table:
        table.write("| Case | Reached t | Completed | Wall s | L1 at end | L2 at end | Linf at end | max L2 |\n")
        table.write("|---|---|---|---|---|---|---|---|\n")
        for name, (errors, metadata) in runs.items():
            table.write(
                f"| {CASES[name]['label']} | {errors['t'][-1]:.3g} | {'yes' if metadata.get('completed') else 'no'} "
                f"| {metadata.get('seconds', float('nan')):.0f} | {errors['L1'][-1]:.3e} | {errors['L2'][-1]:.3e} "
                f"| {errors['L\u221e'][-1]:.3e} | {max(errors['L2']):.3e} |\n"
            )
    print(f"figures in {figures}")


def plot_fallback(name, args, figures, plt):
    """Plots where and why the volumes of an ADER-WENO run repeated their step with the fallback
    scheme: the status of every volume at three snapshots over the exact shoreline, and the number
    of volumes per reason at every snapshot."""
    import numpy as np
    from matplotlib.colors import ListedColormap

    path = args.output / name / "solutions" / "patch" / "FVSolver.vtkhdf"
    if not path.exists():
        return
    sys.path.insert(0, str(ROOT / "postprocessing"))
    from solution_io import SolutionSource

    source = SolutionSource.open(path)
    if "LimiterStatus" not in source.load(0).cell_data:
        return
    times, counts, maps = [], [], {}
    shown = sorted({source.count // 4, source.count // 2, source.count - 1})
    for index in range(source.count):
        snapshot = source.load(index)
        status = snapshot.cell_data["LimiterStatus"].astype(int)
        times.append(source.times[index])
        counts.append(np.bincount(status, minlength=len(FALLBACK_REASONS) + 1)[1:len(FALLBACK_REASONS) + 1])
        if index in shown:
            corners = snapshot.points[:, :2][snapshot.cells]
            maps[index] = (corners.mean(axis=1), corners.max(axis=1) - corners.min(axis=1), status,
                           snapshot.cell_data["b"])
    counts = np.array(counts)

    figure = plt.figure(figsize=(12.8, 7.6), dpi=150)
    grid = figure.add_gridspec(2, len(shown), height_ratios=[1.35, 1])
    colours = ListedColormap([ACCEPTED_FILL] + [colour for _, colour in FALLBACK_REASONS])
    g, depth, radius, displacement = 9.81, 0.1, 1.0, 0.5
    frequency = (2.0 * g * depth) ** 0.5 / radius
    for column, index in enumerate(shown):
        axes = figure.add_subplot(grid[0, column])
        centres, widths, status, bathymetry = maps[index]
        width = widths[:, 0].min()
        lower = centres.min(axis=0)
        cells = np.rint((centres - lower) / width).astype(int)
        image = np.zeros(cells.max(axis=0) + 1, dtype=int)
        image[cells[:, 0], cells[:, 1]] = status
        extent = [lower[0] - width / 2, lower[0] + width * (image.shape[0] - 0.5),
                  lower[1] - width / 2, lower[1] + width * (image.shape[1] - 0.5)]
        axes.imshow(image.T, origin="lower", extent=extent, cmap=colours, vmin=-0.5,
                    vmax=len(FALLBACK_REASONS) + 0.5, interpolation="nearest")
        t = times[index]
        x, y = np.meshgrid(np.linspace(-1.6, 1.6, 500), np.linspace(-1.6, 1.6, 500))
        surface = displacement * depth / radius**2 * (
            2 * x * np.cos(frequency * t) + 2 * y * np.sin(frequency * t) - displacement)
        axes.contour(x, y, surface + depth * (1 - (x**2 + y**2) / radius**2), levels=[0.0],
                     colors=TEXT, linewidths=0.8)
        axes.set_xlim(-1.6, 1.6)
        axes.set_ylim(-1.6, 1.6)
        axes.set_aspect("equal")
        axes.set_title(f"t = {t:.2f}", loc="left", fontsize=10, color=TEXT)
        axes.tick_params(colors=MUTED, labelsize=7)
        for spine in axes.spines.values():
            spine.set_color(GRID)
    axes = figure.add_subplot(grid[1, :])
    for reason, (label, colour) in enumerate(FALLBACK_REASONS):
        if counts[:, reason].any():
            axes.plot(times, counts[:, reason], color=colour, linewidth=2, marker="o", markersize=3,
                      label=label)
    style_axes(axes, args.end_time)
    axes.set_yscale("linear")
    axes.set_ylabel("volumes falling back", color=MUTED)
    axes.legend(frameon=False, fontsize=8, loc="upper right", labelcolor=TEXT, ncols=len(FALLBACK_REASONS))
    figure.suptitle(f"{CASES[name]['label']}: fallback volumes (black: exact shoreline)", x=0.01,
                    ha="left", fontsize=11, color=TEXT)
    figure.tight_layout()
    figure.savefig(figures / f"fallback-{name}.png")
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cases", nargs="+", default=list(CASES), choices=list(CASES))
    parser.add_argument("--dg-order", type=int, default=5)
    parser.add_argument("--weno-order", type=int, default=3)
    parser.add_argument("--grid-levels", type=int, default=4)
    parser.add_argument("--end-time", type=float, default=10.0)
    parser.add_argument("--cfl", type=float, default=0.5)
    parser.add_argument("--dry-threshold", type=float, default=1e-5)
    parser.add_argument("--galerkin-flux-coupling", choices=["on", "off"], default="on",
                        help="Coupling of the DG subcell limiter to the Galerkin flux.")
    parser.add_argument("--error-samples", type=int, default=1000)
    parser.add_argument("--snapshots", type=int, default=10)
    parser.add_argument("--threads", type=int, default=16)
    parser.add_argument("--build-jobs", type=int, default=24)
    parser.add_argument("--peano-build", type=Path, default=ROOT / "cmake-build-relwithdebinfo-peanohost")
    parser.add_argument("--cuda-home", type=Path,
                        help="CUDA toolkit for a --peano-build with a CUDA backend, e.g. /usr/local/cuda-13.0.")
    parser.add_argument("--force", action="store_true", help="Rebuild and rerun finished cases.")
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--plot", action="store_true", help="Only plot the errors of the finished cases.")
    args = parser.parse_args()
    if not args.plot:
        for name in args.cases:
            experiment(name, args)
    plot(args)


if __name__ == "__main__":
    main()
