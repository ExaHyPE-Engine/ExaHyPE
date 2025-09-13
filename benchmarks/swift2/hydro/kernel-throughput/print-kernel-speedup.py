import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import re
from math import log
import sys
import os.path
import math
from pathlib import Path
import matplotlib.ticker as mticker
from matplotlib.lines import Line2D

def log10_tick_formatter(val, pos=None):
    return r"$10^{:.0f}$".format(val)

def log2_tick_formatter(val, pos=None):
    return r"$2^{:.0f}$".format(val)

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

cells = ['0064', '0128', '0256', '0512', '1024']
arches = ['x64', 'aarch64']
predicates = ['true', 'false']
symmetry = ['true', 'false']

kernels = {
    'force': ['scattered,generic',
              'continuous,generic',
              'continuous,local-active-coalesced',
              'continuous,active-local-coalesced'],
    'density': ['scattered,generic',
                'continuous,generic',
                'continuous,local-active-coalesced',
                'continuous,active-local-coalesced'],
    'kick1': ['scattered,generic',
              'continuous,generic',
              'continuous,coalesced'],
    'kick2': ['scattered,generic',
              'continuous,generic',
              'continuous,coalesced'],
    'drift': ['scattered,generic',
              'continuous,generic',
              'continuous,coalesced'],
}

kernel_names = {
    'scattered,generic': 'generic scat.',
    'continuous,generic': 'generic cont.',
    'continuous,coalesced': 'coalesced cont.',
    'continuous,local-active-coalesced': 'coalesced local-active cont.',
    'continuous,active-local-coalesced': 'coalesced active-local cont.',
}

def read_kernel_speedup(data_aos, data_soa, kernel, kernel_variant):
    variant_prefix = f'{kernel} ({kernel_variant}):\t'
    variant_times_aos = [float(line.split(variant_prefix)[1].split(' ')[0]) for line in data_aos.split('\n') if variant_prefix in line]
    variant_times_soa = [float(line.split(variant_prefix)[1].split(' ')[0]) for line in data_soa.split('\n') if variant_prefix in line]
    variant_speedups = np.array(variant_times_aos) / np.array(variant_times_soa)

    particle_sizes = [int(line.split( "Experiment:" )[1].split( "particles" )[0]) for line in data_aos.split('\n') if 'Experiment' in line]
    return particle_sizes, variant_speedups

def read_kernel_speedup_classic(folder, arch, kernel, kernel_variant, pred, sym='false', cell=cells[-1]):
    data_aos = Path(f'{folder}/out-{arch}-soafalse-cell{cell}-pred{pred}-sym{sym}.log').read_text()
    data_soa = Path(f'{folder}/out-{arch}-soatrue-cell{cell}-pred{pred}-sym{sym}.log').read_text()
    return read_kernel_speedup(data_aos, data_soa, kernel, kernel_variant)

def read_kernel_speedup_offload(folder, arch, kernel, kernel_variant, cell=cells[-1]):
    data_aos = Path(f'{folder}/out-{arch}-soafalse-cell{cell}-predfalse-symfalse-gpufalse.log').read_text()
    data_soa = Path(f'{folder}/out-{arch}-soatrue-cell{cell}-predfalse-symfalse-gputrue.log').read_text()
    return read_kernel_speedup(data_aos, data_soa, kernel, kernel_variant)

def read_kernel_speedup_offload_shmem(folder, arch, kernel, kernel_variant, cell=cells[-1]):
    data_aos = Path(f'{folder}/out-{arch}-soafalse-cell{cell}-predfalse-symfalse-gpufalse-shmem.log').read_text()
    data_soa = Path(f'{folder}/out-{arch}-soatrue-cell{cell}-predfalse-symfalse-gputrue-shmem.log').read_text()
    return read_kernel_speedup(data_aos, data_soa, kernel, kernel_variant)

