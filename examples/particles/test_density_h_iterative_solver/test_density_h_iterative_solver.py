#!/usr/bin/env python
# coding: utf-8

# (Unit?) test for the Newton-Raphson iterative solver.
# This is used to solve the density and smoothing length of the particles.

import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2
import numpy as np
from numpy import pi

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

project = peano4.Project( ["examples", "particles"], "test_density_h_iterative_solver", "." )


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

# Define properties
v_attr = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
mass_attr    = dastgen2.attributes.Double("mass")
smoothL_attr = dastgen2.attributes.Double("smoothingLength")
density_attr = dastgen2.attributes.Double("density")
pressure_attr= dastgen2.attributes.Double("pressure")
u_attr       = dastgen2.attributes.Double("u")
soundSpeed_attr = dastgen2.attributes.Double("soundSpeed")
wcount_attr   = dastgen2.attributes.Double("wcount")
wcount_dh_attr= dastgen2.attributes.Double("wcount_dh")
Ncount_attr   = dastgen2.attributes.Integer("Ncount")

# Variable smoothing length scheme
f_attr     = dastgen2.attributes.Double("f")      # SWIFT's equivalent of Omega
Omega_attr = dastgen2.attributes.Double("Omega")
rho_dh_attr= dastgen2.attributes.Double("rho_dh")
hDot_attr  = dastgen2.attributes.Double("hDot")
hleft_attr = dastgen2.attributes.Double("hLeft")
hright_attr= dastgen2.attributes.Double("hRight")

# Artificial viscosity scheme
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
residual_attr = dastgen2.attributes.Double("residual")
iteration_count_attr = dastgen2.attributes.Integer("iterCount")

particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )

# Add particle properties
particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(mass_attr)
particle.data.add_attribute(density_attr)
particle.data.add_attribute(pressure_attr)
particle.data.add_attribute(u_attr)
particle.data.add_attribute(smoothL_attr)
particle.data.add_attribute(Ncount_attr)
particle.data.add_attribute(wcount_attr)
particle.data.add_attribute(wcount_dh_attr)
particle.data.add_attribute(soundSpeed_attr)

# For variable smoothing length
particle.data.add_attribute(f_attr)
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

particle.data.add_attribute(residual_attr)
particle.data.add_attribute(iteration_count_attr)
particle.data.add_attribute(has_no_neighbours_attr)
particle.data.add_attribute(is_boundary_part_attr)

particle.data.add_attribute(has_been_kicked_attr)
particle.data.add_attribute(has_converged_attr)
particle.data.add_attribute(smoothL_prev_attr)

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
#KERNEL_SUPPORT_RAD = "2"        # "gamma" parameter. =2 for Cubic Spline. Different in other kernels.
KERNEL_SUPPORT_RAD = "1.732051"  # 1D SWIFT

HYDRO_h_MAX = str( 1.0 / 3.0 / float(KERNEL_SUPPORT_RAD))  # for grid_size = 1/3
HYDRO_h_MIN = "1e-6"

# Artificial viscosity parameters
# Here they only affect the balsara switch calculation
ALPHA_AV= "0.8"     # =0.8 in SWIFT
BETA_AV = "3"       # =3 in SWIFT/SPHENIX

# Smoothing-length free parameter (resolution "eta_neighbours"). 
#SMOOTH_L_FAC = str( 0.75 )  # 3 nbs in 1D
SMOOTH_L_FAC = str( 1.1 )  # Josh's thesis. 4.4 nbs in 1D
#SMOOTH_L_FAC = str( 1.235 ) 

if(HYDRO_DIMENSIONS==1):
    N_MEAN_TARGET = str( 2.0 *           (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )    )
elif(HYDRO_DIMENSIONS==2):
    N_MEAN_TARGET = str(         np.pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )

