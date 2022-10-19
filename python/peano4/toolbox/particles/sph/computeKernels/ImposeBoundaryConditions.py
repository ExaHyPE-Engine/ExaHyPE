# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Impose BC on particles
# -----------------------------------------------------------------------------


class ImposeBoundaryConditions:
  """
  Kernels to impose BC on particles.
  After the final velocity has been calculated (and the particle already been
  drifted), we need to impose the boundary conditions. In this case we use
  reflecting ones.
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* @brief: prepares particle to be potentially moved.
     */

    if( localParticle->getParallelState() == globaldata::Particle::ParallelState::Local ){

      localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );

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

    if(
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      localParticle->getMoveState() != globaldata::Particle::MoveState::Moved
      and
      not localParticle->getIsBoundaryParticle() )
      {
    
      for( int d=0; d<Dimensions; d++ ){
        if( localParticle->getX()(d)<0.0 ){

          /* Reflecting BC */   
          localParticle->setX     ( d, std::abs(localParticle->getX     ()(d)) );
          localParticle->setV     ( d, std::abs(localParticle->getV_full()(d)) );
          localParticle->setV_full( d, std::abs(localParticle->getV_full()(d)) );

        }
        if( localParticle->getX()(d)>1.0 ){

          /* Reflecting BC */   
          localParticle->setX     ( d,    2.0 - localParticle->getX     ()(d)  );
          localParticle->setV     ( d,-std::abs(localParticle->getV     ()(d)) );
          localParticle->setV_full( d,-std::abs(localParticle->getV_full()(d)) );

        }
      
        localParticle->setMoveState( globaldata::Particle::MoveState::Moved );
        
      } /* End for dimensions */
        
    }

    /* End of inserted block */

    """

