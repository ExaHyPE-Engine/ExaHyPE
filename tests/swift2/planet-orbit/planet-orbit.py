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
    default=0.3,
    help="Cell size (default 0.3)",
)
parser.add_argument(
    "-pd", "--peano-dir", dest="peanodir", default="../../..", help="Peano4 directory"
)
parser.add_argument(
    "-asserts",
    "--asserts",
    dest="asserts",
    action="store_true",
    default=False,
    help="Switch on assertions",
)
parser.add_argument(
    "-integrator",
    "--time-integrator",
    dest="time_integrator",
    choices=["leapfrog", "euler", "euler-dynamic", "Euler", "Euler-dynamic"],
    required=True,
    help="Type of time integrator to use (default: leapfrog)",
)
args = parser.parse_args()


########################################################################################
# Generate SWIFT project
########################################################################################

dimensions = args.dimensions


#! [tutorials swift2 planet-orbit Create project]
project = swift2.Project(
    namespace=["tests", "swift2", "planetorbit"],
    project_name="planetorbit",
    executable="orbit-" + str(args.cell_size),
)
#! [tutorials swift2 planet-orbit Create project]


"""
Declare the particle type:
We need to specify the time integration scheme that will be used to evolve
this particle. Current choices include:
  - Leapfrog (default)
  - Explicit Euler with fixed search radius
  - Explicit Euler with variable search radius

"""

if args.time_integrator == "leapfrog":
    #! [tutorials swift2 planet-orbit Instantiate particle with leapfrog ODE integrator]
    particle = swift2.particle.LeapfrogFixedSearchRadius(
        name="Planet",
        cfl_factor=0.2,
        initial_time_step_size=args.timestep_size,
        particles_per_cell=0,
        min_h=args.cell_size,
        max_h=0.3,
    )
    particle_set = project.add_particle_species(particle)
    #! [tutorials swift2 planet-orbit Instantiate particle with leapfrog ODE integrator]
elif args.time_integrator == "euler" or "Euler":
    particle = swift2.particle.ExplicitEulerFixedSearchRadius(
        name="Planet",
        cfl_factor=0.2,
        initial_time_step_size=args.timestep_size,
        particles_per_cell=0,
        min_h=args.cell_size,
        max_h=0.3,
    )
    particle_set = project.add_particle_species(particle)
elif args.time_integrator == "euler-dynamic" or "Euler-dynamic":
    particle = swift2.particle.ExplicitEulerDynamicSearchRadius(
        name="Planet",
        cfl_factor=0.2,
        initial_time_step_size=args.timestep_size,
        particles_per_cell=0,
        min_h=args.cell_size,
        max_h=0.3,
    )
    particle_set = project.add_particle_species(particle)
else:
    print("Time integrator not supported. Please check the list of options.")


#! [tutorials swift2 planet-orbit Declare additional attributes]
energy_kin_attr = dastgen2.attributes.Double("energyKin")
energy_pot_attr = dastgen2.attributes.Double("energyPot")
energy_tot_attr = dastgen2.attributes.Double("energyTot")
particle.data.add_attribute(energy_kin_attr)
particle.data.add_attribute(energy_pot_attr)
particle.data.add_attribute(energy_tot_attr)
#! [tutorials swift2 planet-orbit Declare additional attributes]


#! [tutorials swift2 planet-orbit Configure project]
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
#! [tutorials swift2 planet-orbit Configure project]


#! [tutorials swift2 planet-orbit Define constants]
# Orbital parameters
N_orbits = 8
# Number of orbits
G_NEWTON = 6.67384e-11
# Newton's constant  [m^3/kg/s^2]
M_SUN = 1.9885e30
# Sun mass [kg]
M_EARTH = 5.97219e24
# Earth mass [kg]
R_MAX = 152097701000.0
# [m]
R_MIN = 147098074000.0
# [m]
V_MAX = 30287.0
# [m/s]

# Initial positions in internal units
LENGTH_UNIT = 4.0 * R_MAX
SUN_X_COORD = str(0.5)
SUN_Y_COORD = str(0.5)
EARTH_X_INI = str(float(SUN_X_COORD) + R_MAX / LENGTH_UNIT)
EARTH_Y_INI = SUN_Y_COORD

# Derived quantities
e = (R_MAX - R_MIN) / (R_MAX + R_MIN)
# Eccentricity [dimensionless]
b = np.sqrt(R_MAX * R_MIN)
# Semi-minor axis [m]
p = b * np.sqrt(1.0 - e * e)
# Semi-lactus rectum [m]
a = p / (1.0 - e * e)
# Semi-major axis [m]
T = np.sqrt(4.0 * np.pi * np.pi * a * a * a / (G_NEWTON * (M_SUN + M_EARTH)))
# Period [s] */

