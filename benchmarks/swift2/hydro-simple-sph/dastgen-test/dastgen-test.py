import os
import argparse
import numpy as np

import peano4
import swift2
import swift2.sphtools as sphtools
import dastgen2

import shutil


########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description="Dastgen2 testing script")
parser.add_argument(
    "-j", "--parallel-builds", dest="j", type=int, default=-1, help="Parallel builds"
)
parser.add_argument(
    "-v",
    "--verbose",
    dest="verbose",
    action="store_true",
    default=False,
    help="Verbose",
)
parser.add_argument(
    "-cs",
    "--cell-size",
    dest="cell_size",
    type=float,
    default=0.3,
    help="Cell size (default 0.3)",
)
parser.add_argument(
    "-pd",
    "--peano-dir",
    dest="peanodir",
    default="../../../..",
    help="Peano4 directory",
)
parser.add_argument(
    "-asserts",
    "--asserts",
    dest="asserts",
    action="store_true",
    default=False,
    help="Switch on assertions",
)
args = parser.parse_args()


########################################################################################
# Generate SWIFT project
########################################################################################

project_namespace = ["benchmarks", "swift2", "dastgen_test"]

project = swift2.Project(
    namespace=project_namespace, project_name="dastgen_test", executable="dastgen_test"
)


# Initialise the Dummy particle --------------------------------------------------
dimensions = 2  # Peano dimensions


particle = swift2.particle.DastgenTestDummyParticle(
    "dummyPart",
    particles_per_cell=20,
    min_h=args.cell_size / 100,
    max_h=args.cell_size,
)

# Additional Particle Parameters -----------------------------------------------
hydro_h_max = args.cell_size / 2
particle.h_hydro_max = hydro_h_max

# Add particle to project --------------------------------------------
# Notice that any change to the particle will not take effect after adding
# the particle_set to the project
particle_set = project.add_particle_species(particle)

# Always build with asserts on.
build_mode = peano4.output.CompileMode.Asserts

# Set Peano4 simulation parameters ---------------------------------------------
offset = [0, 0, 0]
domain_size = [1, 1, 1]
periodic_boundary_conditions = [False, False, False]

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset,
    domain_size=domain_size,
    min_end_time=1.0,
    max_end_time=1.0,
    first_plot_time_stamp=0.0,
    time_in_between_plots=1.0,
    periodic_BC=periodic_boundary_conditions,
    plotter_precision=8,
)

project.set_Peano4_installation(args.peanodir, build_mode)


###############################################################################
# Initialisation snippet for the particle
###############################################################################

# Alternative method: InsertParticlesByCoordinates

# Set up the coordinates for the particles
# We construct a regular (lattice) configuration.
Npart = 100
offset = 1 / Npart

coords = np.zeros((Npart**dimensions, 3))

x = np.linspace(offset, 1, Npart, endpoint=False)
y = np.linspace(offset, 1, Npart, endpoint=False)

xp, yp = np.meshgrid(x, y)
xp = xp.reshape((np.prod(xp.shape),))
yp = yp.reshape((np.prod(yp.shape),))

coords[:, 0] = xp
coords[:, 1] = yp

#  Sedov-Taylor Blast snippet  --------------------------------------------------
#  The particle attributes are set via an initalisation snippet that sets
#  the initial conditions specific for this problem, i.e. the velocity field
#  that converges towards the center of the domain and that generates the
#  collapse.

initialisation_snippet = """
      /*
       * Initial Conditions: None. The dummy particles aren't even proper SPH
       * particles. Notably, they don't even contain fields like mass, density,
       * etc. Nothing to do here except read in particle positions (which is
       * done elsewhere) and give particles a search radius, which needs to
       * exist.
       */
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0);
      particle->setSearchRadius( GridSize_x / 2.0);

      std::cout << "CALLING IC STUFF " << particle->getSearchRadius() << std::endl;
"""

#  Finally, pass this configuration to the initialisation script
project.algorithm_step_initial_conditions.add_action_set(
    swift2.input.InsertParticlesByCoordinates(
        particle_set=particle_set,
        initialisation_call=initialisation_snippet,
        coordinates=coords,
        additional_includes="",
    )
)


###############################################################################
# Generate plain Peano 4 project
###############################################################################
#
# The value of some constants such as the adiabatic index cannot be arbitrary
# but rather we have a set of pre-defined options. The same happens for some
# functions, e.g. the SPH smoothing kernel. This allows us to define some
# symbols and fix the values at compile time.
#
#
###############################################################################

peano4_project = project.generate_Peano4_project(verbose=args.verbose)


###############################################################################
# Cleanup and compilation
###############################################################################

# Always build project from scratch
dirs = ["vertexdata/", "repositories/", "globaldata/", "observers/"]
for dir in dirs:
    if os.path.exists(dir):
        shutil.rmtree(dir)

# It is safe (and recommended) to overwrite any existing main file, since this
# is generated automatically by the Swift2 graph compiler.
peano4_project.generate(
    overwrite=peano4.output.Overwrite.Always, throw_away_data_after_generation=True
)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)

# remove output files from previous runs
output_files = [
    f
    for f in os.listdir(".")
    if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd")
]
for f in output_files:
    os.remove(f)
