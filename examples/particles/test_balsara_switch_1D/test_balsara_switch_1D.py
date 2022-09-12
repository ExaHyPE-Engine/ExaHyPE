#!/usr/bin/env python
# coding: utf-8

# (Unit?) test for the Balsara Switch calculation
# This is used to solve the density and smoothing length of the particles.

import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2
import numpy as np
from numpy import pi

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

parser = argparse.ArgumentParser(description='benchmarking script')
parser.add_argument("-m",     "--mode",               dest="mode",                         default="release",           help="|".join(modes.keys()) )
parser.add_argument("-c",     "--clean",              dest="clean_temporary_directories",  action="store_true",         help="Remove temporary directories before script starts" )
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

project = peano4.Project( ["examples", "particles"], "test_balsara_switch_1D", "." )


################################################################################
# ## Prepare code
# Peano's API does not know which settings to use on the present system. 
# To make it copy/clone the settings identified by ./configure, we ask it 
# to parse the generated configuration scripts. 
################################################################################

build_mode = modes[args.mode]
project.output.makefile.parse_configure_script_outcome( "../../.." )
project.output.makefile.set_mode( build_mode )


################################################################################
###################### Model the particle ######################################
################################################################################

particle = peano4.toolbox.particles.Particle("Particle")

# Core properties --------------------------------------------------------------
v_attr       = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr       = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
mass_attr    = dastgen2.attributes.Double("mass")
smoothL_attr = dastgen2.attributes.Double("smoothingLength")
density_attr = dastgen2.attributes.Double("density")
pressure_attr= dastgen2.attributes.Double("pressure")
u_attr       = dastgen2.attributes.Double("u")
uDot_attr    = dastgen2.attributes.Double("uDot")

# Variable smoothing length scheme ---------------------------------------------
f_attr        = dastgen2.attributes.Double("f")   
wcount_dh_attr= dastgen2.attributes.Double("wcount_dh")
rho_dh_attr   = dastgen2.attributes.Double("rho_dh")
hleft_attr    = dastgen2.attributes.Double("hLeft")
hright_attr   = dastgen2.attributes.Double("hRight")
Omega_attr    = dastgen2.attributes.Double("Omega")        # PHANTOM SPH (~1/f)

# Newton-Raphson iterations ----------------------------------------------------
smoothL_prev_attr    = dastgen2.attributes.Double("smoothingLengthPrevious")
wcount_attr          = dastgen2.attributes.Double("wcount")
hDot_attr            = dastgen2.attributes.Double("hDot")
residual_attr        = dastgen2.attributes.Double("residual")
iteration_count_attr = dastgen2.attributes.Integer("iterCount")
has_converged_attr   = dastgen2.attributes.Boolean("hasConverged")

# Artificial viscosity scheme -------------------------------------------------
balsara_attr    = dastgen2.attributes.Double("balsara")
rot_v_attr      = dastgen2.attributes.Double("rot_v")    # In 2D sims it only has 1 component (z)
#rot_v_attr      = peano4.dastgen2.Peano4DoubleArray("rot_v","Dimensions")
div_v_attr      = dastgen2.attributes.Double("div_v")
v_sig_AV_attr   = dastgen2.attributes.Double("v_sig_AV")
soundSpeed_attr = dastgen2.attributes.Double("soundSpeed")

# Time integration -------------------------------------------------------------
timeStepSize_attr     = dastgen2.attributes.Double("timeStepSize")
v_full_attr           = peano4.dastgen2.Peano4DoubleArray("v_full","Dimensions")
u_full_attr           = dastgen2.attributes.Double("u_full")
has_been_kicked_attr  = dastgen2.attributes.Boolean("hasBeenKicked")

# Extra ------------------------------------------------------------------------
has_no_neighbours_attr= dastgen2.attributes.Boolean("hasNoNeighbours")
is_boundary_part_attr = dastgen2.attributes.Boolean("isBoundaryParticle")
time_attr             = dastgen2.attributes.Double("time")
a_prev_step_attr      = peano4.dastgen2.Peano4DoubleArray("a_prev_step","Dimensions") # for PHANTOM corrector
uDot_prev_step_attr   = dastgen2.attributes.Double("uDot_prev_step")                  # for PHANTOM corrector
Ncount_attr           = dastgen2.attributes.Integer("Ncount")                         # integer number count

# Move state flags ------------------------------------------------------------
particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )

# Add the above properties to the particle data model
particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(mass_attr)
particle.data.add_attribute(smoothL_attr)
particle.data.add_attribute(density_attr)
particle.data.add_attribute(pressure_attr)
particle.data.add_attribute(u_attr)
particle.data.add_attribute(uDot_attr)

