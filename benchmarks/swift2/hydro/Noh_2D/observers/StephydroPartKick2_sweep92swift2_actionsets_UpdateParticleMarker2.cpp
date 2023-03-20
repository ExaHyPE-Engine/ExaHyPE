#include "StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::_log( "benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2");





benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::~StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  auto& particles  = fineGridVertexhydroPartSet;

  for (auto* particle: particles) {
    particle->setCellHasUpdatedParticle(false);
  }
}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  for (int i=0; i<TwoPowerD; i++) {
    for (auto* particle: fineGridVerticeshydroPartSet(i) ) {
      if ( marker.isContained( particle->getX() ) ) {
        particle->setCellHasUpdatedParticle(true);
      }
    }
  }
}


