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
import multiprocessing


parser = argparse.ArgumentParser(
    description="""
    Compare Smoothing Length Results With Expectations. This script assumes that
    particles may have variable smoothing lengths.

    This script has two main modes of usage, which are determined by the
    provided initial conditions file via the `--ic-file` argument.

    If the IC file is a hdf5 file, the script assumes that we're running a
    unit test where swift2 is computing smoothing lengths from randompy sampled
    particles. It will then look for a `particles-0.vtu` file in this directory
    to compare results to.

    If the IC file is a .vtu file, the script assumes that you want to verify a
    swift2 snapshot. It will then compute smoothing lengths for the particle positions
    given in the .vtu file, and compare them to the smoothing lengths from the same
    .vtu file.
    """
)
parser.add_argument(
    "-ic",
    "--ic-file",
    dest="ic_filename",
    help="Initial conditions file. Must be a .hdf5 or .vtu file.",
    required=True,
)
parser.add_argument(
    "-n",
    "--neighbours",
    dest="nneigh",
    type=float,  # this is not a typo. We want average number of neighbours, which needn't be an int.
    default=None,
    help="Number of neighbours to search for. You must specify either --eta or --neighbours",
)
parser.add_argument(
    "-e",
    "--eta",
    dest="eta",
    type=float,
    default=None,
    help="Target resolution eta. You must specify either --eta or --neighbours",
)
parser.add_argument(
    "-d",
    "--ndim",
    dest="ndim",
    type=int,
    help="Number of dimensions to work with",
    required=True,
)
parser.add_argument(
    "-ht",
    "--h-tolerance",
    dest="h_tol",
    type=float,
    default=1.0e-6,
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
h_tolerance = args.h_tol
kernel = args.kernel
if kernel == "quartic_spline":
    kernel = "quartic_spline_vectorized"
ndim = args.ndim

eta = args.eta
nneigh = args.nneigh

if eta is None and nneigh is None:
    raise ValueError("You need to specify either --eta or --neighbours")
if eta is not None and nneigh is not None:
    raise ValueError("You need to specify either --eta or --neighbours, not both")
if nneigh is not None:
    eta = swift2.sphtools.eta_from_number_of_neighbours(
        nneigh, kernel=kernel, ndim=ndim
    )
else:
    nneigh = swift2.sphtools.number_of_neighbours_from_eta(
        eta, kernel=kernel, ndim=ndim
    )

# above this (relative) threshold, treat deviations as an error
# Make this tolerance dependant on the used h_tolerance. Add a
# factor of 10 to allow for float round-off errors and the likes.
# Naturally this assumes that h_tolerance is small. Typically <= 1e-4
comparison_tolerance = 10.0 * h_tolerance
comparison_tolerance = max(comparison_tolerance, 1.0e-4)


print(" Running comparison")
print(" --- IC file:                    ", icfile)
print(" --- Dimensions:                 ", ndim)
print(" --- SPH Kernel:                 ", kernel)
print(" --- Target number of neighbours:", nneigh)
print(" --- Target eta:                 ", eta)
print(" --- h tolerance:                ", h_tolerance)
print(" --- comparison tolerance:       ", comparison_tolerance)

# Get IC data
# -------------------------

if icfile.endswith(".hdf5"):
    # Doing a hdf5 run.
    vtufile = "particles-1.vtu"

    hfile = h5py.File(icfile, "r")
    gas = hfile["PartType0"]
    coords = gas["Coordinates"][:]
    coords = coords[:, :ndim]
    ids = gas["ParticleIDs"][:]
    hfile.close()

elif icfile.endswith(".vtu"):
    # Doing a snapshot verification
    vtufile = icfile

    reader = ParticleVTUReader(vtufile=vtufile, verbose=False)
    partData = reader.load()
    coords = partData.x[:, :ndim]
    ids = partData.partid

else:
    raise ValueError("Unrecognized file type {icfile}, looking for .vtu or .hdf5 file.")

# Sort by particle ID
sort_ic = np.argsort(ids)
ids = ids[sort_ic]
coords = coords[sort_ic]


# Get expected results
# -------------------------

tree = KDTree(coords)
distances, indexes = tree.query(coords, k=2 * nneigh + 10 * ndim)
neighbours = coords[indexes]

npart = coords.shape[0]
sml_python = np.zeros((npart))


def sml_search(i):
    # the KDTree returns the particle itself as a neighbour too.
    # it is stored at the first index, with distance 0.
    xp = neighbours[i, 0, :]
    xn = neighbours[i, 1:, :]

    verb = False
    h = swift2.sphtools.find_smoothing_length(
        xp, xn, kernel=kernel, eta=eta, h_tolerance=h_tolerance, ndim=ndim, verbose=verb
    )
    return h


pool = multiprocessing.Pool()
sml_python[:] = pool.map(sml_search, (i for i in range(npart)))


# Read in peano output
# -------------------------------

reader = ParticleVTUReader(vtufile=vtufile, verbose=False)
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
errorstr = (
    "Found difference: "
    + "particle ID = {0:4d} "
    + "h_peano = {1:.5e} "
    + "h_python = {2:.5e} "
    + "diff = {3:.5e}"
)

for i in range(npart):
    diff = 1.0 - sml_peano[i] / sml_python[i]

    if abs(diff) > comparison_tolerance:
        errors += 1
        print(errorstr.format(int(partIDs_peano[i]), sml_peano[i], sml_python[i], diff))


print(f"\nSummary run with {nneigh} neighbours:")
print(f"Finished with {errors} errors.")
if errors > 0:
    print("Before you panic:")
    print(" - Are you using the correct kernel in this script?")
    print(" - Are you using the correct resolution eta in this script?")
    print(" - Are you hitting h_min/h_max limits?")
    print("Other ToDo's left for this script: ")
    print(" - corrections for periodic wrapping ")


sys.exit(errors)
