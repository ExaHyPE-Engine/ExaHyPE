import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import re
from math import log
import sys
import os.path
import math
from matplotlib.lines import Line2D


params = {
    "axes.labelsize": 20,
    "axes.titlesize": 20,
    "font.size": 20,
    "font.family": "serif",
    "legend.fontsize": 20,
    "xtick.labelsize": 16,
    "ytick.labelsize": 16,
    "xtick.direction": "in",
    "ytick.direction": "in",
    "xtick.top": True,
    "ytick.right": True,
    "xtick.major.width": 1.5,
    "ytick.major.width": 1.5,
    "axes.linewidth": 1.5,
    "axes.labelsize": 20,
    "text.usetex": True,
    "figure.dpi": 300,
    "figure.autolayout": True,
    # 'figure.figsize': (10,10),
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
            timings.append( float(time) )

    assert len(particle_counts)==len(timings) or len(timings)==0, "array dimensions don't match: {} vs {} for kernel {} in {}".format(particle_counts, timings, kernel_name, file_name)
    return particle_counts, timings



Kernels  = [ "density", "force", "kick1", "kick2", "drift" ]

KernelName = {
    'density' : 'density',
    'force' : 'force',
    'kick1' : '1st kick',
    'kick2' : '2nd kick',
    'drift' : 'drift',
}

ImplementationVariant = {
    'force': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)', # not really true, but ensures consistent colours in plots
              '(continuous,local-active-coalesced)',
              '(continuous,active-local-coalesced)'],
    'density': ['(scattered,generic)',
                '(continuous,generic)',
                '(continuous,coalesced)', # not really true, but ensures consistent colours in plots
                '(continuous,local-active-coalesced)',
                '(continuous,active-local-coalesced)'],
    'kick1': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)'],
    'kick2': ['(scattered,generic)',
               '(continuous,generic)',
               '(continuous,coalesced)'],
    'drift': ['(scattered,generic)',
               '(continuous,generic)',
               '(continuous,coalesced)'],
}

ImplementationVariantName = {
    '(scattered,generic)': 'generic (scat.)',
    '(continuous,generic)': 'generic (cont.)',
    '(continuous,coalesced)': 'coalesced (cont.)',
    '(continuous,local-active-coalesced)': 'coalesced local-active (cont.)',
    '(continuous,active-local-coalesced)': 'coalesced active-local (cont.)',
}

Colors   = [ "#20a040", "#a04020", "#4020a0", "#f0a0f0", "#f06030", "#6030f0" ]
Symbols  = [ "-s", "-o", "-^", "-v", "-<", "->" ]

ParticleSize = 272

L2CacheSizeMap = {
    'gi001': 1024*1024*2*56,
    'gh003': 1024*1024*72,
}

def process_file(filename):
    print( "process file {}".format(filename) )

    plt.clf()
    mpl.rcParams.update(params)

    fig, ax = plt.subplots()

    kernel_legend_handles = []
    variants_legend_handles = []
    for k, kernel in enumerate(Kernels):
      kernel_legend_handles += [Line2D([0], [0], marker=Symbols[k][-1], color=Colors[0], label=f'{KernelName[kernel]}', linestyle='--')]
      for im, implementation in enumerate(ImplementationVariant[kernel]):
        if 'gpu' in filename and im > 2: continue
        x_data, y_data = read_file(filename,
                                   kernel + " " + implementation,
                                    )

        symbol = Symbols[ k % len(Symbols) ]
        color  = Colors[ im % len(Colors) ]
        label = f'{KernelName[kernel]} - {ImplementationVariantName[implementation]}'

        if len( y_data) > 0:
            plt.plot( x_data,
                      y_data,
                      symbol,
                      color=color,
                      label=label,
                      markevery=(im,len(ImplementationVariant[kernel]))
                    )

    for i, impl in enumerate(ImplementationVariant['force']):
      if 'gpu' in filename and i > 2: continue
      variants_legend_handles += [Line2D([0], [0], marker=Symbols[0][-1], color=Colors[i], label=f'... {ImplementationVariantName[impl]}', linestyle='--')]

    plt.xlabel("Particles")
    plt.ylabel("Time per particle update [t]=s")

    plt.xscale('log', base=10)
    plt.yscale('log', base=10)

    plt.minorticks_on()
    plt.grid(True, which='both')

    filename = filename.replace('.log', '')

    if 'gi001' in filename:
        plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
    elif 'gh003' in filename:
        plt.axvline(x = L2CacheSizeMap['gh003']/ParticleSize, color = 'k', linestyle = '--')
    else:
        raise Exception("I don't know what CPU I'm working on!")

    plt.ylim(1e-10, 1e-3)

    plt.savefig(f"{filename}.pdf")
    plt.savefig(f"{filename}.png")

    plt.clf()
    fig, ax = plt.subplots(figsize=(15, 1.2), constrained_layout=True)

    ax.axis("off")

    other_legend = ax.legend(loc="center right", handles=variants_legend_handles, fontsize=20, ncol=2, frameon=False)
    ax.add_artist(other_legend)

    plt.legend(loc="center left", handles=kernel_legend_handles, fontsize=20, ncol=3, frameon=False)

    plt.savefig(f"{filename}-legend.pdf")
    plt.savefig(f"{filename}-legend.png")

if __name__== "__main__":
    if len(sys.argv) < 2:
        print( "ERROR: Please pass in file to work on. ")
    else:
        for file in sys.argv[1:]:
            process_file(file)
