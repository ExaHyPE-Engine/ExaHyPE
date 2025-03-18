# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

import matplotlib.pyplot as plt

import pandas as pd
import math
import sys

import exahype2.postprocessing


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="""
ExaHyPE 2 Load Balancing Plotter:

Default plotter for various standard ExaHyPE 2 dumps. The plotter examines
and illustrates the load balancing quality and other metrics.
"""
    )

    parser.add_argument(
        "file",
        nargs="+",
        help="Filename of the file/directory to parse",
    )
    parser.add_argument(
        "-p",
        "--plot",
        help="Type of plot: grid-creation-time, loadbalancing-threads, or loadbalancing-time.",
        default="loadbalancing-time",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-o", "--output", help="Name of the output file", default="output"
    )
    parser.add_argument(
        "-c",
        "--cells",
        help="Type of cells to examine for load balancing quality e.g. local cells, remote cells, all cells.",
        default="local",
    )
    parser.add_argument(
        "-s",
        "--strategy",
        help="Load balancing strategy to be examined e.g. SpreadOutOnceGridStagnates, SpreadOut, etc.",
        default="SpreadOut",
    )

    args = parser.parse_args()

    if args.plot == "loadbalancing-threads":
        data_points = []

        if os.path.isdir(args.file[0]):
            folder = args.file[0]
            files = []
            print("Parse directory " + folder)
            for data_file in os.listdir(folder):

                print(
                    "========================================================================"
                )
                print(folder + "/" + data_file + " from " + folder)
                print(
                    "========================================================================"
                )
                new_data = exahype2.postprocessing.PerformanceData(
                    folder + "/" + data_file, verbose=args.verbose
                )

                files.append(folder + "/" + data_file)

                data_points.append(new_data)

        else:
            print(args.file[0] + " is not a directory")

            data_points.append(
                exahype2.postprocessing.PerformanceData(
                    file_name=args.file[0], verbose=args.verbose
                )
            )

        transformed_data = []

        max_num_threads = 1

        for data_point in data_points:
            if (
                data_point.load_balancing_strategy != ""
                and data_point.load_balancing_strategy != "None"
                and data_point.load_balancing_strategy == args.strategy
            ):
                max_num_threads = max(max_num_threads, data_point._threads)

                if data_point._threads == 1:
                    data_point.relative_max_deviation_of_all_avg_cells = 1

                if args.cells == "local":
                    transformed_data.append(
                        (
                            data_point._threads,
                            data_point.relative_max_deviation_of_local_avg_cells,
                            data_point.load_balancing_strategy,
                        )
                    )
                elif args.cells == "remote":
                    transformed_data.append(
                        (
                            data_point._threads,
                            data_point.relative_max_deviation_of_remote_avg_cells,
                            data_point.load_balancing_strategy,
                        )
                    )
                else:
                    transformed_data.append(
                        (
                            data_point._threads,
                            data_point.relative_max_deviation_of_all_avg_cells,
                            data_point.load_balancing_strategy,
                        )
                    )

        data = transformed_data

        ideal_x = 1

        base = int(math.log2(max_num_threads))

        if 2**base != max_num_threads:
            max_num_threads = 2 ** (base + 1)

        while ideal_x <= max_num_threads:
            data.append((ideal_x, 1, "Ideal"))

            ideal_x *= 2

        if args.cells == "local":
            relative_max_deviation_cells_column_name = (
                "Relative Deviation of Local Cells"
            )
        elif args.cells == "remote":
            relative_max_deviation_cells_column_name = (
                "Relative Deviation of Virtual Cells"
            )
        else:
            relative_max_deviation_cells_column_name = "Relative Deviation of All Cells"

        df = pd.DataFrame(
            data,
            columns=[
                "Threads",
                relative_max_deviation_cells_column_name,
                "LoadBalancingStrategy",
            ],
        )

        grouped_data = df.groupby(
            ["Threads", "LoadBalancingStrategy"], as_index=False
        ).mean()

        x = grouped_data["Threads"]
        y = grouped_data[relative_max_deviation_cells_column_name]
        categories = grouped_data["LoadBalancingStrategy"]

        unique_categories = categories.unique()

        category_colors = {
            category: color
            for category, color in zip(unique_categories, plt.cm.tab10.colors)
        }
        category_colors["Ideal"] = "grey"

        colors = [category_colors[category] for category in categories]

        plt.scatter(x, y, c=colors, label="Data points")

        for strategy, group in grouped_data.groupby("LoadBalancingStrategy"):
            if strategy == "Ideal":
                plt.plot(
                    group["Threads"].values,
                    group[relative_max_deviation_cells_column_name].values,
                    color=category_colors[strategy],
                    linestyle="--",
                )
            else:
                plt.plot(
                    group["Threads"].values,
                    group[relative_max_deviation_cells_column_name].values,
                    color=category_colors[strategy],
                )

        plt.xlabel("Number of Threads")
        plt.ylabel(relative_max_deviation_cells_column_name)

        plt.xscale("log", base=2)

        i = 1
        xticks = []
        while i <= max_num_threads:
            xticks.append(i)
            i *= 2

        plt.xticks(xticks, labels=xticks)

        plt.grid(True)

        from matplotlib.lines import Line2D

        legend_elements = [
            Line2D(
                [0],
                [0],
                marker="o",
                color="w",
                markerfacecolor=category_colors[cat],
                markersize=10,
                label=cat,
            )
            for cat in unique_categories
        ]
        plt.legend(handles=legend_elements, title="Load Balancing Strategies")

        plt.tight_layout()

        plt.savefig(f"{args.output}.png")

        with open(f"{args.output}.txt", "w") as file:
            file.write("X_value\tY_value\tCategory\n")

            for xi, yi, category in zip(x, y, categories):
                file.write(f"{xi}\t{yi}\t{category}\n")

        print(
            f"Scatter plot with categorical color key and legend saved as '{args.output}.png'."
        )
        print(f"Data and categories saved as '{args.output}.txt'.")
    elif args.plot == "grid-creation-time":
        for file in args.file:
            if os.path.isdir(file):
                print("Parse file " + file)
                data_points = []
                for data_file in os.listdir(file):
                    print(
                        "========================================================================"
                    )
                    print(folder + "/" + data_file + " from " + folder)
                    print(
                        "========================================================================"
                    )
                    new_data = exahype2.postprocessing.PerformanceData(
                        file + "/" + data_file
                    )

                    data_points.append(new_data)
            else:
                print(file + " is not a directory")

        transformed_data = []

        ideal_added = False

        max_num_threads = 1

        max_construction_time = 0
        min_construction_time = sys.maxsize

        for data_point in data_points:
            if (
                data_point.load_balancing_strategy != ""
                and data_point.load_balancing_strategy == args.strategy
            ):
                max_num_threads = max(max_num_threads, data_point._threads)
                max_construction_time = max(
                    max_construction_time, data_point.total_construction_time
                )
                min_construction_time = min(
                    min_construction_time, data_point.total_construction_time
                )

                transformed_data.append(
                    (
                        data_point._threads,
                        data_point.total_construction_time,
                        data_point.load_balancing_strategy,
                    )
                )

                if data_point._threads == 1:
                    ideal_y = data_point.total_construction_time
                    ideal_added = True

        data = transformed_data

        base = int(math.log2(max_num_threads))

        if 2**base != max_num_threads:
            max_num_threads = 2 ** (base + 1)

        if ideal_added:
            ideal_x = 1

            while ideal_x <= max_num_threads:
                data.append((ideal_x, ideal_y, "Ideal"))

                ideal_x *= 2
                ideal_y /= 2

        df = pd.DataFrame(
            data, columns=["Threads", "GridCreationTime", "LoadBalancingStrategy"]
        )

        grouped_data = df.groupby(
            ["Threads", "LoadBalancingStrategy"], as_index=False
        ).mean()

        x = grouped_data["Threads"]
        y = grouped_data["GridCreationTime"]
        categories = grouped_data["LoadBalancingStrategy"]

        unique_categories = categories.unique()

        category_colors = {
            category: color
            for category, color in zip(unique_categories, plt.cm.tab10.colors)
        }
        category_colors["Ideal"] = "grey"

        colors = [category_colors[category] for category in categories]

        plt.scatter(x, y, c=colors, label="Data points")

        for strategy, group in grouped_data.groupby("LoadBalancingStrategy"):
            if strategy == "Ideal":
                plt.plot(
                    group["Threads"].values,
                    group["GridCreationTime"].values,
                    color=category_colors[strategy],
                    linestyle="--",
                )
            else:
                plt.plot(
                    group["Threads"].values,
                    group["GridCreationTime"].values,
                    color=category_colors[strategy],
                )

        plt.xlabel("Number of Threads")
        plt.ylabel("Grid Creation Time (in seconds)")

        plt.xscale("log", base=2)
        plt.yscale("log", base=2)

        i = 1
        xticks = []
        while i <= max_num_threads:
            xticks.append(i)
            i *= 2

        plt.xticks(xticks, labels=xticks)

        base = int(math.log2(max_construction_time))

        if 2**base != max_construction_time:
            max_construction_time = 2 ** (base + 1)

        base = int(math.log2(min_construction_time))
        min_construction_time = 2 ** (base)

        yticks = []
        i = min_construction_time / 4

        while i <= max_construction_time * 4:
            yticks.append(i)
            i *= 2

        plt.yticks(
            yticks,
            labels=yticks,
        )

        plt.grid(True)

        from matplotlib.lines import Line2D

        legend_elements = [
            Line2D(
                [0],
                [0],
                marker="o",
                color="w",
                markerfacecolor=category_colors[cat],
                markersize=10,
                label=cat,
            )
            for cat in unique_categories
        ]
        plt.legend(handles=legend_elements, title="Load Balancing Strategies")

        plt.tight_layout()

        plt.savefig(f"{args.output}.png")

        with open(f"{args.output}.txt", "w") as file:
            file.write("X_value\tY_value\tCategory\n")

            for xi, yi, category in zip(x, y, categories):
                file.write(f"{xi}\t{yi}\t{category}\n")

        print(
            f"Scatter plot with categorical color key and legend saved as '{args.output}.png'."
        )
        print(f"Data and categories saved as '{args.output}.txt'.")

    elif args.plot == "loadbalancing-time":
        data_points = []

        if os.path.isdir(args.file[0]):
            folder = args.file[0]
            files = []
            print("Parse directory " + folder)
            for data_file in os.listdir(folder):

                print(
                    "========================================================================"
                )
                print(folder + "/" + data_file + " from " + folder)
                print(
                    "========================================================================"
                )
                new_data = exahype2.postprocessing.PerformanceData(
                    folder + "/" + data_file, verbose=args.verbose
                )

                data_points.append(new_data)

                files.append(folder + "/" + data_file)

        else:
            print(args.file[0] + " is not a directory")
            files = args.file[0]
            data_points.append(
                exahype2.postprocessing.PerformanceData(
                    file_name=files, verbose=args.verbose
                )
            )

        transformed_data = []

        for data_point in data_points:
            if data_point.load_balancing_strategy == args.strategy:
                if args.cells == "local":
                    for (
                        time_stamp,
                        load_balancing_quality,
                    ) in data_point.previous_relative_max_deviation_of_local_avg_cells:
                        transformed_data.append(
                            (
                                time_stamp,
                                load_balancing_quality,
                                data_point.load_balancing_strategy,
                            )
                        )
                elif args.cells == "remote":
                    for (
                        time_stamp,
                        load_balancing_quality,
                    ) in data_point.previous_relative_max_deviation_of_remote_avg_cells:
                        transformed_data.append(
                            (
                                time_stamp,
                                load_balancing_quality,
                                data_point.load_balancing_strategy,
                            )
                        )
                else:
                    for (
                        time_stamp,
                        load_balancing_quality,
                    ) in data_point.previous_relative_max_deviation_of_all_avg_cells:
                        transformed_data.append(
                            (
                                time_stamp,
                                load_balancing_quality,
                                data_point.load_balancing_strategy,
                            )
                        )

        data = transformed_data

        if args.cells == "local":
            relative_max_deviation_cells_column_name = (
                "Relative Deviation of Local Cells"
            )
        elif args.cells == "remote":
            relative_max_deviation_cells_column_name = (
                "Relative Deviation of Virtual Cells"
            )
        else:
            relative_max_deviation_cells_column_name = "Relative Deviation of All Cells"

        df = pd.DataFrame(
            data,
            columns=[
                "Time",
                relative_max_deviation_cells_column_name,
                "LoadBalancingStrategy",
            ],
        )

        grouped_data = df.groupby(
            ["Time", "LoadBalancingStrategy"], as_index=False
        ).mean()

        x = grouped_data["Time"]
        y = grouped_data[relative_max_deviation_cells_column_name]
        categories = grouped_data["LoadBalancingStrategy"]

        unique_categories = categories.unique()

        category_colors = {
            category: color
            for category, color in zip(unique_categories, plt.cm.tab10.colors)
        }
        category_colors["Ideal"] = "grey"

        colors = [category_colors[category] for category in categories]

        plt.scatter(x, y, c=colors, label="Data points", s=5)

        for strategy, group in grouped_data.groupby("LoadBalancingStrategy"):
            if strategy == "Ideal":
                plt.plot(
                    group["Time"].values,
                    group[relative_max_deviation_cells_column_name].values,
                    color=category_colors[strategy],
                    linestyle="--",
                )
            else:
                plt.plot(
                    group["Time"].values,
                    group[relative_max_deviation_cells_column_name].values,
                    color=category_colors[strategy],
                )

        plt.xlabel("Time (in seconds)")
        plt.ylabel(relative_max_deviation_cells_column_name)
        plt.grid(True)

        from matplotlib.lines import Line2D

        legend_elements = [
            Line2D(
                [0],
                [0],
                marker="o",
                color="w",
                markerfacecolor=category_colors[cat],
                markersize=10,
                label=cat,
            )
            for cat in unique_categories
        ]
        plt.legend(handles=legend_elements, title="Load Balancing Strategies")

        plt.tight_layout()

        plt.savefig(f"{args.output}.png")

        with open(f"{args.output}.txt", "w") as file:
            file.write("X_value\tY_value\tCategory\n")

            for xi, yi, category in zip(x, y, categories):
                file.write(f"{xi}\t{yi}\t{category}\n")

        print(
            f"Scatter plot with categorical color key and legend saved as '{args.output}.png'."
        )
        print(f"Data and categories saved as '{args.output}.txt'.")