# Kernel root value 
if(HYDRO_DIMENSIONS==1):
#    CUBIC_SPLINE_SIGMA = str(2./3.)
    CUBIC_SPLINE_SIGMA = str(8./3.)     # 1D SWIFT
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
  
  // Get SPH interaction radius
  const double cutoffR = KERNEL_SUPPORT_RAD * h_a;

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
      W_a = w_a / std::pow( h_a, HYDRO_DIMENSIONS );
      dW_a_dh = - ( HYDRO_DIMENSIONS*w_a + q_a*dw_a_dq ) / std::pow( h_a, HYDRO_DIMENSIONS +1 );

      /*************************************************************/

      // SWIFT M4 KERNEL TEST
      double W;
      kernel_eval(q_a, W);
      W_a = W / std::pow( h_a, HYDRO_DIMENSIONS );

      /*************************************************************/

      /* Increment density */
      localParticle->setDensity( localParticle->getDensity() + m_b * W_a ); 

      /* Increment fractional number density */
      localParticle->setWcount ( localParticle->getWcount () +       W_a );
      
      /* Debug: Increment number of detected neighbours */
      localParticle->setNcount ( localParticle->getNcount () + 1 ); 

      /* Increment drho/dh */
      localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_b*dW_a_dh ); 

      /* Increment Omega */
      /* Notice that this is the same as drho_dh in SWIFT */
      localParticle->setOmega    ( localParticle->getOmega()     + m_b*dW_a_dh );

      /* Increment dWcount/dh */
      localParticle->setWcount_dh( localParticle->getWcount_dh() +     dW_a_dh ); 

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

      assertion3( not std::isnan( localParticle->getOmega() ), localParticle->toString(), localParticle->getOmega(), dW_a_dh );
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
  ){
    /* Retrieve some properties */
    const double m_a = localParticle->getMass();
    const double h_a = localParticle->getSmoothingLength();

    /* Some smoothing length multiples */
    const double h_inv = 1.0 / h_a;
    const double h_inv_dim = std::pow( h_inv, HYDRO_DIMENSIONS );
    const double h_inv_dim_plus_one = h_inv_dim * h_inv;

    /* Default SPH kernel (cubic spline) */
    double W_a = CUBIC_SPLINE_SIGMA ; // kernel form at q=0

      /*************************************************************/

      // SWIFT M4 KERNEL TEST
      double W;
      kernel_eval(0, W);
      W_a = W;

      /*************************************************************/

    /* Final operation. Add the self-contributions (i.e. at q=0) */
    localParticle->setDensity  ( localParticle->getDensity()   + m_a * W_a * h_inv_dim );
    localParticle->setWcount   ( localParticle->getWcount()    +       W_a * h_inv_dim );
    localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_a * (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );
    localParticle->setWcount_dh( localParticle->getWcount_dh() +       (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );

    /* Not needed in SWIFT */
    localParticle->setNcount   ( localParticle->getNcount()    + 1 );
    localParticle->setOmega    ( localParticle->getOmega()     + m_a * (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );

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
 
      const double h_inv = 1.0 / h_a;
    
      /* Compute the norm of the curl */
      /* 2D sims */
      const double curl_v = std::sqrt( localParticle->getRot_v() * localParticle->getRot_v() );
    
      /* Compute the norm of div v */
      const double abs_div_v = tarch::la::abs( localParticle->getDiv_v() );
      
      /* Compute the sound speed */
      const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );
    
      /* Construct the final expression of the grad-h-term.
       * Swift use "f" (see also Springel and Hernquist). 
       * PHANTOM use Omega, which is proportional to ~ 1/f. (Price et al 2018) */
     
      const double dh_drho_a = - h_a / rho_a / HYDRO_DIMENSIONS;
      const double common_factor = h_a / HYDRO_DIMENSIONS / localParticle->getWcount();
      
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

        /* Verbose */
        logInfo( "touchVertexLastTime()", "Setting grad_h_term=0 for part:  " << localParticle->toString() );
        logInfo( "touchVertexLastTime()", "grad_W_term =  " << grad_W_term );
        logInfo( "touchVertexLastTime()", "common_factor =  " << common_factor );
        logInfo( "touchVertexLastTime()", "Wcount_dh =  " << localParticle->getWcount_dh() );

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
      
      /* SWIFT */ 
      localParticle->setF( grad_h_term );      
      
      /* Price: finish calculation of Omega */
      localParticle->setOmega( 1.0 - dh_drho_a * localParticle->getOmega() ); 
      /* Re-set if gone ill */
      if( tarch::la::smallerEquals( localParticle->getOmega(), 0.0) ){
        localParticle->setOmega(1.0);
      }

      localParticle->setPressure(P_a);
      localParticle->setSoundSpeed(soundspeed);
      localParticle->setBalsara(balsara);

    } /* End hydro_prepare_force */
    
    
    //
    // =============================
    // Newton's Apples
    // =============================
    //
    {
      const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
      const double hydro_eta_dim = std::pow(SMOOTH_L_FAC, HYDRO_DIMENSIONS);
      const double eps = H_TOLERANCE; 

      /* h value at the start of iteration procedure 
       * Here we use the last update on previous time-step */
      const double h_0 = localParticle->getSmoothingLengthPrevious();

      /* Set initial guess for NR */
//      const double h_init = h_0;
      const double h_init = h_0 + localParticle->getHDot() * timeStepSize;

      /* Get some useful values */
      double h_old = localParticle->getSmoothingLength();

      if( localParticle->getIterCount() == 0 ){ /* use initial guess h_init */
        h_old = h_init;
      }

      const double h_old_dim = std::pow(h_old, HYDRO_DIMENSIONS);
      const double h_old_dim_minus_one = std::pow(h_old, HYDRO_DIMENSIONS - 1);
      
      double h_new;

      /* Check that smoothing length from previous iteration is non-zero */
      assertion3( h_old > tarch::la::NUMERICAL_ZERO_DIFFERENCE, localParticle->toString(), h_init, h_0 );
    
      if( localParticle->getWcount() < 1e-5 ) { /* No neighbours case */

        /* Flag that there were no neighbours */
        localParticle->setHasNoNeighbours(true);

        /* Verbose */
        logInfo( "touchVertexLastTime()", "no neighbour case for particle " << localParticle->toString() );

        /* Double h and try again */
        h_new = 2.0 * h_old;

      } else {

        /* Compute one step of the Newton-Raphson scheme */
        
        /* SWIFT's calcualtion based on Wcount */
/*
        const double n_sum = localParticle->getWcount() * h_old_dim;
        const double n_target = hydro_eta_dim;
        const double f = n_sum - n_target;
        const double f_prime = localParticle->getWcount_dh() * h_old_dim +
                                  HYDRO_DIMENSIONS * localParticle->getWcount() * h_old_dim_minus_one;
*/
        
        /* Price's calculation based on Omega */

        const double one_over_dh_drho_a = - HYDRO_DIMENSIONS * localParticle->getDensity() / h_old;
        const double rho_sum = localParticle->getDensity();
        const double rho_target = m_a * std::pow( SMOOTH_L_FAC / h_old, HYDRO_DIMENSIONS);
        const double f = rho_sum - rho_target; // Price et al 2018 sign
        const double f_prime = - one_over_dh_drho_a * localParticle->getOmega(); // Opposite sign of Price et al 2018

        /* Verbose about rho_sum and rho_target values */

        if( tarch::la::equals(localParticle->getX(0),0.5) ){
          logInfo( "touchVertexLastTime()", " rho_sum= " << rho_sum );
          logInfo( "touchVertexLastTime()", " rho_target= " << rho_target );
          logInfo( "touchVertexLastTime()", " particle string: " << localParticle->toString() );
        }

        // Debug
        assertion6( f_prime !=0, localParticle->toString(), f_prime, h_new, h_old, HYDRO_h_MIN, HYDRO_h_MAX );       

        /* Skip if h is already h_max and we don't have enough neighbours */
        /* Same if we are below h_min */
        /* Same if we have hit the Peano cutoff radius 
         */

        tarch::la::Vector<Dimensions,double> GridSize = marker.h();
        const double GridSize_x = GridSize(0); 
        const double CutoffParticleH = KERNEL_SUPPORT_RAD * localParticle->getSmoothingLength();
 
        if ( ((localParticle->getSmoothingLength() >= HYDRO_h_MAX) && (f < 0.0)) ||
             ((localParticle->getSmoothingLength() <= HYDRO_h_MIN) && (f > 0.0)) ||
               tarch::la::greaterEquals(CutoffParticleH, GridSize_x) ){

          /* We have a particle whose smoothing length is already set (wants
           * to be larger but has already hit the maximum OR wants to be
           * smaller but has already reached the minimum). So, just tidy up
           * as if the smoothing length had converged correctly  */

          /* Verbose about this particle */
          logInfo( "touchVertexLastTime()", " This particle is unable to shrink/grow anymore:  " << localParticle->toString() );

          /* Re-set the value from previous time-step */
          h_new = h_0;
        }

        /* Normal case: 
         * We are able to use Newton-Raphson to get a better value of h */

        /* Avoid floating point exception from f_prime = 0 */
        h_new = h_old - f / ( f_prime + tarch::la::NUMERICAL_ZERO_DIFFERENCE );
          
        /* Safety check: truncate to the range [ h_old/2 , 2h_old ]. */
        h_new = std::min( h_new, 2.0 * h_old );
        h_new = std::max( h_new, 0.5 * h_old );

      }   /* end of if( localParticle->getWcount() < 1e-5 ) else block */

      /* By now we have got h_new for either of the yes/no neighbour cases */

      /* Check convergence */
      const double residual = std::abs(h_new - h_old) / h_0;
      localParticle->setResidual ( residual );

      if(residual > eps){ /* this particle has not converged */
  
        /* Ok, correct then */

        localParticle->setSmoothingLength( h_new );

        /* Increase the iteration counter */
        localParticle->setIterCount( localParticle->getIterCount() + 1 );
  
      } else {
        
        /* No need to update */

        /* However, if this is iter 0, we still update */
        if (localParticle->getIterCount() == 0) {
          localParticle->setSmoothingLength( h_new );
        }

        /* Flag that particle has already converged */
        localParticle->setHasConverged( true );

      }

    } /* End of Newton-Raphson */

  } /* End of localParticle->getParallelState()==globaldata::Particle::ParallelState::Local ... */

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
calc_densities.add_action_set( particle_tree_analysis )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
calc_densities.add_action_set( peano4.toolbox.particles.UpdateParallelState(particles) )
project.solversteps.add_step(calc_densities)

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
