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

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-m",     "--mode",               dest="mode",                         default="release",           help="|".join(modes.keys()) )
parser.add_argument("-c",     "--clean",              dest="clean_temporary_directories",  action="store_true",         help="Remove temporary directories before script starts" )
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

# ## Model the particle
# 
# - Velocity array ${\bf v}\equiv{\rm d}{\bf x}/{\rm d}t$
# - Acceleration array ${\bf a}\equiv{\rm d}{\bf v}/{\rm d}t$
# - Velocity array ${\bf v}_{\rm full}$ (not accessed by SPH kernels)

# Extras
# - ${\rm has\ been\ kicked}$
# 

particle = peano4.toolbox.particles.Particle("Particle")

# Define properties
v_attr = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
v_full_attr = peano4.dastgen2.Peano4DoubleArray("v_full","Dimensions")

has_been_kicked_attr = dastgen2.attributes.Boolean("hasBeenKicked")
energy_kin_attr = dastgen2.attributes.Double("energyKin")
energy_pot_attr = dastgen2.attributes.Double("energyPot")
energy_tot_attr = dastgen2.attributes.Double("energyTot")

# Add particle properties
particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(v_full_attr)
particle.data.add_attribute(has_been_kicked_attr)
particle.data.add_attribute(energy_kin_attr)
particle.data.add_attribute(energy_pot_attr)
particle.data.add_attribute(energy_tot_attr)

particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )

particles = peano4.toolbox.particles.ParticleSet(particle)

# This particle has to be added to the project. This way the project knows that we are working with this guy. We associate the particle with the vertices, as we work with the pidt (particle in dual tree) algorithm. 

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
EARTH_X_INI = str(float(SUN_X_COORD) + R_MAX / LENGTH_UNIT)
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

  particle->setHasBeenKicked(false);
  
  /* Set initial energy */
  /* G_NEWTON in internal units */
  double const L3 = LENGTH_UNIT * LENGTH_UNIT * LENGTH_UNIT;
  double const T2 = PERIOD * PERIOD;
  double const GN = G_NEWTON * M_SUN * T2 / L3;

  /* Sun-Earth initial distance */
  const double r_ini = particle->getX()(0) - SUN_X_COORD;
  const double r_inv = r_ini ? 1.0 / r_ini : 0.0;     
  particle->setEnergyKin( 0.5 * tarch::la::norm2(particle->getV()) );
  particle->setEnergyPot( - GN * r_inv );
  
  /* Overall minus sign for plotting */
  particle->setEnergyTot( -( particle->getEnergyKin() + particle->getEnergyPot() ) );

  /* Initialise the "extended" arrays */
  particle->setV_full(particle->getV());

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

  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

  }

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

   if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

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

    if( tarch::la::greaterEquals( r, 0.5 * R_MIN / LENGTH_UNIT ) ) {

      /* Gravitational acceleration */
      localParticle->setA( - GN * dx * (r_inv * r_inv * r_inv) );

    }

    /* Update potential Energy */
    const double E_pot = - GN * r_inv;
    localParticle->setEnergyPot( E_pot );

    assertion1(  std::isfinite(localParticle->getA(0)), localParticle->toString() );
    assertion1(  std::isfinite(localParticle->getA(1)), localParticle->toString() );

   }

""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#force_gravity_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
force_gravity_particles.add_action_set( particle_tree_analysis )
force_gravity_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
force_gravity_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(force_gravity_particles)


# ## KDK/leapfrog steps
# 
# In the KDK (leapfrog) scheme, we update positions and velocities according to
# 
# \begin{align}
# {\bf v}^{n+\frac{1}{2}} &= {\bf v}^{n} + {\bf a}^{n}\frac{\Delta t}{2}\\
# {\bf x}^{n+1} &= {\bf x}^{n} + {\bf v}^{n+\frac{1}{2}}{\Delta t}\\
# {\bf v}^{n+1} &= {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta t}{2}\\
# \end{align}
#  
# Notice that the last kick in this sequence is implicit if ${\bf a}$ depends on ${\bf v}$, which requires extra treatments (see Price et al.).
# 

# ## Kick 1

kick1_particles = peano4.solversteps.Step( "Kick1", add_user_defined_actions=False )
kick1_particles.use_vertex(particles)
kick1_particles.use_cell(particle_tree_analysis.cell_marker)
kick1_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_first_time_compute_particle_update_kernel="""

    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

      /* Prepare the particle for kick */
      localParticle->setHasBeenKicked(false);

    }

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

  const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;

  if(
      not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    ){

    /* Kick the particle */
    localParticle->setV_full( localParticle->getV_full() + dt_kick_hydro * localParticle->getA() );

    assertion1(  std::isfinite(localParticle->getV(0)), localParticle->toString() );
    assertion1(  std::isfinite(localParticle->getV(1)), localParticle->toString() );

    localParticle->setHasBeenKicked(true);

    }

""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#kick_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
kick1_particles.add_action_set( particle_tree_analysis )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick1_particles)


# ## Kick 2
# 
# Same as kick 1 but does not update the accelation array of previous time step

kick2_particles = peano4.solversteps.Step( "Kick2", add_user_defined_actions=False )
kick2_particles.use_vertex(particles)
kick2_particles.use_cell(particle_tree_analysis.cell_marker)
kick2_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_first_time_compute_particle_update_kernel="""

    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

      /* Prepare the particle for kick */
      localParticle->setHasBeenKicked(false);
    }

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

  const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;

  if (not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

    /* Kick the particle */
    localParticle->setV_full( localParticle->getV_full() + dt_kick_hydro * localParticle->getA() );

    assertion1(  std::isfinite(localParticle->getV(0)), localParticle->toString() );
    assertion1(  std::isfinite(localParticle->getV(1)), localParticle->toString() );

    localParticle->setHasBeenKicked(true);

    /*
     * Reset predicted values for next Drift step 
     * Done by "hydro_reset_predicted_values" in SWIFT
     */

    /* Re-set the predicted velocities */
    localParticle->setV( localParticle->getV_full() );

    /* Update potential Energy */
    const double E_kin = 0.5 * tarch::la::norm2( localParticle->getV() );
    localParticle->setEnergyKin( E_kin);

    /* Update total energy (overall minus sign) */
    localParticle->setEnergyTot( - ( E_kin + localParticle->getEnergyPot() ) );

  }

