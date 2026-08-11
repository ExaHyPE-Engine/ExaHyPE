import numpy as np
import json
import matplotlib.pyplot as plt
import argparse
from enum import Enum

# For CPU runs compute time and launch time are functionally equivalent

class MeasurementType(Enum):
    COMPUTE_TIME = 0
    COMPUTE_TIME_NORMALISED = 1
    LAUNCH_TIME = 2
    LAUNCH_TIME_NORMALISED = 3

def string_to_enum(measurement_type: str) -> MeasurementType:
    if measurement_type == "ComputeTime":
        return MeasurementType.COMPUTE_TIME
    if measurement_type == "ComputeTimeNormalised":
        return MeasurementType.COMPUTE_TIME_NORMALISED
    if measurement_type == "LaunchTime":
        return MeasurementType.LAUNCH_TIME
    if measurement_type == "LaunchTimeNormalised":
        return MeasurementType.LAUNCH_TIME_NORMALISED
    raise ValueError(f"Unknown measurement type: {measurement_type}")

def enum_to_axis_label(measurement_type: MeasurementType):
    match measurement_type:
        case MeasurementType.COMPUTE_TIME:
            return "Run Time (s)"
        case MeasurementType.COMPUTE_TIME_NORMALISED:
            return "Run Time per Cell (s)"
        case MeasurementType.LAUNCH_TIME:
            return "Launch Time (s)"
        case MeasurementType.LAUNCH_TIME_NORMALISED:
            return "Launch Time per Cell (s)"
    

MeasurementTypes = [ "ComputeTime", "ComputeTimeNormalised", "LaunchTime", "LaunchTimeNormalised" ]

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - CCZ4 Finite Volume Rusanov Kernel Benchmarking Script"
)
parser.add_argument(
    "-m",
    "--measurement-type",
    choices=MeasurementTypes,
    required=True,
    help="|".join(MeasurementTypes)
)
args = parser.parse_args()

def plot_parallel_kernels():
    patch_sizes = [3, 6, 16]
    numbers_of_threads = [1, 2, 4, 8, 16, 32]

    for patch_size in patch_sizes:
        data = {}
        number_of_patches = None
        id = None
        measurement_number = 0

        for number_of_threads in numbers_of_threads:
            print (number_of_threads)
            input_file = f"mlir_results/ccz4_rusanov_parallel_size_{patch_size}_threads_{number_of_threads}.txt"
            with open(input_file, 'r') as file:
                for line in file:
                    if 'Number of patches per rank:' in line:
                        words = line.split()
                        number_of_patches = int(words[-1])
                    if not any(char.isdigit() for char in line) and ':' in line and 'Kernel ID' not in line:
                        # Only ids left (e.g. "host, AoS, serial")
                        id = line.split(':')[0].strip()
                    if '|' in line and '(' not in line and any(char.isdigit() for char in line):
                        measurement_type = MeasurementType(measurement_number)
                        if id not in data:
                            data[id] = {}
                        if number_of_threads not in data[id]:
                            data[id][number_of_threads] = {}
                        if measurement_type not in data[id][number_of_threads]:
                            data[id][number_of_threads][measurement_type] = {}
                        data[id][number_of_threads][measurement_type][number_of_patches] = float(line.split('|')[0].strip())
                        measurement_number += 1
                        if measurement_number > 3:
                            measurement_number = 0

        measurement_type = string_to_enum(args.measurement_type)
        color_cycle = plt.rcParams["axes.prop_cycle"].by_key()["color"]
        color_map = {}
        for i, number_of_threads in enumerate(numbers_of_threads):
            color_map[number_of_threads] = color_cycle[i]
        
        for id in data.keys():
            print(id)
            for number_of_threads in numbers_of_threads:
                print(number_of_threads)
                x_axis = list(data[id][number_of_threads][measurement_type].keys())
                y_axis = list(data[id][number_of_threads][measurement_type].values())
                if "mlir" not in id:
                    plt.plot(x_axis, y_axis, "--", color = color_map[number_of_threads], label=id)
                else:
                    plt.plot(x_axis, y_axis, color = color_map[number_of_threads], label=id)

        plt.title(f"CCZ4 Rusanov Kernel Performance (Patch Size: {patch_size})")
        plt.xlabel("Number of Patches")
        plt.ylabel(enum_to_axis_label(measurement_type))
        plt.xscale("log", base=2)
        if 'Normalised' not in args.measurement_type:
            plt.yscale("log", base=2)
        else:
            plt.ylim(bottom=0)
        plt.legend()
        plt.show()

def plot_serial_kernels():
    patch_sizes = [3, 6, 16]

    for patch_size in patch_sizes:
        data = {}
        number_of_patches = None
        id = None
        measurement_number = 0

        input_files = [f"mlir_results/ccz4_rusanov_cpu_serial_size_{patch_size}.txt",
                    f"mlir_results/ccz4_rusanov_parallel_size_{patch_size}_threads_1.txt"]

        # Read the input files and parse the data into nested dictionaries
        for input_file in input_files:
            with open(input_file, 'r') as file:
                for line in file:
                    if 'Number of patches per rank:' in line:
                        words = line.split()
                        number_of_patches = int(words[-1])
                    if not any(char.isdigit() for char in line) and ':' in line and 'Kernel ID' not in line:
                        # Only ids left (e.g. "host, AoS, serial")
                        id = line.split(':')[0].strip()
                    if '|' in line and '(' not in line and any(char.isdigit() for char in line):
                        measurement_type = MeasurementType(measurement_number)
                        if id not in data:
                            data[id] = {}
                        if measurement_type not in data[id]:
                            data[id][measurement_type] = {}
                        data[id][measurement_type][number_of_patches] = float(line.split('|')[0].strip())
                        print (data[id][measurement_type][number_of_patches])
                        measurement_number += 1
                        if measurement_number > 3:
                            measurement_number = 0

        measurement_type = string_to_enum(args.measurement_type)
        for id in data.keys():
            x_axis = list(data[id][measurement_type].keys())
            y_axis = list(data[id][measurement_type].values())
            plt.plot(x_axis, y_axis, label=id)

        plt.title(f"CCZ4 Rusanov Kernel Performance (Patch Size: {patch_size})")
        plt.xlabel("Number of Patches")
        plt.ylabel(enum_to_axis_label(measurement_type))
        plt.xscale("log", base=2)
        if 'Normalised' not in args.measurement_type:
            plt.yscale("log", base=2)
        else:
            plt.ylim(bottom=0)
        plt.legend()
        plt.show()

plot_parallel_kernels()
plot_serial_kernels()