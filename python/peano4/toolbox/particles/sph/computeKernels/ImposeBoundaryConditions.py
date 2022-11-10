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

    if( particle->getParallelState() == globaldata::Particle::ParallelState::Local ){

      particle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );

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
      particle->getParallelState() == globaldata::Particle::ParallelState::Local
      and
      particle->getMoveState() != globaldata::Particle::MoveState::Moved
      and
      not particle->getIsBoundaryParticle() )
      {
    
      for( int d=0; d<Dimensions; d++ ){
        if( particle->getX()(d)<0.0 ){

          /* Reflecting BC */   
          particle->setX     ( d, std::abs(particle->getX     ()(d)) );
          particle->setV     ( d, std::abs(particle->getV_full()(d)) );
          particle->setV_full( d, std::abs(particle->getV_full()(d)) );

        }
        if( particle->getX()(d)>1.0 ){

          /* Reflecting BC */   
          particle->setX     ( d,    2.0 - particle->getX     ()(d)  );
          particle->setV     ( d,-std::abs(particle->getV     ()(d)) );
          particle->setV_full( d,-std::abs(particle->getV_full()(d)) );

        }
      
        particle->setMoveState( globaldata::Particle::MoveState::Moved );
        
      } /* End for dimensions */
        
    }

    /* End of inserted block */

    """

