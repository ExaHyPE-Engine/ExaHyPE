import h5py
import argparse
import numpy as np

"""!
Generates 1D random particle distribution.
"""

fileName = "test_sml_1D.hdf5"
boxSize = 1.0

parser = argparse.ArgumentParser(description="SPH benchmarking script")
parser.add_argument(
    "-np",
    "--particle-number",
    dest="particle_number",
    required=True,
    type=int,
    help="Particle number (1D)",
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
coords = np.zeros((npart, 3))
v = np.zeros((npart, 3))
ids = np.linspace(1, npart, npart)
m = np.ones(npart)
dx = boxSize / npart
h = np.ones(npart) * 2.0 * dx
u = np.ones(npart)

# Get uniform coordinates.
for i in range(npart):
    coords[i, 0] = (i + 0.5) * dx

# Now randomize them a bit.
# Taking a completely randomly sampled particle distribution may lead
# to trouble the codes aren't set up to handle, such as particles which
# are either too close or too far from each other. So use a somewhat
# regular distribution instead, by randomly displacing an initially
# uniform particle distribution.

rng = np.random.RandomState(seed=seed)
#  rng.random returns random numbers in [0, 1). Scale that.
displacement = (2.0 * rng.random(coords.shape) - 1.0) * 0.499 * dx
coords[:, 0] += displacement[:, 0]

# File
file = h5py.File(fileName, "w")

# Header
grp = file.create_group("/Header")
grp.attrs["BoxSize"] = boxSize
grp.attrs["NumPart_Total"] = [npart, 0, 0, 0, 0, 0]
grp.attrs["NumPart_Total_HighWord"] = [0, 0, 0, 0, 0, 0]
grp.attrs["NumPart_ThisFile"] = [npart, 0, 0, 0, 0, 0]
# This is an addition for this test to be able to estimate the search radius.
grp.attrs["NumPart_base"] = [npart]
grp.attrs["Time"] = 0.0
grp.attrs["NumFilesPerSnapshot"] = 1
grp.attrs["MassTable"] = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
grp.attrs["Flag_Entropy_ICs"] = 0
grp.attrs["Dimension"] = 1
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
