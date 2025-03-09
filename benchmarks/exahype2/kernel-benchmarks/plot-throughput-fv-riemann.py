# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter, FuncFormatter
from matplotlib.backends.backend_pgf import FigureCanvasPgf

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

def main():
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - Finite Volume Riemann Kernel Benchmark Throughput Plotting Script"
    )
    parser.add_argument(dest="file", help="File to parse")
    args = parser.parse_args()
    lines = parse_file(args)

    print("Create throughput plot for {}".format(args.file))

    kernel_labels = []
    kernel_throughputs = []

    for line in lines:
        for device in ["host", "device"]:
            if device in line:
                label = device + line.split(device)[-1].split(":")[0]
                throughput = 1.0 / float(line.split(device)[-1].split(":")[1].split("|")[1])
                kernel_labels.append(label)
                kernel_throughputs.append(throughput)
                print("data found for {}".format(label))

    plt.clf()
    fig, ax = plt.subplots(figsize=(20, 6))

    colors = plt.cm.viridis(np.linspace(0, 1, len(kernel_throughputs)))

    bars = plt.bar(
        kernel_labels,
        kernel_throughputs,
        color=colors,
        edgecolor="black",
        linewidth=1.2,
        zorder=2,
    )

    for bar in bars:
        bar.set_edgecolor('black')
        bar.set_linewidth(1.2)
        bar.set_alpha(0.9)
        bar.set_hatch('//')

    plt.grid(axis="y", linestyle="--", alpha=0.7, linewidth=0.7, zorder=1)

    plt.ylabel("Volume updates per second", fontsize=15)

    ax.set_facecolor('#d3d3d3')

    ax.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
    ax.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
    ax.yaxis.get_offset_text().set_fontsize(15)

    ax.tick_params(axis='both', which='major', labelsize=15)

    ax.set_xticks(range(len(kernel_labels)))
    ax.set_xticklabels(kernel_labels, fontsize=15)

    plt.xticks(fontsize=15)
    plt.yticks(fontsize=15)

    plt.savefig(args.file + ".png", bbox_inches="tight", transparent=False)
    plt.savefig(args.file + ".pdf", bbox_inches="tight", transparent=False)
    fig = plt.gcf()
    fig.patch.set_alpha(0)
    canvas = FigureCanvasPgf(fig)
    canvas.print_figure(args.file + ".pgf", bbox_inches="tight",)


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

if __name__ == "__main__":
    main()
