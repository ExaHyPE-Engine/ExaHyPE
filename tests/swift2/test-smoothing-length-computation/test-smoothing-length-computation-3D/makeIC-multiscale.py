#!/usr/bin/env python3

import h5py
import argparse
import numpy as np

"""!
Generates 3D randomized particle distribution with
multiple scales, i.e. recursively nested boxes
such that the grid will contain multiple scales.
"""

fileName = "test_sml_multiscale_3D.hdf5"
boxSize = 1.0

parser = argparse.ArgumentParser(description="SPH benchmarking script")
parser.add_argument(
    "-np",
    "--particle-number",
    dest="particle_number",
    required=True,
    type=int,
    help="Particle number per dimension",
)
parser.add_argument(
    "-s",
    "--seed",
    dest="random_seed",
    required=True,
    type=int,
    help="Seed for RNG",
)

args = parser.parse_args()
npart = args.particle_number
seed = args.random_seed


# ---------------------------------------------------

# Build the arrays
npart_tot = 4 * (npart**3)
coords = np.zeros((npart_tot, 3))
v = np.zeros((npart_tot, 3))
ids = np.linspace(1, npart_tot, npart_tot)
m = np.ones(npart_tot)
dx = boxSize / npart
h = np.ones(npart_tot) * 2.0 * dx
u = np.ones(npart_tot)


rng = np.random.RandomState(seed=seed)


def get_coords(box_xmin, box_xmax, box_ymin, box_ymax, box_zmin, box_zmax, npart):
    """
    Generate coordinates for particles between box_min and box_max
    npart: number of particles in each dimension
    """

    dx = (box_xmax - box_xmin) / npart
    dy = (box_ymax - box_ymin) / npart
    dz = (box_zmax - box_zmin) / npart

    npart3 = npart**3

    # First get uniform coordinates.
    new_coords = np.zeros((npart3, 3))
    ind = 0
    for i in range(npart):
        x = (i + 0.5) * dx
        for j in range(npart):
            y = (j + 0.5) * dy
            for k in range(npart):
                z = (k + 0.5) * dy
                new_coords[ind, 0] = box_xmin + x
                new_coords[ind, 1] = box_ymin + y
                new_coords[ind, 2] = box_zmin + z
                ind += 1

    # Now randomize them a bit.
    # Taking a completely randomly sampled particle distribution may lead
    # to trouble the codes aren't set up to handle, such as particles which
    # are either too close or too far from each other. So use a somewhat
    # regular distribution instead, by randomly displacing an initially
    # uniform particle distribution.

    # rng.random returns random numbers in [0, 1). Scale that to [-1, 1)
    displacement_x = (2.0 * rng.random((npart3, 1)) - 1.0) * 0.499 * dx
    displacement_y = (2.0 * rng.random((npart3, 1)) - 1.0) * 0.499 * dy
    displacement_z = (2.0 * rng.random((npart3, 1)) - 1.0) * 0.499 * dz
    new_coords[:, 0] += displacement_x[:, 0]
    new_coords[:, 1] += displacement_y[:, 0]
    new_coords[:, 2] += displacement_z[:, 0]

    return new_coords


# Now generate actual coordinates
# First the "background"
npart3 = npart**3
new_coords = get_coords(0.0, boxSize, 0.0, boxSize, 0.0, boxSize, npart)
coords[:npart3, :3] = new_coords[:, :3]

new_coords = get_coords(
    0.0, 2.0 / 3 * boxSize, 0.0, 2.0 / 3 * boxSize, 0.0, 2.0 / 3 * boxSize, npart
)
coords[npart3 : 2 * npart3, :3] = new_coords[:, :3]

new_coords = get_coords(
    1.0 / 6,
    2.0 / 3 * boxSize,
    1.0 / 6,
    2.0 / 3 * boxSize,
    1.0 / 6,
    2.0 / 3 * boxSize,
    npart,
)
coords[2 * npart3 : 3 * npart3, :3] = new_coords[:, :3]

new_coords = get_coords(
    2.0 / 6,
    2.0 / 3 * boxSize,
    2.0 / 6,
    2.0 / 3 * boxSize,
    2.0 / 6,
    2.0 / 3 * boxSize,
    npart,
)
coords[3 * npart3 :, :3] = new_coords[:, :3]


# too many particles mean that the comparison python script takes ages.
# too few particles mean that the search radii are going to be too big
# for the cell size.
# So let's scale the particles down into a smaller box than the actual
# box size.
scale = 1.0 / 5
coords *= scale
h *= scale
# and shift them into the center
coords += 0.4


from matplotlib import pyplot as plt

plt.figure()
plt.scatter(coords[:, 0], coords[:, 1], s=1, marker=",")
plt.show()


# File
file = h5py.File(fileName, "w")

# Header
grp = file.create_group("/Header")
grp.attrs["BoxSize"] = boxSize
grp.attrs["NumPart_Total"] = [npart**3, 0, 0, 0, 0, 0]
grp.attrs["NumPart_Total_HighWord"] = [0, 0, 0, 0, 0, 0]
grp.attrs["NumPart_ThisFile"] = [npart**3, 0, 0, 0, 0, 0]
# This is an addition for this test to be able to estimate the search radius.
grp.attrs["NumPart_base"] = [npart / scale]
grp.attrs["Time"] = 0.0
grp.attrs["NumFilesPerSnapshot"] = 1
grp.attrs["MassTable"] = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
grp.attrs["Flag_Entropy_ICs"] = 0
grp.attrs["Dimension"] = 3
grp.attrs["RandomSeed"] = seed

# Units
grp = file.create_group("/Units")
grp.attrs["Unit length in cgs (U_L)"] = 1.0
grp.attrs["Unit mass in cgs (U_M)"] = 1.0
grp.attrs["Unit time in cgs (U_t)"] = 1.0
grp.attrs["Unit current in cgs (U_I)"] = 1.0
grp.attrs["Unit temperature in cgs (U_T)"] = 1.0

# Particle group
grp = file.create_group("/PartType0")
grp.create_dataset("Coordinates", data=coords, dtype="d")
grp.create_dataset("Velocities", data=v, dtype="f")
grp.create_dataset("Masses", data=m, dtype="f")
grp.create_dataset("SmoothingLength", data=h, dtype="f")
grp.create_dataset("InternalEnergy", data=u, dtype="f")
grp.create_dataset("ParticleIDs", data=ids, dtype="L")


file.close()
