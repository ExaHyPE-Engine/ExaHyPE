#!/usr/bin/env python
# coding: utf-8

# Test the time integrator (KDK leapfrog) via a Sun-Earth simulation 

import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2
import numpy as np

# SPH compute kernels library
import peano4.toolbox.particles.sph.computeKernels

# To execute shell commands via python
import subprocess
import argparse

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='SPH benchmarking script')
parser.add_argument("-m",     "--mode",               dest="mode",                         default="release",           help="|".join(modes.keys()) )
parser.add_argument("-c",     "--clean",              dest="clean_temporary_directories",  action="store_true",         help="Remove temporary directories before script starts" )
parser.add_argument("-p",     "--plot",               dest="plot",                         action="store_true",        help="Plot result" )
parser.add_argument("-r",     "--cut-off",            dest="cut_off_radius",               default=0.1,                 help="Radius guiding AMR around particle" )
args = parser.parse_args()

if args.mode not in modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ",join(modes.keys()[:-1]),modes.keys()[-1],args.mode))
    import sys
    sys.exit(1)

print( "clean directories: {}".format(args.clean_temporary_directories) )


# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
  bashCommand = 'make clean; rm -r actions/ vertexdata/ repositories/ globaldata/ celldata/ observers/'
  subprocess.check_output(['bash','-c', bashCommand])

project = peano4.Project( ["examples", "particles"], "test_Time_Integration", "." )


# ## Prepare code
# 
# Peano's API does not know which settings to use on the present system. To make it copy/clone the settings identified by ./configure, we ask it to parse the generated configuration scripts. 

build_mode = modes[args.mode]
project.output.makefile.parse_configure_script_outcome( "../../.." )
project.output.makefile.set_mode( build_mode )

# Define particle and its properties
particle = peano4.toolbox.particles.Particle("Particle")

# Define properties
v_attr      = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr      = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
v_full_attr = peano4.dastgen2.Peano4DoubleArray("v_full","Dimensions")

has_been_kicked_attr = dastgen2.attributes.Boolean("hasBeenKicked")
energy_kin_attr = dastgen2.attributes.Double("energyKin")
energy_pot_attr = dastgen2.attributes.Double("energyPot")
energy_tot_attr = dastgen2.attributes.Double("energyTot")

# Dummy properties (not relevant)
density_attr         = dastgen2.attributes.Double("density")
pressure_attr        = dastgen2.attributes.Double("pressure")
is_boundary_part_attr= dastgen2.attributes.Boolean("isBoundaryParticle")
u_attr               = dastgen2.attributes.Double("u")
v_sig_AV_attr        = dastgen2.attributes.Double("v_sig_AV")
uDot_attr            = dastgen2.attributes.Double("uDot")
u_full_attr          = dastgen2.attributes.Double("u_full")
smoothL_attr         = dastgen2.attributes.Double("smoothingLength")
smoothL_prev_attr    = dastgen2.attributes.Double("smoothingLengthPrevious")
hleft_attr           = dastgen2.attributes.Double("hLeft")
hright_attr          = dastgen2.attributes.Double("hRight")
iteration_count_attr = dastgen2.attributes.Integer("iterCount")
has_converged_attr   = dastgen2.attributes.Boolean("hasConverged")
soundSpeed_attr      = dastgen2.attributes.Double ("soundSpeed")
a_prev_step_attr     = peano4.dastgen2.Peano4DoubleArray("a_prev_step","Dimensions") 

# Move state flags ------------------------------------------------------------
particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )

# Add particle properties to data model
particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(v_full_attr)
particle.data.add_attribute(has_been_kicked_attr)
particle.data.add_attribute(energy_kin_attr)
particle.data.add_attribute(energy_pot_attr)
particle.data.add_attribute(energy_tot_attr)

# Dummy attrs
particle.data.add_attribute(density_attr)
particle.data.add_attribute(pressure_attr)
particle.data.add_attribute(is_boundary_part_attr)
particle.data.add_attribute(a_prev_step_attr)
particle.data.add_attribute(u_attr)
particle.data.add_attribute(uDot_attr)
particle.data.add_attribute(u_full_attr)
particle.data.add_attribute(v_sig_AV_attr)
particle.data.add_attribute(smoothL_attr)
particle.data.add_attribute(smoothL_prev_attr)
particle.data.add_attribute(hleft_attr)
particle.data.add_attribute(hright_attr)
particle.data.add_attribute(has_converged_attr)
particle.data.add_attribute(iteration_count_attr)
particle.data.add_attribute(soundSpeed_attr)

# Add particle to project.
particles = peano4.toolbox.particles.ParticleSet(particle)
project.datamodel.add_global_object(particle)
project.datamodel.add_vertex       (particles)

