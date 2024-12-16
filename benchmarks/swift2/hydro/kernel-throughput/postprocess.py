import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import re
from math import log
import sys
import os.path
import math


params = {
    "axes.labelsize": 14,
    "axes.titlesize": 14,
    "font.size": 14,
    "font.family": "serif",
    "legend.fontsize": 14,
    "xtick.labelsize": 14,
    "ytick.labelsize": 14,
    "xtick.direction": "in",
    "ytick.direction": "in",
    "xtick.top": True,
    "ytick.right": True,
    "xtick.major.width": 1.5,
    "ytick.major.width": 1.5,
    "axes.linewidth": 1.5,
    "axes.labelsize": 16,
    "text.usetex": True
}
mpl.rcParams.update(params)


global_y_value_min = 1e100
global_y_value_max = 0


def read_file(file_name,
              kernel_name,
              ):
    """!

    Read one output file

    Parses one input file identified through file_name and returns two
    lists: the first one is a list of the number of particles, the second gives
    the timings. We only parse data from one kernel type identified through
    kernel_name.

    """
    particle_counts = []
    timings         = []

    file = open( file_name, "r" )

    for line in file:
        if "Experiment:" in line:
            number_of_particles = line.split( "Experiment:" )[1].split( "particles" )[0]
            particle_counts.append( float(number_of_particles) )
        if kernel_name in line:
            time = line.split( "kernel:" )[1].split( "(" )[0]
            timings.append( float(time) )

    assert len(particle_counts)==len(timings), "array dimensions don't match: {} vs {}".format(particle_counts, timings)
    return particle_counts, timings


def calibrate_data( x_data, y_data, variant ):
    """!

    The data read from the file are pure data, i.e. raw timings. We
    want to plot througput however, i.e. calculations per second.

    @return Flag that clarifies if to plot this data set or not

    """
    global global_y_value_min
    global global_y_value_max

    for i in range(0,len(x_data)):
        y_data[i]= x_data[i] / y_data[i]

    global_y_value_min = min( global_y_value_min, min(y_data) )
    global_y_value_max = max( global_y_value_max, max(y_data) )

    return True



def calibrate_ranges():
    global global_y_value_min
    global global_y_value_max

    global_y_value_min = 10**math.floor(math.log10(global_y_value_min))
    global_y_value_max = 10**math.ceil(math.log10(global_y_value_max))



def write_file():
    calibrate_ranges()

    plt.savefig(f"{directory}/plot-{kernel_name}.pdf")
    plt.savefig(f"{directory}/plot-{kernel_name}.png")



Variants = [ "original", "compressed" ]
Colors  = [ "#20a040", "#a04020", "#4020a0", "#30f060", "#f06030", "#6030f0" ]
Symbols = [ "s", "o", "^", "v", "<", ">" ]

def plot_kernel(kernel_name,
                directory,
                plot_postprocessing = write_file,
                calibrate_data = calibrate_data,
                ):
    """!

    @param plot_postprocessing: Functor without arguments or none

    """
    plt.clf()

    global global_y_value_min
    global global_y_value_max

    global_y_value_min = 1e100
    global_y_value_max = 0

    plot_counter = 0

    for threads in [f"{number:02d}" for number in range(0,256)]:
        for variant in Variants:
            filename = directory + "/" + variant + "-" + str(threads) + "-threads.log"

            if os.path.isfile(filename):
                print( "- found input data file {} for kernel {} with variant {} and thread count {}".format(filename, kernel_name, variant, threads) )
                x_data, y_data = read_file(filename,
                                           kernel_name,
                                           )

                if calibrate_data( x_data, y_data, variant ):
                    symbol = Symbols[ plot_counter % len(Symbols) ]
                    if variant==Variants[0]:
                        symbol = symbol + "-"
                    elif variant==Variants[1]:
                        symbol = symbol + "--"

                    label = "{}, {} threads".format(variant,threads)
                    plt.plot( x_data,
                              y_data,
                              symbol,
                              color=Colors[ plot_counter % len(Colors) ],
                              label=label
                              )

                if variant==Variants[1]:
                    plot_counter += 1

    plt.xlabel("\#Particles")
    plt.ylabel("Throughput")
    kernel_name = kernel_name[0].upper() + kernel_name[1:]
    kernel_name_title = kernel_name.replace("1","").replace("2","")
    plt.title(f"{kernel_name_title} kernel")

    plt.xscale('log', base=10)
    plt.yscale('log', base=10)

    plt.minorticks_on()
    plt.grid(True, which='both')

    plt.legend()

    plot_postprocessing()


def process_directory(directory):
    print( "process content of directory {}".format(directory) )
    try:
        plot_kernel("density", directory)
    except Exception as exc:
        print( "density plots failed: {}".format(exc) )
    try:
        plot_kernel("force",   directory)
    except Exception as exc:
        print( "force plots failed: {}".format(exc) )
    try:
        plot_kernel("kick1",   directory)
    except Exception as exc:
        print( "kick1 plots failed: {}".format(exc) )
    try:
        plot_kernel("kick2",   directory)
    except Exception as exc:
        print( "kick2 plots failed: {}".format(exc) )
    try:
        plot_kernel("drift",   directory)
    except Exception as exc:
        print( "drift plots failed: {}".format(exc) )



if __name__== "__main__":
    if len(sys.argv)!=2:
        print( "ERROR: Please pass in directory to work on. Expected 1 argument, got {}".format(sys.argv[1:]) )
    else:
        directory = sys.argv[1]
        process_directory(directory)
