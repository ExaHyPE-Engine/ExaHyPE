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

CELL_SIZES = ['0064', '0128', '0256', '0512', '1024']

def read_data(folder_name,
              kernel_name,
              ):
    """!

    returns {
        'cell size' : [ (xdata, ydata) for aos, (xdata, ydata) for soa ],
    }

    """

    results = {}

    for CELL_SIZE in CELL_SIZES:
        result = []
        for SOA in [False, True]:
            particle_counts = []
            timings         = []

            file_name = f"{folder_name}/out-{'x64' if 'gi001' in folder_name else 'aarch64'}-soa{'true' if SOA else 'false'}-cell{CELL_SIZE}.log"
            file = open( file_name, "r" )

            for line in file:
                if "Experiment:" in line:
                    number_of_particles = line.split( "Experiment:" )[1].split( "particles" )[0]
                    particle_counts.append( float(number_of_particles) )
                if kernel_name in line:
                    time = line.split( kernel_name + ":" )[1].split( "(" )[0]
                    timings.append( float(time) )

            assert len(particle_counts)==len(timings) or len(timings)==0, "array dimensions don't match: {} vs {} for kernel {} in {}".format(particle_counts, timings, kernel_name, file_name)
            result += [(particle_counts, timings)]
        results[CELL_SIZE] = result
    return results

def read_data_gpu(folder_name,
              kernel_name,
              ):
    """!

    returns {
        'cell size' : [ (xdata, ydata) for aos - always empty, (xdata, ydata) for soa ],
    }

    """

    results = {}

    for CELL_SIZE in CELL_SIZES:
        result = [[]]
        particle_counts = []
        timings         = []

        file_name = f"{folder_name}/out-{'gh' if 'gn003' in folder_name else 'mi300a'}-soatrue-cell{CELL_SIZE}.log"
        file = open( file_name, "r" )

        for line in file:
            if "Experiment:" in line:
                number_of_particles = line.split( "Experiment:" )[1].split( "particles" )[0]
                particle_counts.append( float(number_of_particles) )
            if kernel_name in line:
                time = line.split( kernel_name + ":" )[1].split( "(" )[0]
                timings.append( float(time) )

        assert len(particle_counts)==len(timings), "array dimensions don't match: {} vs {} for kernel {} in {}".format(particle_counts, timings, kernel_name, file_name)
        result += [(particle_counts, timings)]
        results[CELL_SIZE] = result
    return results

def read_data_gpu_shmem(folder_name,
                  kernel_name,
                  ):
    """!

    returns {
        'cell size' : [ (xdata, ydata) for aos - always empty, (xdata, ydata) for soa ],
    }

    """

    results = {}

    for CELL_SIZE in CELL_SIZES:
        result = [[]]
        particle_counts = []
        timings         = []

        file_name = f"{folder_name}/out-{'gh' if 'gn003' in folder_name else 'mi300a'}-soatrue-cell{CELL_SIZE}-shmem.log"
        file = open( file_name, "r" )

        for line in file:
            if "Experiment:" in line:
                number_of_particles = line.split( "Experiment:" )[1].split( "particles" )[0]
                particle_counts.append( float(number_of_particles) )
            if kernel_name in line:
                time = line.split( kernel_name + ":" )[1].split( "(" )[0]
                timings.append( float(time) )

        assert len(particle_counts)==len(timings), "array dimensions don't match: {} vs {} for kernel {} in {}".format(particle_counts, timings, kernel_name, file_name)
        result += [(particle_counts, timings)]
        results[CELL_SIZE] = result
    return results


Kernels  = [ "density", "force", "kick1", "kick2", "drift" ]

KernelName = {
    'density' : 'density',
    'force' : 'force',
    'kick1' : '1st kick',
    'kick2' : '2nd kick',
    'drift' : 'drift',
}