""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#kick2_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
kick2_particles.add_action_set( particle_tree_analysis )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick2_particles)


# ## Drift step (+prediction)
# 
# In this step, we update the particle's position via
# 
# \begin{align}
# {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t}
# \end{align}
# 
# We also need to get predicted values of some quantities.

drift_particles = peano4.solversteps.Step( "Drift", add_user_defined_actions=False )
drift_particles.use_vertex(particles)
drift_particles.use_cell(particle_tree_analysis.cell_marker)
drift_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_last_time_compute_particle_update_kernel = """
  const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
  
  if (
    localParticle->getMoveState()!=globaldata::Particle::MoveState::Moved
    and
    localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    ){
  
    // Drift the particle (update position for a full time-step) 
    localParticle->setX( localParticle->getX() + timeStepSize * localParticle->getV_full() );
    
    localParticle->setMoveState( globaldata::Particle::MoveState::Moved );

    }
""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#drift_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
drift_particles.add_action_set( particle_tree_analysis )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(drift_particles)


# ## Initialize some variables for the current time-step

init_time_step = peano4.solversteps.Step( "InitTimeStep", add_user_defined_actions=False )
init_time_step.use_vertex(particles)
init_time_step.use_cell(particle_tree_analysis.cell_marker)
init_time_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_first_time_compute_particle_update_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

  if( localParticle->getParallelState()==globaldata::Particle::ParallelState::Local ){

    /* Also prepare particle for Drift step */
    localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );

  }

""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#init_time_step.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
init_time_step.add_action_set( particle_tree_analysis )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(init_time_step)


# ## Impose Boundary Conditions
# After the final velocity has been calculated (and the particle already been drifted), we need to impose the boundary conditions. In this case we use reflecting ones.

impose_BC_particles = peano4.solversteps.Step( "ImposeBC", add_user_defined_actions=False )
impose_BC_particles.use_vertex(particles)
impose_BC_particles.use_cell(particle_tree_analysis.cell_marker)
impose_BC_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
   if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
     localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
   }

""",
  touch_vertex_last_time_compute_particle_update_kernel="""
    
  if (
  localParticle->getMoveState()!=globaldata::Particle::MoveState::Moved
  and
  localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
  ){
  
    for (int d=0; d<Dimensions; d++) {
      if ( localParticle->getX()(d)<0.0 ) {

        /* Reflecting BC */   
        localParticle->setX     ( d, std::abs(localParticle->getX     ()(d)) );
        localParticle->setV     ( d, std::abs(localParticle->getV_full()(d)) );
        localParticle->setV_full( d, std::abs(localParticle->getV_full()(d)) );

      }
      if ( localParticle->getX()(d)>1.0 ) {

        /* Reflecting BC */   
        localParticle->setX     ( d,    2.0 - localParticle->getX     ()(d)  );
        localParticle->setV     ( d,-std::abs(localParticle->getV     ()(d)) );
        localParticle->setV_full( d,-std::abs(localParticle->getV_full()(d)) );

      }
    
      localParticle->setMoveState( globaldata::Particle::MoveState::Moved );
      
      } /* End for dimensions */
      
    }
    
""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
impose_BC_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=0,max_particles_per_cell=1) )
impose_BC_particles.add_action_set( particle_tree_analysis )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(impose_BC_particles)

# ## For Debugging: collapse problem into 1D
# 
# This is an initial, extra step to move particles into a straight line when the simulation starts.

rearrange_particles = peano4.solversteps.Step( "RearrangeAtStart", add_user_defined_actions=False )
rearrange_particles.use_vertex(particles)
rearrange_particles.use_cell(particle_tree_analysis.cell_marker)
rearrange_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
rearrange_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_first_time_compute_particle_update_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

  /* Select the initialized particle (Earth)
   * and put this close to the box center.
   */

  if( localParticle->getX(0) < 1e-2 and localParticle->getX(1) < 1e-2){
    localParticle->setX(0, EARTH_X_INI );
    localParticle->setX(1, EARTH_Y_INI );
  }


""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#rearrange_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
rearrange_particles.add_action_set( particle_tree_analysis )
project.solversteps.add_step(rearrange_particles)

# ### Plots setup
# 
# I will have to plot the outcome (or any snapshot). With particles, each of my dumps produces two types of files: particle data and mesh data. For the mesh, I rely on the toolbox's grid plotting. The plot is the step where we don't need the AMR criterion - the particles don't move here after all. We however run the tree analysis, so it is up-to-date for the next step.

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