# ## Export global constant and parameters
Dimensions = 2
project.output.makefile.set_dimension(Dimensions)

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
SUN_Y_COORD = str(0.55)
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

project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
project.constants.export_const_with_type("G_NEWTON"   , str(G_NEWTON)   , "double")
project.constants.export_const_with_type("LENGTH_UNIT", str(LENGTH_UNIT), "double")
project.constants.export_const_with_type("M_SUN"      , str(M_SUN), "double")
project.constants.export_const_with_type("SUN_X_COORD", SUN_X_COORD, "double")
project.constants.export_const_with_type("SUN_Y_COORD", SUN_Y_COORD, "double")
project.constants.export_const_with_type("EARTH_X_INI", EARTH_X_INI, "double")
project.constants.export_const_with_type("EARTH_Y_INI", EARTH_Y_INI, "double")
project.constants.export_const_with_type("EARTH_V_INI_X", EARTH_V_INI_X, "double")
project.constants.export_const_with_type("EARTH_V_INI_Y", EARTH_V_INI_Y, "double")
project.constants.export_const_with_type("ECCENTRICITY", str(e), "double")
project.constants.export_const_with_type("PERIOD", str(T), "double")
project.constants.export_const_with_type("R_MIN", str(R_MIN), "double")
project.constants.export_const_with_type("R_MAX", str(R_MAX), "double")
if args.plot:
  project.constants.export_const_with_type("PLOT", "true", "bool")
else:
  project.constants.export_const_with_type("PLOT", "false", "bool")

# Dummy 
HYDRO_PART_NUMBER = str(1000);
GAMMA = str( 5./3. )
HYDRO_h_MAX = str( 1.0 / 3.0 / float(2.0) ) 
HYDRO_h_MIN = "1e-6"
project.constants.export_const_with_type("HYDRO_PART_NUMBER", HYDRO_PART_NUMBER   , "double")
project.constants.export_const_with_type("GAMMA"            , GAMMA               , "double")
project.constants.export_const_with_type("HYDRO_h_MAX"      , HYDRO_h_MAX         , "double")
project.constants.export_const_with_type("HYDRO_h_MIN"      , HYDRO_h_MIN         , "double")

################################################################################################
###############################  Model the setup phases  #######################################
################################################################################################

# Initialize regular grid

mesh_size = 0.5
particle_tree_analysis = peano4.toolbox.particles.ParticleTreeAnalysis(particles)
project.datamodel.add_cell(particle_tree_analysis.cell_marker)   # read docu of ParticleTreeAnalysis

create_grid = peano4.solversteps.Step( name="CreateGrid", add_user_defined_actions=False )
create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(mesh_size) )
create_grid.use_vertex(particles)
create_grid.use_cell(particle_tree_analysis.cell_marker)
project.solversteps.add_step(create_grid)

# ## Initial conditions for particles

initialisation_snippet = """

  /*
   * In this problem we will initialize just a single particle (Earth)
   * This will orbit around a point where the hypothetical Sun is (no particle needed here)
   */

  /* Internal cutoff radious used by Peano. 
   * Should always be larger than actual SPH cutoff radious (2h). 
   * But no really used here. */
  particle->setCutOffRadius(""" + str(args.cut_off_radius) + """);

  /* Select the initialized particle (Earth)
   * and set its initial conditions */

  if( particle->getX(0) > 0.55 ){
    particle->setX(0, EARTH_X_INI );
    particle->setX(1, EARTH_Y_INI );
    particle->setV(0, EARTH_V_INI_X) ;
    particle->setV(1, EARTH_V_INI_Y) ;
  } else {
    particle->setX(0, SUN_X_COORD);
    particle->setX(1, SUN_Y_COORD);
    particle->setV(0, 0);
    particle->setV(1, 0);
  }

  particle->setA(0,0);
  particle->setA(1,0);

  particle->setHasBeenKicked( false );
  
  /* Set initial energy */
  /* G_NEWTON in internal units */
  double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
  double const T2 = PERIOD * PERIOD;
  double const GN = G_NEWTON * M_SUN * T2 / L3;

  /* Sun-Earth initial distance */
  const double r_ini = particle->getX()(0) - SUN_X_COORD;
  const double r_inv = r_ini ? 1.0 / r_ini : 0.0;     

  const double vel = tarch::la::norm2( particle->getV() );
//  const double E_kin = 0.5 * vel;
  const double E_kin = 0.5 * vel * vel;

  const double E_pot = - GN * r_inv;

  particle->setEnergyKin( E_kin );
  particle->setEnergyPot(-E_pot );
  
  /* Overall minus sign for plotting */
  particle->setEnergyTot( -( E_kin + E_pot ) );

  /* Initialise the "extended" arrays */
  particle->setV_full(particle->getV());

  /* Initialise other dummy properties */
  particle->setDensity(1.0);
  particle->setPressure(1e-6);
  particle->setIsBoundaryParticle( false );
  particle->setA_prev_step(0.0);
  particle->setU(1e-6);
  particle->setUDot(0.0);
  particle->setU_full(0.0);
  particle->setV_sig_AV(0.0);
  particle->setSmoothingLength(0.0);
  particle->setSmoothingLengthPrevious(0.0);
  particle->setHLeft (0.0);
  particle->setHRight(0.0);
  particle->setIterCount(0);
  particle->setHasConverged( false );
  particle->setSoundSpeed(1e-6);

  logInfo( "Init()", " =====================================" );
  logInfo( "Init()", "Initial energy:");
  logInfo( "Init()", "Kinetic energy  :" << E_kin);
  logInfo( "Init()", "Potential energy:" << E_pot);
  logInfo( "Init()", "Total energy    :" << E_kin + E_pot);
  logInfo( "Init()", " =====================================" );

"""

