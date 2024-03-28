#!/usr/bin/env python3

"""!
Compare whether initial states and states after 1 loop through box are identical
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
    Compare whether initial states and states after 1 loop through box are
    identical.
    """
)
parser.add_argument(
    "-f",
    "--pvd-file",
    dest="pvdfile",
    help=".pvd file to read in",
    default = "./particles.pvd"
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
    "-et",
    "--end-time",
    dest="t_end",
    type=float,
    help="What end time to read in",
    required=True,
)
parser.add_argument(
    "-tol",
    "--tolerance",
    dest="tolerance",
    type=float,
    default=1.0e-6,
    help="Threshold for comparison tolerance",
)

args = parser.parse_args()

ndim = args.ndim
comparison_tolerance=args.tolerance
end_time = args.t_end
pvdfile = args.pvdfile


reader_0 = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=0.)
partData_0 = reader_0.load()

reader_1 = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=end_time)
partData_1 = reader_1.load()

coords_0 = partData_0.x[:, :ndim]
ids_0 = partData_0.partid
sort0 = np.argsort(ids_0)
coords_0 = coords_0[sort0]
ids_0 = ids_0[sort0]

coords_1 = partData_1.x[:, :ndim]
ids_1 = partData_1.partid
sort1 = np.argsort(ids_1)
coords_1 = coords_1[sort1]
ids_1 = ids_1[sort1]

print(coords_0)
print(coords_1)


if coords_0.shape != coords_1.shape :
    print("Error: coordinate arrays are not of equal length.")
    print("       coords_0.shape", coords_0.shape)
    print("       coords_1.shape", coords_1.shape)
    quit()


errors = 0

for i in range(ids_0.shape[0]):

    for n in range(ndim):

        diff = coords_1[i,n] / coords_0[i,n] - 1.
        if abs(diff) > comparison_tolerance:
            print("Error: Coordinates disagree.")
            print("    Particle ID:   ", ids_0[i], ids_1[i])
            print("    Dimension:     ", n)
            print("    Diff:          ", diff)
            print("    Coords_0:      ", coords_0[i])
            print("    Coords_1:      ", coords_1[i])
            print("----------------------------")
            errors += 1
        else:
            print(diff)

print("Finished with", errors, "errors.")

sys.exit(errors)
