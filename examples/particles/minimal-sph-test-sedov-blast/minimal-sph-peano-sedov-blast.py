#!/usr/bin/env python
# coding: utf-8

# # Minimal SPH (Swift)
# ## Sedov Blast test for Peano SPH
# 
# 
# In this notebook, we set up the Sedov-Taylor Blast test, which is part of a battery of well-established tests for SPH codes.
# 
# In this test, particles are initialised at rest in regular grid configuration and periodic BC. Gas has negligible presure. Energy ($E$) is injected to a (or group of) central particle and the blast should propagate isotropically, up to boundary effects. See, e.g., Springel and Hernquist, Mon. Not. R. Astron. Soc. 333, 649–664 (2002).

# ## Particle behaviour
# 
# As usual, in peano 4 the particle-particle interactions are handled in terms of the active and local particle sets. Notice that in this we now do not simply rely on the particles of the adjacent vertices of a cell -- see the sketch below. To understand what the individual particles in the sketch are supposed to mean and how they are maintained and exposed to the user, please consult the documentation of the ParticleParticleInteraction class in the Python toolbox directory. It contains a lot explanations.
# 
# **Remarks**:
# - For particle-particle interactions: bear in mind that the local set is a subset of the active set, so be careful about any unwanted self-interactions.
# - In this scheme, the following relation should be satisfied:
# 
# \begin{align}
# H_{\rm SPH} \leq R^{\rm peano}_{\rm cutoff} \leq \frac{\Delta x}{2}\,,
# \end{align}
# 
# where $H_{\rm SPH}$($\equiv\gamma_K h$) is the SPH interaction radius, $R^{\rm peano}_{\rm cutoff}$ is the internal cut-off radius and $\Delta x$ the cell size.

# In[ ]:


import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2

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
parser.add_argument("-r",     "--random",             dest="random_particle_layout",       action="store_true",         help="Use a randomised initial particle layout" )
parser.add_argument("-dist",  "--particle-distance",  dest="particle_distance",            required=True,  type=float,  help="Particle distance (start with 1/64 by default)" )
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

  
project = peano4.Project( ["examples", "particles"], "minimal-sph-peano-test-sedov", "." )


# ## Prepare code
# 
# Peano's API does not know which settings to use on the present system. To make it copy/clone the settings identified by ./configure, we ask it to parse the generated configuration scripts. 

# In[ ]:


build_mode = modes[args.mode]
project.output.makefile.parse_configure_script_outcome( "../../.." )
project.output.makefile.set_mode( build_mode )


# ## Model the particle
# 
# We create the particle through the toolbox. A particle by definition has a position ${\bf x}$, so we just need to add everything else for SPH:
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
# 
# We also add an array to store the new position of a particle after a timestep is completed. We will use this to move the particle at the start of the subsequent timestep.

# In[ ]:


particle = peano4.toolbox.particles.Particle("Particle")


# Define properties
v_attr = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
v_full_attr = peano4.dastgen2.Peano4DoubleArray("v_full","Dimensions")
a_prev_step_attr = peano4.dastgen2.Peano4DoubleArray("a_prev_step","Dimensions")
mass_attr    = dastgen2.attributes.Double("mass")
smoothL_attr = dastgen2.attributes.Double("smoothingLength")
density_attr = dastgen2.attributes.Double("density")
pressure_attr= dastgen2.attributes.Double("pressure")
u_attr       = dastgen2.attributes.Double("u")
u_full_attr  = dastgen2.attributes.Double("u_full")
uDot_attr    = dastgen2.attributes.Double("uDot")
uDot_prev_step_attr  = dastgen2.attributes.Double("uDot_prev_step")
soundSpeed_attr = dastgen2.attributes.Double("soundSpeed")
wcount_attr   = dastgen2.attributes.Double("wcount")
wcount_dh_attr= dastgen2.attributes.Double("wcount_dh")
Ncount_attr   = dastgen2.attributes.Integer("Ncount")

# Variable smoothing length scheme
Omega_attr = dastgen2.attributes.Double("Omega")
rho_dh_attr= dastgen2.attributes.Double("rho_dh")
hDot_attr  = dastgen2.attributes.Double("hDot")
hleft_attr = dastgen2.attributes.Double("hLeft")
hright_attr= dastgen2.attributes.Double("hRight")

# Artificial viscosity scheme
#rot_v_attr = peano4.dastgen2.Peano4DoubleArray("rot_v","Dimensions")
rot_v_attr    = dastgen2.attributes.Double("rot_v")    # For 2D sims it only has 1 component (z)
div_v_attr    = dastgen2.attributes.Double("div_v")
v_sig_AV_attr = dastgen2.attributes.Double("v_sig_AV")
balsara_attr  = dastgen2.attributes.Double("balsara")

# Needed?
has_been_kicked_attr = dastgen2.attributes.Boolean("hasBeenKicked")
has_converged_attr= dastgen2.attributes.Boolean("hasConverged")
has_no_neighbours_attr= dastgen2.attributes.Boolean("hasNoNeighbours")
smoothL_prev_attr = dastgen2.attributes.Double("smoothingLengthPrevious")
is_boundary_part_attr = dastgen2.attributes.Boolean("isBoundaryParticle")

# Debug
time_attr     = dastgen2.attributes.Double("time")
timeStepSize_attr = dastgen2.attributes.Double("timeStepSize")
residual_attr = dastgen2.attributes.Double("residual")
iteration_count_attr = dastgen2.attributes.Integer("iterCount")

particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )

# Add particle properties
particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(v_full_attr)
particle.data.add_attribute(a_prev_step_attr)
particle.data.add_attribute(mass_attr)
particle.data.add_attribute(density_attr)
particle.data.add_attribute(pressure_attr)
particle.data.add_attribute(u_attr)
particle.data.add_attribute(u_full_attr)
particle.data.add_attribute(uDot_attr)
particle.data.add_attribute(uDot_prev_step_attr)
particle.data.add_attribute(smoothL_attr)
particle.data.add_attribute(Ncount_attr)
particle.data.add_attribute(wcount_attr)
particle.data.add_attribute(wcount_dh_attr)
particle.data.add_attribute(soundSpeed_attr)

# For variable smoothing length
particle.data.add_attribute(Omega_attr)
particle.data.add_attribute(rho_dh_attr)
particle.data.add_attribute(hDot_attr)
particle.data.add_attribute(hleft_attr)
particle.data.add_attribute(hright_attr)

# Artificial viscosity
particle.data.add_attribute(rot_v_attr)
particle.data.add_attribute(div_v_attr)
particle.data.add_attribute(v_sig_AV_attr)
particle.data.add_attribute(balsara_attr)

