# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Init time step
# -----------------------------------------------------------------------------


class InitializeTimeStep:
  """
  Kernels to initialise various particle properties for a time-step.
  Only acts on `touch_particle_last_time_compute_kernel`.
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

    if( localParticle->getParallelState() == globaldata::Particle::ParallelState::Local ){
    
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
      const double max_dist_boundary = 3 * (1.0 /HYDRO_PART_NUMBER); 

      for (int d=0; d<Dimensions; d++) {
        if ( localParticle->getX()(d) < max_dist_boundary || 
             localParticle->getX()(d) > (1.0 - max_dist_boundary) ) 
        {

          localParticle->setIsBoundaryParticle(true);

        }
      }

      // Fix right boundary of the Sod shock, which as fewer particles
      const double max_dist_boundary_right = 8 * max_dist_boundary;
      for (int d=0; d<Dimensions; d++) {
        if ( localParticle->getX()(d) > 0.5 &&
             localParticle->getX()(d) > (1.0 - max_dist_boundary_right) ) 
        {
          localParticle->setIsBoundaryParticle(true);
        }
      }


      /* Also prepare particle for Drift step */
      localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );    

    }

    /* End of inserted block */

    """