ImplementationVariant = {
    'force': ['(scattered,generic,dist-check)',
              '(continuous,generic,dist-check)',
              '(continuous,local-active-coalesced,dist-check)',
              '(continuous,active-local-coalesced,dist-check)',
              '(continuous,local-active-preamble,dist-check)',
              '(continuous,active-local-preamble,dist-check)',
              '(scattered,generic,no-dist-check)',
              '(continuous,generic,no-dist-check)',
              '(continuous,local-active-coalesced,no-dist-check)',
              '(continuous,active-local-coalesced,no-dist-check)'],
    'density': ['(scattered,generic,dist-check)',
                '(continuous,generic,dist-check)',
                '(continuous,local-active-coalesced,dist-check)',
                '(continuous,active-local-coalesced,dist-check)',
                '(continuous,local-active-preamble,dist-check)',
                '(continuous,active-local-preamble,dist-check)',
                '(scattered,generic,no-dist-check)',
                '(continuous,generic,no-dist-check)',
                '(continuous,local-active-coalesced,no-dist-check)',
                '(continuous,active-local-coalesced,no-dist-check)'],
    'kick1': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '(continuous,preamble)'],
    'kick2': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '(continuous,preamble)'],
    'drift': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '(continuous,preamble)'],
}

ImplementationVariant_FixedPPC = {
    'force': ['(scattered,generic,dist-check)',
              '(continuous,generic,dist-check)',
              '(continuous,local-active-coalesced,dist-check)',
              '(continuous,active-local-coalesced,dist-check)',
              '#', # (continuous,local-active-preamble,dist-check) skipped, placeholder to keep colors etc consistent
              '#', # (continuous,active-local-preamble,dist-check) skipped, placeholder to keep colors etc consistent
              '(scattered,generic,no-dist-check)',
              '(continuous,generic,no-dist-check)',
              '(continuous,local-active-coalesced,no-dist-check)',
              '(continuous,active-local-coalesced,no-dist-check)'],
    'density': ['(scattered,generic,dist-check)',
                '(continuous,generic,dist-check)',
                '(continuous,local-active-coalesced,dist-check)',
                '(continuous,active-local-coalesced,dist-check)',
                '#', # (continuous,local-active-preamble,dist-check) skipped, placeholder to keep colors etc consistent
                '#', # (continuous,active-local-preamble,dist-check) skipped, placeholder to keep colors etc consistent
                '(scattered,generic,no-dist-check)',
                '(continuous,generic,no-dist-check)',
                '(continuous,local-active-coalesced,no-dist-check)',
                '(continuous,active-local-coalesced,no-dist-check)'],
    'kick1': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
    'kick2': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
    'drift': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
}

ImplementationVariant_GPU = {
    'force': ['(scattered,generic,dist-check)',
              '(continuous,generic,dist-check)',
              '(continuous,local-active-coalesced,dist-check)',
              '(continuous,active-local-coalesced,dist-check)',
              '#', # (continuous,local-active-preamble,dist-check) skipped, placeholder to keep colors etc consistent
              '#', # (continuous,active-local-preamble,dist-check) skipped, placeholder to keep colors etc consistent
              '(scattered,generic,no-dist-check)',
              '(continuous,generic,no-dist-check)',
              '(continuous,local-active-coalesced,no-dist-check)',
              '(continuous,active-local-coalesced,no-dist-check)'],
    'density': ['(scattered,generic,dist-check)',
                '(continuous,generic,dist-check)',
                '(continuous,local-active-coalesced,dist-check)',
                '(continuous,active-local-coalesced,dist-check)',
                '#', # (continuous,local-active-preamble,dist-check) skipped, placeholder to keep colors etc consistent
                '#', # (continuous,active-local-preamble,dist-check) skipped, placeholder to keep colors etc consistent
                '(scattered,generic,no-dist-check)',
                '(continuous,generic,no-dist-check)',
                '(continuous,local-active-coalesced,no-dist-check)',
                '(continuous,active-local-coalesced,no-dist-check)'],
    'kick1': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
    'kick2': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
    'drift': ['(scattered,generic)',
              '(continuous,generic)',
              '(continuous,coalesced)',
              '#' # (continuous,preamble) skipped, placeholder ...
              ],
}