particle.data.add_attribute(timeStepSize_attr)
particle.data.add_attribute(residual_attr)
particle.data.add_attribute(iteration_count_attr)
particle.data.add_attribute(has_no_neighbours_attr)
particle.data.add_attribute(is_boundary_part_attr)

# Needed?
particle.data.add_attribute(has_been_kicked_attr)
particle.data.add_attribute(has_converged_attr)
particle.data.add_attribute(smoothL_prev_attr)

particles = peano4.toolbox.particles.ParticleSet(particle)


# This particle has to be added to the project. This way the project knows that we are working with this guy. We associate the particle with the vertices, as we work with the pidt (particle in dual tree) algorithm. 

# In[ ]:


project.datamodel.add_global_object(particle)
project.datamodel.add_vertex       (particles)


# ## Export global SPH constants and parameters
# 
# There are plenty of constant simulation paramaters to specify for our SPH simulation - we do that here. 

# In[ ]:


Dimensions = 2
project.output.makefile.set_dimension(Dimensions)


# In[ ]:


from numpy import pi

GLOBAL_TIME_STEP_SIZE = "1e-4"

# Particle number along 1 dimension
HYDRO_PART_NUMBER = str(round(1.0 / args.particle_distance));

# Maximum allowed number of Newton-Raphson iterations
HYDRO_DEFAULT_MAX_ITERATIONS = "10"

# Tolerance for Newton-Raphson convergence
H_TOLERANCE = "1e-4"

# Time-integration parameters
CFL_CONSTANT = "0.2"

# EoS parameter
GAMMA = str(5./3.)

# SPH kernel parameters
KERNEL_SUPPORT_RAD = "2"  # "gamma" parameter. =2 for Cubic Spline. Different in other kernels.

HYDRO_h_MAX = str( 0.166 / 2.0 / 2.0) 
HYDRO_h_MIN = "1e-6"

# Artificial viscosity parameters
ALPHA_AV= "0.8"     # =0.8 in SWIFT
BETA_AV = "3"       # =3 in SWIFT/SPHENIX

# Smoothing-length free parameter (resolution "eta_neighbours"). 
# Can be set by specifying a desired number of neighbours.
if(Dimensions==2):
#    N_ngbs_mean = 10.0  
#    SMOOTH_L_FAC = str( (1.   /pi*N_ngbs_mean)**(1./2.)/float(KERNEL_SUPPORT_RAD) ) 
# SWIFT value. Should be equivalent to N_ngbs_mean = 15.14.   
    SMOOTH_L_FAC = str( 1.235 ) 
elif(Dimensions==3):
    N_ngbs_mean = 57.9
    SMOOTH_L_FAC = str( (3./4./pi*N_ngbs_mean)**(1./3.)/float(KERNEL_SUPPORT_RAD) ) 

# Kernel root value 
if(Dimensions==2):
    CUBIC_SPLINE_SIGMA = str(10/(7*pi))
elif(Dimensions==3):
    CUBIC_SPLINE_SIGMA = str(1/pi)

print("Smoothing length free factor: " + SMOOTH_L_FAC)
print("Kernel Root value (sigma): " + CUBIC_SPLINE_SIGMA)

# parameters specific to a given problem (e.g., Sedov blast, Sod wave, ...)
BLAST_CENTER = "0.5" 
BLAST_ENERGY = "0.2"
#BLAST_ENERGY = "1.0"
SPIKE_RADIUS = "0.03"

project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
project.constants.export_const_with_type("HYDRO_PART_NUMBER", HYDRO_PART_NUMBER, "double")
project.constants.export_const_with_type("HYDRO_DEFAULT_MAX_ITERATIONS", HYDRO_DEFAULT_MAX_ITERATIONS, "int")
project.constants.export_const_with_type("H_TOLERANCE", H_TOLERANCE, "double")
project.constants.export_const_with_type("KERNEL_SUPPORT_RAD"   , KERNEL_SUPPORT_RAD, "double")
project.constants.export_const_with_type("HYDRO_h_MAX", HYDRO_h_MAX, "double")
project.constants.export_const_with_type("HYDRO_h_MIN", HYDRO_h_MIN, "double")
project.constants.export_const_with_type("SMOOTH_L_FAC"         , SMOOTH_L_FAC      , "double")
project.constants.export_const_with_type("CUBIC_SPLINE_SIGMA"   , CUBIC_SPLINE_SIGMA, "double")
project.constants.export_const_with_type("BLAST_CENTER", BLAST_CENTER, "double")
project.constants.export_const_with_type("BLAST_ENERGY", BLAST_ENERGY, "double")
project.constants.export_const_with_type("SPIKE_RADIUS", SPIKE_RADIUS, "double")
project.constants.export_const_with_type("CFL_CONSTANT", CFL_CONSTANT, "double")
project.constants.export_const_with_type("GAMMA"    , GAMMA    , "double")
project.constants.export_const_with_type("ALPHA_AV" , ALPHA_AV , "double")
project.constants.export_const_with_type("BETA_AV"  , BETA_AV  , "double")


# ## Model the setup phases

# ## Initialize regular grid

# In[ ]:


mesh_size = 0.5
#mesh_size = 0.2

particle_tree_analysis = peano4.toolbox.particles.ParticleTreeAnalysis(particles)
project.datamodel.add_cell(particle_tree_analysis.cell_marker)   # read docu of ParticleTreeAnalysis

create_grid = peano4.solversteps.Step( name="CreateGrid", add_user_defined_actions=False )
create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(mesh_size) )
create_grid.use_vertex(particles)
create_grid.use_cell(particle_tree_analysis.cell_marker)
project.solversteps.add_step(create_grid)


# ## Initial conditions for particles

# In[ ]:


