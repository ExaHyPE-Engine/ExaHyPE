"""

 A simple Python script to postprocess, inspect and convert the csv
 files dumped by the particle database. 

"""


import os, sys
import argparse
import peano4.toolbox.particles.postprocessing

import matplotlib.pyplot as plt


parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Particle toolbox postprocessing"
)
parser.add_argument(
    "files",
    metavar="files",
    nargs="+",
    help="Input files. If the filename does not contain the postfix .csv, the script will load all files fitting filename*.csv.",
)
parser.add_argument(
    "-d",
    "--dim",
    dest="dimensions",
    type=int,
    required=True,
    help="Dimensions of underlying simulation, i.e. 2 or 3",
)
parser.add_argument(
    "-o",
    "--output",
    dest="output",
    help="Output name. An appropritae filename extension is added automatically depending on the output format",
)
parser.add_argument(
    "-t",
    "--target",
    dest="target",
    choices=["seismogram", "initial-tracer-location"],
    help="Pick output type",
)
parser.add_argument(
    "-v",
    "--value",
    dest="value",
    required=True,
    help="Pick value to go with target (quantity of interest of Q data projected onto tracer)",
)
parser.add_argument(
    "-p",
    "--pick",
    dest="pick",
    default="",
    help="Pick particles. Empty means all particles, otherwise you specify the particles through '(1,4)(7,8)'",
)
parser.add_argument(
    "-l", "--legend", dest="legend", default=None, help="Legend (comma-separated)"
)
args = parser.parse_args()


print(
    """

ExaHyPE 2 - Particle toolbox postprocessing
===========================================
(C) Mladen Ivkovic, Tobias Weinzierl, Han Zhang

Type in --help to find out about the arguments.

"""
)

datasets = []
for file in args.files:
    if not ".csv" in file:
        print(
            "filename " + file + " is not complete. Search for files " + file + "*.csv"
        )
        new_dataset = peano4.toolbox.particles.postprocessing.load_file_sequence(
            file, args.dimensions
        )
    else:
        new_dataset = peano4.toolbox.particles.postprocessing.Dataset()
        new_dataset.parse(file, args.dimensions)
        if new_dataset.valid:
            datasets.append(new_dataset)
        else:
            print(
                "ERROR: file "
                + file
                + " was not a valid Peano 4 particle database file"
            )

print("merge {} datasets".format(len(datasets)))
flattened_dataset = datasets[0]
for i in datasets[1:-1]:
    flattened_dataset.append(i)


if args.pick != "":
    for pick in args.pick.split("(")[1:]:
        filter = (int(pick.split(",")[0]), int(pick.split(",")[1].split(")")[0]))
        flattened_dataset.add_filter(filter)


if args.target == "seismogram":
    column = int(args.value)
    plt.clf()
    labels = None
    if args.legend != None:
        labels = args.legend.split(",")
    peano4.toolbox.particles.postprocessing.seismogram(
        flattened_dataset, column, labels
    )
    plt.legend()
    output = args.output
    if output == None:
        output = args.files[0]
        plt.title(output)
    plt.xlabel("Simulation time $t$")
    plt.ylabel("$Q_{" + str(column) + "}(t)$")
    plt.savefig(output + ".pdf")
    plt.savefig(output + ".png")
elif args.target == "initial-tracer-location" and args.dimensions == 2:
    keys = flattened_dataset.extract_particle_keys()
    for particle in keys:
        time, pos_x, pos_y = flattened_dataset.get_position(particle)
        print(
            "Particle {},{}: {}, {}".format(
                particle[0], particle[1], pos_x[0], pos_y[0]
            )
        )
elif args.target == "initial-tracer-location" and args.dimensions == 3:
    keys = flattened_dataset.extract_particle_keys()
    for particle in keys:
        time, pos_x, pos_y, pos_z = flattened_dataset.get_position(particle)
        print(
            "Particle {},{}: {}, {}, {}".format(
                particle[0], particle[1], pos_x[0], pos_y[0], pos_z[0]
            )
        )
else:
    print("Unknown target")
