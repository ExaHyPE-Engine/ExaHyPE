# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter, FuncFormatter
from matplotlib.backends.backend_pgf import FigureCanvasPgf

import exahype2.postprocessing

# Define the LaTeX preamble for the output file
preamble = r"\usepackage{amsmath,amssymb}"

# Set the rcParams dictionary to use the pgf backend and include the LaTeX preamble
plt.rcParams.update(
    {
        "font.family": "sans-serif",
        "pgf.texsystem": "pdflatex",
        "pgf.preamble": preamble,
    }
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 Degrees-of-Freedom-Updates Plotter"
    )
    parser.add_argument(
        "file",
        nargs="+",
        help="Filename of the file to parse. You can specify multiple files.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-log-y",
        "--log-ysaxis",
        help="Use log scale",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-l",
        "--label",
        nargs="*",
        help="Label of dataset (optional; if used, you have to add --label/-l after the actual files and you have to embed the labels into hyphens)",
    )
    parser.add_argument(
        "-dof",
        "--degrees-of-freedom",
        type=int,
        help="Number of degrees-of-freedom one cell holds.",
        required=True,
    )
    parser.add_argument(
        "-nc", "--number-of-cells", type=int, help="Number of cells.", required=True
    )
    parser.add_argument(
        "-o", "--output-file", help="Name of output file", default="output"
    )
    parser.add_argument(
        "-f",
        "--output-format",
        help="Output format",
        choices=["png", "pdf", "pgf"],
        default="png",
    )
    args = parser.parse_args()

    # If no labels are provided, generate default labels
    if not args.label:
        args.label = [f"Dataset {i+1}" for i in range(len(args.file))]

    labels = []
    dof_updates_per_second = []
    for file, label in zip(args.file, args.label):
        labels.append(label)
        performance_data = exahype2.postprocessing.PerformanceData(
            file_name=file, verbose=args.verbose
        )
        time_per_time_step_in_seconds = performance_data.time_per_time_step()
        dof_updates_per_second.append(
            (args.degrees_of_freedom * args.number_of_cells)
            / time_per_time_step_in_seconds
        )

    plt.clf()
    if args.log_ysaxis:
        plt.yscale("log")

    fig, ax = plt.subplots(figsize=(10, 6))

    colors = plt.cm.viridis(np.linspace(0, 1, len(labels)))

    # Creating the bar plot
    bars = plt.bar(
        labels,
        dof_updates_per_second,
        color=colors,
        width=0.35,
        edgecolor="black",
        zorder=2,
    )  # Ensure bars are in front

    # Adding grid lines
    plt.grid(
        axis="y", linestyle="--", alpha=1, linewidth=1, zorder=1
    )  # Increase line width and set behind bars

    plt.xticks(fontsize=15)
    plt.yticks(fontsize=15)

    plt.ylabel("Degrees of Freedom Updates per Second", fontsize=15)

    ax.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
    ax.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
    ax.yaxis.get_offset_text().set_fontsize(15)
    # ax.set_xticks(range(len(labels)))
    # ax.set_xticklabels(labels, fontsize=15, rotation=45, ha="right")
    # ax.set_yticks(ax.get_yticks())
    # ax.set_yticklabels(ax.get_yticks(), fontsize=15)

    plt.title(
        "{} Degrees of Freedom".format(args.number_of_cells * args.degrees_of_freedom),
        fontsize=15,
    )

    if args.output_format == "pdf":
        plt.savefig(args.output_file + ".pdf", bbox_inches="tight", transparent=False)
    elif args.output_format == "png":
        plt.savefig(args.output_file + ".png", bbox_inches="tight", transparent=False)
    elif args.output_format == "pgf":
        fig = plt.gcf()
        fig.patch.set_alpha(0)
        canvas = FigureCanvasPgf(fig)
        canvas.print_figure(
            args.output_file + ".pgf",
            bbox_inches="tight",
        )

    if args.output_file == "output":
        print(
            "default file name output.{} written. Change file name with -o or --output-file".format(
                args.output_format
            )
        )