initialisation_snippet = """
 
  /*
   * Initial Conditions for the particular SPH test
   * 
   * 1. Sedov Blast: particles are initialised at rest in regular grid configuration and periodic BC. 
   * Energy is injected to a (or group of) central particle and the blast should propagate
   * isotropically, up to boundary effects.
   * See, e.g., Springel and Hernquist, Mon. Not. R. Astron. Soc. 333, 649–664 (2002).
   *
   */
   
  // Internal cutoff radious used by Peano. Should always be larger than actual SPH cutoff radious (2h).
  particle->setCutOffRadius(0.16);
  
  particle->setDensity ( 1.0 );        
  particle->setMass( particle->getDensity()/400.0/2.56069/1.56122/2.56/0.939281 );   // 1/64 
  /* Normalisation for actual number of particles in the simulation*/
  particle->setMass( particle->getMass() * std::pow(HYDRO_PART_NUMBER / 64.0, 2) );

//  particle->setMass    ( particle->getDensity()/400/2.56069/1.56122/2.56/0.969103 );   // 1/63 
//  particle->setPressure( 1e-6);
  
  particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0/Dimensions) );
  particle->setSmoothingLengthPrevious( 0.80 * particle->getSmoothingLength() );
 
  // Initialise the blast
  tarch::la::Vector<Dimensions,double> blast_cent(BLAST_CENTER);
  const double dist_blast = tarch::la::norm2( particle->getX() - blast_cent );
  
   /* Old method */
   /* Inject energy evenly into central particles */
   if ( tarch::la::smaller( dist_blast, SPIKE_RADIUS ) ) {
     particle->setU( BLAST_ENERGY / 12.0 / particle->getMass() );
   } else {
     particle->setU( 0.0 ); 
   }

//   /* New method */
//   /* Smooth the explosion with kernel */
//   const double h = particle->getSmoothingLength();
//   const double R_smooth = 4.0 * h;
//   
//   // Evaluate kernel
//   const double q = dist_blast / h;
//   double w, dw_dq;
//   kernel_deval(q, w, dw_dq);
// 
//   // Normalization factors 
//   const double W = w / std::pow( h, Dimensions );
// 
//  if ( tarch::la::smallerEquals( dist_blast, R_smooth ) ) {
//    particle->setU( BLAST_ENERGY * W ) ;
//  } else {
//    particle->setU( 0.0 ); 
//  }

  /* Set non-zero (background) value in order to get non-zero sound speed across the whole domain */
  if( tarch::la::equals(particle->getU(),0) ){
//    particle->setU( particle->getPressure()/( (GAMMA-1) * particle->getDensity() ) ); 
    particle->setU( 1e-6 ); 
  }

  /* Compute the pressure from the EoS */
  const double pressure = gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
  
  particle->setPressure( pressure );
  
  // Evaluate (global) time-step size using u from exploding particle (h_ini is homogeneous)
  const double soundspeed = std::sqrt( GAMMA * particle->getPressure()/particle->getDensity() );
  particle->setSoundSpeed(soundspeed);
  
  /* Initialize remaining fields  */
  
  // Vectors
  for(int d=0; d<Dimensions; d++){
     particle->setV          (d,0);
     particle->setA          (d,0);
     particle->setA_prev_step(d,0);
  }
 
  /* For 2D sims, we only get one component */
  particle->setRot_v(0);
  
  /* Initialise the "extended" arrays */
  particle->setV_full(particle->getV());
  particle->setU_full(particle->getU());

  particle->setUDot  (0.0);        
  particle->setUDot_prev_step(0.0);        
  particle->setHDot  (0.0);        
  particle->setWcount(0.0);        
  particle->setNcount(0  );        
  particle->setOmega (0.0);        
  particle->setRho_dh(0.0);           
  particle->setWcount_dh(0.0);  

  particle->setHLeft (HYDRO_h_MIN);        
  particle->setHRight(HYDRO_h_MAX);        
  
  particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
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
  distance_between_particles=args.particle_distance,
  initialisation_call=initialisation_snippet,
  noise=args.random_particle_layout,
  additional_includes=""" 
    #include "Constants.h" 
    #include "kernel_hydro.h"
    #include "equation_of_state.h"
    """,
  computational_domain_offset=[0.0, 0.0]
  #, guard="std::abs( 0.5-particle->getX(1) ) < 0.02"
  ))

init_setup.add_action_set( particle_tree_analysis )
project.solversteps.add_step(init_setup)


# # Model the steps of the simulation
# 
# In the following, we model each step that will be invoked by main.cpp to run the simulation. The generated C++ code based on these will be outputted into `/observers/`

# ## Density loop

# In[ ]:


calc_densities = peano4.solversteps.Step( "ComputeDensityAndUpdateSmoothingLength" )
calc_densities.use_vertex(particles)
calc_densities.use_cell(particle_tree_analysis.cell_marker)

touch_particle_first_time_compute_kernel = """

  /* This is equivalent to the hydro_init_part function in SWIFT */

  /* @brief Prepares a particle for the density calculation  
   *
   * Zeroes all the relevant arrays in perparation for the sums taking place
   * in the loop over neighbours (local-active set)
   *
   */

  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
//  and localParticle->getHasConverged()==false 
  and not localParticle->getIsBoundaryParticle()
  ) {  

    // we'll accumulate the data throughout mesh/particle traversal
    localParticle->setDensity (0.0);                      
    localParticle->setWcount  (0.0);                      
    localParticle->setRho_dh  (0.0);   
    localParticle->setWcount_dh(0.0);   
    localParticle->setDiv_v   (0.0);   
    localParticle->setRot_v   (0.0);   

    /* Not needed in SWIFT */
    localParticle->setNcount  (0);
    localParticle->setOmega   (0.0);   

  } 
    

