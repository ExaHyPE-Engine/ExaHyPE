import h5py
import argparse
import numpy as np

"""!
Generates 1D random particle distribution.
"""

fileName = "test_sml_multiscale_1D.hdf5"
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
npart_tot = 4 * npart
coords = np.zeros((npart_tot, 3))
v = np.zeros((npart_tot, 3))
ids = np.linspace(1, npart_tot, npart_tot)
m = np.ones(npart_tot)
h = np.ones(npart_tot) * 4.0 * boxSize / npart_tot  # just a first guess.
u = np.ones(npart_tot)

rng = np.random.RandomState(seed=seed)


def get_coords(box_min, box_max, npart):
    """
    Generate coordinates for particles between box_min and box_max
    """

    dx = (box_max - box_min) / npart

    new_coords = np.zeros((npart, 3))
    # Get uniform coordinates.
    for i in range(npart):
        new_coords[i, 0] = box_min + (i + 0.5) * dx

    # Now randomize them a bit.
    # Taking a completely randomly sampled particle distribution may lead
    # to trouble the codes aren't set up to handle, such as particles which
    # are either too close or too far from each other. So use a somewhat
    # regular distribution instead, by randomly displacing an initially
    # uniform particle distribution.

    # rng.random returns random numbers in [0, 1). Scale that.
    displacement = (2.0 * rng.random((npart, 1)) - 1.0) * 0.499 * dx
    new_coords[:, 0] += displacement[:, 0]

    return new_coords


# Now generate actual coordinates
# First the "background"
new_coords = get_coords(0.0, boxSize, npart)
coords[:npart, 0] = new_coords[:, 0]
new_coords = get_coords(1.0 / 3.0, 5.0 / 6 * boxSize, npart)
coords[npart : 2 * npart, 0] = new_coords[:, 0]
new_coords = get_coords(3.0 / 6.0, 5.0 / 6 * boxSize, npart)
coords[2 * npart : 3 * npart, 0] = new_coords[:, 0]
new_coords = get_coords(4.0 / 6.0, 5.0 / 6 * boxSize, npart)
coords[3 * npart :, 0] = new_coords[:, 0]


from matplotlib import pyplot as plt

plt.figure()
plt.subplot(121)
plt.plot(coords[:, 0], m, marker=".")
plt.subplot(122)
plt.hist(coords[:, 0], bins=200)
plt.show()


# File
file = h5py.File(fileName, "w")

# Header
grp = file.create_group("/Header")
grp.attrs["BoxSize"] = boxSize
grp.attrs["NumPart_Total"] = [npart_tot, 0, 0, 0, 0, 0]
grp.attrs["NumPart_Total_HighWord"] = [0, 0, 0, 0, 0, 0]
grp.attrs["NumPart_ThisFile"] = [npart_tot, 0, 0, 0, 0, 0]
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