init_setup = peano4.solversteps.Step( name="Init", add_user_defined_actions=False )
init_setup.use_vertex(particles)
init_setup.use_cell(particle_tree_analysis.cell_marker)
init_setup.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_setup.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
init_setup.add_action_set( peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells(
  particle_set=particles,
#  distance_between_particles=0.4,               # this will initialize just one particle in the domain at (0,0)
  distance_between_particles=0.45,               # this will initialize just one particle in the domain at (0,0)
  initialisation_call=initialisation_snippet,
  noise=False,
  additional_includes="""
    #include "Constants.h"
    """,
  computational_domain_offset=[0.5, 0.55])
  )

init_setup.add_action_set( particle_tree_analysis )
project.solversteps.add_step(init_setup)

# Force calculation

force_gravity_particles = peano4.solversteps.Step( "ForceGravity", add_user_defined_actions=False )
force_gravity_particles.use_vertex(particles)
force_gravity_particles.use_cell(particle_tree_analysis.cell_marker)
force_gravity_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""

//    /* Here we treat the Sun-Earth interaction in terms of the local/active sets */
//
//    /* Sun-Earth distance */
//    tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - activeParticle->getX();
//    const double r = tarch::la::norm2(dx);
//
//    /*
//     * Calculate the gravitational acceleration on the Earth particle
//     */
//
//    /* G_NEWTON in internal units */
//    double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
//    double const T2 = PERIOD * PERIOD;
//    double const GN = G_NEWTON * M_SUN * T2 / L3;
//
//    if( tarch::la::greaterEquals(r, 1e-1)
//    and localParticle->getX()(0) != SUN_X_COORD
//    and localParticle->getX()(1) != SUN_Y_COORD
//    ) {
//
//      /* Set gravitational acceleration */
//      localParticle->setA( - 1e0 * GN * dx / (r * r * r) );
//
//    }
//
//    assertion1(  std::isfinite(localParticle->getA(0)), localParticle->toString() );
//    assertion1(  std::isfinite(localParticle->getA(1)), localParticle->toString() );


""",

  touch_vertex_first_time_compute_particle_update_kernel="""

  // No action here

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

   if(localParticle->getParallelState() == globaldata::Particle::ParallelState::Local){

    /* Here we only consider the Earth. The Sun particle is not used */

   /*
    * Calculate the gravitational acceleration on the Earth particle
    */

    /* Sun location */
    tarch::la::Vector<Dimensions,double> pos_sun(0.0);
    pos_sun[0] = SUN_X_COORD;
    pos_sun[1] = SUN_Y_COORD;

    /* G_NEWTON in internal units */
    double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
    double const T2 = PERIOD * PERIOD;
    double const GN = G_NEWTON * M_SUN * T2 / L3;

    /* Sun-Earth distance */
    tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - pos_sun;
    const double r = tarch::la::norm2(dx);
    const double r_inv = r ? 1.0 / r : 0.0;     

    /* Only apply force on orbiting particle */
    if( tarch::la::greaterEquals( r, 0.5 * R_MIN / LENGTH_UNIT ) ) {

      /* Gravitational acceleration */
      localParticle->setA( - GN * dx * (r_inv * r_inv * r_inv) );

    }

    /* Update Energy */
    const double vel = tarch::la::norm2( localParticle->getV_full() );
//    const double E_kin = 0.5 * vel;
    const double E_kin = 0.5 * vel * vel;

    const double E_pot = - GN * r_inv;

    /* Update total energy (overall minus sign) */
    localParticle->setEnergyKin( E_kin );
    localParticle->setEnergyPot(-E_pot );
    localParticle->setEnergyTot( - ( E_kin + E_pot ) );

    if( tarch::la::greaterEquals( r, 0.5 * R_MIN / LENGTH_UNIT ) ) {
      logInfo( "VertexTouchLast()", " =====================================" );
      logInfo( "VertexTouchLast()", "Energy of orbiting particle:");
      logInfo( "VertexTouchLast()", "Kinetic energy  :" << E_kin);
      logInfo( "VertexTouchLast()", "Potential energy:" << E_pot);
      logInfo( "VertexTouchLast()", "Total energy    :" << E_kin + E_pot);
      logInfo( "VertexTouchLast()", " =====================================" );
    }

    assertion1(  std::isfinite(localParticle->getA(0)), localParticle->toString() );
    assertion1(  std::isfinite(localParticle->getA(1)), localParticle->toString() );

   }