"""

#
# @todo Tobias: It is not clear if this is clever. If we have only particle-particle 
# interaction kernels, then we can't have an inner while loop per particle. So teh 
# corsett might not be beneficial performance-wisely.
#
particle_particle_compute_kernel = """

 /*
  * Non-symmetric density calculation (as per Swift terminology). 
  
  * In this, only the local particle is updated (in the symmetric case, both 
  * are updated at the same time). This compute kernel is inserted by Peano's
  * Python API into the nested loop over active and local particles. So we can
  * assume that those two terms do exist.
  *
  * The present kernel is a sole particle-particle interaction kernel.
  *
  *
  * Here we calculate the density, Omega and drho/dt sums.
  * The latter can be used in the Newton-Raphson iteration.
  *
  * Subscripts a and b represent indices of Local and Active particles, respectively:
  *
  * h_a: Smoothing length of Local particle
  * h_b: Smoothign length of Active particle
  * dx : Vector separating the two particles (x_loc - x_act)
  * r  : Distance between the two particles  (|| dx ||)
  *
  */


  tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - activeParticle->getX();
  const double r = tarch::la::norm2(dx);

  // Velocity terms for viscosity
  tarch::la::Vector<Dimensions,double> dv_ab;
  tarch::la::Vector<Dimensions,double> curlvr(0.0);

  // Retrieve basic properties of local and active particles
  const double rho_b = activeParticle->getDensity();

  const double m_a = localParticle ->getMass();  
  const double m_b = activeParticle->getMass();  

  const double h_a = localParticle->getSmoothingLength();

  // Normalized distance to evaluate kernel
  const double q_a = r / h_a;
  
  // Get actual interaction radius
  const double cutoffR = KERNEL_SUPPORT_RAD*h_a;

  if (
    tarch::la::smaller( r, cutoffR ) 
    and 
    tarch::la::greater( r, 0.0 )         /* Self-contribution is calculated elsewhere */  
//  and localParticle->getHasConverged()==false
    and 
    not localParticle->getIsBoundaryParticle()
  ) {  
      // Evaluate kernel and its derivative
      double w_a, dw_a_dq;
      kernel_deval(q_a, w_a, dw_a_dq);

      // Normalization factors 
      double W_a, dW_a_dh;
      W_a = w_a / std::pow( h_a, Dimensions );
      dW_a_dh = - ( Dimensions*w_a + q_a*dw_a_dq ) / std::pow( h_a, Dimensions +1 );

      // Increment density of Local particle
      localParticle->setDensity( localParticle-> getDensity() + m_b * W_a ); 

      // Increment number density as well
      localParticle->setWcount ( localParticle-> getWcount () +       W_a );
      
      // Increment exact number of neighbours (for debugging)
      localParticle->setNcount ( localParticle-> getNcount () + 1 ); 

      // Increment drho/dh
      localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_b*dW_a_dh ); 

      // Increment Omega of Local particle
      localParticle->setOmega    ( localParticle->getOmega()     + m_b*dW_a_dh );

      // Increment dWcount/dh
      localParticle->setWcount_dh( localParticle->getWcount_dh() +     dW_a_dh ); 

      assertion3( not std::isnan( localParticle->getOmega() ), localParticle->toString(), localParticle->getOmega(), dW_a_dh );
      
      /* Now we need to compute the div terms */
          
      const double r_inv = r ? 1.0 / r : 0.0;     
      const double fac_a = m_a * dw_a_dq * r_inv;

      /* Compute dv dot r */
      dv_ab = localParticle->getV() - activeParticle->getV();
      const double dvdr = tarch::la::dot( dv_ab , dx );
      localParticle->setDiv_v( localParticle->getDiv_v() - fac_a * dvdr );
      
      /* Compute dv cross r */
      /* 2D Term only has 1 component (z) */
      const double curlvr2D = dv_ab[0]*dx[1] - dv_ab[1]*dx[0];
      localParticle->setRot_v( localParticle->getRot_v() + fac_a * curlvr2D );
    } 
