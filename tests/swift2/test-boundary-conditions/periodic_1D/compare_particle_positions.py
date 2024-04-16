#!/usr/bin/env python3

"""!
Compare whether initial states and states after 1 loop through box are identical
"""

import numpy as np
import argparse
import sys
import peano4
from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader


parser = argparse.ArgumentParser(
    description="""
    Compare whether the particles are at the correct position.
    The assumption is that particles move with a constant velocity
    across periodic boundaries.
    """
)
parser.add_argument(
    "-f",
    "--pvd-file",
    dest="pvdfile",
    help=".pvd file to read in",
    default="./particles.pvd",
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
parser.add_argument(
    "-bs",
    "--boxsize",
    dest="boxsize",
    type=float,
    default=1.0,
    help="Box size in all dimensions.",
)

args = parser.parse_args()

ndim = args.ndim
comparison_tolerance = args.tolerance
boxsize = args.boxsize
end_time = args.t_end
pvdfile = args.pvdfile


reader_0 = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=0.0)
partData_0 = reader_0.load()

reader_1 = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=end_time)
partData_1 = reader_1.load()

actual_end_time = reader_1.get_snapshot_time()

coords_0 = partData_0.x[:, :ndim]
vels_0 = partData_0.v[:, :ndim]
ids_0 = partData_0.partid
sort0 = np.argsort(ids_0)
coords_0 = coords_0[sort0]
vels_0 = vels_0[sort0]
ids_0 = ids_0[sort0]

coords_expect = coords_0 + actual_end_time * vels_0
for dim in range(ndim):
    mask = coords_expect[:, dim] > boxsize
    while mask.any():
        coords_expect[mask, dim] -= boxsize
        mask = coords_expect[:, dim] > boxsize

    mask = coords_expect[:, dim] < 0.0
    while mask.any():
        coords_expect[mask, dim] += boxsize
        mask = coords_expect[:, dim] < 0.0


coords_1 = partData_1.x[:, :ndim]
ids_1 = partData_1.partid
sort1 = np.argsort(ids_1)
coords_1 = coords_1[sort1]
ids_1 = ids_1[sort1]


if coords_0.shape != coords_1.shape:
    print("Error: coordinate arrays are not of equal length.")
    print("       coords_0.shape", coords_0.shape)
    print("       coords_1.shape", coords_1.shape)
    quit()


errors = 0

for i in range(ids_0.shape[0]):
    for n in range(ndim):
        diff = coords_expect[i, n] / coords_1[i, n] - 1.0
        if abs(diff) > comparison_tolerance:
            print("Error: Coordinates disagree.")
            print("    Particle ID:   ", ids_0[i], ids_1[i])
            print("    Dimension:     ", n)
            print("    Diff:          ", diff)
            print("    Coords_0:      ", coords_0[i])
            print("    Coords_1:      ", coords_1[i])
            print("    Coords expect: ", coords_expect[i])
            print("----------------------------")
            errors += 1
        else:
            print(diff)

print("Finished with", errors, "errors.")

sys.exit(errors)