""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
force_gravity_particles.add_action_set( particle_tree_analysis )
force_gravity_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
force_gravity_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(force_gravity_particles)


# -----------------------------------------------------------------------------
# Time integration steps (leapfrog)
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Kick 1
# -----------------------------------------------------------------------------
kick1_particles = peano4.solversteps.Step( "Kick1", add_user_defined_actions=False )
kick1_particles.use_vertex(particles)
kick1_particles.use_cell(particle_tree_analysis.cell_marker)

# Kick 1 step from SPH library
Kick1 = peano4.toolbox.particles.sph.computeKernels.LeapfrogKick( kickNumber=1 )

particle_particle_compute_kernel = Kick1.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Kick1.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Kick1.touch_particle_last_time_compute_kernel

# Add step to project
kick1_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes = """
#include "Constants.h"
#include <cmath>
"""
  ))
kick1_particles.add_action_set( particle_tree_analysis )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick1_particles)


# -----------------------------------------------------------------------------
# Kick 2
# -----------------------------------------------------------------------------
kick2_particles = peano4.solversteps.Step( "Kick2", add_user_defined_actions=False )
kick2_particles.use_vertex(particles)
kick2_particles.use_cell(particle_tree_analysis.cell_marker)

# Kick 2 step from SPH library
Kick2 = peano4.toolbox.particles.sph.computeKernels.LeapfrogKick( kickNumber=2 )

particle_particle_compute_kernel = Kick2.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Kick2.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Kick2.touch_particle_last_time_compute_kernel

# Add step to project
kick2_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include "equation_of_state.h"
#include <cmath>
"""
  ))
kick2_particles.add_action_set( particle_tree_analysis )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick2_particles)


# -----------------------------------------------------------------------------
# Drift step (+prediction)
# -----------------------------------------------------------------------------
drift_particles = peano4.solversteps.Step( "Drift", add_user_defined_actions=False )
drift_particles.use_vertex(particles)
drift_particles.use_cell(particle_tree_analysis.cell_marker)

# Drift step from SPH library
Drift = peano4.toolbox.particles.sph.computeKernels.LeapfrogDrift()

particle_particle_compute_kernel = Drift.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Drift.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Drift.touch_particle_last_time_compute_kernel

# Add step to project
drift_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes = """
#include "Constants.h"
#include "equation_of_state.h"
#include <cmath>
"""
  ))
drift_particles.add_action_set( particle_tree_analysis )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(drift_particles)


# -----------------------------------------------------------------------------
# Initialize some variables for the current time-step
# -----------------------------------------------------------------------------
init_time_step = peano4.solversteps.Step( "InitTimeStep", add_user_defined_actions=False )
init_time_step.use_vertex(particles)
init_time_step.use_cell(particle_tree_analysis.cell_marker)

# InitTimeStep step from SPH library
InitTimeStep = peano4.toolbox.particles.sph.computeKernels.InitializeTimeStep()

particle_particle_compute_kernel = InitTimeStep.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= InitTimeStep.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = InitTimeStep.touch_particle_last_time_compute_kernel

# Add action to project
init_time_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
init_time_step.add_action_set( particle_tree_analysis )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(init_time_step)


# ------------------------------------------------------------------------------
# Plot setup 
# ------------------------------------------------------------------------------

# Particle plotter
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particles, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(v_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(a_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(energy_kin_attr  , 1)
particle_plotter.add_attribute_to_plot(energy_pot_attr  , 1)
particle_plotter.add_attribute_to_plot(energy_tot_attr  , 1)

# Solution plotter
print_solution = peano4.solversteps.Step( "Plot", add_user_defined_actions=False )
print_solution.use_vertex(particles)
print_solution.use_cell(particle_tree_analysis.cell_marker)
print_solution.add_action_set( particle_tree_analysis )
print_solution.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
print_solution.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
print_solution.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( filename="grid", time_stamp_evaluation=peano4.toolbox.PlotGridInPeanoBlockFormat.CountTimeSteps ) )
print_solution.add_action_set( particle_plotter )

project.solversteps.add_step(print_solution)

# ## Generate the actual C++ code
project.generate()
# ## Compile the code
# We can do this here or in the terminal.
project.build()

# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)