"""

touch_particle_last_time_compute_kernel = """
  /* This is equivalent to the hydro_end_density function in SWIFT */

  /* @brief Finishes the calculations from the density loop
   *
   * Multiplies the denstiy and number of neighbours by the appropiate constants
   * and add the self-contribution term.
   * Additional quantities such as the velocity gradients will also get the final
   * terms added to them here.
   *
   * Also adds/multiplies the cosmological terms if need be.
   *
   */

  if(
    localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
//  and localParticle->getHasConverged()==false
    and 
    not localParticle->getIsBoundaryParticle()
  ) {
    /* Retrieve some properties */
    const double m_a = localParticle->getMass();
    const double h_a = localParticle->getSmoothingLength();

    /* Some smoothing length multiples */
    const double h_inv = 1.0 / h_a;
    const double h_inv_dim = std::pow( h_inv, Dimensions );
    const double h_inv_dim_plus_one = h_inv_dim * h_inv;

    /* Default SPH kernel (cubic spline) */
    double W_a = CUBIC_SPLINE_SIGMA ; // kernel form at q=0

    /* Final operation. Add the self-contributions (i.e. at q=0) */
    localParticle->setDensity  ( localParticle->getDensity()   + m_a * W_a * h_inv_dim );
    localParticle->setWcount   ( localParticle->getWcount()    +       W_a * h_inv_dim );
    localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_a * (-Dimensions * W_a * h_inv_dim_plus_one) );
    localParticle->setWcount_dh( localParticle->getWcount_dh() +       (-Dimensions * W_a * h_inv_dim_plus_one) );

    /* Not needed in SWIFT */
    localParticle->setNcount   ( localParticle->getNcount()    + 1 );
    localParticle->setOmega    ( localParticle->getOmega()     + m_a * (-Dimensions * W_a * h_inv_dim_plus_one) );

    const double rho_inv = 1.0 / localParticle->getDensity();
    // @TODO: implement cosmological factors
    const double a_inv2 = 1.0;

    /* Finish calculation of the velocity curl components */
    localParticle->setRot_v( localParticle->getRot_v() * h_inv_dim_plus_one * a_inv2 * rho_inv);

    /* Finish calculation of the velocity divergence */
    localParticle->setDiv_v( localParticle->getDiv_v() * h_inv_dim_plus_one * a_inv2 * rho_inv);
    
    
    //
    // =====================
    // prepare forces
    // =====================
    //
    {
      /* Retrieve basic properties of local and active particles */
      const double rho_a = localParticle->getDensity();
      const double m_a = localParticle->getMass();  
      const double h_a = localParticle->getSmoothingLength();
 
        const double h_inv = 1.0/h_a;
      
        /* Compute the norm of the curl */
        /* 2D sims */
        const double curl_v = std::sqrt( localParticle->getRot_v() * localParticle->getRot_v() );
      
        /* Compute the norm of div v */
        const double abs_div_v = tarch::la::abs( localParticle->getDiv_v() );
        
        /* Compute the sound speed */
        const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );
    
        // Construct final expression of Omega
        /* Swift Expression for equivalent to Omega term ("f", see also Springel and Hernquist) */
       
        const double dh_drho_a = - h_a / rho_a / Dimensions;
        const double common_factor = h_a / Dimensions / localParticle->getWcount();
        
        double grad_h_term;
        const double grad_W_term = common_factor * localParticle->getWcount_dh();
        
        // Avoid problematic particles
        if(grad_W_term < -0.9999){
             /* if we get here, we either had very small neighbour contributions
               (which should be treated as a no neighbour case in the ghost) or
               a very weird particle distribution (e.g. particles sitting on
               top of each other). Either way, we cannot use the normal
               expression, since that would lead to overflow or excessive round
               off and cause excessively high accelerations in the force loop */
          grad_h_term = 0;
        } else {
          grad_h_term = common_factor * localParticle->getRho_dh()/(1.0 + grad_W_term);
        }
    
        /* Compute the Balsara Switch */
        const double balsara = ALPHA_AV * abs_div_v /
                               (abs_div_v + curl_v + 0.0001 * soundspeed * h_inv);
                                   
        assertion6( std::isfinite(balsara), localParticle->toString(), balsara, abs_div_v, 
                                            curl_v, soundspeed, ALPHA_AV );
      
        /* Update variables */
        const double P_a = gas_pressure_from_internal_energy( rho_a, localParticle->getU() );
        
        /* Grad-h-term (Omega) */
        /* SWIFT */
        localParticle->setOmega( grad_h_term );  
        localParticle->setPressure(P_a);
        localParticle->setSoundSpeed(soundspeed);
        localParticle->setBalsara(balsara);
    }  
    
    
    //
    // =============================
    // Newton's Apples
    // =============================
    //
    {
      const double hydro_eta_dim = std::pow(SMOOTH_L_FAC, Dimensions);
      const double eps = H_TOLERANCE; 

      /* h value at the start of iteration procedure 
       * Here we use the last update on previous time-step */
      const double h_0 = localParticle->getSmoothingLengthPrevious();

      /* Get some useful values */
      const double h_init = h_0;
      const double h_old = localParticle->getSmoothingLength();
      const double h_old_dim = std::pow(h_old, Dimensions);
      const double h_old_dim_minus_one = std::pow(h_old, Dimensions - 1);
      
      double h_new;
      int has_no_neighbours = 0;
    
      if( localParticle->getWcount() < 1e-5 ) { /* No neighbours case */
        logInfo( "touchVertexLastTime()", "no neighbour case for particle " << localParticle->toString() );

       /* Flag that there were no neighbours */
       has_no_neighbours = 1;
       localParticle->setHasNoNeighbours(true);

       /* Double h and try again */
       h_new = 2.0 * h_old;

     } else {

       /* Finish the density calculation */
       /* Skip this as this is already done in touch last step of Density loop */
       //       hydro_end_density(p, cosmo);

       /* Compute one step of the Newton-Raphson scheme */
       const double n_sum = localParticle->getWcount() * h_old_dim;
       const double n_target = hydro_eta_dim;
       const double f = n_sum - n_target;
       const double f_prime = localParticle->getWcount_dh() * h_old_dim +
                                 Dimensions * localParticle->getWcount() * h_old_dim_minus_one;

       /* Improve the bisection bounds */
       if (n_sum < n_target) {
         localParticle->setHLeft ( std::max(localParticle->getHLeft (), h_old) );
       } else if (n_sum > n_target) {
         localParticle->setHRight( std::min(localParticle->getHRight(), h_old) );
       }
 
       /* Skip if h is already h_max and we don't have enough neighbours */
       /* Same if we are below h_min */
       /* Same if we have hit the Peano cutoff radius 
        */
       tarch::la::Vector<Dimensions,double> GridSize = marker.h();
       const double GridSizeHalf = 0.5 * GridSize(0); 
       const double CutoffParticleH = KERNEL_SUPPORT_RAD * localParticle->getSmoothingLength();
 
       if ( ((localParticle->getSmoothingLength() >= HYDRO_h_MAX) && (f < 0.0)) ||
            ((localParticle->getSmoothingLength() <= HYDRO_h_MIN) && (f > 0.0)) ||
              tarch::la::greaterEquals(CutoffParticleH, GridSizeHalf) ){

         /* We have a particle whose smoothing length is already set (wants
          * to be larger but has already hit the maximum OR wants to be
          * smaller but has already reached the minimum). So, just tidy up
          * as if the smoothing length had converged correctly  */

         // Debug
//         assertion9( localParticle->getSmoothingLength()<1e-5,                                                                                                                                                                                                              
//               localParticle->toString(), h_new, h_old, f, CutoffParticleH, GridSizeHalf, HYDRO_h_MIN, HYDRO_h_MAX, GridSize(0) );       

         /* Here SWIFT would prepare the gradient and force calculations 
          * Skipped. 
          */

         /* Ok, we are done with this particle */
//         continue;
       }

       /* Normal case: Use Newton-Raphson to get a better value of h */

       /* Avoid floating point exception from f_prime = 0 */
//      h_new = h_old - f / (f_prime + FLT_MIN);
       h_new = h_old - f / (f_prime );
         
       /* Be verbose about the particles that struggle to converge */
       /* @TODO */

       // Debug
//       assertion6( f!=0,                                                                                                                                                                                                              
//             localParticle->toString(), h_new, h_old, CutoffParticleH, HYDRO_h_MIN, HYDRO_h_MAX );       

//       assertion3( ((f > 0.0 && h_new > h_old) || (f < 0.0 && h_new < h_old)) &&
//              (h_old < 0.999 * HYDRO_h_MAX), localParticle->toString(), h_new, h_old );

       /* Safety check: truncate to the range [ h_old/2 , 2h_old ]. */
       h_new = std::min(h_new, 2.0 * h_old);
       h_new = std::max(h_new, 0.5 * h_old);

       /* Verify that we are actually progressing towards the answer */
       h_new = std::max( h_new, localParticle->getHLeft ());
       h_new = std::min( h_new, localParticle->getHRight());
     }   // end of if( localParticle->getWcount() < 1e-5 ) else block

     /* By now we have got h_new for either of the yes/no neighbour cases */

     /* Check whether the particle has an inappropriate smoothing length
      */
     if (std::abs(h_new - h_old) > eps * h_old) {
  
       /* Ok, correct then */
  
       /* Case where we have been oscillating around the solution */
       if( (h_new == localParticle->getHLeft() && h_old == localParticle->getHRight()) ||
           (h_old == localParticle->getHLeft() && h_new == localParticle->getHRight()) ){
  
         /* Bissect the remaining interval */
         localParticle->setSmoothingLength( 1.0 / std::pow (
           0.5 * (std::pow( localParticle->getHLeft(),Dimensions ) + std::pow( localParticle->getHRight(),Dimensions) ), Dimensions ) );
  
       } else { 
         
         /* Normal case */
         localParticle->setSmoothingLength( h_new );
       } 

     }

     /* By now the smoothing length has been updated */

     /* Increase the iteration counter */
     localParticle->setIterCount( localParticle->getIterCount() + 1 );

     /* Compute residual */
     assertion1( h_old>tarch::la::NUMERICAL_ZERO_DIFFERENCE, localParticle->toString() );
     const double residual = std::abs(h_new - h_old)/h_old;
     localParticle->setResidual ( residual );
    
     /* Flag if particle has converged */
     if(residual < eps){
       localParticle->setHasConverged(true);
     }
    }
  }
"""


calc_densities.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel=particle_particle_compute_kernel,
  touch_vertex_first_time_compute_particle_update_kernel=touch_particle_first_time_compute_kernel,
  touch_vertex_last_time_compute_particle_update_kernel=touch_particle_last_time_compute_kernel,
  additional_includes="""