ImplementationVariantName = {
    '(scattered,generic)': 'generic (scat.)',
    '(continuous,generic)': 'generic (cont.)',
    '(continuous,coalesced)': 'coalesced (cont.)',
    '(continuous,preamble)': 'coalesced (prea.)',
    '(scattered,generic,dist-check)': 'generic (scat. with branching)',
    '(continuous,generic,dist-check)': 'generic (cont. with branching)',
    '(continuous,local-active-coalesced,dist-check)': 'coalesced local-active (cont. with branching)',
    '(continuous,active-local-coalesced,dist-check)': 'coalesced active-local (cont. with branching)',
    '(continuous,active-local-preamble,dist-check)': 'coalesced active-local (cont. with branching and preamble)',
    '(continuous,local-active-preamble,dist-check)': 'coalesced local-active (cont. with branching and preamble)',
    '(scattered,generic,no-dist-check)': 'generic (scat. with masking)',
    '(continuous,generic,no-dist-check)': 'generic (cont. with masking)',
    '(continuous,local-active-coalesced,no-dist-check)': 'coalesced local-active (cont. with masking)',
    '(continuous,active-local-coalesced,no-dist-check)': 'coalesced active-local (cont. with masking)',
}

Colors   = [ "#20a040", "#a04020", "#4020a0", "#f0a0f0", "#f06030", "#6030f0", "#2080a0", "#a0a020", "#a020a0", "#20a080", "#a08020" ]
Symbols  = [ "s", "o", "^", "v", "<", ">", "1", "2", "3", "4", "p" ]
LineStyles = { 'aos': '--', 'soa': ':'}

ParticleSize = 272

L2CacheSizeMap = {
    'gi001': 1024*1024*2*56,
    'gn003': 1024*1024*72,
}

