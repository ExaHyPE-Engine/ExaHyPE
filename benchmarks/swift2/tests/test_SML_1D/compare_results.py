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
import swift2.sphtools


comparison_tolerance = 1e-4

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
parser.add_argument(
    "-ht",
    "--h-tolerance",
    dest="h_tol",
    type=float,
    default=1.e-6,
    help="Threshold for convergence of smoothing length",
)
parser.add_argument(
    "-k",
    "--kernel",
    dest="kernel",
    type=str,
    default="quartic_spline",
    help="SPH kernel function to use.",
    choices=swift2.sphtools.sph_kernel_list,
)

args = parser.parse_args()
icfile = args.ic_filename
nneigh = args.nneigh
h_tolerance = args.h_tol
kernel = args.kernel
eta = swift2.sphtools.eta_from_number_of_neighbours(nneigh, kernel=kernel, ndim=1)


print(" Running comparison")
print(" --- IC file:                    ", icfile)
print(" --- SPH Kernel:                 ", kernel)
print(" --- Target number of neighbours:", nneigh)
print(" --- Target eta:                 ", eta)
print(" --- h tolerance:                ", h_tolerance)
print(" --- comparison tolerance:       ", comparison_tolerance)

# Get IC data
# -------------------------

hfile = h5py.File(icfile, "r")
gas = hfile["PartType0"]
coords = gas["Coordinates"][:]
ids = gas["ParticleIDs"][:]
hfile.close()

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
    h = swift2.sphtools.find_smoothing_length(xp, xn, kernel = kernel, eta=eta, h_tolerance=h_tolerance, ndim = 1)
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
    print("Before you panic:")
    print(" - Are you using the correct KERNEL_GAMMA in this script?")
    print("   They vary from kernel to kernel, and for different dimensions.")
    print(" - Are you using the correct resolution eta in this script?")
    print("Other ToDo's left for this script: ")
    print(" - corrections for periodic wrapping ")



#  outputfile = "sml_peano.txt"
#  f = open(outputfile, "w")
#  for i in range(sml_peano.shape[0]):
#      f.write("{0:8d},{1:12.6f}\n".format(ids[i], sml_peano[i]))
#  f.close()

sys.exit(errors)


