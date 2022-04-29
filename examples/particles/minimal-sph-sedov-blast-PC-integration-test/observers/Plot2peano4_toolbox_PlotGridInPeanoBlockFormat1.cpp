#include "Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1.h"


tarch::logging::Log examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::_log( "examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1");



tarch::mpi::BooleanSemaphore  examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::_semaphore("examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1");



examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::~Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "grid-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>0 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  
  static int timeStep = -1;
  timeStep++;
  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "grid",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    timeStep
  );
  
      
  _dataWriter = _writer->createCellDataWriter( "cell-marker", 1, 5, "is-refined,has-been-refined,local,enclave" );
}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::endTraversal(
      ) {

  assertion1(_dataWriter!=nullptr,_treeNumber);

  _dataWriter->close();
  _writer->writeToFile();
  
  delete _dataWriter;
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;
}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
 
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


void examples::particles::observers::Plot2peano4_toolbox_PlotGridInPeanoBlockFormat1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


