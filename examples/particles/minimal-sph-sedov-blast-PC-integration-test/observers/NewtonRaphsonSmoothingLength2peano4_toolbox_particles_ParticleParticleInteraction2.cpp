#include "NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::~NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

    /* Prepare particles for Newton-Rhapson */
    
    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    and localParticle->getHasConverged()==false ) {  

//      localParticle->setHasConverged(false);
      
      // DEBUG
      assertion3( not localParticle->getDensity()==0, localParticle->toString(), localParticle->getDensity(), localParticle->getOmega() );  

    }  

  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

   if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
   and localParticle->getHasConverged()==false ) {  

    /* Do a Newton-Raphson iteration.
     * The HasConverged attribute is always initialize to false so we always do at least one update.
     * Notice that after each iteration we need to update the density, i.e. to re-run the full density loop.
     */
     
    /* Retrieve basic properties of the local particle */
    const double rho_a = localParticle->getDensity();
    const double m_a = localParticle->getMass();  
    const double h_old = localParticle->getSmoothingLength();
    
    const double hydro_eta_dim = std::pow(SMOOTH_L_FAC, Dimensions);
    const double h_old_dim = std::pow(h_old, Dimensions);
    const double h_old_dim_minus_one = std::pow(h_old, Dimensions - 1);
      
    const double dh_drho_a = - h_old/rho_a/Dimensions;
    const double Omega_a = localParticle->getOmega();
   
    /* Value at the start of iteration procedure (last update on previous time-step) */
    const double h_0 = localParticle->getSmoothingLengthPrevious();

    const double htolerance = H_TOLERANCE; 
       
    // Safety checks before doing NR
    assertion4( not rho_a==0, localParticle->toString(), localParticle->getDensity(), localParticle->getOmega(),rho_a );  
    
    /*******************************************************************************/    
    /* Swift method */
    /* This is based on number density, and does not read Omega.
     */
    
    const double n_sum = localParticle->getWcount() * h_old_dim;
    const double n_target = hydro_eta_dim;
    const double f = n_sum - n_target;
    const double f_prime = localParticle->getWcount_dh() * h_old_dim +
                              Dimensions * localParticle->getWcount() * h_old_dim_minus_one;

    /* Get new value of smoothing length via N-R iteration */
    double h_new = localParticle->getSmoothingLength() - f/f_prime;

    /*******************************************************************************/    

    /* Safety check: truncate to the range [ h_old/2 , 2h_old ]. */
    h_new = std::min(h_new, 2.0 * h_old);
    h_new = std::max(h_new, 0.5 * h_old);
    
    // Fall back to h_old if it has become too small or Nan
    if(localParticle->getSmoothingLength()<= 1e-5 || std::isnan(localParticle->getSmoothingLength()) ) {
      localParticle->setSmoothingLength(h_old);
    }
    
    // Update value of h
    localParticle->setSmoothingLength( h_new );

    /* Check if convergence has been achieved */
    const double residual = std::abs(h_new - h_old)/h_0 ;
    
    if(residual <= htolerance){
      localParticle->setHasConverged(true);  
    }
    
    localParticle->setResidual ( residual );
    localParticle->setIterCount( localParticle->getIterCount() + 1 );
    
    // DEBUG checks
    assertion6( localParticle->getSmoothingLength()>1E-5, localParticle->toString(), localParticle->getSmoothingLength(), f, f_prime, localParticle->getOmega(),residual );
      
  }


  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  if ( true ) {
    std::list< globaldata::Particle* >  localParticles;
    _numberOfActiveParticlesAdded.push_back(0);
    for (int i=0; i<TwoPowerD; i++) {
      for (auto& p: fineGridVerticesParticleSet(i) ) {
        _activeParticles.push_front( p );
        _numberOfActiveParticlesAdded.back()++;
      }
      for (auto& p: fineGridVerticesParticleSet(i) ) {
        bool append = marker.isContained( p->getX() );
        if (append) {
          localParticles.push_front( p );
        }
      }
    }

    std::list< globaldata::Particle* >&  activeParticles = _activeParticles;
    for (auto& localParticle: localParticles )
    for (auto& activeParticle: activeParticles ) {
      

    /* We do not excecute any specific action here */
    

    }
  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
    _activeParticles.pop_back();
  }
  _numberOfActiveParticlesAdded.pop_back();
}


