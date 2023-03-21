#include "CreateGrid2peano4_toolbox_CreateRegularGrid0.h"


tarch::logging::Log benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::_log( "benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0");





benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::CreateGrid2peano4_toolbox_CreateRegularGrid0(int treeNumber) {
}


benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::~CreateGrid2peano4_toolbox_CreateRegularGrid0() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::getGridControlEvents() const {

  std::vector< peano4::grid::GridControlEvent > result;
  peano4::grid::GridControlEvent newEntry;
  newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
  newEntry.setOffset(tarch::la::Vector<Dimensions,double>( -std::numeric_limits<double>::max()/2.0/3.0 ));
  newEntry.setWidth(tarch::la::Vector<Dimensions,double>(  std::numeric_limits<double>::max()/2.0 ));
  newEntry.setH(tarch::la::Vector<Dimensions,double>( 0.1 ));
  result.push_back(newEntry);
  return result;
}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::CreateGrid2peano4_toolbox_CreateRegularGrid0::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


