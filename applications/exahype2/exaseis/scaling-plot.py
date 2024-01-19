import sys
import re
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator
from matplotlib.ticker import ScalarFormatter
from matplotlib.backends.backend_pgf import FigureCanvasPgf

# Define the LaTeX preamble for the output file
preamble = r"\usepackage{amsmath,amssymb}"

# Set the rcParams dictionary to use the pgf backend and include the LaTeX preamble
plt.rcParams.update(
    {
        #"font.family": "serif",
        "pgf.texsystem": "pdflatex",
        "pgf.preamble": preamble,
        'xtick.minor.size': 0,
        'xtick.minor.width': 0,
        'ytick.minor.size': 0,
        'ytick.minor.width': 0,
    }
)

output_file_name = "scaling-plot_GPUs_Leonardo"
output_format = "png"
title = "Strong Scaling - 1 GPU per Node"
xlabel = "Number of Nodes"
ylabel = "Number of Timesteps per Second (TPS)"
label = "Order 7"
plot_color = "red"
log_x = True
log_y = True


def parse_file(file_path):
    with open(file_path, 'r') as file:
        parsing_started = False
        num_ranks = None
        num_cores = None
        num_gpus = None
        throughput_value = None

        for line in file:
            pattern = r"(\d+) Rank"
            result = re.search(pattern, line)
            if result:
              num_ranks = int(result.group(1))

            pattern = r"(\d+) Core"
            result = re.search(pattern, line)
            if result:
              num_cores = int(result.group(1))

            pattern = r"(\d+) GPU"
            result = re.search(pattern, line)
            if result:
              num_gpus = int(result.group(1))

            if 'Terminated successfully' in line:
                throughput_value = -1

            if 'TPS:' in line and throughput_value == -1:
                throughput_value = float(line.split('TPS:')[1].split()[0])

            if num_ranks is not None and num_cores is not None and num_gpus is not None and \
                (throughput_value is not None and throughput_value is not -1):
                return throughput_value, num_ranks, num_cores, num_gpus

    raise Exception("Cannot find data in file {} or your simulation has not been successfully terminated!".format(file_path))


def plot_scaling(input_files):
    print("Create plot for {}".format(input_files))
    plt.clf()

    throughput_values = []
    num_ranks_values = []
    num_cores_values = []
    num_gpus_values = []

    for file_path in input_files:
        throughput_value, num_ranks, num_cores, num_gpus = parse_file(file_path)
        throughput_values.append(throughput_value)
        num_ranks_values.append(num_ranks)
        num_cores_values.append(num_cores)
        num_gpus_values.append(num_gpus)

    ideal_values = [throughput_values[0]]
    for i in range(1, len(throughput_values)):
      ideal_value = 2 * ideal_values[i - 1]
      ideal_values.append(ideal_value)

    plt.plot(num_gpus_values, throughput_values, marker='o', linestyle='-', color=plot_color, label=label)
    plt.plot(num_gpus_values, ideal_values, linestyle='--', color='grey', label='Ideal')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.grid(True)

    if log_x:
        plt.xscale("log", base=2)
    if log_y:
        plt.yscale("log", base=2)

    plt.xticks(num_gpus_values, num_gpus_values)
    #plt.gca().yaxis.set_major_formatter(ScalarFormatter())

    if output_format == "pgf":
        fig = plt.gcf()
        canvas = FigureCanvasPgf(fig)
        canvas.print_figure(output_file_name + ".pgf", bbox_inches="tight")
    else:
        plt.savefig(output_file_name + "." + output_format, bbox_inches="tight")

    print("Number of Ranks:", num_ranks_values)
    print("Number of Cores:", num_cores_values)
    print("Number of GPUs:", num_gpus_values)
    print("Throughput Values:", throughput_values)
    print("Ideal Throughput Values:", ideal_values)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 plot_scaling.py input1.txt input2.txt input3.txt")
        sys.exit(1)

    input_files = sys.argv[1:]
    plot_scaling(input_files)
