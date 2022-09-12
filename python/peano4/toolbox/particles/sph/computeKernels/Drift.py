# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Drift step kernels
# -----------------------------------------------------------------------------

# Drift step (+prediction)
# 
# In this step, we update the particle's position via
# 
# \begin{align}
# {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t}
# \end{align}
# 
# We also need to get predicted values of some quantities.

class LeapfrogDrift:
  """
  Kernels for the Drift step. We update positions and at the same time predict
  the value of some particle properties. Note that the position of the particle
  needs to be updated during the `touch_particle_last_time_compute_kernel` step
  in order to avoid updating it multiple times when in crosses a cell and gets
  assigned to a new vertex.
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

    /* @brief Update particle's position. We update it in this stage to avoid
     * updating multiple times when a particle moves into a different cell. 
     */
    
    const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
    
    if(
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and 
      localParticle->getMoveState() != globaldata::Particle::MoveState::Moved
      and
      not localParticle->getIsBoundaryParticle() )
      {
    
      /* Drift the particle (update position for a full time-step) */
      localParticle->setX( localParticle->getX() + timeStepSize * localParticle->getV_full() );
      
      /* Flag this particle as Moved */
      localParticle->setMoveState( globaldata::Particle::MoveState::Moved );
    }

    /* End of inserted block */

    """


