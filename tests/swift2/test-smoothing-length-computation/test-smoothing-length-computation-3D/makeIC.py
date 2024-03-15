import h5py
import argparse
import numpy as np

"""!
Generates 3D randomized particle distribution.
"""

fileName = "test_sml_3D.hdf5"
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
coords = np.zeros((npart**3, 3))
v = np.zeros((npart**3, 3))
ids = np.linspace(1, npart**3, npart**3)
m = np.ones(npart**3)
dx = boxSize / npart
h = np.ones(npart**3) * 2.0 * dx
u = np.ones(npart**3)

# Get uniform coordinates.
ind = 0
for i in range(npart):
    x = (i + 0.5) * dx
    for j in range(npart):
        y = (j + 0.5) * dx
        for k in range(npart):
            z = (k + 0.5) * dx

            coords[ind, 0] = x
            coords[ind, 1] = y
            coords[ind, 2] = z
            ind += 1

# Now randomize them a bit.
# Taking a completely randomly sampled particle distribution may lead
# to trouble the codes aren't set up to handle, such as particles which
# are either too close or too far from each other. So use a somewhat
# regular distribution instead, by randomly displacing an initially
# uniform particle distribution.

rng = np.random.RandomState(seed=seed)
# rng.random returns random numbers in [0, 1). Scale that.
displacement = (2.0 * rng.random(coords.shape) - 1.0) * 0.299 * dx
#  displacement = np.zeros(coords.shape)
coords[:, 0] += displacement[:, 0]
coords[:, 1] += displacement[:, 1]
coords[:, 2] += displacement[:, 2]


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