def process_folder(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for im, implementation in enumerate(ImplementationVariant[kernel]):
            if '#' in implementation: continue

            impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []
            for cs, CELL_SIZE in enumerate(CELL_SIZES):
                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[cs], color=Colors[cs], label=f'{int(CELL_SIZE)}', linestyle='-')]
                cell_size_legend_labels += [f'{int(CELL_SIZE)}']

                impl_data_aos = impl_data_all[CELL_SIZE][0]
                plt.plot( impl_data_aos[0],
                          impl_data_aos[1],
                          Symbols[cs],
                          color=Colors[cs],
                          # label=label,
                          linestyle=LineStyles['aos']
                          )

                impl_data_soa = impl_data_all[CELL_SIZE][1]
                plt.plot( impl_data_soa[0],
                          impl_data_soa[1],
                          Symbols[cs],
                          color=Colors[cs],
                          # label=label,
                          linestyle=LineStyles['soa']
                          )

            plt.xlabel("Particles")
            plt.ylabel("Time per particle update [t]=s")

            plt.xscale('log', base=10)
            plt.yscale('log', base=10)

            if 'density' in kernel: plt.ylim(2e-7, 2e-5)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}_{implementation}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

    fig_legend = plt.figure(figsize=(10, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=cell_size_legend_handles, labels=cell_size_legend_labels, loc="center", ncol=5, frameon=False)
    fig_legend.savefig(f"{folder_name}/cell_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/cell_legend.png", bbox_inches='tight', dpi=300)

    fig_legend = plt.figure(figsize=(4, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=[
        Line2D([0], [0], color='black', label='AoS', linestyle=LineStyles['aos']),
        Line2D([0], [0], color='black', label='SoA', linestyle=LineStyles['soa'])
    ], labels=['AoS', 'SoA'], loc="center", ncol=2, frameon=False)
    fig_legend.savefig(f"{folder_name}/layout_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/layout_legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')
    plt.close()
    plt.rcdefaults()

def process_folder_gpu(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for im, implementation in enumerate(ImplementationVariant_GPU[kernel]):
            if '#' in implementation: continue

            impl_data_all = read_data_gpu(folder_name, f"{kernel}-gpu {implementation}")

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []
            for cs, CELL_SIZE in enumerate(CELL_SIZES):
                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[cs], color=Colors[cs], label=f'{int(CELL_SIZE)}', linestyle='-')]
                cell_size_legend_labels += [f'{int(CELL_SIZE)}']

                # impl_data_aos = impl_data_all[CELL_SIZE][0]
                # plt.plot( impl_data_aos[0],
                #           impl_data_aos[1],
                #           Symbols[cs],
                #           color=Colors[cs],
                #           # label=label,
                #           linestyle=LineStyles['aos']
                #           )

                impl_data_soa = impl_data_all[CELL_SIZE][1]
                plt.plot( impl_data_soa[0],
                          impl_data_soa[1],
                          Symbols[cs],
                          color=Colors[cs],
                          # label=label,
                          linestyle=LineStyles['soa']
                          )

            plt.xlabel("Particles")
            plt.ylabel("Time per particle update [t]=s")

            plt.xscale('log', base=10)
            plt.yscale('log', base=10)

            if 'density' in kernel: plt.ylim(2e-7, 2e-5)
            if 'drift' in kernel: plt.ylim(7e-8, 4e-6)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}_gpu_{implementation}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            elif 'mi300a' in filename:
                pass
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

    fig_legend = plt.figure(figsize=(10, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=cell_size_legend_handles, labels=cell_size_legend_labels, loc="center", ncol=5, frameon=False)
    fig_legend.savefig(f"{folder_name}/cell_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/cell_legend.png", bbox_inches='tight', dpi=300)

    fig_legend = plt.figure(figsize=(4, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=[
        Line2D([0], [0], color='black', label='AoS', linestyle=LineStyles['aos']),
        Line2D([0], [0], color='black', label='SoA', linestyle=LineStyles['soa'])
    ], labels=['AoS', 'SoA'], loc="center", ncol=2, frameon=False)
    fig_legend.savefig(f"{folder_name}/layout_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/layout_legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')
    plt.close()
    plt.rcdefaults()

def process_folder_gpu_shmem(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for im, implementation in enumerate(ImplementationVariant_GPU[kernel]):
            if '#' in implementation: continue

            impl_data_all = read_data_gpu_shmem(folder_name, f"{kernel}-gpu {implementation}")

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []
            for cs, CELL_SIZE in enumerate(CELL_SIZES):
                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[cs], color=Colors[cs], label=f'{int(CELL_SIZE)}', linestyle='-')]
                cell_size_legend_labels += [f'{int(CELL_SIZE)}']

                # impl_data_aos = impl_data_all[CELL_SIZE][0]
                # plt.plot( impl_data_aos[0],
                #           impl_data_aos[1],
                #           Symbols[cs],
                #           color=Colors[cs],
                #           # label=label,
                #           linestyle=LineStyles['aos']
                #           )

                impl_data_soa = impl_data_all[CELL_SIZE][1]
                plt.plot( impl_data_soa[0],
                          impl_data_soa[1],
                          Symbols[cs],
                          color=Colors[cs],
                          # label=label,
                          linestyle=LineStyles['soa']
                          )

            plt.xlabel("Particles")
            plt.ylabel("Time per particle update [t]=s")

            plt.xscale('log', base=10)
            plt.yscale('log', base=10)

            if 'density' in kernel: plt.ylim(2e-7, 2e-5)
            if 'drift' in kernel: plt.ylim(7e-8, 4e-6)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}_gpu_{implementation}_shmem".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            elif 'mi300a' in filename:
                pass
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

    fig_legend = plt.figure(figsize=(10, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=cell_size_legend_handles, labels=cell_size_legend_labels, loc="center", ncol=5, frameon=False)
    fig_legend.savefig(f"{folder_name}/cell_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/cell_legend.png", bbox_inches='tight', dpi=300)

    fig_legend = plt.figure(figsize=(4, 1))
    ax_legend = fig_legend.add_subplot(111)
    ax_legend.axis("off")
    ax_legend.legend(handles=[
        Line2D([0], [0], color='black', label='AoS', linestyle=LineStyles['aos']),
        Line2D([0], [0], color='black', label='SoA', linestyle=LineStyles['soa'])
    ], labels=['AoS', 'SoA'], loc="center", ncol=2, frameon=False)
    fig_legend.savefig(f"{folder_name}/layout_legend.pdf", bbox_inches='tight', dpi=300)
    fig_legend.savefig(f"{folder_name}/layout_legend.png", bbox_inches='tight', dpi=300)

    plt.close('all')
    plt.close()
    plt.rcdefaults()


def process_folder_speedup(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for im, implementation in enumerate(ImplementationVariant[kernel]):
            if '#' in implementation: continue

            impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []
            for cs, CELL_SIZE in enumerate(CELL_SIZES):
                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[cs], color=Colors[cs], label=f'{int(CELL_SIZE)}', linestyle='-')]
                cell_size_legend_labels += [f'{int(CELL_SIZE)}']

                impl_data_aos = impl_data_all[CELL_SIZE][0]

                impl_data_soa = impl_data_all[CELL_SIZE][1]

                x_data = impl_data_aos[0]
                y_data = np.array(impl_data_aos[1]) / np.array(impl_data_soa[1])

                plt.ylim(0.4, 3.4)
                plt.plot( x_data,
                          y_data,
                          Symbols[cs],
                          color=Colors[cs],
                          # label=label,
                          linestyle='-'
                          )

            plt.xlabel("Particles")
            plt.ylabel("Relative speedup")

            plt.xscale('log', base=10)
            if 'density' in kernel or 'drift' in kernel: plt.ylim(0.1, 3)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}_{implementation}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}-relative"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.axhline(y = 1, color = 'k', linestyle = '--')

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

    plt.close('all')
    plt.close()
    plt.rcdefaults()

