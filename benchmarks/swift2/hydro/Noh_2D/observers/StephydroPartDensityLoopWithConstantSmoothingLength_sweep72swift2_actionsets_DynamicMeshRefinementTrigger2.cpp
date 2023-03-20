#include "StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::_log( "benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2");





benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::~StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::getGridControlEvents() const {

  return ::swift2::committedGridControlEvents;
}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


