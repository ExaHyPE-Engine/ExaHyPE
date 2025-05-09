#!/usr/bin/env python3

# ----------------------------------------------------------------------------
# Makes a 3D scatterplot of the results.
#
# Usage: python3 plotSolution.py <snapshot-time>
#
# The script will look for the snapshot output with simulation time closest to
# the provided <snapshot-time> in `particles.pvd` and create a plot of that
# time.
# ----------------------------------------------------------------------------


import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import numpy as np
import sys

from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader

params = {
    "xtick.labelsize": 12,
    "ytick.labelsize": 12,
    "xtick.direction": "in",
    "ytick.direction": "in",
    "xtick.top": True,
    "ytick.right": True,
    "figure.subplot.left": 0.045,
    "figure.subplot.right": 0.99,
    "figure.subplot.bottom": 0.05,
    "figure.subplot.top": 0.93,
    "figure.subplot.wspace": 0.15,
    "text.usetex": True,
}
mpl.rcParams.update(params)


if len(sys.argv) < 2:
    time = 0.6
else:
    time = float(sys.argv[1])

pvdfile = "particles.pvd"
reader = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=time)
partData = reader.load()
time = reader.get_snapshot_time()


# show me what particle fields are stored
#  partData.show_attribute_list()

x = partData.x[:, 0]
y = partData.x[:, 1]
z = partData.x[:, 2]
vx = partData.v[:, 0]
vy = partData.v[:, 1]
vz = partData.v[:, 2]
u = partData.u
#  S = partData.S # doesn't exist yet?
P = partData.pressure
rho = partData.density



def plot(quantity):
    """
    Plot the specified quantity. Options:
    - "rho" (density)
    - "p" (pressure)
    - "u" (internal energy)
    """

    if quantity == "rho":

        #  minval = 0.
        #  maxval = 65.
        title = "Density "
        qarr = rho

    elif quantity == "p" or quantity == "P":
        #  minval = 0.
        #  maxval = 46. # regular grid IC
        #  maxval = 68. # noisy IC
        title = "Pressure "
        qarr = P

    elif quantity == "u":
        #  minval = 0.
        #  maxval = 4.  # regular grid IC
        #  maxval = 6.1 # noisy IC
        title = "Internal Energy "
        qarr = u

    minval = None
    maxval = None
    alpha = 0.2 + 0.8 * qarr / (maxval - minval)
    title += f"$t=${time:.2f}"



    fig = plt.figure(figsize=(6, 6), dpi=300)
    ax1 = fig.add_subplot(111, projection="3d", aspect="equal")
    ax1.set_title(title, fontsize=16)


    scatterkwargs = {
            "s": 4,
            "lw": 0,
            "marker": "o",
            "alpha": 0.6,
            "depthshade": True,
            #  "depthshade": False
            "vmin": minval,
            "vmax": maxval,
            }

    sc = ax1.scatter(
            x, y, z, c=qarr,
            #  s = 4. + 4. * alpha/alpha.max(),
            #  alpha = alpha,
            cmap = "hot",
            **scatterkwargs
    )
    #  ax1.view_init(elev=26, azim=81)
    #  ax1.view_init(elev=25, azim=13)
    ax1.view_init(elev=30, azim=45)
    #  ax1.view_init(elev=20, azim=60)
    fig.colorbar(sc, shrink=0.8, aspect=40, pad=0.0)

    ax1.set_xlim(0, 2)
    ax1.set_ylim(0, 2)
    ax1.set_zlim(0, 2)


    plt.tight_layout()
    fname = quantity + "-" + f"_{time}.png"
    plt.savefig(fname)
    print("saved", fname)
    plt.close()


plot("rho")
