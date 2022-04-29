#include "DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::~DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    
  
  const double HalftimeStepSize = 0.5 * GLOBAL_TIME_STEP_SIZE;
  
  if(
    localParticle->getMoveState()!=globaldata::Particle::MoveState::Moved
    and
    localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){
  
    /* Drift the particle (prediction) */
    localParticle->setX_prev( localParticle->getX() );
    localParticle->setX( localParticle->getX() + HalftimeStepSize * localParticle->getV_full() );
    
    localParticle->setMoveState( globaldata::Particle::MoveState::Moved );
      
    }


  }
}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    
  
 
    

  }
}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
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


void examples::particles::observers::DriftPredictHalfStep2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
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