# Time-step size
dt = 1e-3 * T
# [s]
dt /= T
N = N_orbits / dt
GLOBAL_TIME_STEP_SIZE = str(dt)

# Initial velocity in internal units
EARTH_V_INI_X = str(0.0)
EARTH_V_INI_Y = str(V_MAX / LENGTH_UNIT * T)
#! [tutorials swift2 planet-orbit Define constants]


#! [tutorials swift2 planet-orbit Particle force]
###############################################################################
# Declare the force acting on the particle
###############################################################################
#
# We effectively solve a 2d setup even in 3d. Peano's vector classes can be
# used over the compiler symbol Dimensions, but then we have to use different
# initialisation lists - one with two entries and one if three - and
# distinguish them via an ifdef. If we compile without assertions, an
# additional entry in the initialisation list will simply be ignored. If we
# compile with assertions however the code will complain, as initialisation
# lists with "too many" entries are a hint that something in the code is
# wrong.
#
# We have two variants covered by this script: Either we work with only one
# particle or we resolve the sun explicitly. If we model the sun as explicit
# particle, then we have to be careful not to update it.
#
# The code snippets here rely on some global constants that we compute here
# in the Python code base. We have various ways how to deal with such
# constants:
#
# - We can calculate them within Python and then export them into the Peano
#   project. This is the strategy implemented here: The statements
#
#            peano4_project.constants.export_const_with_type
#
#   below make the constants known to the build environment. Consequently, we
#   can use them within our C code snippets, as they will be well-defined by
#   the time we compile.
# - We can construct the string in Python and replace the constants
#   straightaway. In this case, we don't have to add the constants to the
#   makefile, as they are not required throughout the compile. In return, we
#   have to ensure that the constants within the code snippet here are
#   replaced with their actual value before we hand the kernel particle_force
#   over.
#
###############################################################################
particle_force = """
::swift2::kernels::forAllLocalParticles(
  marker,
  localParticles,
  [=](
    const peano4::datamanagement::CellMarker& marker,
    globaldata::Planet&                       localParticle
  ) -> void {
    if ( ::swift2::kernels::localParticleCanBeUpdatedInCellKernel(
      marker,
      localParticle
    )) {
      #if Dimensions==2
      tarch::la::Vector<Dimensions,double> pos_sun = {SUN_X_COORD,SUN_Y_COORD};
      #else
      tarch::la::Vector<Dimensions,double> pos_sun = {SUN_X_COORD,SUN_Y_COORD,0.0};
      #endif

       /* G_NEWTON in internal units */
      double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
      double const T2 = PERIOD * PERIOD;
      double const GN = G_NEWTON * M_SUN * T2 / L3;
      //double GN = 1.0;

      tarch::la::Vector<Dimensions,double> earthSunDistance = localParticle.getX() - pos_sun;
      const double r = tarch::la::norm2(earthSunDistance);
      if (tarch::la::greater(r,0.0)) {
        const double r_inv =  1.0 / r;
        localParticle.setA( - GN * earthSunDistance * (r_inv * r_inv * r_inv) );

        /* Update Energy */
        const double vel = tarch::la::norm2( localParticle.getV() );
        const double E_kin = 0.5 * vel * vel;
        const double E_pot = - GN * r_inv;

        /* Update total energy (overall minus sign) */
        localParticle.setEnergyKin( E_kin );
        localParticle.setEnergyPot(-E_pot );
        localParticle.setEnergyTot( - ( E_kin + E_pot ) );
      } // end of radius check
    } // end of "may I update" check
  } // end of functor
);
"""

particle.cell_kernel = particle_force
#! [tutorials swift2 planet-orbit Particle force]


