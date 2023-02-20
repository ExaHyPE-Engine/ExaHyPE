# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Kick leapfrog kernels
# -----------------------------------------------------------------------------

# In the KDK (leapfrog) scheme, we update positions and velocities according to
# 
# \begin{align}
# {\bf v}^{n+\frac{1}{2}} &= {\bf v}^{n} + {\bf a}^{n}\frac{\Delta t}{2}\\
# {\bf x}^{n+1} &= {\bf x}^{n} + {\bf v}^{n+\frac{1}{2}}{\Delta t}\\
# {\bf v}^{n+1} &= {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta t}{2}\\
# \end{align}
#  
# Notice that the last kick in this sequence is implicit if ${\bf a}$ depends on
# ${\bf v}$, which requires extra treatments (see Price et al.).
# 

class LeapfrogKick:
  """
  Compute kernels for the Kick 1 and Kick 2 steps in the leapfrog scheme. These
  slightly differ in `touch_particle_last_time_compute_kernel`: Kick 2 not
  only updates the velocity but also re-set some values that have been
  previously predicted.

  @param kickNumber: whether we are doing first (kick 1) or second (kick 2) in
  the leapfrog scheme.
  """

  def __init__(self, kickNumber):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* @brief: Prepare particle to be kicked.
     */

    if( 
//      particle->getParallelState() == globaldata::Particle::ParallelState::Local 
//      and 
      not particle->getIsBoundaryParticle() )
      {
    
      /* Prepare the particle for kick */
      particle->setHasBeenKicked(false);
      
    }
  
    /* End of inserted block */

    """
    #
    # @todo Tobias: It is not clear if this is clever. If we have only particle-particle 
    # interaction kernels, then we can't have an inner while loop per particle. So teh 
    # corsett might not be beneficial performance-wisely.
    #
    self.particle_particle_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
    
    /* End of inserted block */

    """
    if kickNumber==1:
      self.touch_particle_last_time_compute_kernel = """

      /* Code inserted via the python snippet library.
       * Do not modify unless you know what you are doing.
       */

      const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;
      
      if(
//        particle->getParallelState() == globaldata::Particle::ParallelState::Local
//        and 
        not particle->getHasBeenKicked()
        and
        not particle->getIsBoundaryParticle() )
        {
      
        /* Kick the particle */
        particle->setV_full( particle->getV_full() + dt_kick_hydro * particle->getA() );
        
        /* If predictor is going to be used, store old acceleration */
        particle->setA_prev_step( particle->getA() );

        assertion1(  std::isfinite(particle->getV(0)), particle->toString() );
        assertion1(  std::isfinite(particle->getV(1)), particle->toString() );
        
        particle->setHasBeenKicked(true);
        
      }

      /* End of inserted block */

      """
    elif kickNumber==2:
      self.touch_particle_last_time_compute_kernel = """

      /* Code inserted via the python snippet library.
       * Do not modify unless you know what you are doing.
       */

      const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;

      if( 
//        particle->getParallelState()==globaldata::Particle::ParallelState::Local
//        and 
        not particle->getHasBeenKicked()
        and
        not particle->getIsBoundaryParticle() )
        {
      
        /* Kick the particle */
        particle->setV_full( particle->getV_full() + dt_kick_hydro * particle->getA() );
        
        assertion1(  std::isfinite(particle->getV(0)), particle->toString() );
        assertion1(  std::isfinite(particle->getV(1)), particle->toString() );
        
        particle->setHasBeenKicked(true);
     
        /*
         * Reset predicted values for next Drift step 
         * Done by "hydro_reset_predicted_values" in SWIFT
         */
        
        /* Re-set the predicted velocities */
        particle->setV( particle->getV_full() );
        
        /* Re-set the entropy */
        particle->setU( particle->getU_full() );

        /* Re-compute the pressure */
        const double pressure = 
          gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );

        /* Compute the new sound speed */
        const double soundspeed = gas_soundspeed_from_internal_energy( particle->getU() );
     
        /* Update variables */
        particle->setPressure(pressure);
        particle->setSoundSpeed(soundspeed);
        particle->setV_sig_AV( std::max(particle->getV_sig_AV(), 2.0 * soundspeed) );

      }

      /* End of inserted block */

      """
    else:
      print('Kick number not supported!')


class LeapfrogKickExtra:
  """
  Compute kernels for the Kick extra step, i.e. update some thermodynamic
  quantities.
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    if( 
//      particle->getParallelState() == globaldata::Particle::ParallelState::Local 
//      and 
      not particle->getIsBoundaryParticle() )
      {
    
      /* Prepare the particle for kick */
      particle->setHasBeenKicked(false);
      
    }

    /* End of inserted block */

    """
    self.particle_particle_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
    
    /* End of inserted block */

    """
    self.touch_particle_last_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /**
     * @brief Kick the additional variables
     *
     * Additional hydrodynamic quantites are kicked forward in time here. These
     * include thermal quantities (thermal energy or total energy or entropy, ...).
     *
     */

    const double dt_therm = 0.5 * GLOBAL_TIME_STEP_SIZE;
    
    if(
//      particle->getParallelState() == globaldata::Particle::ParallelState::Local
//      and 
      not particle->getHasBeenKicked()
      and
      not particle->getIsBoundaryParticle() )
      {
        
      /* Integrate the internal energy forward in time */
      const double delta_u = particle->getUDot() * dt_therm;
    
      /* Do not decrease the energy by more than a factor of 2 */
      particle->setU_full( std::max( particle->getU_full() + delta_u,
                                          particle->getU_full() * 0.5      ) );

      /* Check against absolute minimum.
       * Default value is zero. */
      if (particle->getU_full() < 0.0) 
      {
        particle->setU_full(0.0);
        particle->setUDot(0.0);
      }
      
    }

    /* End of inserted block */

    """


