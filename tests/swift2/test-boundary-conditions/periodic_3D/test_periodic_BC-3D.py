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

dimensions = 3
hydro_dimensions = 3


project = swift2.Project(
    namespace=["tests", "swift2", "periodicBoundaryConditions3D"],
    project_name="periodicBoundaryConditions3D",
    executable="periodicBCTest3D",
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
else:
    build_mode = peano4.output.CompileMode.Release

offset = [0, 0, 0]
domain_size = [1, 1, 1]
periodic_boundary_conditions = [True, True, True]  # Periodic BC

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

  const double boxsize_x = """
    + str(domain_size[0])
    + """;
  const double boxsize_y = """
    + str(domain_size[1])
    + """;
  const double boxsize_z = """
    + str(domain_size[2])
    + """;
  const double offset_x = """
    + str(offset[0])
    + """;
  const double offset_y = """
    + str(offset[1])
    + """;
  const double offset_z = """
    + str(offset[2])
    + """;
  const double sq3 = std::sqrt(3);
  const double sq2 = std::sqrt(2);


  if( particle->getX(0) < offset_x + 1./ 3. * boxsize_x ) {

    if (particle->getX(1) < offset_y + 1./3. * boxsize_y) {
      // Low left square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Low left square, z ~ 0

        particle->setV(0, -sq3);
        particle->setV(1, -sq3);
        particle->setV(2, -sq3);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Low left square, z ~ 0.5

        particle->setV(0, -sq2);
        particle->setV(1, -sq2);
        particle->setV(2, 0.);

      } else {

        // Low left square, z ~ 1.

        particle->setV(0, -sq3);
        particle->setV(1, -sq3);
        particle->setV(2, sq3);

      }

    } else if (particle->getX(1) < offset_y + 2./3. * boxsize_y) {
      // Mid left square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Mid left square, z ~ 0

        particle->setV(0, -sq2);
        particle->setV(1, 0.);
        particle->setV(2, -sq2);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Mid left square, z ~ 0.5

        particle->setV(0, 0.);
        particle->setV(1, -1.);
        particle->setV(2, 0.);

      } else {

        // Mid left square, z ~ 1.

        particle->setV(0, -sq2);
        particle->setV(1, 0.);
        particle->setV(2, sq2);

      }

    } else {
      // Top left square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Top left square, z ~ 0

        particle->setV(0, -sq3);
        particle->setV(1, sq3);
        particle->setV(2, -sq3);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Top left square, z ~ 0.5

        particle->setV(0, -sq2);
        particle->setV(1, 0.);
        particle->setV(2, -sq2);

      } else {

        // Top left square, z ~ 1.

        particle->setV(0, -sq3);
        particle->setV(1, sq3);
        particle->setV(2, sq3);

      }
    }
  } else if( particle->getX(0) < offset_x + 2./ 3. * boxsize_x ) {

    if (particle->getX(1) < offset_y + 1./3. * boxsize_y) {
      // Low Center square, z ~ 0

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Low Center square, z ~ 0

        particle->setV(0, 0.);
        particle->setV(1, -sq2);
        particle->setV(2, -sq2);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Low Center square, z ~ 0.5

        particle->setV(0, 0.);
        particle->setV(1, -1.);
        particle->setV(2, 0.);

      } else {

        // Low Center square, z ~ 1.

        particle->setV(0, 0.);
        particle->setV(1, -sq2);
        particle->setV(2, sq2);

      }

    } else if (particle->getX(1) < offset_y + 2./3. * boxsize_y) {
      // Mid Center square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Mid Center square, z ~ 0

        particle->setV(0, 0.);
        particle->setV(1, 0.);
        particle->setV(2, -1.);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Mid Center square, z ~ 0.5

        particle->setV(0, 0.);
        particle->setV(1, 0.);
        particle->setV(2, 0.);

      } else {

        // Mid Center square, z ~ 1.

        particle->setV(0, 0.);
        particle->setV(1, 0.);
        particle->setV(2, 1.);

      }

    } else {
      // Top Center square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Top Center square, z ~ 0

        particle->setV(0, 0);
        particle->setV(1, sq2);
        particle->setV(2, -sq2);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Top Center square, z ~ 0.5

        particle->setV(0, 0.);
        particle->setV(1, 1.);
        particle->setV(2, 0. );

      } else {

        // Top Center square, z ~ 1.

        particle->setV(0, 0.);
        particle->setV(1, sq2);
        particle->setV(2, sq2);

      }
    }

  } else {

    if (particle->getX(1) < offset_y + 1./3. * boxsize_y) {
      // Low Right square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Low Right square, z ~ 0

        particle->setV(0, sq3);
        particle->setV(1, sq3);
        particle->setV(2, -sq3);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Low Right square, z ~ 0.5

        particle->setV(0, sq2);
        particle->setV(1, -sq2);
        particle->setV(2, 0.);

      } else {

        // Low Right square, z ~ 1.

        particle->setV(0, sq3);
        particle->setV(1, -sq3);
        particle->setV(2, sq3);

      }

    } else if (particle->getX(1) < offset_y + 2./3. * boxsize_y) {
      // Mid Right square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Mid Right square, z ~ 0

        particle->setV(0, sq2);
        particle->setV(1, 0.);
        particle->setV(2, -sq2);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Mid Right square, z ~ 0.5

        particle->setV(0, 0.);
        particle->setV(1, 1.);
        particle->setV(2, 0.);

      } else {

        // Mid Right square, z ~ 1.

        particle->setV(0, sq2);
        particle->setV(1, 0.);
        particle->setV(2, sq2);

      }

    } else {
      // Top Right square

      if (particle->getX(2) < offset_z + 1./3. * boxsize_z) {

        // Top Right square, z ~ 0

        particle->setV(0, sq3);
        particle->setV(1, sq3);
        particle->setV(2, -sq3);

      } else if (particle->getX(2) < offset_z + 2./3. * boxsize_z) {

        // Top Right square, z ~ 0.5

        particle->setV(0, sq2);
        particle->setV(1, sq2);
        particle->setV(2, 0.);

      } else {

        // Top Right square, z ~ 1.

        particle->setV(0, sq3);
        particle->setV(1, sq3);
        particle->setV(2, sq3);

      }
    }
  }

  // hack to get a somewhat flexible, but always unique
  // and reproducible particle ID: Assume we have a 1600^3
  // uniform grid particle distribution. (1600^3 is close)
  // to the numeric limits of integers.

  double dx = boxsize_x / 1600.;
  double dy = boxsize_y / 1600.;

  int i = (int) ((particle->getX(0) - offset_x) / dx + 0.5);
  int j = (int) ((particle->getX(1) - offset_y) / dy + 0.5);
  particle->setPartid(i + 1600*j);

