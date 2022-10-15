import os
import argparse
import numpy as np

import peano4
import swift2
import dastgen2



########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../..", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-et",  "--end-time",          dest="end_time",       type=float, default=1.0, help="End of simulation" )
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",      type=float, default=0.3, help="Cell size (default 0.3)" )
parser.add_argument("-d",   "--dimensions",        dest="dimensions",     type=int,   default=2,   help="Spatial dimensions (default 2)" )
parser.add_argument("-asserts",  "--asserts",      dest="asserts",        action="store_true", default=False, help="Switch on assertions" )
args = parser.parse_args()


########################################################################################
# Generate SWIFT project
########################################################################################

dimensions  = args.dimensions

project = swift2.Project( namespace=["benchmarks", "swift2", "planetorbit"], 
                          project_name="planetorbit", 
                          executable="orbit-" + str(args.cell_size) )


# Declare the particle type
particle   = swift2.particle.ExplicitEulerFixedInteractionRadius( name                   = "Planet", 
                                                                  cfl_factor             = 0.01, 
                                                                  initial_time_step_size = 1e-5 )


if args.asserts:
  build_mode = peano4.output.CompileMode.Asserts
else:
  build_mode = peano4.output.CompileMode.Release
    
offset      = [ 0, 0, 0 ] 
domain_size = [ 1, 1, 1 ]
periodic_boundary_conditions = [False,False,False]          # Periodic BC


project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  0.1,     0.001,
  args.end_time,
  0.0, 
  args.end_time/20,                      #args.plot_step_size,   # snapshots
  periodic_boundary_conditions,
  8  # plotter precision
)

# additional global constants (to be exported down below)

# In order to make quantities dimensionless, we will use
# T: Period of orbit [s]
# L=4*R_MAX: 4 * maximum distance
# M_SUN: Sun mass [kg]

# Orbital parameters
N_orbits= 8;          # Number of orbits
G_NEWTON= 6.67384e-11; # Newton's constant  [m^3/kg/s^2]
M_SUN   = 1.9885e30;   # Sun mass [kg] 
M_EARTH = 5.97219e24;  # Earth mass [kg]
R_MAX = 152097701000.; # [m] 
R_MIN = 147098074000.; # [m] 
V_MAX = 30287.0;       # [m/s]

# Initial positions in internal units
LENGTH_UNIT = 4.0 * R_MAX
SUN_X_COORD = str(0.5)
SUN_Y_COORD = str(0.5)
EARTH_X_INI = str( float(SUN_X_COORD) + R_MAX / LENGTH_UNIT )
EARTH_Y_INI = SUN_Y_COORD

# Derived quantities 
e = (R_MAX - R_MIN) / (R_MAX + R_MIN);   # Eccentricity [dimensionless]
b = np.sqrt(R_MAX * R_MIN);              # Semi-minor axis [m]
p = b * np.sqrt(1.0 - e * e);            # Semi-lactus rectum [m]
a = p / (1.0 - e * e);                   # Semi-major axis [m]
T = np.sqrt( 4.0 * np.pi * np.pi * a * a * a /
                  (G_NEWTON * (M_SUN + M_EARTH)) ); # Period [s] */

# Time-step size 
dt = 1e-3 * T; # [s]
dt/= T;
N = N_orbits / dt;
GLOBAL_TIME_STEP_SIZE = str(dt)

# Initial velocity in internal units
EARTH_V_INI_X = str(0.0)
EARTH_V_INI_Y = str(V_MAX / LENGTH_UNIT * T)


project.set_Peano4_installation(args.peanodir, build_mode)

project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision", "new toolbox::loadbalancing::RecursiveSubdivision::DefaultConfiguration()")


#
# Constants don't work everywhere
# 1 vs 2 particles
# parallel does not work
# Clear flag
#


########################################################################################
# Declare the force acting on the particle
########################################################################################

# @TODO include Constants.h in the generated code