particle.data.add_attribute(f_attr)
particle.data.add_attribute(wcount_dh_attr)
particle.data.add_attribute(rho_dh_attr)
particle.data.add_attribute(hleft_attr)
particle.data.add_attribute(hright_attr)
particle.data.add_attribute(Omega_attr)

particle.data.add_attribute(smoothL_prev_attr)
particle.data.add_attribute(wcount_attr)
particle.data.add_attribute(hDot_attr)
particle.data.add_attribute(residual_attr)
particle.data.add_attribute(iteration_count_attr)
particle.data.add_attribute(has_converged_attr)

particle.data.add_attribute(balsara_attr)
particle.data.add_attribute(rot_v_attr)
particle.data.add_attribute(div_v_attr)
particle.data.add_attribute(v_sig_AV_attr)
particle.data.add_attribute(soundSpeed_attr)

particle.data.add_attribute(timeStepSize_attr)
particle.data.add_attribute(v_full_attr)
particle.data.add_attribute(u_full_attr)
particle.data.add_attribute(has_been_kicked_attr)

particle.data.add_attribute(has_no_neighbours_attr)
particle.data.add_attribute(is_boundary_part_attr)
particle.data.add_attribute(time_attr)
particle.data.add_attribute(a_prev_step_attr)
particle.data.add_attribute(uDot_prev_step_attr)
particle.data.add_attribute(Ncount_attr)

# Add particle to project.
# This particle has to be added to the project. This way the project knows that
# we are working with this guy. We associate the particle with the vertices, as
# we work with the pidt (particle in dual tree) algorithm. 
particles = peano4.toolbox.particles.ParticleSet(particle)
project.datamodel.add_global_object(particle)
project.datamodel.add_vertex       (particles)


################################################################################
# Constants required for the iterative density solver
################################################################################

# Peano dimension
Dimensions = 2
project.output.makefile.set_dimension(Dimensions)

# Effective SPH dimensions
HYDRO_DIMENSIONS = 1

# Dummy time step size
GLOBAL_TIME_STEP_SIZE = "1e-4"

# Number of particles initialised along 1 dimension
HYDRO_PART_NUMBER = str(10);

# Maximum allowed number of Newton-Raphson iterations
HYDRO_DEFAULT_MAX_ITERATIONS = "10"

# Tolerance for Newton-Raphson convergence
H_TOLERANCE = "1e-4"

# Time-integration parameters
CFL_CONSTANT = "0.2"

# EoS parameter
GAMMA = str(5./3.)

# SPH kernel parameters
#KERNEL_SUPPORT_RAD = "2"           # "gamma" parameter. =2 for Cubic Spline. Different in other kernels.
KERNEL_SUPPORT_RAD = "1.936492"     # SWIFT

HYDRO_h_MAX = str( 1.0 / 3.0 / float(KERNEL_SUPPORT_RAD))  # for grid_size = 1/3
HYDRO_h_MIN = "1e-6"

# Artificial viscosity parameters
# Here they only affect the balsara switch calculation
ALPHA_AV= "0.8"     # =0.8 in SWIFT
BETA_AV = "3"       # =3 in SWIFT/SPHENIX

# Smoothing-length free parameter (resolution "eta_neighbours"). 
#SMOOTH_L_FAC = str( 0.75 )  # 3 nbs in 1D
SMOOTH_L_FAC = str( 1.1 )  # SWIFT -- see Josh's PhD thesis
#SMOOTH_L_FAC = str( 1.235 ) 

if(HYDRO_DIMENSIONS==1):
    N_MEAN_TARGET = str( 2.0 *           (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )    )
elif(HYDRO_DIMENSIONS==2):
    N_MEAN_TARGET = str(         np.pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )

# Kernel root value 
if(HYDRO_DIMENSIONS==1):
    CUBIC_SPLINE_SIGMA = str(3125./768.)
elif(HYDRO_DIMENSIONS==2):
    CUBIC_SPLINE_SIGMA = str(10/(7*pi))

print("Smoothing length free factor: " + SMOOTH_L_FAC)
print("Target number of mean neighbours: " + N_MEAN_TARGET)
print("Kernel Root value (sigma): " + CUBIC_SPLINE_SIGMA)

