#!/usr/bin/env python

import numpy as np
from tkinter import Tcl
import glob, os
import re
from matplotlib import gridspec
import matplotlib.pyplot as plt
import matplotlib as mpl
import pandas as pd


"""
Script to read the likwid output data and make plots.
Read likwid cvs files using Pandas.
"""

# Input
folder_input = "/home/cristianbarrahinojosa/Overleaf/paper-llvm2022/experiments/18_sph_simulation/hamilton8/Noh_2D/output_np_400_et_0.01/likwid/"

datatype_inputs = ["MEM", "CACHE"]

for datatype_input in datatype_inputs:
    filename_template = "output_" + datatype_input + "_cores_*_*.cvs"
    filename_template_regex = "output_" + datatype_input + "_cores_(.*)_(.*)"

    cores_array = [1, 2, 4, 8, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64]

    # Initialize template dataframe
    if datatype_input == "MEM":
        data_template_SUM = {
            "Metric SUM": [
                "Runtime (RDTSC) [s] STAT",
                "Runtime unhalted [s] STAT",
                "Clock [MHz] STAT",
                "CPI STAT",
                "Memory bandwidth [MBytes/s] STAT",
                "Memory data volume [GBytes] STAT",
            ],
        }
        data_template_AVG = {
            "Metric AVG": [
                "Runtime (RDTSC) [s] STAT",
                "Runtime unhalted [s] STAT",
                "Clock [MHz] STAT",
                "CPI STAT",
                "Memory bandwidth [MBytes/s] STAT",
                "Memory data volume [GBytes] STAT",
            ],
        }
    elif datatype_input == "CACHE":
        data_template_SUM = {
            "Metric SUM": [
                "CPI STAT",
                "data cache requests STAT",
                "data cache request STAT ",
                "data cache misses STAT",
                "data cache miss rate STAT",
                "data cache miss ratio STAT",
            ],
        }
        data_template_AVG = {
            "Metric AVG": [
                "CPI STAT",
                "data cache requests STAT",
                "data cache request rate STAT",
                "data cache misses STAT",
                "data cache miss rate STAT",
                "data cache miss ratio STAT",
            ],
        }

    # Read cvs data and transform into numpy array
    data_SUM_on = pd.DataFrame(data_template_SUM)
    data_SUM_off = pd.DataFrame(data_template_SUM)
    data_AVG_on = pd.DataFrame(data_template_AVG)
    data_AVG_off = pd.DataFrame(data_template_AVG)

    # The relevant data is at the bottom of the file, so lets skip some rows
    N_metrics = len(data_SUM_on)
    offset_row = -N_metrics

    # List all files matching the template
    path = glob.glob(folder_input + filename_template)
    path = Tcl().call("lsort", "-dict", path)

    assert path != "", "No matching file found in path. Check input data."

    for file in path:
        absfilename = file
        tempname = os.path.basename(absfilename)
        filename = os.path.splitext(tempname)[0]

        # Get cores number via regex
        pf = re.compile(filename_template_regex)

        match = pf.search(filename)
        cores = match.group(1)
        clang = match.group(2)

        # Read cvs file and trim
        df_input = pd.read_csv(file, index_col=0, header=0, on_bad_lines="skip")

        # Column with SUM STAT
        SUM = df_input["Info"].iloc[offset_row:]

        # Column with Avg STAT
        if cores != "1":
            AVG = df_input["Unnamed: 4"].iloc[offset_row:]
        else:
            AVG = SUM  # no difference for 1 core

        if clang == "on":
            data_SUM_on["cores " + str(cores)] = SUM.values
            data_AVG_on["cores " + str(cores)] = AVG.values
        elif clang == "off":
            data_SUM_off["cores " + str(cores)] = SUM.values
            data_AVG_off["cores " + str(cores)] = AVG.values

    # Convert results into python arrays
    SUM_on = np.zeros((N_metrics, len(cores_array)))
    AVG_on = np.zeros((N_metrics, len(cores_array)))

    SUM_off = np.zeros((N_metrics, len(cores_array)))
    AVG_off = np.zeros((N_metrics, len(cores_array)))

    for i, cores in enumerate(cores_array):
        for metric in range(N_metrics):
            SUM_on[metric, i] = float(data_SUM_on["cores " + str(cores)][metric])
            AVG_on[metric, i] = float(data_AVG_on["cores " + str(cores)][metric])

            SUM_off[metric, i] = float(data_SUM_off["cores " + str(cores)][metric])
            AVG_off[metric, i] = float(data_AVG_off["cores " + str(cores)][metric])

    # Store data for plotting
    if datatype_input == "MEM":
        N_dt = 1e2
        runtime_on = AVG_on[0] / N_dt
        runtime_off = AVG_off[0] / N_dt

        clock_speed_on = AVG_on[2]
        clock_speed_off = AVG_off[2]

    elif datatype_input == "CACHE":
        cache_req_on = AVG_on[1] / 1e12
        cache_req_off = AVG_off[1] / 1e12

        cache_miss_ratio_on = AVG_on[5]
        cache_miss_ratio_off = AVG_off[5]

    print(f"Done Reading cvs data for datatype: {datatype_input}.")