"""
)


def rotation_matrix(axis, theta):
    """
    Return the rotation matrix associated with counterclockwise rotation about
    the given axis by theta radians.

    https://stackoverflow.com/questions/6802577/rotation-of-3d-vector
    """
    axis = np.asarray(axis)
    axis = axis / np.sqrt(np.dot(axis, axis))
    a = np.cos(theta / 2.0)
    b, c, d = -axis * np.sin(theta / 2.0)
    aa, bb, cc, dd = a * a, b * b, c * c, d * d
    bc, ad, ac, ab, bd, cd = b * c, a * d, a * c, a * b, b * d, c * d
    return np.array(
        [
            [aa + bb - cc - dd, 2 * (bc + ad), 2 * (bd - ac)],
            [2 * (bc - ad), aa + cc - bb - dd, 2 * (cd + ab)],
            [2 * (bd + ac), 2 * (cd - ab), aa + dd - bb - cc],
        ]
    )


def draw_R(
    scale=1.0, origin=[0.0, 0.0, 0.0], plane=0, random_shift=True, shift_scale=0.05
):
    """
    Draw a letter R consisting of 17 particles.

    scale: what the total size of the letter R should be.
    origin: where to put lower left corner of R
    plane: along which plane to put the letter:
            0: x-y plane.
            1: y-z plane.
            2: x-z plane.
            3: rotate by 45deg around axis (1, 1, 1,)
    perturb: Whether to randomly shift the resulting array
             by up to `shift_scale`. Use this to avoid having
             particles end up overlapping once the simulation
             is running.

    returns: numpy array of 3D coordinates of 17 particles
    """

    points = [
        [0.0, 0.0],
        [0.0, 0.2],
        [0.0, 0.4],
        [0.0, 0.6],
        [0.0, 0.8],
        [0.0, 1.0],
        [0.1, 1.0],
        [0.2, 0.95],
        [0.3, 0.8],
        [0.2, 0.625],
        [0.05, 0.6],
        [0.1, 0.5],
        [0.15, 0.4],
        [0.2, 0.3],
        [0.25, 0.2],
        [0.3, 0.1],
        [0.35, 0.0],
    ]

    R = np.array(points)

    result = np.zeros((R.shape[0], 3))

    if plane == 0:
        result[:, 0] = R[:, 0]
        result[:, 1] = R[:, 1]
    elif plane == 1:
        result[:, 1] = R[:, 0]
        result[:, 2] = R[:, 1]
    elif plane == 2:
        result[:, 0] = R[:, 0]
        result[:, 2] = R[:, 1]
    elif plane == 3:
        result[:, 0] = R[:, 0]
        result[:, 1] = R[:, 1]
        rot = rotation_matrix([1.0, 1.0, 1], np.pi / 4.0)
        for i in range(result.shape[0]):
            result[i, :] = np.dot(rot, result[i, :])

    if random_shift:
        shift = np.random.uniform(0, shift_scale, size=3)
        result += shift

    result *= scale
    result += np.array(origin)

    return result


# Generate coorinates.

R1 = draw_R(scale=0.2, origin=[0.1, 0.4, 0.5], plane=0)
R2 = draw_R(scale=0.2, origin=[0.8, 0.4, 0.5], plane=0)
R3 = draw_R(scale=0.2, origin=[0.5, 0.1, 0.4], plane=1)
R4 = draw_R(scale=0.2, origin=[0.5, 0.8, 0.4], plane=1)
R5 = draw_R(scale=0.2, origin=[0.4, 0.5, 0.1], plane=2)
R6 = draw_R(scale=0.2, origin=[0.4, 0.5, 0.8], plane=2)
R7 = draw_R(scale=0.2, origin=[0.1, 0.1, 0.1], plane=3)
R8 = draw_R(scale=0.2, origin=[0.8, 0.1, 0.1], plane=3)
R9 = draw_R(scale=0.2, origin=[0.1, 0.8, 0.1], plane=3)
R10 = draw_R(scale=0.2, origin=[0.8, 0.8, 0.1], plane=3)
R11 = draw_R(scale=0.2, origin=[0.1, 0.1, 0.8], plane=3)
R12 = draw_R(scale=0.2, origin=[0.8, 0.1, 0.8], plane=3)
R13 = draw_R(scale=0.2, origin=[0.1, 0.8, 0.8], plane=3)
R14 = draw_R(scale=0.2, origin=[0.8, 0.8, 0.8], plane=3)

coords = np.concatenate(
    (R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14), axis=0
)


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
peano4_project.output.makefile.add_CXX_flag("-D" + "HYDRO_DIMENSION=3")

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