Colors   = [ "#20a040", "#a04020", "#4020a0", "#f0a0f0", "#f06030", "#6030f0" ]
Symbols  = [ "-s", "-o", "-^", "-v", "-<", "->" ]

# Graph1 is a 2d graph that shows the normalised speedup over the AoS version
# for a given cell size (a default in `read_kernel_speedup`)
def print_graph_1(folder, arch, kernel):
    plt.clf()
    mpl.rcParams.update(params)

    for counter, variant in enumerate(kernels[kernel]):
        for pred in predicates:
            particle_sizes, speedups = read_kernel_speedup_classic(folder, arch, kernel, variant, pred)
            pred = True if pred == 'true' else False
            symbol = f'{'-' if not pred else ''}{Symbols[counter]}'
            variant_name = kernel_names[variant]
            plt.plot(particle_sizes, speedups, symbol, color=Colors[counter], label=f'{variant_name}(pred. {'on' if pred else 'off'})')

    plt.xlabel("Particles")
    plt.ylabel("Norm kernel speedup over AoS version")

    plt.xscale('log', base=10)

    plt.minorticks_on()
    plt.grid(True, which='both')

    plt.axhline(y = 1, color = 'k', linestyle = '--')

    plt.savefig(f"{folder}/plot-1-{arch}-{kernel}.pdf")
    plt.savefig(f"{folder}/plot-1-{arch}-{kernel}.png")

    legend = plt.legend()
    fig_legend = plt.figure(figsize=(14, 2))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=legend.legend_handles, labels=[text.get_text() for text in legend.get_texts()], loc="center", ncol=4, frameon=False)
    fig_legend.savefig(f"{folder}/plot-1-{arch}-{kernel}-legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder}/plot-1-{arch}-{kernel}-legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')  # Close all figures
    plt.rcdefaults()  # Reset rcParams to defaults

# Graph2 is a 3d graph that shows the normalised speedup over the AoS version
# over all cell sizes but turned off predicates and symmetric interactions
def print_graph_2(folder, arch, kernel):
    plt.clf()
    mpl.rcParams.update(params)

    ax = plt.axes(projection='3d')

    ax.set_xlabel("Particles")
    ax.set_ylabel("Cell size")
    ax.set_zlabel('Norm kernel speedup over AoS version', rotation=60)

    ax.xaxis.set_major_formatter(mticker.FuncFormatter(log10_tick_formatter))
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(log2_tick_formatter))

    legend_handles = []
    for counter, variant in enumerate(kernels[kernel]):
        variant_name = kernel_names[variant]
        marker = f'{Symbols[counter][-1]}'
        colour = Colors[counter]
        legend_handles += [Line2D([0], [0], marker=marker, color=colour, label=f'{variant_name}', linestyle='--')]
        for cell in cells:
            particle_sizes, speedups = read_kernel_speedup_classic(folder, arch, kernel, variant, pred='false', sym='false', cell=cell)
            ax.plot(np.log10(particle_sizes), np.log2(np.full_like(particle_sizes, int(cell))), speedups, color=colour, marker=marker, linestyle='--')

    plt.savefig(f"{folder}/plot-2-{arch}-{kernel}.pdf")
    plt.savefig(f"{folder}/plot-2-{arch}-{kernel}.png")

    fig_legend = plt.figure(figsize=(14, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=legend_handles, loc="center", ncol=4, frameon=False)
    fig_legend.savefig(f"{folder}/plot-2-{arch}-{kernel}-legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder}/plot-2-{arch}-{kernel}-legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')  # Close all figures
    plt.rcdefaults()  # Reset rcParams to defaults

# Graph3 is a 3d graph that shows normalised speedups for offloaded kernels over vanilla AoS version
# Only generic schemes are tested
def print_graph_3(folder, arch, kernel):
    plt.clf()
    mpl.rcParams.update(params)

    ax = plt.axes(projection='3d')

    ax.set_xlabel("Particles")
    ax.set_ylabel("Cell size")
    ax.set_zlabel('Norm kernel speedup over AoS version')

    ax.xaxis.set_major_formatter(mticker.FuncFormatter(log10_tick_formatter))
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(log2_tick_formatter))

    legend_handles = []
    for counter, variant in enumerate(kernels[kernel][0:2]):
        variant_name = kernel_names[variant]
        marker = f'{Symbols[counter][-1]}'
        colour = Colors[counter]
        legend_handles += [Line2D([0], [0], marker=marker, color=colour, label=f'{variant_name}', linestyle='--')]
        for cell in cells:
            particle_sizes, speedups = read_kernel_speedup_offload(folder, arch, kernel, variant, cell=cell)
            ax.plot(np.log10(particle_sizes), np.log2(np.full_like(particle_sizes, int(cell))), speedups, color=colour, marker=marker, linestyle='--')

    plt.savefig(f"{folder}/plot-3-{arch}-{kernel}.pdf")
    plt.savefig(f"{folder}/plot-3-{arch}-{kernel}.png")

    fig_legend = plt.figure(figsize=(14, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=legend_handles, loc="center", ncol=4, frameon=False)
    fig_legend.savefig(f"{folder}/plot-3-{arch}-{kernel}-legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder}/plot-3-{arch}-{kernel}-legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')  # Close all figures
    plt.rcdefaults()  # Reset rcParams to defaults

# Graph4 is a 3d graph that shows normalised speedups for offloaded kernels over vanilla AoS version
# using shared memory
# Only generic schemes are tested
def print_graph_4(folder, arch, kernel):
    plt.clf()
    mpl.rcParams.update(params)

    ax = plt.axes(projection='3d')

    ax.set_xlabel("Particles")
    ax.set_ylabel("Cell size")
    ax.set_zlabel('Norm kernel speedup over AoS version')

    ax.xaxis.set_major_formatter(mticker.FuncFormatter(log10_tick_formatter))
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(log2_tick_formatter))

    legend_handles = []
    for counter, variant in enumerate(kernels[kernel][0:2]):
        variant_name = kernel_names[variant]
        marker = f'{Symbols[counter][-1]}'
        colour = Colors[counter]
        legend_handles += [Line2D([0], [0], marker=marker, color=colour, label=f'{variant_name}', linestyle='--')]
        for cell in cells:
            particle_sizes, speedups = read_kernel_speedup_offload_shmem(folder, arch, kernel, variant, cell=cell)
            ax.plot(np.log10(particle_sizes), np.log2(np.full_like(particle_sizes, int(cell))), speedups, color=colour, marker=marker, linestyle='--')

    plt.savefig(f"{folder}/plot-4-{arch}-{kernel}.pdf")
    plt.savefig(f"{folder}/plot-4-{arch}-{kernel}.png")

    fig_legend = plt.figure(figsize=(14, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=legend_handles, loc="center", ncol=4, frameon=False)
    fig_legend.savefig(f"{folder}/plot-4-{arch}-{kernel}-legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder}/plot-4-{arch}-{kernel}-legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')  # Close all figures
    plt.rcdefaults()  # Reset rcParams to defaults

def print_graphs(folder):
    for arch in arches:
        for kernel in kernels:
            try:
                print_graph_1(folder, arch, kernel)
            except:
                pass
    for arch in arches:
        for kernel in kernels:
            try:
                print_graph_2(folder, arch, kernel)
            except:
                pass
    for arch in arches:
        for kernel in kernels:
            try:
                print_graph_3(folder, arch, kernel)
            except:
                pass
    for arch in arches:
        for kernel in kernels:
            try:
                print_graph_4(folder, arch, kernel)
            except:
                pass

if __name__== "__main__":
    if len(sys.argv) < 2:
        print( "ERROR: Please pass in folders to work on. ")
    else:
        for folder in sys.argv[1:]:
            print_graphs(folder)
