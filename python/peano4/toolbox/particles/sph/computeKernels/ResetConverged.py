# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Step to Reset HasConverged for particles
# -----------------------------------------------------------------------------


class ResetHasConverged:
  """
  Kernels to reset the HasConverged attribute of a particle. Only acts on
  'touch_particle_last_time_compute_kernel'.
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

    /* Reset HasConverged property back to false so we can perform new N-R iterations */
    
    if(
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      not localParticle->getIsBoundaryParticle() )
      {
      
      localParticle->setHasConverged(false);
      
    }

    /* End of inserted block */

    """