# Plot Setup
font = {"size": 12}
plt.rc("font", **font)
plt.rc("font", family="serif")
plt.rc("text", usetex=False)

fig = plt.figure(figsize=(14, 4))
gs = gridspec.GridSpec(1, 3, wspace=0.2)

ax0 = plt.subplot(gs[0])

# Runtime plot
plt.scatter(
    cores_array,
    runtime_on,
    marker="o",
    edgecolors="r",
    facecolors="r",
    s=25,
    zorder=10,
    label=r"on",
)
plt.plot(cores_array, runtime_on, c="r", lw=1.2)
plt.scatter(cores_array, runtime_off, marker="^", c="b", s=25, zorder=10, label=r"off")
plt.plot(cores_array, runtime_off, c="b", lw=1.2, zorder=11)

plt.xlabel(r"Cores")
plt.ylabel(r"Time per time step [s]")
plt.xscale("log", base=2)
# plt.yscale( "log", base=2 )
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.tick_params(direction="in", top=True, right=True)
plt.gca().xaxis.set_major_formatter(mpl.ticker.ScalarFormatter())
plt.gca().yaxis.set_major_formatter(mpl.ticker.ScalarFormatter())
plt.xticks([1, 2, 4, 8, 16, 32, 64])
# plt.yticks([1,2,4,8,16,32,64])

leg = plt.legend(loc="upper right", fontsize=10)

# Cache requests
ax1 = plt.subplot(gs[1])
plt.scatter(
    cores_array,
    cache_req_on,
    marker="o",
    edgecolors="r",
    facecolors="r",
    s=25,
    zorder=10,
    label=r"on",
)
plt.plot(cores_array, cache_req_on, c="r", lw=1.2, zorder=-1)
plt.scatter(
    cores_array, cache_req_off, marker="^", c="b", s=25, zorder=10, label=r"off"
)
plt.plot(cores_array, cache_req_off, c="b", lw=1.2, zorder=11)
plt.xlabel(r"Cores")
plt.ylabel(r"Average cache requests [$\times10^{12}$]")
plt.margins(0.05, 0.1)
plt.xscale("log", base=2)
# plt.yscale( "log", base=10)
plt.gca().xaxis.set_major_formatter(mpl.ticker.ScalarFormatter())
# plt.gca().yaxis.set_major_formatter(mpl.ticker.ScalarFormatter())
plt.xticks([1, 2, 4, 8, 16, 32, 64])
plt.tick_params(direction="in", top=True, right=True)

# Cache miss rate
ax2 = plt.subplot(gs[2])
plt.scatter(
    cores_array,
    100 * cache_miss_ratio_on,
    marker="o",
    edgecolors="r",
    facecolors="r",
    s=25,
    zorder=10,
    label=r"on",
)
plt.plot(cores_array, 100 * cache_miss_ratio_on, c="r", lw=1.2)
plt.scatter(
    cores_array,
    100 * cache_miss_ratio_off,
    marker="^",
    c="b",
    s=25,
    zorder=10,
    label=r"off",
)
plt.plot(cores_array, 100 * cache_miss_ratio_off, c="b", lw=1.2)
plt.xlabel(r"Cores")
plt.ylabel(r"Average cache miss rate [%]")
plt.margins(0.05, 0.1)
plt.xscale("log", base=2)
plt.gca().xaxis.set_major_formatter(mpl.ticker.ScalarFormatter())
plt.xticks([1, 2, 4, 8, 16, 32, 64])
plt.tick_params(direction="in", top=True, right=True)

plt.show()
fig.savefig("plot_likwid_np_400" + ".pdf", bbox_inches="tight")
plt.close()
