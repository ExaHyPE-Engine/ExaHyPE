import os
import argparse
import numpy as np

import peano4
import swift2
import dastgen2

import shutil

########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description="Test periodic boundary conditions")
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
    help="Global timestep size",
)
parser.add_argument(
    "-plot",
    "--plot-delta",
    dest="plot_delta",
    type=float,
    default=0.01,
    help="Plot delta (0 switches off)",
)
parser.add_argument(
    "-cs",
    "--cell-size",
    dest="cell_size",
    type=float,
    default=0.1,
    help="Cell size (default 0.1)",
)
parser.add_argument(
    "-pd",
    "--peano-dir",
    dest="peanodir",
    default="../../../../",
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

# peano doesn't like 1D grids
dimensions = 2
hydro_dimensions = 1


project = swift2.Project(
    namespace=["tests", "swift2", "periodicBoundaryConditions1D"],
    project_name="periodicBoundaryConditions1D",
    executable="periodicBCTest1D",
)


particle = swift2.particle.LeapfrogFixedSearchRadius(
    name="GenericParticle",
    cfl_factor=1.0,
    initial_time_step_size=args.timestep_size,
    particles_per_cell=0,
    min_h=args.cell_size,
    max_h=0.3,
)
particle_set = project.add_particle_species(particle)


if args.asserts:
    build_mode = peano4.output.CompileMode.Asserts
    #  build_mode = peano4.output.CompileMode.Debug
else:
    build_mode = peano4.output.CompileMode.Release

offset = [0, 0, 0]
domain_size = [1, 1, 1]
periodic_boundary_conditions = [True, False, False]  # Periodic BC


project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset,
    domain_size=domain_size,
    min_end_time=args.end_time,
    max_end_time=0.0,
    first_plot_time_stamp=0.0,
    time_in_between_plots=args.plot_delta,
    periodic_BC=periodic_boundary_conditions,
    plotter_precision=8,  # plotter precision
)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
    "new toolbox::loadbalancing::DefaultConfiguration()",
)

project.set_Peano4_installation(args.peanodir, build_mode)


###############################################################################
# Declare the force acting on the particle
###############################################################################
# No force. We just need the particles to drift along calmly with the same
# velocity they start out with.

particle_force = ""

#  particle.cell_kernel = particle_force


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
search_radius = args.cell_size / 100.0
initialisation_snippet = (
    """
  /* Internal search radius used by Peano.
   * Not really used here. */
  particle->setSearchRadius("""
    + str(search_radius)
    + """);
  particle->setA( {0, 0, 0} );

  if( particle->getX(0) > 0.5 ){
    particle->setV( {1, 0, 0} );
  } else {
    particle->setV( {-1, 0, 0} );
  }

  // hack to get a somewhat flexible, but always unique
  // and reproducible particle ID: Assume we have a 1600^3
  // uniform grid particle distribution. (1600^3 is close)
  // to the numeric limits of integers.

  double boxsize_x = """
    + str(domain_size[0])
    + """;
  double offset_x = """
    + str(offset[0])
    + """;

  double dx = boxsize_x / 1600.;

  int id = (int) ((particle->getX(0) - offset_x) / dx + 0.5);
  particle->setPartid(id);

"""
)

coords = np.array([[0.21, 0.5, 0.0], [0.81, 0.5, 0.0]])

project.algorithm_step_initial_conditions.add_action_set(
    swift2.input.InsertParticlesByCoordinates(
        particle_set=particle_set,
        initialisation_call=initialisation_snippet,
        coordinates=coords,
    )
)


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
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat(
    "particles",
    particle_set,
    "globaldata::{}::getSpecies().getMinTimeStamp()".format(particle.name),
)
particle_plotter.add_attribute_to_plot(particle.velocity, dimensions)
particle_plotter.add_attribute_to_plot(particle.part_id, 1)
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
peano4_project.output.makefile.add_CXX_flag("-D" + "HYDRO_DIMENSION=1")

peano4_project.constants.export_const_with_type(
    "GLOBAL_TIME_STEP_SIZE", str(args.timestep_size), "double"
)

# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
    dirs = ["vertexdata/", "repositories/", "globaldata/", "observers/"]
    for dir in dirs:
        shutil.rmtree(dir)

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