def process_folder_fixed_ppc(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for cs, CELL_SIZE in enumerate(CELL_SIZES):
            cell_size_legend_handles = []
            cell_size_legend_labels = []

            plt.clf()
            mpl.rcParams.update(params)

            for im, implementation in enumerate(ImplementationVariant_FixedPPC[kernel]):
                if '#' in implementation: continue

                impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[im], color=Colors[im], linestyle=LineStyles['aos'])]
                cell_size_legend_labels += [f'{ImplementationVariantName[implementation]}']

                impl_data_aos = impl_data_all[CELL_SIZE][0]

                x_data = impl_data_aos[0]
                y_data = impl_data_aos[1]

                plt.plot( x_data,
                          y_data,
                          Symbols[im],
                          color=Colors[im],
                          # label=label,
                          linestyle=LineStyles['aos']
                          )

            plt.xlabel("Particles")
            plt.ylabel("Time per particle update [t]=s")

            plt.xscale('log', base=10)

            if 'drift' in kernel:
                plt.ylim(5e-10, 5e-7)
                plt.yscale('log', base=10)

            if 'density' in kernel and int(CELL_SIZE) == 64: plt.ylim(0, 1.3e-6)
            if 'density' in kernel and int(CELL_SIZE) == 1024: plt.ylim(0, 2e-5)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}_cell{CELL_SIZE}_aos"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []

            for im, implementation in enumerate(ImplementationVariant_FixedPPC[kernel]):
                if '#' in implementation: continue

                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[im], color=Colors[im], linestyle=LineStyles['soa'])]
                cell_size_legend_labels += [f'{ImplementationVariantName[implementation]}']

                impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

                impl_data_soa = impl_data_all[CELL_SIZE][1]

                x_data = impl_data_soa[0]
                y_data = impl_data_soa[1]

                plt.plot( x_data,
                          y_data,
                          Symbols[im],
                          color=Colors[im],
                          # label=label,
                          linestyle=LineStyles['soa']
                          )

            plt.xlabel("Particles")
            plt.ylabel("Time per particle update [t]=s")

            plt.xscale('log', base=10)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}_cell{CELL_SIZE}_soa"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

        fig_legend = plt.figure(figsize=(10, 2))
        ax_legend = fig_legend.add_subplot(111)
        ax_legend.axis("off")
        ax_legend.legend(handles=cell_size_legend_handles, labels=cell_size_legend_labels, loc="center", ncol=2, frameon=False)
        fig_legend.savefig(f"{folder_name}/{kernel}_variants_legend.pdf", bbox_inches='tight', dpi=300)
        fig_legend.savefig(f"{folder_name}/{kernel}_variants_legend.png", bbox_inches='tight', dpi=300)

        plt.close('all')
        plt.close()
        plt.rcdefaults()

    plt.close('all')
    plt.close()
    plt.rcdefaults()


