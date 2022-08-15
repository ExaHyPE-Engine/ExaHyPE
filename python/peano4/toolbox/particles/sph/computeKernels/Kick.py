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
  differt in the `touch_particle_last_time_compute_kernel` kernel, as they
  update the particles slightly differently. 

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
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local 
      and 
      not localParticle->getIsBoundaryParticle() )
      {
    
      /* Prepare the particle for kick */
      localParticle->setHasBeenKicked(false);
      
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
        localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
        and 
        not localParticle->getHasBeenKicked()
        and
        not localParticle->getIsBoundaryParticle() )
        {
      
        /* Kick the particle */
        localParticle->setV_full( localParticle->getV_full() + dt_kick_hydro * localParticle->getA() );
        
        /* If predictor is going to be used, store old acceleration */
        localParticle->setA_prev_step( localParticle->getA() );

        assertion1(  std::isfinite(localParticle->getV(0)), localParticle->toString() );
        assertion1(  std::isfinite(localParticle->getV(1)), localParticle->toString() );
        
        localParticle->setHasBeenKicked(true);
        
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
        localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
        and 
        not localParticle->getHasBeenKicked()
        and
        not localParticle->getIsBoundaryParticle() )
        {
      
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
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local 
      and 
      not localParticle->getIsBoundaryParticle() )
      {
    
      /* Prepare the particle for kick */
      localParticle->setHasBeenKicked(false);
      
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
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and 
      not localParticle->getHasBeenKicked()
      and
      not localParticle->getIsBoundaryParticle() )
      {
        
      /* Integrate the internal energy forward in time */
      const double delta_u = localParticle->getUDot() * dt_therm;
    
      /* Do not decrease the energy by more than a factor of 2 */
      localParticle->setU_full( std::max( localParticle->getU_full() + delta_u,
                                          localParticle->getU_full() * 0.5      ) );

      /* Check against absolute minimum.
       * Default value is zero. */
      if (localParticle->getU_full() < 0.0) 
      {
        localParticle->setU_full(0.0);
        localParticle->setUDot(0.0);
      }
      
    }

    /* End of inserted block */

    """


