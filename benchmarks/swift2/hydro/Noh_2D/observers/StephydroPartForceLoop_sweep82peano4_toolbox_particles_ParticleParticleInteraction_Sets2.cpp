#include "StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::_log( "benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2");





benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::~StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::prepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::unprepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  
                                         ::swift2::kernels::prepareHydroForceCalculation( localParticles );
                                         
  
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  
                                         ::swift2::kernels::endHydroForceCalculation(
                                             localParticles,
                                             2
                                             );
                                         
  
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  std::unordered_set< globaldata::hydroPart* >  localParticles;
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVerticeshydroPartSet(i) ) {
      _activeParticles.insert( p );
    }
    for (auto* p: fineGridVerticeshydroPartSet(i) ) {
      bool append = marker.isContained( p->getX() );
      if (append) {
        localParticles.insert( p );
      }
    }
  }

  
  std::unordered_set< globaldata::hydroPart* >&  activeParticles = _activeParticles;
  
                                         ::swift2::kernels::computeHydroForce( 
                                           localParticles, 
                                           activeParticles,
                                           2,
                                           3.0
                                         );
                                         
  
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVerticeshydroPartSet(i) ) {
      _activeParticles.erase( p );
    }
  }
}