def process_folder_fixed_ppc_relative(folder_name):
    print( f"process folder {folder_name}" )

    for k, kernel in enumerate(Kernels):
        for cs, CELL_SIZE in enumerate(CELL_SIZES):
            cell_size_legend_handles = []
            cell_size_legend_labels = []

            plt.clf()
            mpl.rcParams.update(params)

            impl_data_all_baseline = read_data(folder_name, f"{kernel} {ImplementationVariant[kernel][0]}")

            for im, implementation in enumerate(ImplementationVariant[kernel]):
                if '#' in implementation: continue

                impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[im], color=Colors[im], linestyle=LineStyles['aos'])]
                cell_size_legend_labels += [f'{implementation}']

                impl_data_aos_baseline = impl_data_all_baseline[CELL_SIZE][0]
                impl_data_aos = impl_data_all[CELL_SIZE][0]

                x_data = impl_data_aos_baseline[0]
                y_data = np.array(impl_data_aos_baseline[1]) / np.array(impl_data_aos[1])

                plt.ylim(0.4, 3.4)
                plt.plot( x_data,
                          y_data,
                          Symbols[im],
                          color=Colors[im],
                          # label=label,
                          linestyle=LineStyles['aos']
                          )

            plt.xlabel("Particles")
            plt.ylabel(f"Relative performance")

            plt.xscale('log', base=10)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}_cell{CELL_SIZE}_aos_relative"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.axhline(y = 1, color = 'k', linestyle = '--')

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

            plt.clf()
            mpl.rcParams.update(params)

            cell_size_legend_handles = []
            cell_size_legend_labels = []

            for im, implementation in enumerate(ImplementationVariant[kernel]):
                if '#' in implementation: continue

                cell_size_legend_handles += [Line2D([0], [0], marker=Symbols[im], color=Colors[im], linestyle=LineStyles['soa'])]
                cell_size_legend_labels += [f'{implementation}']

                impl_data_all = read_data(folder_name, f"{kernel} {implementation}")

                impl_data_soa_baseline = impl_data_all_baseline[CELL_SIZE][1]
                impl_data_soa = impl_data_all[CELL_SIZE][1]

                x_data = impl_data_soa_baseline[0]
                y_data = np.array(impl_data_soa_baseline[1]) / np.array(impl_data_soa[1])

                plt.ylim(0.4, 3.4)
                plt.plot( x_data,
                          y_data,
                          Symbols[im],
                          color=Colors[im],
                          # label=label,
                          linestyle=LineStyles['soa']
                          )

            plt.xlabel("Particles")
            plt.ylabel(f"Relative performance")

            plt.xscale('log', base=10)

            plt.minorticks_on()
            plt.grid(True, which='both')

            filename = f"/{kernel}".replace('(', '').replace(')', '').replace(',', '_').replace('-', '_')
            filename = f"{folder_name}/{filename}_cell{CELL_SIZE}_soa_relative"

            if 'gi001' in filename:
                plt.axvline(x = L2CacheSizeMap['gi001']/ParticleSize, color = 'k', linestyle = '--')
            elif 'gn003' in filename:
                plt.axvline(x = L2CacheSizeMap['gn003']/ParticleSize, color = 'k', linestyle = '--')
            else:
                raise Exception("I don't know what CPU I'm working on!")

            plt.axhline(y = 1, color = 'k', linestyle = '--')

            plt.savefig(f"{filename}.pdf")
            plt.savefig(f"{filename}.png")

            plt.close('all')
            plt.rcdefaults()

    plt.close('all')
    plt.close()
    plt.rcdefaults()

if __name__== "__main__":
    if len(sys.argv) < 2:
        print( "ERROR: Please pass in file to work on. ")
    else:
        for folder in sys.argv[1:]:
            process_folder(folder)
            process_folder_speedup(folder)
            process_folder_fixed_ppc(folder)
            process_folder_fixed_ppc_relative(folder)
            try:
                process_folder_gpu(folder)
                process_folder_gpu_shmem(folder)
            except:
                pass
