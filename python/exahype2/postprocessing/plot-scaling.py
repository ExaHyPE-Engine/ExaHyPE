# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os
import exahype2.postprocessing
import sys

from tkinter import Tcl
import numpy as np

import matplotlib.pyplot as plt


Colors = ["#aa0000", "#00aa00", "#0000aa", "#797900", "#790079", "#007979", "#ababab"]
#
# Number of symbols should differ from colours by one
#
Symbols = ["o", "s", "<", ">", "v", "^"]


def add_linear_speedup_trend_line():
    global max_x
    global max_serial_y
    global min_serial_y

    plt.plot([1, max_x], [max_serial_y, max_serial_y / max_x], "--", color="#ababab")
    plt.plot([1, max_x], [min_serial_y, min_serial_y / max_x], "--", color="#ababab")


def compute_min_max(x_data, y_data):
    global max_x
    global max_serial_y
    global min_serial_y

    if len(x_data) > 0:
        max_x = max(max_x, x_data[-1])

    for i in range(0, len(x_data)):
        serialised_value = y_data[i] * x_data[i]
        max_serial_y = max(max_serial_y, serialised_value)
        min_serial_y = min(min_serial_y, serialised_value)


def visualise(data_points, symbol_counter, colour_counter):
    global max_x
    global max_time
    global max_serial_y
    global min_serial_y

    (x_data, y_data) = exahype2.postprocessing.extract_times_per_step(
        data_points, args.max_cores_per_rank
    )

    compute_min_max(x_data, y_data)

    if args.plot_efficiency:
        normalised_fasted_time = y_data[0] * x_data[0]
        for i in range(0, len(x_data)):
            y_data[i] = normalised_fasted_time / y_data[i] / x_data[i]
        if args.max_cores_per_rank > 0:
            y_data = [y / float(args.max_cores_per_rank) for y in y_data]
        y_data = [min(y, 1.1) for y in y_data]

    symbol = "-" + Symbols[symbol_counter % len(Symbols)]
    my_color = Colors[colour_counter % len(Colors)]
    # my_markevery = ( 0.1 + 0.8 * (colour_counter+1/len(Colors) )**3, 0.1 * 0.8 * symbol_counter / len(Symbols) )
    # First entry is first marker (start to to count with 0), second is spacing
    my_markevery = (
        symbol_counter % len(Symbols) % (len(x_data) + 1),
        int(len(x_data) / 8) + 1,
    )

    if len(y_data) > 0:
        max_time = max(y_data[0], max_time)

    if args.labels == "":
        plt.plot(x_data, y_data, symbol, color=my_color, markevery=my_markevery)
    else:
        try:
            my_label = args.labels.split(",")[args.file.index(file)]
        except:
            raise Exception(
                "Unable to extract "
                + str(args.file.index(file))
                + "th entry from "
                + args.labels
                + ": "
                + str(args.labels.split(","))
            )
        plt.plot(
            x_data,
            y_data,
            symbol,
            label=my_label,
            color=my_color,
            markevery=my_markevery,
        )

        # Save plot data into a file
        if args.export_data:
            np.savetxt(
                args.output + ".txt", np.c_[x_data, y_data], fmt=["%1.f", "%1.2f"]
            )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="""
ExaHyPE 2 scaling plotter:

A generic script to create speedup plots.

"""
    )
    parser.add_argument(
        "file",
        nargs="+",
        help="filename of the file to parse. You can specify multiple files",
    )
    parser.add_argument(
        "-v", "--verbose", help="increase output verbosity", action="store_true"
    )
    parser.add_argument(
        "--max-cores-per-rank",
        dest="max_cores_per_rank",
        type=int,
        help="max number of cores per rank (pick 0 if you have only one core count per rank, pick -1 if you want to plot single-node data)",
        default=0,
    )
    parser.add_argument(
        "--group-data",
        dest="group_data",
        help="group k consecutive measurements as one piece of data, i.e. use similar styles (default=0, i.e. off)",
        default=0,
        type=int,
    )
    parser.add_argument(
        "--log-x", dest="log_x", help="plot with logarithmic axes", action="store_true"
    )
    parser.add_argument(
        "--log-y", dest="log_y", help="plot with logarithmic axes", action="store_true"
    )
    parser.add_argument(
        "--plot-efficiency",
        dest="plot_efficiency",
        help="Don't plot raw times but efficiency",
        action="store_true",
    )
    parser.add_argument(
        "--labels",
        dest="labels",
        help="Plot labels. You can use $...$ to use the math mode, but you will have to escape the dollars with a \\. Separate with , and ensure there's exactly one entry per file",
        default="",
    )
    parser.add_argument(
        "--output",
        dest="output",
        help="output file prefix (file name extension is added automatically)",
        default="",
    )
    parser.add_argument(
        "--solver-name",
        dest="solver_name",
        help="Solver name (leave blank if there is only one solver)",
        default="",
    )
    parser.add_argument(
        "--export-data",
        dest="export_data",
        help="Export plot data into a txt file",
        action="store_true",
    )
    parser.add_argument(
        "--sort-files",
        dest="sort_files",
        help="Switch file sorting (natural sort)",
        default=True,
    )
    parser.add_argument("--title", dest="title", help="Title of plot", default=None)
    args = parser.parse_args()

    max_x = -1
    max_time = -1
    min_serial_y = sys.float_info.max
    max_serial_y = 0

    plt.clf()

    symbol_counter = 0
    colour_counter = 0

    for file in args.file:
        if os.path.isdir(file):
            print("Parse file " + file)
            data_points = []
            for data_file in os.listdir(file):
                print(
                    "========================================================================"
                )
                print(file + "/" + data_file + " from " + file)
                print(
                    "========================================================================"
                )
                new_data = exahype2.postprocessing.PerformanceData(
                    file + "/" + data_file, args.solver_name, args.verbose
                )
                if new_data.valid:
                    data_points.append(new_data)
            visualise(data_points, symbol_counter, colour_counter)
        elif file.endswith("tar") or file.endswith("tar.gz"):
            print("Parse archive " + file)
            open_mode = ""
            if file.endswith("tar.gz"):
                open_mode = "r:gz"
            elif file.endswith("tar"):
                open_mode = "r:"

            tar = None
            try:
                tar = tarfile.open(file, open_mode)
                data_files = tar.getnames()

                # Sort input data
                if args.sort_files:
                    data_files = Tcl().call("lsort", "-dict", data_files)

                data_points = []
                for data_file in data_files:
                    print(
                        "========================================================================"
                    )
                    print(data_file + " from " + file)
                    print(
                        "========================================================================"
                    )
                    tar.extract(data_file)
                    new_data = exahype2.postprocessing.PerformanceData(
                        data_file, args.solver_name, args.verbose
                    )
                    if new_data.valid:
                        data_points.append(new_data)
                    os.remove(data_file)

                if args.verbose:
                    print("have successfully parsed {} files".format(len(data_files)))

                if len(data_files) > 0:
                    visualise(data_points, symbol_counter, colour_counter)
                else:
                    print("ERROR: no files in archive")
            except Exception as e:
                print("Error: " + str(e))
                tar = None
            if tar != None:
                tar.close()
        else:
            print(file + " is neither a directory or an archive")

        symbol_counter += 1
        if args.group_data > 0:
            if symbol_counter >= args.group_data:
                symbol_counter = 0
                colour_counter += 1
        else:
            colour_counter += 1

    if args.plot_efficiency:
        plt.ylabel("Efficiency")
    else:
        add_linear_speedup_trend_line()
        plt.ylabel("Time per time step [t]=s")
    if args.log_x:
        plt.xscale("log", base=2)
    if args.log_y:
        plt.yscale("log", base=2)

    if args.max_cores_per_rank < 0:
        plt.xlabel("Cores")
    else:
        plt.xlabel("Ranks")

    xtics = [1]
    xlabels = ["1"]
    while xtics[-1] < max_x:
        xtics.append(xtics[-1] * 2)
        xlabels.append(str(xtics[-1]))
    plt.xticks(xtics, xlabels)

    plt.legend()
    output_file_name = args.output
    if output_file_name == "":
        output_file_name = args.file[0].replace(".tar.gz", "").replace(".tar", "")

    if args.title != None:
        plt.title(args.title)

    plt.savefig(output_file_name + ".pdf")
    plt.savefig(output_file_name + ".png")
