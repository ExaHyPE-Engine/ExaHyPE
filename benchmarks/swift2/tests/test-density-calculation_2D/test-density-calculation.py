import os
import argparse
import numpy as np

import peano4
import swift2
import dastgen2

import shutil

import swift2.particle.tests

########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description="SPH benchmarking script")
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
    "-c",
    "--clean",
    dest="clean_temporary_directories",
    action="store_true",
    help="Remove temporary directories before script starts",
)
parser.add_argument(
    "-d",
    "--dimensions",
    dest="dimensions",
    type=int,
    default=2,
    help="Spatial dimensions (default 2)",
)
parser.add_argument(
    "-np",
    "--particle-number",
    dest="particle_number",
    required=True,
    type=int,
    help="Particle number (1D)",
)
parser.add_argument(
    "-et",
    "--end-time",
    dest="end_time",
    type=float,
    default=1.0,
    help="End of simulation",
)
parser.add_argument(
    "-dt",
    "--timestep-size",
    dest="timestep_size",
    type=float,
    default=1e-3,
    help="Global timestep size (initial value)",
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

project_namespace = ["benchmarks", "swift2", "densityTest"]

dimensions = args.dimensions

project = swift2.Project(
    namespace=project_namespace, project_name="test_density", executable="test_density"
)

# Effective SPH dimensions ----------------------------------------------------
HYDRO_DIMENSIONS = dimensions

# Particle number along 1 dimension -------------------------------------------
HYDRO_PART_NUMBER = str(args.particle_number)

# Declare the particle type
particle = swift2.particle.tests.testDensityCalculation(
    name="testDensityCalculation",
    dimensions_hydro=HYDRO_DIMENSIONS,
    cfl_factor=0.1,
    initial_time_step_size=1e-4,
    particles_per_cell=5,
    min_h=args.cell_size,
    max_h=args.cell_size,
)

# Add particle to the SWIFT project
particle_set = project.add_particle_species(particle)

if args.asserts:
    build_mode = peano4.output.CompileMode.Asserts
else:
    build_mode = peano4.output.CompileMode.Release

offset = [0, 0, 0]
domain_size = [1, 1, 1]
periodic_boundary_conditions = [False, False, False]  # Periodic BC

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset,
    domain_size=domain_size,
    min_end_time=args.end_time,
    max_end_time=0.1,
    first_plot_time_stamp=0,
    time_in_between_plots=args.end_time,
    periodic_BC=periodic_boundary_conditions,
    plotter_precision=8,
)


project.set_Peano4_installation(args.peanodir, build_mode)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::RecursiveBipartition",
    "new toolbox::loadbalancing::DefaultConfiguration()",
)


###############################################################################
# Initialisation snippet for the particle
###############################################################################
#
# If we work with only one particle, then we can simply set its velocity.
# However, our script works both if we only module the earth but also when we
# model earth and sun explicitly. In the latter case, we have to ensure that
# only the earth is given an initial velocity.
#
###############################################################################

# Snippet from library
ICs = swift2.scenario.TestDensityCalculation()
initialisation_snippet = ICs.initialisation_snippet

#
# Insert particles by coordinate
#

Npart = args.particle_number
offset = 1 / Npart

coords = np.zeros((Npart**HYDRO_DIMENSIONS, 3))

x = np.linspace(offset, 1, Npart, endpoint=False)
y = np.linspace(offset, 1, Npart, endpoint=False)

xp, yp = np.meshgrid(x, y)
xp = xp.reshape((np.prod(xp.shape),))
yp = yp.reshape((np.prod(yp.shape),))

coords[:, 0] = xp
coords[:, 1] = yp

print("inserting particles at:", coords)

project.algorithm_step_initial_conditions.add_action_set(
    swift2.input.InsertParticlesByCoordinates(
        particle_set=particle_set,
        initialisation_call=initialisation_snippet,
        coordinates=coords,
    )
)

#
#  Insert particles from file
#
#  project.algorithm_step_initial_conditions.add_action_set(
#      swift2.input.InsertParticlesFromHDF5File(
#          particle_set=particle_set, filename="sodShock.hdf5"
#      )