#include "Constants.h"
#include "kernel_hydro.h"
#include "equation_of_state.h"
  """
))
#calc_densities_var_h.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
calc_densities.add_action_set( particle_tree_analysis )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(calc_densities)




# ## Force loop

# In[ ]:


force_non_sym_particles = peano4.solversteps.Step( "ForceNonSym" )
force_non_sym_particles.use_vertex(particles)
force_non_sym_particles.use_cell(particle_tree_analysis.cell_marker)
force_non_sym_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
  
/*
 * Non-symmetric force interaction between two particles (as per Swift terminology). 
 * In this, only the Local particle is updated (in the symmetric case, both are updated at the same time).
 * Time derivative of the internal energy, du/dt, is also updated here.
 * Subscripts a and b represent indices of Local and Active particles, respectively:
 *
 * h_a: Smoothing length of Local particle
 * h_b: Smoothign length of Active particle
 * dx : Vector separating the two particles (x_loc - x_act)
 * r  : Distance between the two particles  (|| dx ||)
 *
 */
  
  
  /* Distance between the two particles */
  tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - activeParticle->getX();
  const double r = tarch::la::norm2(dx);
  const double r_inv = r ? 1.0 / r : 0.0;

  // Retrieve basic properties of local and active particles
  const double rho_a = localParticle ->getDensity();
  const double rho_b = activeParticle->getDensity();
  
  const double P_a = localParticle ->getPressure();
  const double P_b = activeParticle->getPressure(); 
  
  const double m_a = localParticle ->getMass();  
  const double m_b = activeParticle->getMass();  

  // Smoothing lengths
  const double h_a = localParticle ->getSmoothingLength();
  const double h_b = activeParticle->getSmoothingLength();

  // Dimensionless distance to evaluate kernel
  const double q_a = r / h_a;
  const double q_b = r / h_b;
  
  // Get actual interaction radius
  const double cutoffR = KERNEL_SUPPORT_RAD*h_a;
 
  if ( tarch::la::smaller( r,cutoffR ) ) {
    if ( tarch::la::greater( r,0.0 ) ) {

      // Evaluate kernel and its derivative for particle a
      double w_a, dw_a_dq;
      kernel_deval(q_a, w_a, dw_a_dq);

      // Evaluate kernel and its derivative for particle b
      double w_b, dw_b_dq;
      kernel_deval(q_b, w_b, dw_b_dq);

      // Normalization factors 
      double wa_dr = dw_a_dq / std::pow( h_a, Dimensions + 1 );
      double wb_dr = dw_b_dq / std::pow( h_b, Dimensions + 1 );

      /* Variable smoothing length term */
      const double f_ab = 1.0 - localParticle ->getOmega()/m_b;
      const double f_ba = 1.0 - activeParticle->getOmega()/m_a;

      /* Compute gradient terms */
      const double P_over_rho2_a = P_a / (rho_a * rho_a) * f_ab;
      const double P_over_rho2_b = P_b / (rho_b * rho_b) * f_ba;

      /* Compute dv dot r. */
      double dvdr = tarch::la::dot( (localParticle->getV() - activeParticle->getV()), dx );

      /* Are the particles moving towards each others ? */
      const double omega_ab = std::min(dvdr, 0.0);
      const double mu_ab = r_inv * omega_ab; /* This is 0 or negative */
  
      /* Compute sound speeds and signal velocity */
      const double v_sig = localParticle->getSoundSpeed() + activeParticle->getSoundSpeed() -
                           BETA_AV * mu_ab;
  
      /* Balsara term */
      const double balsara_a = localParticle ->getBalsara();
      const double balsara_b = activeParticle->getBalsara();
  
      /* Construct the full viscosity term */
      const double rho_ab = 0.5 * (rho_a + rho_b);
      const double visc = -0.25 * v_sig * mu_ab * (balsara_a + balsara_b) / rho_ab;

      /* Convolve with the kernel */
      const double visc_acc_term = 0.5 * visc * (wa_dr * f_ab + wb_dr * f_ba) * r_inv;

      /* SPH acceleration term */
      const double sph_acc_term = (P_over_rho2_a * wa_dr + P_over_rho2_b * wb_dr) * r_inv;
      
      /* Assemble the acceleration */
      const double acc = sph_acc_term + visc_acc_term;
            
      /* Use the force Luke ! */
      assertion1( not tarch::la::isEntryNan( localParticle->getA() ), localParticle->toString() );
      localParticle->setA( localParticle->getA() - m_b * acc * dx );
              
      /* Get the time derivative for u. */
      const double sph_du_term_a = P_over_rho2_a * dvdr * r_inv * wa_dr;

      /* Viscosity term */
      const double visc_du_term = 0.5 * visc_acc_term * dvdr;

      /* Assemble the energy equation term */
      const double du_dt_a = sph_du_term_a + visc_du_term;

      /* Internal energy time derivative */
      localParticle->setUDot(localParticle->getUDot() + m_b * du_dt_a);

      /* Get the time derivative for h. */
      localParticle->setHDot(localParticle->getHDot() - m_b * dvdr * r_inv/rho_b * wa_dr * f_ab);

      /* Update the signal velocity */
      localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), v_sig) );
    }
  }
  
""",

  touch_vertex_first_time_compute_particle_update_kernel="""

  /* Done by "hydro_reset_acceleration" in SWIFT */
  
  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

    /* Reset the acceleration*/
    localParticle->setA   (0.0);   
    
    /* Reset the time derivatives */
    localParticle->setUDot(0.0);   
    localParticle->setHDot(0.0);   
    localParticle->setV_sig_AV(2.0 * localParticle->getSoundSpeed());

  }

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

   /* Done by "hydro_end_force" does in SWIFT */

   if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

     localParticle->setHDot( 
       localParticle->getHDot() * localParticle->getSmoothingLength() / Dimensions);
  
   }

""",
  additional_includes="""
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

# In[ ]:


kick1_particles = peano4.solversteps.Step( "Kick1" )
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
  
  if (
      not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    ){
  
    /* Kick the particle */
    localParticle->setV_full( localParticle->getV_full() + dt_kick_hydro * localParticle->getA() );
    
    /* If predictor is going to be used, store old acceleration */
    localParticle->setA_prev_step( localParticle->getA() );

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
kick1_particles.add_action_set( particle_tree_analysis )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
kick1_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(kick1_particles)


# ## Kick 2
# 
# Same as kick 1 but does not update the accelation array of previous time step

# In[ ]:


kick2_particles = peano4.solversteps.Step( "Kick2" )
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
    
    /* Re-set the entropy */
    localParticle->setU( localParticle->getU_full() );

    /* Re-compute the pressure */
    const double pressure = 
      gas_pressure_from_internal_energy( localParticle->getDensity(), localParticle->getU() );

    /* Compute the new sound speed */
    const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );
 
    /* Update variables */
    localParticle->setPressure(pressure);
    localParticle->setSoundSpeed(soundspeed);
    localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), 2.0 * soundspeed) );

  }
    
""",
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