#! [tutorials swift2 planet-orbit Initial conditions]
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
  /*
   * In this problem we will initialize just a single particle (Earth)
   * This will orbit around a point where the hypothetical Sun is (no particle needed here)
   */

  /* Internal search radius used by Peano.
   * Should always be larger than actual SPH interaction radius (~2h).
   * But no really used here. */
  particle->setSearchRadius("""
    + str(search_radius)
    + """);
  particle->setV( {0, 0, 0} );
  particle->setA( {0, 0, 0} );

  // Orbiting particle: We can also have a free moving sun, but by default, this
  // one rests and the earth is actually orbiting.
  if( particle->getX(0) == EARTH_X_INI ){
    particle->setV(0, EARTH_V_INI_X) ;
    particle->setV(1, EARTH_V_INI_Y) ;
  }

  /*
   * Set initial energy
   */

  /* G_NEWTON in internal units */
  double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
  double const T2 = PERIOD * PERIOD;
  double const GN = G_NEWTON * M_SUN * T2 / L3;

  /* Sun-Earth initial distance */
  const double r_ini = particle->getX()(0) - SUN_X_COORD;
  const double r_inv = r_ini ? 1.0 / r_ini : 0.0;
  const double vel = tarch::la::norm2( particle->getV() );

  const double E_kin = 0.5 * vel * vel;
  const double E_pot = - GN * r_inv;

  particle->setEnergyKin( E_kin );
  particle->setEnergyPot(-E_pot );

  /* Overall minus sign for plotting */
  particle->setEnergyTot( -( E_kin + E_pot ) );

  logInfo( "Init()", "=====================================" );
  logInfo( "Init()", "Initial energy:");
  logInfo( "Init()", "Kinetic energy  :" << E_kin);
  logInfo( "Init()", "Potential energy:" << E_pot);
  logInfo( "Init()", "Total energy    :" << E_kin + E_pot);
  logInfo( "Init()", "Search Radius   :" << particle->getSearchRadius() );
  logInfo( "Init()", "Initial velocity:" << particle->getV() );
  logInfo( "Init()", "=====================================" );


"""
)

#
# Insert Earth into system
#
project.algorithm_step_initial_conditions.add_action_set(
    swift2.input.InsertParticlesByCoordinates(
        particle_set=particle_set,
        initialisation_call=initialisation_snippet,
        coordinates=[(EARTH_X_INI, EARTH_Y_INI, 0)],
    )
)
#! [tutorials swift2 planet-orbit Initial conditions]


#! [tutorials swift2 planet-orbit Plot]
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
particle_plotter.add_attribute_to_plot(energy_kin_attr, 1)
particle_plotter.add_attribute_to_plot(energy_pot_attr, 1)
particle_plotter.add_attribute_to_plot(energy_tot_attr, 1)
project.algorithm_step_plot.add_action_set(particle_plotter)
#! [tutorials swift2 planet-orbit Plot]


#! [tutorials swift2 planet-orbit Validation]
particle.add_to_reduction(
    f"""
nonCriticalAssertion( vertexdata::{particle.name}Set::getNumberOfRemainingLocalParticles() == 1 );
"""
)
#! [tutorials swift2 planet-orbit Validation]


###############################################################################
# Generate plain Peano 4 project
###############################################################################
#
# See commments above (for the force interaction kernel) why we might want to
# have some of the constants from the Python script within the C++ code base
# that's generated.
#
###############################################################################
#! [tutorials swift2 planet-orbit Create Peano project]
peano4_project = project.generate_Peano4_project(verbose=args.verbose)
#! [tutorials swift2 planet-orbit Create Peano project]

#! [tutorials swift2 planet-orbit Export constants]

# Export symbols and values for compilation (e.g. for SPH kernel)
# Dummy values for non SPH
peano4_project.output.makefile.add_CXX_flag("-D" + "HYDRO_DIMENSION=1")
peano4_project.output.makefile.add_CXX_flag("-D" + "HYDRO_GAMMA_5_3")
peano4_project.output.makefile.add_CXX_flag("-D" + "QUARTIC_SPLINE_KERNEL")

peano4_project.constants.export_const_with_type(
    "GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double"
)
peano4_project.constants.export_const_with_type("G_NEWTON", str(G_NEWTON), "double")
peano4_project.constants.export_const_with_type(
    "LENGTH_UNIT", str(LENGTH_UNIT), "double"
)
peano4_project.constants.export_const_with_type("M_SUN", str(M_SUN), "double")
peano4_project.constants.export_const_with_type("SUN_X_COORD", SUN_X_COORD, "double")
peano4_project.constants.export_const_with_type("SUN_Y_COORD", SUN_Y_COORD, "double")
peano4_project.constants.export_const_with_type("EARTH_X_INI", EARTH_X_INI, "double")
peano4_project.constants.export_const_with_type("EARTH_Y_INI", EARTH_Y_INI, "double")
peano4_project.constants.export_const_with_type(
    "EARTH_V_INI_X", EARTH_V_INI_X, "double"
)
peano4_project.constants.export_const_with_type(
    "EARTH_V_INI_Y", EARTH_V_INI_Y, "double"
)
peano4_project.constants.export_const_with_type("ECCENTRICITY", str(e), "double")
peano4_project.constants.export_const_with_type("PERIOD", str(T), "double")
peano4_project.constants.export_const_with_type("R_MIN", str(R_MIN), "double")
peano4_project.constants.export_const_with_type("R_MAX", str(R_MAX), "double")
#! [tutorials swift2 planet-orbit Export constants]

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
