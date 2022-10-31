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

    if( particle->getParallelState() == globaldata::Particle::ParallelState::Local ){
    
      /* Store value of h at the beggining of current time-step */
      particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );
          
      /* Reset for N-R of this time-step */
      particle->setHasConverged(false);
      particle->setIterCount(0);

      /* Reset the bisection bounds */
      particle->setHLeft (HYDRO_h_MIN);        
      particle->setHRight(HYDRO_h_MAX);        
      
      /* Flag if particle is at boundary of the simulation box */
      particle->setIsBoundaryParticle(false);
      const double max_dist_boundary = 3 * (1.0 /HYDRO_PART_NUMBER); 

      for (int d=0; d<Dimensions; d++) {
        if ( particle->getX()(d) < max_dist_boundary || 
             particle->getX()(d) > (1.0 - max_dist_boundary) ) 
        {

          particle->setIsBoundaryParticle(true);

        }
      }

//      // Fix right boundary of the Sod shock, which as fewer particles
//      const double max_dist_boundary_right = 8 * max_dist_boundary;
//      for (int d=0; d<Dimensions; d++) {
//        if ( particle->getX()(d) > 0.5 &&
//             particle->getX()(d) > (1.0 - max_dist_boundary_right) ) 
//        {
//          particle->setIsBoundaryParticle(true);
//          particle->setDensity(0.125);
//          particle->setPressure(0.1);
//        }
//      }


      /* Also prepare particle for Drift step */
      particle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );    

    }

    /* End of inserted block */

    """

