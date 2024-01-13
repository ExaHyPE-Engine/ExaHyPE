import sys
import re
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pgf import FigureCanvasPgf

# Define the LaTeX preamble for the output file
preamble = r"\usepackage{amsmath,amssymb}"

# Set the rcParams dictionary to use the pgf backend and include the LaTeX preamble
plt.rcParams.update(
    {
        #"font.family": "serif",
        "pgf.texsystem": "pdflatex",
        "pgf.preamble": preamble,
    }
)

output_file_name = "Plot_ADERDG_CPUs_Leonardo"
output_format = "png"
title = "Strong Scaling"
xlabel = "Number of Cores"
ylabel = "Number of Timesteps per Second (TPS)"
log_x = False
log_y = False


def parse_file(file_path):
    with open(file_path, 'r') as file:
        parsing_started = False
        num_ranks = None
        num_cores = None
        num_devices = None
        throughput_value = None

        for line in file:
            match = re.search(r'tarch::dumpStatistics\(\)\s+(\d+) Rank\(s\),\s+(\d+) Core\(s\)(?:,\s+(\d+) Device\(s\))?', line)
            if match:
                num_ranks = int(match.group(1))
                num_cores = int(match.group(2))
                num_devices = int(match.group(3))

            if 'Terminated successfully' in line:
                throughput_value = -1

            if 'TPS:' in line and throughput_value == -1:
                throughput_value = float(line.split('TPS:')[1].split()[0])
                print(throughput_value)

            if num_ranks is not None and (throughput_value is not None and throughput_value is not -1):
                return throughput_value, num_ranks, num_cores, num_devices

    raise Exception("Cannot find TPS or your simulation has not been successfully terminated!")


def plot_scaling(input_files):
    print("Create plot for {}".format(input_files))
    plt.clf()

    num_ranks_values = []
    num_cores_values = []
    num_devices_values = []
    throughput_values = []

    for file_path in input_files:
        throughput_value, num_ranks, num_cores, num_devices = parse_file(file_path)

        if throughput_value is not None:
            throughput_values.append(throughput_value)
            num_cores_values.append(num_cores)
            num_ranks_values.append(num_ranks)
            num_devices_values.append(num_devices)

    plt.plot(num_cores_values, throughput_values, marker='o', linestyle='-', label='Throughput')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.xticks(num_cores_values)
    #plt.yticks(throughput_values)

    if log_x:
        plt.xscale("log")
    if log_y:
        plt.yscale("log")

    if output_format == "pgf":
        fig = plt.gcf()
        canvas = FigureCanvasPgf(fig)
        canvas.print_figure(output_file_name + ".pgf", bbox_inches="tight")
    else:
        plt.savefig(output_file_name + "." + output_format, bbox_inches="tight")

    print("Number of Ranks:", num_ranks_values)
    print("Number of Cores:", num_cores_values)
    print("Number of Devices:", num_devices_values)
    print("Throughput Values:", throughput_values)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 plot_scaling.py input1.txt input2.txt input3.txt")
        sys.exit(1)

    input_files = sys.argv[1:]
    plot_scaling(input_files)
