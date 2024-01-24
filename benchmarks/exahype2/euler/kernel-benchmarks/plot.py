import os
import sys
import argparse

from enum import Enum

import matplotlib.pyplot as plt
from matplotlib.backends.backend_pgf import FigureCanvasPgf
import numpy as np

# Define the LaTeX preamble for the output file
preamble = r"\usepackage{amsmath,amssymb}"

# Set the rcParams dictionary to use the pgf backend and include the LaTeX preamble
plt.rcParams.update(
    {
        "font.family": "serif",
        "pgf.texsystem": "pdflatex",
        "pgf.preamble": preamble,
    }
)


Devices = ["host", "device(s)"]
CallSemantics = ["functors", "stateless"]
DataFlow = ["batched", "patch-wise", "volume-wise"]

Markers = ["o", "s", "<", ">", "^", "v", "D"]
Colours = [
    "#ff0000",
    "#00ff00",
    "#0000ff",
    "#abab00",
    "#ab00ab",
    "#00abab",
    "#787878",
    "#ffab50",
    "#ff50ab",
    "#abff50",
    "#50ffab",
    "#50abff",
    "#ab50ff",
]
OutputFormats = [
    "png",
    "pdf",
    "pgf",
]
output_format = "png"


def main():
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - Kernel Benchmarks Plotting Script"
    )
    parser.add_argument(dest="file", help="File to parse")
    parser.add_argument(
        "-l",
        "--log-log",
        dest="log_log",
        action="store_true",
        help="Scale axes to log-log",
    )
    parser.add_argument(
        "-o",
        "--format",
        dest="output_format",
        choices=OutputFormats,
        default=output_format,
        help="|".join(OutputFormats),
    )
    args = parser.parse_args()
    data_lines = parse_file(args)

    print("Create plot for {}".format(args.file))
    plt.clf()
    marker_counter = 0
    number_of_patches = "xxx"
    number_of_launching_threads = "xxx"
    number_of_compute_threads = "xxx"

    for device in Devices:
        for call_semantics in CallSemantics:
            for data_flow in DataFlow:
                (
                    throughput_kernel,
                    throughput_total,
                    number_of_launching_threads,
                    number_of_compute_threads,
                    number_of_patches,
                ) = search_for_entries(data_lines, device, call_semantics, data_flow)
                assert len(throughput_total) == len(
                    throughput_kernel
                ), "data cardinalities {} vs {}".format(
                    len(throughput_total), len(throughput_kernel)
                )
                if len(throughput_total) > 0:
                    print(
                        "data found for {}, {}, {}".format(
                            device, call_semantics, data_flow
                        )
                    )

                    # Find the key corresponding to the maximal value
                    max_throughput_kernel_label = max(
                        throughput_kernel, key=lambda k: throughput_kernel[k]
                    )
                    # Get the maximal value
                    max_throughput_kernel_value = throughput_kernel[
                        max_throughput_kernel_label
                    ]

                    # Find the key corresponding to the maximal value
                    max_throughput_total_label = max(
                        throughput_total, key=lambda k: throughput_total[k]
                    )
                    # Get the maximal value
                    max_throughput_total_value = throughput_total[
                        max_throughput_total_label
                    ]

                    plt.scatter(
                        max_throughput_kernel_value,
                        max_throughput_total_value,
                        c=Colours[marker_counter % len(Colours)],
                        marker=Markers[marker_counter % len(Markers)],
                        label=device
                        + ", "
                        + call_semantics
                        + ", "
                        + data_flow
                        + ", ("
                        + max_throughput_kernel_label
                        + " / "
                        + max_throughput_total_label
                        + ")",
                    )
                    marker_counter += 1

    plt.title("Measured throughput - higher is better; only best case labels are shown")
    plt.legend(bbox_to_anchor=(1.05, 1), loc="upper left")
    plt.xlabel("Throughput for one kernel (dofs/s)")
    plt.ylabel("Total throughput (dofs/s)")
    plt.annotate(
        f"Number of patches: {number_of_patches}",
        xy=(0.5, 0.95),
        xycoords="axes fraction",
        ha="center",
    )
    plt.annotate(
        f"Number of launching threads: {number_of_launching_threads}",
        xy=(0.5, 0.90),
        xycoords="axes fraction",
        ha="center",
    )
    plt.annotate(
        f"Number of compute threads: {number_of_compute_threads}",
        xy=(0.5, 0.85),
        xycoords="axes fraction",
        ha="center",
    )
    plt.grid(color="grey", alpha=0.5, linestyle="dashed", linewidth=0.5)

    if args.log_log:
        plt.xscale("log")
        plt.yscale("log")

    if args.output_format == "pgf":
        fig = plt.gcf()
        canvas = FigureCanvasPgf(fig)
        canvas.print_figure(args.file + ".pgf", bbox_inches="tight")
    else:
        plt.savefig(args.file + "." + args.output_format, bbox_inches="tight")


def parse_file(args):
    # Initialize an empty list to store the lines
    data_lines = []

    with open(args.file, "r") as file:
        current_line = ""
        for line in file:
            line = line.strip()
            if line and line[:8].replace(":", "").isdigit():
                # Start a new line with the timestamp
                if current_line:
                    data_lines.append(current_line)
                current_line = line
            else:
                # Append subsequent lines to the current line
                current_line += " " + line
    # Add the last line to the list
    if current_line:
        data_lines.append(current_line)

    return data_lines


def search_for_entries(data_lines, device, call_semantics, data_flow):
    """
    Search for entries for particular argument combination
    We return two lists: The first one gives the throughput for one compute kernel
    while the second one gives the total throughput per measurement including kernel
    launch overhead.
    """
    throughput_kernel = {}
    throughput_total = {}
    number_of_patches = "xxx"
    number_of_launching_threads = "xxx"
    number_of_compute_threads = "xxx"

    for line in data_lines:
        if "Number of threads launching compute kernels" in line:
            number_of_launching_threads = line.split(":")[-1].strip()
        if "Number of compute threads" in line:
            number_of_compute_threads = line.split(":")[-1].strip()
        if "Number of patches per thread/compute kernel launch" in line:
            number_of_patches = line.split(":")[-1].strip()
        if device in line and call_semantics in line and data_flow in line:
            memory_layout = line.split(",")[3]
            parallelisation = line.split(",")[4].split(":")[0]
            current_throughput_kernel = 1.0 / float(line.split("|")[1])
            current_throughput_total = 1.0 / float(line.split("|")[4])
            throughput_kernel[
                memory_layout + ", " + parallelisation
            ] = current_throughput_kernel
            throughput_total[
                memory_layout + ", " + parallelisation
            ] = current_throughput_total
    return (
        throughput_kernel,
        throughput_total,
        number_of_launching_threads,
        number_of_compute_threads,
        number_of_patches,
    )


if __name__ == "__main__":
    main()