# ## Drift step (+prediction)
# 
# In this step, we update the particle's position via
# 
# \begin{align}
# {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t}
# \end{align}
# 
# We also need to get predicted values of some quantities.

# In[ ]:


drift_particles = peano4.solversteps.Step( "Drift" )
drift_particles.use_vertex(particles)
drift_particles.use_cell(particle_tree_analysis.cell_marker)
drift_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
  /* We update the position in this stage to avoid updating multiple times 
   * when a particle moves into a different cell. */
  
  const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
  
  if (
    localParticle->getMoveState()!=globaldata::Particle::MoveState::Moved
    and
    localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    ){
  
    /* Drift the particle (update position for a full time-step) */
    localParticle->setX( localParticle->getX() + timeStepSize * localParticle->getV_full() );
    
    /* Flag this particle as Moved */
    localParticle->setMoveState( globaldata::Particle::MoveState::Moved );
  }
  
    
""",
  additional_includes="""
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


# ## Hydro_kick_extra

# In[ ]:


hydro_kick_extra = peano4.solversteps.Step( "HydroKickExtra" )
hydro_kick_extra.use_vertex(particles)
hydro_kick_extra.use_cell(particle_tree_analysis.cell_marker)
hydro_kick_extra.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
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
  
/**
 * @brief Kick the additional variables
 *
 * Additional hydrodynamic quantites are kicked forward in time here. These
 * include thermal quantities (thermal energy or total energy or entropy, ...).
 *
 */

  const double dt_therm = 0.5 * GLOBAL_TIME_STEP_SIZE;
  
  if (not localParticle->getHasBeenKicked()
  and
  localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      
    /* Integrate the internal energy forward in time */
    const double delta_u = localParticle->getUDot() * dt_therm;
  
    /* Do not decrease the energy by more than a factor of 2 */
    localParticle->setU_full( std::max( localParticle->getU_full() + delta_u,
                                        localParticle->getU_full() * 0.5      ) );

    /* Check against absolute minimum.
     * Default value is zero. */
    if (localParticle->getU_full() < 0.0) {
      localParticle->setU_full(0.0);
      localParticle->setUDot(0.0);
    }
    
  }
    
""",
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

##  Prediction step (relevant when using local time-stepping)

predict_hydro_particles = peano4.solversteps.Step( "PredictHydro" )
predict_hydro_particles.use_vertex(particles)
predict_hydro_particles.use_cell(particle_tree_analysis.cell_marker)
predict_hydro_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""

  /* We do not excecute any specific action here */

""",
  touch_vertex_last_time_compute_particle_update_kernel="""

  /* Predict hydro terms (SWIFT) 
   * Relevant for inactive particles when using local-timestepping */
  
  const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
  
  const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;
  const double dt_drift = GLOBAL_TIME_STEP_SIZE;
  const double dt_therm = dt_kick_hydro;
//  const double dt_therm = dt_drift;
  
  if (localParticle->getParallelState()==globaldata::Particle::ParallelState::Local ) {
  
      /* Predict velocities for hydro terms */
      localParticle->setV( localParticle->getV() + localParticle->getA() * dt_kick_hydro );

      /* 
       * Predict additional particle fields forward in time after the Drift step
       */
      
      /* Predict the internal energy */
      localParticle->setU( localParticle->getU() + localParticle->getUDot() * dt_therm );

      /* Predict smoothing length */
      const double h_inv = 1.0 / localParticle->getSmoothingLength();
      const double w1 = localParticle->getHDot() * h_inv * dt_drift;
      localParticle->setSmoothingLength( localParticle->getSmoothingLength() * std::exp(w1) );
      
      /* SWIFT custom exponential functions -- @TODO implement */
//      
//    if (fabsf(w1) < 0.2f) {
//      p->h *= approx_expf(w1); /* 4th order expansion of exp(w) */
//    } else {
//      p->h *= expf(w1);
//    }
//

      /* Predict density */
      const double w2 = - Dimensions * w1;
      localParticle->setDensity( localParticle->getDensity() * std::exp(w2) );

//
//    if (fabsf(w2) < 0.2f) {
//      p->rho *= approx_expf(w2); /* 4th order expansion of exp(w) */
//    } else {
//      p->rho *= expf(w2);
//    }
//

      /* Check against entropy floor */
//      const float floor_A = entropy_floor(p, cosmo, floor_props);
//      const float floor_u = gas_internal_energy_from_entropy(p->rho, floor_A);

      /* Check against absolute minimum */
//      const float min_u =
//          hydro_props->minimal_internal_energy / cosmo->a_factor_internal_energy;

//      p->u = max(p->u, floor_u);
//      p->u = max(p->u, min_u);

      localParticle->setU( std::max( localParticle->getU(), 0.0) );

      /* Compute the new pressure */
      const double pressure = 
        gas_pressure_from_internal_energy( localParticle->getDensity(), localParticle->getU() );

      /* Compute the new sound speed */
//      const float soundspeed = gas_soundspeed_from_pressure(p->rho, pressure);

      const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );

      localParticle->setPressure(pressure);
      localParticle->setSoundSpeed(soundspeed);
      localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), 2.0 * soundspeed) );
    
    }
    
""",
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



# ## Corrector step (mini-kick) (Phantom code implementation)

# In[ ]:


kick_particles_corrector = peano4.solversteps.Step( "KickCorrector" )
kick_particles_corrector.use_vertex(particles)
kick_particles_corrector.use_cell(particle_tree_analysis.cell_marker)
kick_particles_corrector.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      localParticle->setHasBeenKicked(false);
    }
    
""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
  const double timeStepSize_half = 0.5 * GLOBAL_TIME_STEP_SIZE;
  
  if( not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
 ){
  
    /* Kick the particle */
    localParticle->setV_full( localParticle->getV() + 
                         timeStepSize_half * (localParticle->getA() - localParticle->getA_prev_step()) );

    localParticle->setHasBeenKicked(true);

    /* Store old acceleration for a potential additional corrector step */
    localParticle->setA_prev_step( localParticle->getA() );  
    
  }
    
""",
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


# ## Corrector for Extra (internal energy)

# In[ ]:


kick_corrector_extra = peano4.solversteps.Step( "KickCorrectorExtra" )
kick_corrector_extra.use_vertex(particles)
kick_corrector_extra.use_cell(particle_tree_analysis.cell_marker)
kick_corrector_extra.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      localParticle->setHasBeenKicked(false);
    }
    
""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
  const double timeStepSize_half = 0.5*GLOBAL_TIME_STEP_SIZE;
  
  if( not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
  
    /* Kick the internal energy of the particle */
    
    localParticle->setU_full( localParticle->getU_full() + 
                         timeStepSize_half * (localParticle->getUDot() - localParticle->getUDot_prev_step()) );

    localParticle->setHasBeenKicked(true);

    /* Store old value for a potential additional corrector step */
    localParticle->setUDot_prev_step( localParticle->getUDot() );  
    
  }
    
""",
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


