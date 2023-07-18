#!/usr/bin/env python3

"""!
Compare smoothing lengths that Peano yields and what you expect them to be.
"""

import h5py
from scipy.spatial import KDTree
import numpy as np
import argparse
import sys
import peano4
from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader



kernel = "quartic_spline"
#  KERNEL_GAMMA = 1.936492
comparison_tolerance = 1e-4
h_tolerance = 1e-12
eta =  1.5492085688967474



from sph_kernels import sph_kernel_W, sph_kernel_dWdr, sph_kernel_get_H

def neighbour_loop(r: np.ndarray, h:float, kernel: str, ndim:int):
    """!
    TODO: docu
    """
    wcount = 0.
    wcount_dh = 0.
    H = sph_kernel_get_H(h, kernel)

    # Collect neighbour values
    for n in range(r.shape[0]):
        q = r[n] / H
        w =  sph_kernel_W(r[n], h, kernel, ndim)
        dwdr =  sph_kernel_dWdr(r[n], h, kernel, ndim)
        wcount += w
        wcount_dh -= (ndim * w + q * dwdr)

    #  pi->density.wcount_dh -= (hydro_dimension * wi + ui * wi_dx);

    # Add self contribution
    w = sph_kernel_W(0., h, kernel, ndim)
    dwdr =  sph_kernel_dWdr(0., h, kernel, ndim)
    wcount += w
    wcount_dh -= dwdr

    return wcount, wcount_dh





def find_smoothing_length(xp: np.ndarray, xn: np.ndarray, kernel: str, eta: float = 1.2348, h_tolerance: float = 1e-4, ndim=3):
    """!
    Determine the smoothing length of a particle with coordinates xp

    TODO: finish documentation

    Parameters
    ----------


    Returns
    -------

    h: float
        the smoothing length
    """


    # get distances between particles
    dx = xn - xp
    r2 = dx[:,0]**2 + dx[:,1]**2 + dx[:,2]**2
    r = np.sqrt(r2)

    # Initial guess
    h = 0.5 * r.max()

    iteration = 0
    n_target = eta ** ndim

    while iteration < 100:
        iteration += 1

        h_dim = h**ndim
        h_dim_minus_one = h**(ndim-1.)
        wcount, wcount_dh = neighbour_loop(r, h, kernel, ndim)

        n_sum = wcount * h_dim
        f = n_sum - n_target
        f_prime = wcount_dh * h_dim + ndim * wcount * h_dim_minus_one

        h_new = h - f / f_prime

        diff = 1. - h_new / h

        if abs(diff < h_tolerance):
            print("Finished! iter=", iteration)
            return h_new
            break
        else:
            print("iter = {0:4d} diff = {1:.4e}".format(iteration, diff))
            h = h_new

    print("DIDN'T CONVERGE.")

    return h




parser = argparse.ArgumentParser(description="Compare Smoothing Length Results With Expectations")
parser.add_argument(
    "-ic",
    "--ic-file",
    dest="ic_filename",
    default="test_sml_1D.hdf5",
    help="Initial conditions hdf5 file",
)
parser.add_argument(
    "-n",
    "--neighbours",
    dest="nneigh",
    type=int,
    default=5,
    help="Number of neighbours to search for",
)
args = parser.parse_args()

icfile = args.ic_filename
nneigh = args.nneigh


# Get IC data
# -------------------------

hfile = h5py.File(icfile, "r")
gas = hfile["PartType0"]
coords = gas["Coordinates"][:]
ids = gas["ParticleIDs"][:]
hfile.close()


# Make problem 1D
#  coords = coords[:,0]
# KDTree needs at least 2dim data
#  coords = np.reshape(coords, (coords.shape[0], 1))

# Sort by particle ID
sort_ic = np.argsort(ids)
ids = ids[sort_ic]
coords = coords[sort_ic]


# Get expected results
# -------------------------

# TODO: to enable periodic wrapping, specify boxsize in KDTree call
tree = KDTree(coords)
# add + 1: KDTree will find particle itself as neighbour too
# TODO: adapt the +10
distances, indexes = tree.query(coords, k=nneigh + 10)
neighbours = coords[indexes]

npart = coords.shape[0]
sml_python = np.zeros((npart))

for i in range(npart):
    # the KDTree returns the particle itself as a neighbour too.
    # it is stored at the first index, with distance 0.
    xp = neighbours[i,0,:]
    xn = neighbours[i,1:,:]
    h = find_smoothing_length(xp, xn, kernel = kernel, eta=eta, h_tolerance=h_tolerance, ndim = 1)
    sml_python[i] = h





# Read in peano output
#-------------------------------

reader = ParticleVTUReader(vtufile="particles-0.vtu", verbose=False)
partData = reader.load()
sml_peano = partData.smoothingLength
partIDs_peano = partData.partid

# sort particles by ID
sort_peano = np.argsort(partIDs_peano)
sml_peano = sml_peano[sort_peano]
partIDs_peano = partIDs_peano[sort_peano]



# Compare resutls
# -----------------------------

npart = sml_peano.shape[0]

errors = 0
errorstr = "Found difference: "+"particle ID = {0:4d} "+"H_peano = {1:.5e} "+"H_python = {2:.5e} "+"diff = {3:.5e}"

for i in range(npart):

    diff = 1. - sml_peano[i]/sml_python[i]

    if abs(diff) > comparison_tolerance:
        errors += 1
        print(errorstr.format(int(partIDs_peano[i]), sml_peano[i], sml_python[i], diff))


print(f"\nSummary run with {nneigh} neighbours:")
print(f"Finished with {errors} errors.")
if errors > 0:
    print("Before you panic: Are you using the correct KERNEL_GAMMA in this script?")
    print("                  They vary from kernel to kernel, and for different dimensions")
    print("Other ToDo's left for this script: ")
    print(" - corrections for periodic wrapping ")



outputfile = "sml_peano.txt"
f = open(outputfile, "w")
for i in range(sml_peano.shape[0]):
    f.write("{0:8d},{1:12.6f}\n".format(ids[i], sml_peano[i]))
f.close()

sys.exit(errors)


