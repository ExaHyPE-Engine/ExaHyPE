#include "HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::~HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    
  
    if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      /* Prepare the particle for kick */
      localParticle->setHasBeenKicked(false);
    }
    

  }
}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    
  
  const double dt_therm = 0.5 * GLOBAL_TIME_STEP_SIZE;
  
  if (not localParticle->getHasBeenKicked()
  and
  localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
      
    /* Integrate the internal energy forward in time */
    const double delta_u = localParticle->getUDot() * dt_therm;
  
    /* Do not decrease the energy by more than a factor of 2 */
    localParticle->setU_full( std::max( localParticle->getU_full() + delta_u,
                                        localParticle->getU_full() * 0.5      ) );

    /* Check against absolute minimum.
     * Default value is zero. */
    if (localParticle->getU_full() < 0.0) {
      localParticle->setU_full(0.0);
      localParticle->setUDot(0.0);
    }
    
  }
    

  }
}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
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


void examples::particles::observers::HydroKickExtra2HalfTimeStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
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