###############################################################################
# Set up the plotter
###############################################################################
#
# A plotter has to be told explicitly which data to visualisation. We visualise
# the velocity here as scalar attribute. To some degree, this is redundant info
# as we implicitly have the position of the particle and can "see" its
# velocity when we play through a movie. But the explicit visualisation shows
# how to handle attributes per particle.
#
# When we create the plotter, we have to inform it how to enumerate the
# individual snapshots. Obviously, we can just count through the snapshots.
# In this case, you have to pass the constant peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps
# into the field time_stamp_evaluation. You can also define your own time
# stamp. For this, it is useful to know that each particle definition in the
# Python script will yield a particle class with the same name in the
# subnamespace globaldata. Every particle class in return has a class attribute
#
#            getSpecies()
#
# which returns an object that holds global information about this particle
# type.
###############################################################################
# particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particle_set, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat(
    "particles",
    particle_set,
    "globaldata::{}::getSpecies().getMinTimeStamp()".format(particle.name),
)
particle_plotter.add_attribute_to_plot(particle.part_id, 1)
particle_plotter.add_attribute_to_plot(particle.velocity, dimensions)
particle_plotter.add_attribute_to_plot(particle.acceleration, dimensions)
particle_plotter.add_attribute_to_plot(particle.mass, 1)
particle_plotter.add_attribute_to_plot(particle.density, 1)
particle_plotter.add_attribute_to_plot(particle.pressure, 1)
particle_plotter.add_attribute_to_plot(particle.smoothL, 1)
particle_plotter.add_attribute_to_plot(particle.Ncount, 1)
project.algorithm_step_plot.add_action_set(particle_plotter)


###############################################################################
# Generate plain Peano 4 project
###############################################################################
#
# See commments above (for the force interaction kernel) why we might want to
# have some of the constants from the Python script within the C++ code base
# that's generated.
#
###############################################################################
peano4_project = project.generate_Peano4_project(verbose=args.verbose)

# -----------------------------------------------------------------------------
# Export global SPH constants and parameters
# -----------------------------------------------------------------------------

# There are plenty of constant simulation paramaters to specify for our SPH
# simulation - we do that here.


# EoS parameter ---------------------------------------------------------------
gamma_hydro_list = [str(5 / 3), str(7 / 5), str(4 / 3), str(2 / 1)]
gamma_hydro_list_symbols = [
    "HYDRO_GAMMA_5_3",
    "HYDRO_GAMMA_7_5",
    "HYDRO_GAMMA_4_3",
    "HYDRO_GAMMA_2_1",
]
GAMMA = gamma_hydro_list[0]
GAMMA_HYDRO_SYMBOL = gamma_hydro_list_symbols[0]


# SPH kernel -------------------------------------------------------
kernel_list = ["M5"]
kernel_list_symbols = ["QUARTIC_SPLINE_KERNEL"]
KERNEL = kernel_list[0]
KERNEL_SYMBOL = kernel_list_symbols[0]

# Smoothing-length free parameter (resolution "eta_neighbours").
# Can be set by specifying a desired number of neighbours.
SMOOTH_ETA_FAC = str(1.235)
particle.set_eta_factor(SMOOTH_ETA_FAC)

if KERNEL == "M5" and HYDRO_DIMENSIONS == 1:
    KERNEL_SUPPORT_RAD = "1.936492"  #  1D SWIFT M5 Kernel
if KERNEL == "M5" and HYDRO_DIMENSIONS == 2:
    KERNEL_SUPPORT_RAD = "1.977173"
else:
    print(f"Unable to determine the kernel support radius. Please check.")

N_MEAN_TARGET = str(2.0 * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_ETA_FAC)))

# Max/min allowed values for smoothing length h
HYDRO_h_MAX = str(1.0 / 3.0 / float(KERNEL_SUPPORT_RAD))
HYDRO_h_MIN = "1e-6"


# Export symbols and values for compilation (e.g. for SPH kernel)
# Dummy values for non SPH
peano4_project.output.makefile.add_CXX_flag(
    "-D" + "HYDRO_DIMENSION=" + str(HYDRO_DIMENSIONS)
)
peano4_project.output.makefile.add_CXX_flag("-D" + GAMMA_HYDRO_SYMBOL)
peano4_project.output.makefile.add_CXX_flag("-D" + KERNEL_SYMBOL)


print("-----------------------------------------------------------------------")
print("SPH parameters summary:")
print(f"Effective dimensions of the Simulation: {HYDRO_DIMENSIONS}")
print(f"Number of particles (1D): {HYDRO_PART_NUMBER}")
print(f"Smoothing length free factor: {SMOOTH_ETA_FAC}")
print("-----------------------------------------------------------------------")

# Export Constants.h file
peano4_project.constants.export_const_with_type(
    "HYDRO_DIMENSIONS", str(HYDRO_DIMENSIONS), "double"
)
peano4_project.constants.export_const_with_type(
    "HYDRO_PART_NUMBER", HYDRO_PART_NUMBER, "double"
)
peano4_project.constants.export_const_with_type("GAMMA", GAMMA, "double")
peano4_project.constants.export_const_with_type(
    "KERNEL_SUPPORT_RAD", KERNEL_SUPPORT_RAD, "double"
)
peano4_project.constants.export_const_with_type("HYDRO_h_MAX", HYDRO_h_MAX, "double")
peano4_project.constants.export_const_with_type("HYDRO_h_MIN", HYDRO_h_MIN, "double")
peano4_project.constants.export_const_with_type(
    "SMOOTH_ETA_FAC", SMOOTH_ETA_FAC, "double"
)

# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
    dirs = ["vertexdata/", "repositories/", "globaldata/", "observers/"]
    for dir in dirs:
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
