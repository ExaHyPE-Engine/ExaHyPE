# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# End time step
# Actions executed once at the end of the time-step
# -----------------------------------------------------------------------------


class EndTimeStep:
  """
  Kernels for End the Time step. Actions executed once at the end of the time-step
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* Mark that particle needs to be updated */
    if( 
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      not localParticle->getIsBoundaryParticle() )
      {

      localParticle->setHasBeenKicked( false );

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
     
    const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
    
    if(
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      not localParticle->getHasBeenKicked()
      and
      not localParticle->getIsBoundaryParticle() )
      {
        
      /* Calculate max time-step size value allowed by the CFL condition */
      const double soundspeed = localParticle->getSoundSpeed();
      const double timeStepSizeCFL = CFL_CONSTANT * localParticle->getSmoothingLength() * KERNEL_SUPPORT_RAD / soundspeed;
      localParticle->setTimeStepSize( timeStepSizeCFL );
      
      assertion1( tarch::la::greaterEquals( localParticle->getTimeStepSize(), timeStepSize ), 
                   localParticle->toString() );

      /* Mark that particle is done */
      localParticle->setHasBeenKicked(true);
      
    }

    /* End of inserted block */

    """

