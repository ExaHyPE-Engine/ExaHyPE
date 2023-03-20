#include "StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::_log( "benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1");





benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::~StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  auto& particles  = fineGridVertexhydroPartSet;

  for (auto* particle: particles) {
    particle->setCellHasUpdatedParticle(false);
  }
}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1::touchCellLastTime(
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


