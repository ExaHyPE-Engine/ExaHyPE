#include "Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::_log( "benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2");



tarch::mpi::BooleanSemaphore  benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::_semaphore("benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2");



benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::~Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "grid-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  
  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "grid",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    repositories::getMinTimeStamp()
  );    
  
      
  _dataWriter = _writer->createCellDataWriter( "cell-marker", 1, 5, "is-refined,has-been-refined,local,enclave" );
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::endTraversal(
      ) {

  assertion1(_dataWriter!=nullptr,_treeNumber);

  _dataWriter->close();
  _writer->writeToFile();
  
  delete _dataWriter;
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {
 
  if ( true ) {
  int vertexIndices[TwoPowerD];

  int indices = _writer->plotPatch(
    marker.x() - marker.h() * 0.5,
    marker.h()
  );

  assertion( _dataWriter!=nullptr );
  
  double markerData[] = {
    marker.hasBeenRefined() ? 1.0 : 0.0,
    marker.willBeRefined() ? 1.0 : 0.0,
    marker.isLocal() ? 1.0 : 0.0,
    marker.hasBeenEnclaveCell() ? 1.0 : 0.0,
    marker.willBeEnclaveCell() ? 1.0 : 0.0
  };
 _dataWriter->plotCell(indices,markerData);
 }
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


