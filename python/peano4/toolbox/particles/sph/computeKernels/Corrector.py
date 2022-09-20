# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Correcto steps kernels -- PHANTOM code implementation
# NOT USED BY SWIFT, but can be useful for testing (and perhaps in some specific
# cases)
# This step can be regarded as a 'mini kick' step.
# -----------------------------------------------------------------------------


class LeapfrogCorrector:
  """
  Kernels for the Corrector step adopted by the PHANTOM code. NOT used in SWIFT.
  Can also be regarded as a 'mini kick' step.
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
  
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

    const double timeStepSize_half = 0.5 * GLOBAL_TIME_STEP_SIZE;
    
    if(
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local 
      and
      not localParticle->getHasBeenKicked()
      and
      not localParticle->getIsBoundaryParticle() )
      {
    
      /* Kick the particle */
      localParticle->setV_full( localParticle->getV() + 
                           timeStepSize_half * (localParticle->getA() - localParticle->getA_prev_step()) );

      localParticle->setHasBeenKicked(true);

      /* Store old acceleration for a potential additional corrector step */
      localParticle->setA_prev_step( localParticle->getA() );  
      
    }

    /* End of inserted block */

    """

class LeapfrogCorrectorExtra:
  """
  Kernels for the Corrector step adopted by the PHANTOM code. NOT used in SWIFT.
  Can also be regarded as a 'mini kick' step for thermodynamics quantities.
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
  
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

    const double timeStepSize_half = 0.5 * GLOBAL_TIME_STEP_SIZE;
    
    if( 
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      not localParticle->getHasBeenKicked()
      and
      not localParticle->getIsBoundaryParticle() )
      {
    
      /* Kick the internal energy of the particle */
      
      localParticle->setU_full( localParticle->getU_full() + 
                           timeStepSize_half * (localParticle->getUDot() - localParticle->getUDot_prev_step()) );

      localParticle->setHasBeenKicked(true);

      /* Store old value for a potential additional corrector step */
      localParticle->setUDot_prev_step( localParticle->getUDot() );  
      
    }

    /* End of inserted block */

    """

