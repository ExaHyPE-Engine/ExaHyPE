#include "Plot2peano4_toolbox_particles_UpdateParallelState0.h"


tarch::logging::Log benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::_log( "benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0");





benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::Plot2peano4_toolbox_particles_UpdateParallelState0(int treeNumber) {

  _treeNumber  = treeNumber;
}


benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::~Plot2peano4_toolbox_particles_UpdateParallelState0() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::prepareTraversal() {

  vertexdata::hydroPartSet::clearParticleStateStatistics();}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::beginTraversal(
      ) {

  _numberOfRemainingLocalParticles = 0;
  _numberOfIncomingLocalParticles  = 0;
  _numberOfLeavingLocalParticles   = 0;
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::endTraversal(
      ) {

  vertexdata::hydroPartSet::updateNumberOfLocalParticleStatistics(
    _numberOfRemainingLocalParticles,
    _numberOfIncomingLocalParticles,
    _numberOfLeavingLocalParticles
  );
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

// run over all adjacent vertices
for (auto p: fineGridVertexhydroPartSet ) {
  logDebug( "enterCell(...)", "Particle " << p->toString() << " is marked as local" );
  p->setNewParallelState( globaldata::hydroPart::NewParallelState::Virtual );
}
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

// run over all adjacent vertices
auto p = fineGridVertexhydroPartSet.begin();
while ( p!=fineGridVertexhydroPartSet.end() ) {
  if ( 
    (*p)->getParallelState()==globaldata::hydroPart::ParallelState::Local 
    and
    (*p)->getNewParallelState()==globaldata::hydroPart::NewParallelState::Local 
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " is local and remains local" );
    _numberOfRemainingLocalParticles++;
    (*p)->setParallelState( globaldata::hydroPart::ParallelState::Local );
    p++;
  }
  else if ( 
    (*p)->getParallelState()==globaldata::hydroPart::ParallelState::Virtual 
    and
    (*p)->getNewParallelState()==globaldata::hydroPart::NewParallelState::Local 
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was remote but now is local" );
    _numberOfIncomingLocalParticles++;
    (*p)->setParallelState( globaldata::hydroPart::ParallelState::Local );
    p++;
  }
  else if ( 
    (*p)->getParallelState()==globaldata::hydroPart::ParallelState::Local
    and
    (*p)->getNewParallelState()==globaldata::hydroPart::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was local but now is remote" );
    _numberOfLeavingLocalParticles++;
    delete *p;
    p = fineGridVertexhydroPartSet.erase(p);
  }
  else if ( 
    (*p)->getParallelState()==globaldata::hydroPart::ParallelState::Virtual
    and
    (*p)->getNewParallelState()==globaldata::hydroPart::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was virtual but remains virtual" );
    delete *p;
    p = fineGridVertexhydroPartSet.erase(p);
  }
}
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

// run over all adjacent vertices
for (int i=0; i<TwoPowerD; i++) {
  for (auto* p: fineGridVerticeshydroPartSet(i) ) {
    if ( 
      marker.isContained(p->getX(), toolbox::particles::relativeSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE )
    ) {
      p->setNewParallelState( globaldata::hydroPart::NewParallelState::Local );
    }
  }
}
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParallelState0::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