# ## Reset HasConverged

# In[ ]:


reset_HasConverged_step = peano4.solversteps.Step( "ResetHasConverged" )
reset_HasConverged_step.use_vertex(particles)
reset_HasConverged_step.use_cell(particle_tree_analysis.cell_marker)
reset_HasConverged_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
  /* We do not excecute any specific action here */

    
""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
  /* Reset HasConverged property back to false so we can perform new N-R iterations */
  
  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
    
    localParticle->setHasConverged(false);
    
  }
    
""",
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


# ## Initialize some variables for the current time-step

# In[ ]:


init_time_step = peano4.solversteps.Step( "InitTimeStep" )
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
  
    /* Store value of h at the beggining of current time-step */
    localParticle->setSmoothingLengthPrevious( localParticle->getSmoothingLength() );
        
    /* Reset for N-R of this time-step */
    localParticle->setHasConverged(false);
    localParticle->setIterCount(0);

    /* Reset the bisection bounds */
    localParticle->setHLeft (HYDRO_h_MIN);        
    localParticle->setHRight(HYDRO_h_MAX);        
    
    /* Flag if particle is at boundary of the simulation box */
    localParticle->setIsBoundaryParticle(false);
    const double max_dist_boundary = 0.05;
    for (int d=0; d<Dimensions; d++) {
      if ( localParticle->getX()(d) < max_dist_boundary || 
           localParticle->getX()(d) > (1.0 - max_dist_boundary) ) {

        localParticle->setIsBoundaryParticle(true);

      }
    }

    
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

# In[ ]:


impose_BC_particles = peano4.solversteps.Step( "ImposeBC" )
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
impose_BC_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
impose_BC_particles.add_action_set( particle_tree_analysis )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
impose_BC_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(impose_BC_particles)


# ## End time-step
# Actions executed once at the end of the time-step

# In[ ]:


end_time_step = peano4.solversteps.Step( "EndTimeStep" )
end_time_step.use_vertex(particles)
end_time_step.use_cell(particle_tree_analysis.cell_marker)
end_time_step.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
    /* Mark particle needs to be updated */

    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      localParticle->setHasBeenKicked(false);
    }
    
""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
  const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
  
  if( not localParticle->getHasBeenKicked()
      and
      localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
 ){
      
    /* Calculate max time-step size value allowed by the CFL condition */
    const double soundspeed = localParticle->getSoundSpeed();
    const double timeStepSizeCFL = CFL_CONSTANT * localParticle->getSmoothingLength()*KERNEL_SUPPORT_RAD/soundspeed;
    localParticle->setTimeStepSize(timeStepSizeCFL);
    
    assertion1( tarch::la::greaterEquals( localParticle->getTimeStepSize(), timeStepSize ), 
                 localParticle->toString() );

    /* Mark particle is done */
    localParticle->setHasBeenKicked(true);
    
  }
    
""",
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


# ## For Debugging: collapse problem into 1D
# 
# This is an initial, extra step to move particles into a straight line when the simulation starts.

# In[ ]:


rearrange_particles = peano4.solversteps.Step( "RearrangeAtStart" )
rearrange_particles.use_vertex(particles)
rearrange_particles.use_cell(particle_tree_analysis.cell_marker)
rearrange_particles.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set=particles,
  particle_particle_interaction_kernel="""
    
  /* We do not excecute any specific action here */
  
""",
  touch_vertex_first_time_compute_particle_update_kernel="""
  
  /* We do not excecute any specific action here */

""",
  touch_vertex_last_time_compute_particle_update_kernel="""
  
/*
  // 1D test -- put particle into straight lines

  if( localParticle->getX(1)>0.51 ){
    localParticle->setX(1, 0.99);
  }else if(localParticle->getX(1)<0.49){
    localParticle->setX(1, 0.01);
  }
  
*/  


/*
  if( localParticle->getX(0)<1e-2 ){
    localParticle->setX(0, -5);
  } else if( localParticle->getX(1)<1e-2 ){
    localParticle->setX(1, -5);
  }  
*/  
  
    
""",
  additional_includes="""
#include "Constants.h"
#include <cmath>
"""
  ))
#rearrange_particles.add_action_set( peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis,min_particles_per_cell=10) )
rearrange_particles.add_action_set( particle_tree_analysis )
rearrange_particles.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
rearrange_particles.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(rearrange_particles)


# ### Plots setup
# 
# I will have to plot the outcome (or any snapshot). With particles, each of my dumps produces two types of files: particle data and mesh data. For the mesh, I rely on the toolbox's grid plotting. The plot is the step where we don't need the AMR criterion - the particles don't move here after all. We however run the tree analysis, so it is up-to-date for the next step.

# In[ ]:


# Particle plotter
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particles, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(v_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(a_attr       , Dimensions)
particle_plotter.add_attribute_to_plot(a_prev_step_attr, Dimensions)
particle_plotter.add_attribute_to_plot(u_attr       , 1)
particle_plotter.add_attribute_to_plot(mass_attr    , 1)
particle_plotter.add_attribute_to_plot(density_attr , 1)
particle_plotter.add_attribute_to_plot(pressure_attr, 1)
particle_plotter.add_attribute_to_plot(Omega_attr ,  1)
particle_plotter.add_attribute_to_plot(rho_dh_attr,  1)
particle_plotter.add_attribute_to_plot(Ncount_attr,  1)
particle_plotter.add_attribute_to_plot(wcount_attr,  1)
particle_plotter.add_attribute_to_plot(wcount_dh_attr,  1)
particle_plotter.add_attribute_to_plot(soundSpeed_attr, 1)
particle_plotter.add_attribute_to_plot(timeStepSize_attr, 1)
particle_plotter.add_attribute_to_plot(smoothL_attr , 1)
particle_plotter.add_attribute_to_plot(smoothL_prev_attr , 1)
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


# ## Generate the actual C++ code
# 
# Next, we generate the actual Peano 4 C++ code:

# In[ ]:


# For clang, we need to add the "-lstdc++fs" flag to the makefile
# We can do this manually in the generated code, or use

#project.output.makefile.add_library("-lstdc++fs")

# which, however, has to be modified to remove the "-l" appended by default
# Otherwise we can just compile
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

# ## Compile the code

# In[ ]:


# We can do this here or in the terminal.
project.build()


# ## Run code

# In[ ]:


# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)

# success = project.run( args=["--threads", "1"], prefix=["mpirun", "-n", "1"] )
#!./peano4


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