project.constants.export_const_with_type("HYDRO_DIMENSIONS"  , str(HYDRO_DIMENSIONS), "double")
project.constants.export_const_with_type("KERNEL_SUPPORT_RAD"   , KERNEL_SUPPORT_RAD, "double")
project.constants.export_const_with_type("CUBIC_SPLINE_SIGMA"   , CUBIC_SPLINE_SIGMA, "double")
project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
project.constants.export_const_with_type("HYDRO_PART_NUMBER", HYDRO_PART_NUMBER, "double")
project.constants.export_const_with_type("HYDRO_DEFAULT_MAX_ITERATIONS", HYDRO_DEFAULT_MAX_ITERATIONS, "int")
project.constants.export_const_with_type("H_TOLERANCE", H_TOLERANCE, "double")
project.constants.export_const_with_type("HYDRO_h_MAX", HYDRO_h_MAX, "double")
project.constants.export_const_with_type("HYDRO_h_MIN", HYDRO_h_MIN, "double")
project.constants.export_const_with_type("SMOOTH_L_FAC"         , SMOOTH_L_FAC      , "double")
project.constants.export_const_with_type("N_MEAN_TARGET"        , N_MEAN_TARGET     , "double")
project.constants.export_const_with_type("CFL_CONSTANT", CFL_CONSTANT, "double")
project.constants.export_const_with_type("GAMMA"    , GAMMA    , "double")
project.constants.export_const_with_type("ALPHA_AV" , ALPHA_AV , "double")
project.constants.export_const_with_type("BETA_AV"  , BETA_AV  , "double")


################################################################################
# Initialize regular grid
################################################################################

mesh_size = 0.5
particle_tree_analysis = peano4.toolbox.particles.ParticleTreeAnalysis(particles)
project.datamodel.add_cell(particle_tree_analysis.cell_marker)   # read docu of ParticleTreeAnalysis

create_grid = peano4.solversteps.Step( name="CreateGrid", add_user_defined_actions=False )
create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(mesh_size) )
create_grid.use_vertex(particles)
create_grid.use_cell(particle_tree_analysis.cell_marker)
project.solversteps.add_step(create_grid)

################################################################################
# ## Initial conditions for particles
################################################################################

initialisation_snippet = """
 
  /*
   * Most of the initialised variables are dummy (i.e. not used)
   */
   
  // Internal cutoff radious used by Peano and with max value R = grid_size/2

  /* For grid_size = 1/3 */
  particle->setCutOffRadius(1.0 / 3.0 / 2.0);
  
  particle->setDensity ( 1.0 );        
  particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );
  particle->setU( 1e-6 ); 

  /* Compute the pressure from the EoS */
  const double pressure = gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
  particle->setPressure( pressure );

  /* Compute sound speed */
  const double soundspeed = std::sqrt( GAMMA * particle->getPressure()/particle->getDensity() );
  particle->setSoundSpeed(soundspeed);
  
  /* Compute initial smoothing length */
  particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );
  particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );
  
  /* Initialize remaining fields  */
  
  // Vectors
  for(int d=0; d<Dimensions; d++){
    particle->setV          (d,0);
    particle->setA          (d,0);
  }

  /* Set velocity to get non-zero divergence */
  particle->setV( 0, particle->getX()(0) );
 
  /* For 2D sims, we only get one component */
  particle->setRot_v(0);
  
  particle->setHDot  (0.0);        
  particle->setWcount(0.0);        
  particle->setNcount(0  );        
  particle->setOmega (1.0);  // set to 1 to avoid problem with boundary particles.
  particle->setF     (0.0);        
  particle->setRho_dh(0.0);           
  particle->setWcount_dh(0.0);  

  particle->setHLeft (HYDRO_h_MIN);        
  particle->setHRight(HYDRO_h_MAX);        
  
  particle->setHasConverged(false);
  particle->setHasNoNeighbours(false);
  particle->setHasBeenKicked(false);
  particle->setResidual(0.0);
  particle->setIterCount(0);
  particle->setIsBoundaryParticle(false);

  // Artificial viscosity scheme
  particle->setDiv_v   (0.0);
  particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
  particle->setBalsara (0.0);
 
 
"""

init_setup = peano4.solversteps.Step( name="Init", add_user_defined_actions=False )
init_setup.use_vertex(particles)
init_setup.use_cell(particle_tree_analysis.cell_marker)
init_setup.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_setup.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
init_setup.add_action_set( peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells(
  particle_set=particles,
  distance_between_particles= 1.0 / float(HYDRO_PART_NUMBER),
  initialisation_call=initialisation_snippet,
  noise=False,
  additional_includes=""" 
    #include "Constants.h" 
    #include "kernel_hydro.h"
    #include "equation_of_state.h"
    """,
  computational_domain_offset=[1.0 / float(HYDRO_PART_NUMBER), 0.0]
  , guard="(std::abs( 0.5 - particle->getX(1) ) < 0.01) and (particle->getX(0) > 0)"
  ))

init_setup.add_action_set( particle_tree_analysis )
project.solversteps.add_step(init_setup)


###############################################################################
# ## Density loop
###############################################################################

calc_densities = peano4.solversteps.Step( "ComputeDensityAndUpdateSmoothingLength" )
calc_densities.use_vertex(particles)
calc_densities.use_cell(particle_tree_analysis.cell_marker)

# Density kernel from SPH library
Density = peano4.toolbox.particles.sph.computeKernels.Density.ComputeDensityAndUpdateSmoothingLength( hydroScheme='minimal' )

particle_particle_compute_kernel = Density.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Density.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Density.touch_particle_last_time_compute_kernel

# Add step to project
calc_densities.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include "kernel_hydro.h"
#include "equation_of_state.h"
  """
))
#calc_densities.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
calc_densities.add_action_set( particle_tree_analysis )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(calc_densities)



###############################################################################
# ## Dump Balsara switch value
###############################################################################

dump_balsara = peano4.solversteps.Step( "DumpBalsara" )
dump_balsara.use_vertex(particles)
dump_balsara.use_cell(particle_tree_analysis.cell_marker)

touch_particle_first_time_compute_kernel = """

    
"""

particle_particle_compute_kernel = """

"""

touch_particle_last_time_compute_kernel = """

    // Dump the value of the balsara switch and other relevant quantities.

    if(
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
      ){

      const double balsara = localParticle->getBalsara();
      const double abs_div_v = tarch::la::abs( localParticle->getDiv_v() );
      const double curl_v = std::sqrt( localParticle->getRot_v() * localParticle->getRot_v() );
      const double soundspeed = localParticle->getSoundSpeed();
      const double h_inv = 1.0 / localParticle->getSmoothingLength();

      /* Verbose */
      if( tarch::la::equals(localParticle->getX(0),0.5) ){
        logInfo( "touchVertexLastTime()", " =====================================" );
        logInfo( "touchVertexLastTime()", " Dumping values for particle at x=0.5:" );
        logInfo( "touchVertexLastTime()", " ALPHA_AV = " << ALPHA_AV );
        logInfo( "touchVertexLastTime()", " balsara = " << balsara );
        logInfo( "touchVertexLastTime()", " abs_div_v = " << abs_div_v );
        logInfo( "touchVertexLastTime()", " curl_v = " << curl_v );
        logInfo( "touchVertexLastTime()", " sound speed = " << soundspeed );
        logInfo( "touchVertexLastTime()", " h_inv = " << h_inv );
        logInfo( "touchVertexLastTime()", " factor = " << 0.0001 * soundspeed * h_inv );
        logInfo( "touchVertexLastTime()", " particle string: " << localParticle->toString() );
        logInfo( "touchVertexLastTime()", " =====================================" );
      }

    }

"""


dump_balsara.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel=particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include "kernel_hydro.h"
#include "equation_of_state.h"
  """
))
dump_balsara.add_action_set( particle_tree_analysis )
dump_balsara.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
dump_balsara.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(dump_balsara)





###############################################################################
# ### Plots setup
###############################################################################

particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particles, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(v_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(a_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(u_attr       , 1)
particle_plotter.add_attribute_to_plot(mass_attr    , 1)
particle_plotter.add_attribute_to_plot(density_attr , 1)
particle_plotter.add_attribute_to_plot(pressure_attr, 1)
particle_plotter.add_attribute_to_plot(f_attr ,  1)
particle_plotter.add_attribute_to_plot(Omega_attr ,  1)
particle_plotter.add_attribute_to_plot(rho_dh_attr,  1)
particle_plotter.add_attribute_to_plot(Ncount_attr,  1)
particle_plotter.add_attribute_to_plot(wcount_attr,  1)
particle_plotter.add_attribute_to_plot(wcount_dh_attr,  1)
particle_plotter.add_attribute_to_plot(soundSpeed_attr, 1)
particle_plotter.add_attribute_to_plot(smoothL_attr , 1)
particle_plotter.add_attribute_to_plot(smoothL_prev_attr , 1)
particle_plotter.add_attribute_to_plot(hDot_attr , 1)
particle_plotter.add_attribute_to_plot(residual_attr, 1)
particle_plotter.add_attribute_to_plot(has_converged_attr, 1)
particle_plotter.add_attribute_to_plot(iteration_count_attr, 1)
particle_plotter.add_attribute_to_plot(hleft_attr, 1)
particle_plotter.add_attribute_to_plot(hright_attr, 1)
particle_plotter.add_attribute_to_plot(is_boundary_part_attr, 1)
particle_plotter.add_attribute_to_plot(has_no_neighbours_attr, 1)

# Artificial viscosity
particle_plotter.add_attribute_to_plot(rot_v_attr   , 1)
particle_plotter.add_attribute_to_plot(div_v_attr   , 1)
particle_plotter.add_attribute_to_plot(v_sig_AV_attr, 1)
particle_plotter.add_attribute_to_plot(balsara_attr, 1)

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

###############################################################################
# ## Generate the actual C++ code
###############################################################################

project.generate()
# ## Compile the code
# We can do this here or in the terminal.
project.build()

# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)
