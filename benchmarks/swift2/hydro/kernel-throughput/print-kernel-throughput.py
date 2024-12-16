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
            time = line.split( kernel_name + ":" )[1].split( "(" )[0]
            timings.append( float(time)/particle_counts[-1] )

    assert len(particle_counts)==len(timings), "array dimensions don't match: {} vs {} for kernel {}".format(particle_counts, timings, kernel_name)
    return particle_counts, timings



Kernels  = [ "density", "force", "kick1", "kick2", "drift" ]
ImplementationVariant = [ "(baseline)", "(predicates)", "(coalesced,native)", "(prefix,native)", "(offloading,omp)" ]
Colors   = [ "#20a040", "#a04020", "#4020a0", "#f0a0f0", "#f06030", "#6030f0" ]
Symbols  = [ "-s", "-o", "-^", "-v", "-<", "->" ]


def process_file(filename):
    print( "process file {}".format(filename) )

    plt.clf()
    mpl.rcParams.update(params)

    for kernel in Kernels:
      for implementation in ImplementationVariant:
        x_data, y_data = read_file(filename,
                                   kernel + " " + implementation,
                                    )

        symbol = Symbols[ Kernels.index(kernel) % len(Symbols) ]
        color  = Colors[ ImplementationVariant.index(implementation) % len(Colors) ]
        label  = None

        if Kernels[0] == kernel and ImplementationVariant[0] == implementation:
          label = kernel + " " + implementation
        elif Kernels[0] == kernel:
          label = "... " + implementation
        elif ImplementationVariant[0] == implementation:
          label = kernel

        MarkerSpacing = 3
        plt.plot( x_data,
                  y_data,
                  symbol,
                  color=color,
                  label=label,
                  markevery=(ImplementationVariant.index(implementation) % MarkerSpacing,MarkerSpacing)
                )

    plt.xlabel("Particles")
    plt.ylabel("Time per particle update [t]=s")

    plt.xscale('log', base=10)
    plt.yscale('log', base=10)

    plt.minorticks_on()
    plt.grid(True, which='both')

    plt.legend(loc="lower left")

    plt.savefig(f"{filename}.pdf")
    plt.savefig(f"{filename}.png")


if __name__== "__main__":
    if len(sys.argv)!=2:
        print( "ERROR: Please pass in directory to work on. Expected 1 argument, got {}".format(sys.argv[1:]) )
    else:
        file = sys.argv[1]
        process_file(file)