particle_force = """
  /* Sun location */
  #if Dimensions==2
//  tarch::la::Vector<Dimensions,double> pos_sun = {SUN_X_COORD,SUN_Y_COORD,0};
  tarch::la::Vector<Dimensions,double> pos_sun = {0.5,0.5};
  #else
  tarch::la::Vector<Dimensions,double> pos_sun = {0.5,0.5,0.0};
  #endif

  //  /* G_NEWTON in internal units */
  //  double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
  // double const T2 = PERIOD * PERIOD;
  //  double const GN = G_NEWTON * M_SUN * T2 / L3;
  double GN = 1.0;

  /* Sun-Earth distance */
  tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - pos_sun;
  const double r = tarch::la::norm2(dx);
  const double r_inv = r ? 1.0 / r : 0.0;     
  localParticle->setA( - GN * dx * (r_inv * r_inv * r_inv) );
"""
particle.particle_acceleration = particle_force

# Add particle to the SWIFT project
particle_set = project.add_particle_species( particle )

########################################################################################
# Initialisation snippet for the particle
########################################################################################

cut_off_radius = 0.1
initialisation_snippet = """
  /*
   * In this problem we will initialize just a single particle (Earth)
   * This will orbit around a point where the hypothetical Sun is (no particle needed here)
   */

  /* Internal cutoff radious used by Peano. 
   * Should always be larger than actual SPH cutoff radious (2h). 
   * But no really used here. */
  particle->setCutOffRadius(""" + str(cut_off_radius) + """);
  particle->setV( {0, 0, 0} );
  particle->setA( {0, 0, 0} );

  // Orbiting particle: We can also have a free moving sun, but by default, this
  // one rests and the earth is actually orbiting.
  if( particle->getX(0) == EARTH_X_INI ){
    particle->setV(0, EARTH_V_INI_X) ;
    particle->setV(1, EARTH_V_INI_Y) ;
  } 


"""

#
# I either insert sun and earth or only the earth
#
project.algorithm_step_init.add_action_set( peano4.toolbox.particles.InsertParticlesbyCoordinates(
  particle_set         = particle_set,
  initialisation_call  = initialisation_snippet,
  coordinates          = [ (EARTH_X_INI,EARTH_Y_INI,0) ] )
  #coordinates          = [ (EARTH_X_INI,EARTH_Y_INI,0), (SUN_X_COORD,SUN_Y_COORD,0) ] )
  )

########################################################################################
# Set up the plotter
########################################################################################
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particle_set, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(particle.velocity, dimensions)
#particle_plotter.add_attribute_to_plot(a_attr       , Dimensions)
#particle_plotter.add_attribute_to_plot(energy_kin_attr  , 1)
#particle_plotter.add_attribute_to_plot(energy_pot_attr  , 1)
#particle_plotter.add_attribute_to_plot(energy_tot_attr  , 1)
project.algorithm_step_plot.add_action_set( particle_plotter )


########################################################################################
# Generate plain Peano 4 project
########################################################################################
peano4_project = project.generate_Peano4_project(verbose=args.verbose)


# Export additional constants
peano4_project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
peano4_project.constants.export_const_with_type("G_NEWTON"   , str(G_NEWTON)   , "double")
peano4_project.constants.export_const_with_type("LENGTH_UNIT", str(LENGTH_UNIT), "double")
peano4_project.constants.export_const_with_type("M_SUN"      , str(M_SUN), "double")
peano4_project.constants.export_const_with_type("SUN_X_COORD", SUN_X_COORD, "double")
peano4_project.constants.export_const_with_type("SUN_Y_COORD", SUN_Y_COORD, "double")
peano4_project.constants.export_const_with_type("EARTH_X_INI", EARTH_X_INI, "double")
peano4_project.constants.export_const_with_type("EARTH_Y_INI", EARTH_Y_INI, "double")
peano4_project.constants.export_const_with_type("EARTH_V_INI_X", EARTH_V_INI_X, "double")
peano4_project.constants.export_const_with_type("EARTH_V_INI_Y", EARTH_V_INI_Y, "double")
peano4_project.constants.export_const_with_type("ECCENTRICITY", str(e), "double")
peano4_project.constants.export_const_with_type("PERIOD", str(T), "double")
peano4_project.constants.export_const_with_type("R_MIN", str(R_MIN), "double")
peano4_project.constants.export_const_with_type("R_MAX", str(R_MAX), "double")


#peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
#peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )
#peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )


peano4_project.generate( throw_away_data_after_generation=False )
peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )

