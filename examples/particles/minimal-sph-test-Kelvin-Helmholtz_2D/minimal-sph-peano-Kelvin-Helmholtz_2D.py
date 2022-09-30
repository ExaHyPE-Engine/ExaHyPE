#!/usr/bin/env python
# coding: utf-8

# ------------------------------------------------------------------------------
# ----------------------- Peano 4 SPH Simulation Project -----------------------
# ------------------------------------------------------------------------------

# SPH scheme: Minimal SPH (Swift)
# Setup: Kelvin-Helmholtz (2D)
# 
# @TODO ADD descripton of the problem
#
#
# Particle behaviour
# 
# As usual, in peano 4 the particle-particle interactions are handled in terms
# of the active and local particle sets. Notice that in this we now do not
# simply rely on the particles of the adjacent vertices of a cell -- see the
# sketch below. To understand what the individual particles in the sketch are
# supposed to mean and how they are maintained and exposed to the user, please
# consult the documentation of the ParticleParticleInteraction class in the
# Python toolbox directory. It contains a lot explanations.
# 
# **Remarks**:
# - For particle-particle interactions: bear in mind that the local set is a
# subset of the active set, so be careful about any unwanted self-interactions.
# - In this scheme, the following relation should be satisfied:
# 
# \begin{align}
# H_{\rm SPH} \leq R^{\rm peano}_{\rm cutoff} \leq \frac{\Delta x}{2}\,,
# \end{align}
# 
# where $H_{\rm SPH}$($\equiv\gamma_K h$) is the SPH interaction radius, 
# $R^{\rm peano}_{\rm cutoff}$ is the internal cut-off radius and $\Delta x$ the
# cell size.
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2
from numpy import pi

# To execute shell commands via python
import subprocess

import argparse

# SPH initialisation snippets library
import peano4.toolbox.particles.sph.initSnippets

# SPH compute kernels library
import peano4.toolbox.particles.sph.computeKernels

# HDF5 Reader
import peano4.toolbox.particles.sph.fileReader

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

# Command line arguments for this script
parser = argparse.ArgumentParser(description='Peano-4 SPH benchmarking script')
parser.add_argument("-m",     "--mode",               dest="mode",                         default="release",           help="|".join(modes.keys()) )
parser.add_argument("-c",     "--clean",              dest="clean_temporary_directories",  action="store_true",         help="Remove temporary directories before script starts" )
parser.add_argument("-r",     "--random",             dest="random_particle_layout",       action="store_true",         help="Use a randomised initial particle layout" )
parser.add_argument("-1d",    "--1d",                 dest="one_dimensions_setup",         action="store_true",         help="Switch to a 1d setup" )
parser.add_argument("-dist",  "--particle-distance",  dest="particle_distance",            required=True,  type=float,  help="Particle distance (start with 1/64 by default)" )
parser.add_argument("-j",     "--parallel-builds",    dest="parallel_builds",              default=-1,       type=int,  help="Number of parallel builds" )
args = parser.parse_args()

if args.mode not in modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ",join(modes.keys()[:-1]),modes.keys()[-1],args.mode))
    import sys
    sys.exit(1)

print( "add noise to initial Cartesian particle layout: {}".format(args.random_particle_layout) )
print( "clean directories: {}".format(args.clean_temporary_directories) )

# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
  bashCommand = 'make clean; rm -r actions/ vertexdata/ repositories/ globaldata/ celldata/ observers/'
  subprocess.check_output(['bash','-c', bashCommand])

# -----------------------------------------------------------------------------
# Initialise Peano4 project  
# -----------------------------------------------------------------------------
project = peano4.Project( ["examples", "particles"],
    "minimal-sph-peano-test-KH", "." )

# Prepare code
# 
# Peano's API does not know which settings to use on the present system. To make
# it copy/clone the settings identified by ./configure, we ask it to parse the
# generated configuration scripts. 

build_mode = modes[args.mode]
project.output.makefile.parse_configure_script_outcome( "../../.." )
project.output.makefile.set_mode( build_mode )

# -----------------------------------------------------------------------------
# Model the particle
# -----------------------------------------------------------------------------
 
# We create the particle through the toolbox. A particle by definition has a
# position ${\bf x}$, so we just need to add everything else for SPH:
# 
# - Velocity array ${\bf v}\equiv{\rm d}{\bf x}/{\rm d}t$
# - Acceleration array ${\bf a}\equiv{\rm d}{\bf v}/{\rm d}t$
# - Velocity array ${\bf v}_{\rm full}$ (not accessed by SPH kernels)
# - Acceleration array prev step ${\bf a}_{\rm prev}$
# - Mass $m$
# - Density $\rho$
# - Pressure $P$
# - Sound speed $c_s$
# - Internal Energy $u$
# - Internal Energy $u_{\rm full}$ (not accessed by SPH kernels)
# - Time derivative of internal energy $\dot u$
# - Kernel smoothing length $h$
# - Kernel smoothing length prev step $h_{\rm prev}$
# - Time derivative of smoothing length $\dot h$
# - "grad-h" term $f$
# - ${{\rm d}\rho}/{{\rm d}h}$
# - Fractional particle number count $W_{\rm count}$
# - ${{\rm d}W_{\rm count}}/{{\rm d}h}$
# 
# For artificial viscosity scheme
# - ${\rm div}{(\bf v)}$
# - ${\rm rot}{(\bf v)}$
# - $v_{\rm sig}$
# - ${\rm balsara\ switch}$
# 
# Extras
# - ${\rm residual}$
# - ${\rm iter\ count}$
# - ${\rm has\ converged}$
# - ${\rm has\ been\ kicked}$

# Define particle and its properties
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

# Particle id 
part_id_attr = dastgen2.attributes.Integer("partid")
particle.data.add_attribute(part_id_attr)

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

# -----------------------------------------------------------------------------
# Export global SPH constants and parameters
# -----------------------------------------------------------------------------
# @TODO: Add dependence on SPH kernel function
 
# There are plenty of constant simulation paramaters to specify for our SPH
# simulation - we do that here. 

# Peano4 dimensions (for data structures)
Dimensions = 2
project.output.makefile.set_dimension(Dimensions)

# Effective SPH dimensions ----------------------------------------------------
HYDRO_DIMENSIONS = 2

# Particle number along 1 dimension -------------------------------------------
HYDRO_PART_NUMBER = str( round(1.0 / args.particle_distance) );

# Time-integration parameters -------------------------------------------------
GLOBAL_TIME_STEP_SIZE = "1e-4"
CFL_CONSTANT = "0.1"

# EoS parameter ---------------------------------------------------------------
GAMMA = str(5./3.)

# Artificial viscosity parameters ---------------------------------------------
ALPHA_AV= "0.8"     # =0.8 in SWIFT
BETA_AV = "3"       # =3 in SWIFT/SPHENIX

# Newton-Raphson parameters --------------------------------------------------
# Maximum allowed number of Newton-Raphson iterations
HYDRO_DEFAULT_MAX_ITERATIONS = "10"

# Tolerance for Newton-Raphson convergence
H_TOLERANCE = "1e-4"

# SPH kernel parameters -------------------------------------------------------
#KERNEL_SUPPORT_RAD = "1.732051"  #  1D SWIFT M4 Kernel
KERNEL_SUPPORT_RAD = "1.936492"  #  1D SWIFT M5 Kernel

# Max/min allowed values for smoothing length h
HYDRO_h_MAX = str( 1.0 / 3.0 / float(KERNEL_SUPPORT_RAD) ) 
HYDRO_h_MIN = "1e-6"

# Smoothing-length free parameter (resolution "eta_neighbours"). 
# Can be set by specifying a desired number of neighbours.
SMOOTH_L_FAC = str( 1.235 ) 

if(HYDRO_DIMENSIONS==1):
    N_MEAN_TARGET = str( 2.0 *           (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )    )
elif(HYDRO_DIMENSIONS==2):
    N_MEAN_TARGET = str(         pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )
elif(HYDRO_DIMENSIONS==3):
    N_MEAN_TARGET = str( 4./3. * pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )

# Kernel root value 
if(HYDRO_DIMENSIONS==1):
  #    CUBIC_SPLINE_SIGMA = str(8./3.)       # 1D M4 SWIFT
    CUBIC_SPLINE_SIGMA = str(3125./768.)     # 1D M5 SWIFT
elif(HYDRO_DIMENSIONS==2):
    CUBIC_SPLINE_SIGMA = str(80/(7*pi))    # 2D SWIFT Kernel
#    CUBIC_SPLINE_SIGMA = str(10/(7*pi))
elif(HYDRO_DIMENSIONS==3):
    CUBIC_SPLINE_SIGMA = str(1/pi)


print('-----------------------------------------------------------------------')
print('SPH parameters summary:')
print(f'Effective dimensions of the Simulation: {HYDRO_DIMENSIONS}'  )
print(f'Global time-step size: {GLOBAL_TIME_STEP_SIZE}'  )
print(f'Number of particles (1D): {HYDRO_PART_NUMBER}'  )
print(f'Smoothing length free factor: {SMOOTH_L_FAC}'   )
print(f'Kernel Root value (sigma): {CUBIC_SPLINE_SIGMA}')
print('-----------------------------------------------------------------------')

# Export Constants.h file
project.constants.export_const_with_type("HYDRO_DIMENSIONS"     , str(HYDRO_DIMENSIONS), "double")
project.constants.export_const_with_type("HYDRO_PART_NUMBER"    , HYDRO_PART_NUMBER    , "double")
project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
project.constants.export_const_with_type("CFL_CONSTANT"         , CFL_CONSTANT         , "double")
project.constants.export_const_with_type("GAMMA"                , GAMMA                , "double")
project.constants.export_const_with_type("ALPHA_AV"             , ALPHA_AV             , "double")
project.constants.export_const_with_type("BETA_AV"              , BETA_AV              , "double")
project.constants.export_const_with_type("HYDRO_DEFAULT_MAX_ITERATIONS", HYDRO_DEFAULT_MAX_ITERATIONS, "int")
project.constants.export_const_with_type("H_TOLERANCE"          , H_TOLERANCE          , "double")
project.constants.export_const_with_type("KERNEL_SUPPORT_RAD"   , KERNEL_SUPPORT_RAD   , "double")
project.constants.export_const_with_type("HYDRO_h_MAX"          , HYDRO_h_MAX          , "double")
project.constants.export_const_with_type("HYDRO_h_MIN"          , HYDRO_h_MIN          , "double")
project.constants.export_const_with_type("SMOOTH_L_FAC"         , SMOOTH_L_FAC         , "double")
project.constants.export_const_with_type("CUBIC_SPLINE_SIGMA"   , CUBIC_SPLINE_SIGMA   , "double")


# -----------------------------------------------------------------------------
# Initialize simulation grid
# -----------------------------------------------------------------------------
mesh_size = 0.5
#mesh_size = 0.2

particle_tree_analysis = peano4.toolbox.particles.ParticleTreeAnalysis(particles)
# Check docu of ParticleTreeAnalysis
project.datamodel.add_cell(particle_tree_analysis.cell_marker)   

create_grid = peano4.solversteps.Step( name="CreateGrid", add_user_defined_actions=False )
create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(mesh_size) )
create_grid.use_vertex(particles)
create_grid.use_cell(particle_tree_analysis.cell_marker)
project.solversteps.add_step(create_grid)


# ------------------------------------------------------------------------------
# Set Initial Conditions from HDF5 FILE 
# ------------------------------------------------------------------------------

init_setup = peano4.solversteps.Step( name="Init", add_user_defined_actions=False )
init_setup.use_vertex(particles)
init_setup.use_cell(particle_tree_analysis.cell_marker)
init_setup.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_setup.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
init_setup.add_action_set( peano4.toolbox.particles.sph.fileReader.InsertParticlesFromHDF5File( 
  particle_set=particles, 
  filename='kelvinHelmholtz.hdf5'
  )) 
init_setup.add_action_set( particle_tree_analysis )
project.solversteps.add_step(init_setup)


# ------------------------------------------------------------------------------
# Model the steps of the simulation --------------------------------------------
# ------------------------------------------------------------------------------
 
# In the following, we model each step that will be invoked by main.cpp to run
# the simulation. The generated C++ code based on these will be outputted into
# `/observers/`


# ------------------------------------------------------------------------------
# Density loop
# ------------------------------------------------------------------------------
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


# -----------------------------------------------------------------------------
# Force loop
# -----------------------------------------------------------------------------
force_non_sym_particles = peano4.solversteps.Step( "ForceNonSym" )
force_non_sym_particles.use_vertex(particles)
force_non_sym_particles.use_cell(particle_tree_analysis.cell_marker)

# Force loop kernel from SPH library
Force = peano4.toolbox.particles.sph.computeKernels.Force( hydroScheme='minimal' )

particle_particle_compute_kernel = Force.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Force.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Force.touch_particle_last_time_compute_kernel

# Add step to project
force_non_sym_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel, 
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes = """
#include "Constants.h"
#include <cmath>
#include "kernel_hydro.h"
#include "equation_of_state.h"
"""
  ))
#force_non_sym_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
force_non_sym_particles.add_action_set( particle_tree_analysis )
force_non_sym_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
force_non_sym_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(force_non_sym_particles)


# -----------------------------------------------------------------------------
# Time integration steps (leapfrog)
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Kick 1
# -----------------------------------------------------------------------------
kick1_particles = peano4.solversteps.Step( "Kick1" )
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
kick2_particles = peano4.solversteps.Step( "Kick2" )
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
#kick2_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
kick2_particles.add_action_set( particle_tree_analysis )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick2_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick2_particles)


# -----------------------------------------------------------------------------
# Drift step (+prediction)
# -----------------------------------------------------------------------------
drift_particles = peano4.solversteps.Step( "Drift" )
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
#drift_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
drift_particles.add_action_set( particle_tree_analysis )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
drift_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(drift_particles)


# -----------------------------------------------------------------------------
# Hydro_kick_extra
# -----------------------------------------------------------------------------
hydro_kick_extra = peano4.solversteps.Step( "HydroKickExtra" )
hydro_kick_extra.use_vertex(particles)
hydro_kick_extra.use_cell(particle_tree_analysis.cell_marker)

# Kick extra step from SPH library
KickExtra = peano4.toolbox.particles.sph.computeKernels.LeapfrogKickExtra()

particle_particle_compute_kernel = KickExtra.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= KickExtra.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = KickExtra.touch_particle_last_time_compute_kernel

# Add step to project
hydro_kick_extra.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#hydro_kick_extra.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
hydro_kick_extra.add_action_set( particle_tree_analysis )
hydro_kick_extra.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
hydro_kick_extra.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(hydro_kick_extra)


# -----------------------------------------------------------------------------
#  Prediction step (relevant when using local time-stepping)
# -----------------------------------------------------------------------------
predict_hydro_particles = peano4.solversteps.Step( "PredictHydro" )
predict_hydro_particles.use_vertex(particles)
predict_hydro_particles.use_cell(particle_tree_analysis.cell_marker)

# Predict hydro step from SPH library
PredictHydro = peano4.toolbox.particles.sph.computeKernels.LeapfrogPredict()

particle_particle_compute_kernel = PredictHydro.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= PredictHydro.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = PredictHydro.touch_particle_last_time_compute_kernel

# Add step to project
predict_hydro_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include "equation_of_state.h"
#include <cmath>
"""
  ))
predict_hydro_particles.add_action_set( particle_tree_analysis )
predict_hydro_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
predict_hydro_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(predict_hydro_particles)


# -----------------------------------------------------------------------------
# Corrector step (mini-kick) (Phantom code implementation)
# -----------------------------------------------------------------------------
kick_particles_corrector = peano4.solversteps.Step( "KickCorrector" )
kick_particles_corrector.use_vertex(particles)
kick_particles_corrector.use_cell(particle_tree_analysis.cell_marker)

# Corrector step from SPH library
Corrector = peano4.toolbox.particles.sph.computeKernels.LeapfrogCorrector()

particle_particle_compute_kernel = Corrector.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= Corrector.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = Corrector.touch_particle_last_time_compute_kernel

# Add step to project
kick_particles_corrector.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#kick_particles_corrector.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
kick_particles_corrector.add_action_set( particle_tree_analysis )
kick_particles_corrector.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick_particles_corrector.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick_particles_corrector)


# -----------------------------------------------------------------------------
# Corrector for Extra (internal energy)
# -----------------------------------------------------------------------------
kick_corrector_extra = peano4.solversteps.Step( "KickCorrectorExtra" )
kick_corrector_extra.use_vertex(particles)
kick_corrector_extra.use_cell(particle_tree_analysis.cell_marker)

# Corrector Extra step from SPH library
CorrectorExtra = peano4.toolbox.particles.sph.computeKernels.LeapfrogCorrectorExtra()

particle_particle_compute_kernel = CorrectorExtra.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= CorrectorExtra.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = CorrectorExtra.touch_particle_last_time_compute_kernel

# Add step to project
kick_corrector_extra.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#kick_corrector_extra.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
kick_corrector_extra.add_action_set( particle_tree_analysis )
kick_corrector_extra.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick_corrector_extra.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick_corrector_extra)


# -----------------------------------------------------------------------------
# Reset HasConverged
# -----------------------------------------------------------------------------
reset_HasConverged_step = peano4.solversteps.Step( "ResetHasConverged" )
reset_HasConverged_step.use_vertex(particles)
reset_HasConverged_step.use_cell(particle_tree_analysis.cell_marker)

# ResetHasConverged step from SPH library
ResetHasConverged = peano4.toolbox.particles.sph.computeKernels.ResetHasConverged()

particle_particle_compute_kernel = ResetHasConverged.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= ResetHasConverged.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = ResetHasConverged.touch_particle_last_time_compute_kernel

# Add step to project
reset_HasConverged_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#reset_HasConverged_step.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
reset_HasConverged_step.add_action_set( particle_tree_analysis )
reset_HasConverged_step.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
reset_HasConverged_step.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(reset_HasConverged_step)


# -----------------------------------------------------------------------------
# Initialize some variables for the current time-step
# -----------------------------------------------------------------------------
init_time_step = peano4.solversteps.Step( "InitTimeStep" )
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
#init_time_step.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
init_time_step.add_action_set( particle_tree_analysis )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
init_time_step.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(init_time_step)


# -----------------------------------------------------------------------------
#  Impose Boundary Conditions
# -----------------------------------------------------------------------------
impose_BC_particles = peano4.solversteps.Step( "ImposeBC" )
impose_BC_particles.use_vertex(particles)
impose_BC_particles.use_cell(particle_tree_analysis.cell_marker)

# ResetHasConverged step from SPH library
ImposeBC = peano4.toolbox.particles.sph.computeKernels.ImposeBoundaryConditions()

particle_particle_compute_kernel = ImposeBC.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= ImposeBC.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = ImposeBC.touch_particle_last_time_compute_kernel

# Add action to project
impose_BC_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
impose_BC_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
impose_BC_particles.add_action_set( particle_tree_analysis )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(impose_BC_particles)


# -----------------------------------------------------------------------------
# End time-step
# -----------------------------------------------------------------------------
end_time_step = peano4.solversteps.Step( "EndTimeStep" )
end_time_step.use_vertex(particles)
end_time_step.use_cell(particle_tree_analysis.cell_marker)

# End time step from SPH library
EndTimeStep = peano4.toolbox.particles.sph.computeKernels.EndTimeStep()

particle_particle_compute_kernel = EndTimeStep.particle_particle_compute_kernel
touch_particle_first_time_compute_kernel= EndTimeStep.touch_particle_first_time_compute_kernel
touch_particle_last_time_compute_kernel = EndTimeStep.touch_particle_last_time_compute_kernel

# Add action to project
end_time_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel = particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel= touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel = touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#end_time_step.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
end_time_step.add_action_set( particle_tree_analysis )
end_time_step.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
end_time_step.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(end_time_step)


# ------------------------------------------------------------------------------
# End of simulation actions' declarations --------------------------------------
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Plot setup 
# ------------------------------------------------------------------------------
 
# I will have to plot the outcome (or any snapshot). With particles, each of my
# dumps produces two types of files: particle data and mesh data. For the mesh,
# I rely on the toolbox's grid plotting. The plot is the step where we don't
# need the AMR criterion - the particles don't move here after all. We however
# run the tree analysis, so it is up-to-date for the next step.

# Particle plotter
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particles, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(part_id_attr    , 1)
particle_plotter.add_attribute_to_plot(v_attr          , Dimensions)
particle_plotter.add_attribute_to_plot(a_attr          , Dimensions)
particle_plotter.add_attribute_to_plot(u_attr       , 1)
particle_plotter.add_attribute_to_plot(mass_attr    , 1)
particle_plotter.add_attribute_to_plot(density_attr , 1)
particle_plotter.add_attribute_to_plot(pressure_attr, 1)
particle_plotter.add_attribute_to_plot(f_attr       ,  1)
particle_plotter.add_attribute_to_plot(Omega_attr   ,  1)
particle_plotter.add_attribute_to_plot(rho_dh_attr  ,  1)
particle_plotter.add_attribute_to_plot(Ncount_attr  ,  1)
particle_plotter.add_attribute_to_plot(wcount_attr  ,  1)
particle_plotter.add_attribute_to_plot(wcount_dh_attr,  1)
particle_plotter.add_attribute_to_plot(soundSpeed_attr, 1)
particle_plotter.add_attribute_to_plot(timeStepSize_attr, 1)
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
#particle_plotter.add_attribute_to_plot(a_prev_step_attr, Dimensions)

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


# ------------------------------------------------------------------------------
# Generate the actual C++ code
# ------------------------------------------------------------------------------
# Next, we generate the actual Peano 4 C++ code:

# If using Pawel's clang, in some cases we need to add the "-lstdc++fs" flag to
# the makefile We can do this manually in the generated code, or use

#project.output.makefile.add_library("-lstdc++fs")

# which, however, has to be modified to remove the "-l" appended by default
# Otherwise we can just compile.
project.generate()

# ## Write the main
# 
# Do not forget to include the generated header files, e.g., 
#     
#     //
#     // =================
#     // inserted manually
#     // =================
#     //
#     #include "observers/CreateGrid.h"
#     #include "observers/Init.h"
#     #include "observers/Plot.h"
#     #include "observers/Move.h"
# 

# ------------------------------------------------------------------------------
#  Compile the code
# ------------------------------------------------------------------------------
bashCommand = "sed -i 's/g++/h5c++/g' Makefile"
subprocess.check_output(['bash','-c', bashCommand])

# We can do this here or in the terminal.
project.build( number_of_parallel_builds = args.parallel_builds )


# ------------------------------------------------------------------------------
# Run code
# ------------------------------------------------------------------------------
# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)

# We can do this here or in the terminal.
# success = project.run( args=["--threads", "1"], prefix=["mpirun", "-n", "1"] )
#!./peano4


# ------------------------------------------------------------------------------
# ---------------------- END OF PEANO 4 SPH PROJECT ----------------------------
# ------------------------------------------------------------------------------



# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ----------------------
# 
# # Summary of SPH equations
# We follow the review paper by Price et al (2018).
# 
# Notice that, throughout this, $h$ denotes the smoothing length.
# 
# #### SPH basics
# 
# \begin{align}
# \rho_a& = \sum_b m_b W(|{\bf r}_a - {\bf r}_b|, h_a)\\
# \delta\Omega_a\equiv\Omega_a &= 1 - \frac{\partial h_a}{\partial \rho_a}\sum_b m_b\frac{\partial W_{ab}(h_a)}{\partial h_a} \qquad\qquad (\delta\Omega_a=0 \quad \textrm{for constant $h$})\\
# \frac{{\rm d}{\rho_a}}{{\rm d}t} &= \frac{1}{\Omega_a}\sum_b m_b{v}_{ab}\cdot{\bf \nabla}_aW_{ab}(h_a)
# \end{align}
# 
# #### Equation of State (EoS) et al
# 
# \begin{align}
# P &=(\gamma-1)\rho u \qquad \textrm{Equation of State (EoS)}
# \end{align}
# 
# 
# 
# #### Euler equation (w/o artificial viscosity terms)
# 
# \begin{align}
# \frac{{\rm d}{\bf v}_a}{{\rm d}t} &=-\sum_b m_b\left[\frac{P_a}{\rho_a^2\Omega_a}{\bf \nabla}_aW_{ab}(h_a) + \frac{P_b}{\rho_b^2\Omega_b}{\bf \nabla}_aW_{ab}(h_b) \right] + {\bf a}^{\rm self-grav}_a + {\bf a}^{\rm others}_a\\
# \end{align}
# 
# 
# #### Evolution of internal energy:
# 
# \begin{align}
# \frac{{\rm d}u_a}{{\rm d}t} &=\frac{P_a}{\rho_a^2\Omega_a}\sum_b m_b{v}_{ab}\cdot{\bf \nabla}_aW_{ab}(h_a)\\
# \end{align}
# 
# 
# #### Newton-Raphson calculation of $\rho$ and (variable) $h$:
# 
# \begin{align}
# \zeta(h_a)&\equiv m_a\left(\frac{\eta}{h_a}\right)^{3}-\sum_b m_b W_{ab}(h_a)=0\\
# \textrm{Initial guess:} &\qquad\qquad h^{\rm ini}_a =h_a + \Delta t \frac{\partial h_a}{\partial\rho_a}\frac{d\rho_a}{dt}\\ 
# \textrm{N-R iteration:}&\qquad\qquad h_{a,{\rm new}}=h_a - \frac{\zeta({h_a})}{\zeta'(h_a)} \qquad \textrm{where:}\qquad \zeta'(h_a)=-\frac{3\rho_a \Omega_a}{h_a} \\
# \end{align}
#  
# 
# #### Useful relations
# 
# \begin{align}
# \nabla_aW_{ab}&=\frac{C_{\rm norm}}{h^4}f'(q)\hat{{\bf r}}_{ab} \\
# \frac{\partial W_{ab}}{\partial h}(r,h)&=-\frac{C_{\rm norm}}{h^4}[3f(q)+qf'(q)] \\
# \frac{\partial h_a}{\partial \rho_a} &= -\frac{3h_a}{\rho_a} \\
# \end{align}
