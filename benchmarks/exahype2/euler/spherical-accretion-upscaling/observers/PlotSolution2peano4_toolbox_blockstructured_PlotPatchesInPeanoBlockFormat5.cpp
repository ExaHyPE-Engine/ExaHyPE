#include "PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5");



tarch::mpi::BooleanSemaphore  benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::_semaphore("benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5");



benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::~PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "./solution-SelfSimilarInfallFV-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "./solution-SelfSimilarInfallFV",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())
  );    
      
  #if Dimensions==2
  double* mapping = nullptr;
  #else
  double* mapping = nullptr;
  #endif

  _dataWriter = _writer->createCellDataWriter( "SelfSimilarInfallFVQ", 4, 5, "rho,j_x,j_y,j_z,E,aux", "", mapping );
  _dataWriter->setPrecision( PlotterPrecision );

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::endTraversal(
      ) {

  assertion1( _dataWriter!=nullptr, _treeNumber );
  assertion1( _writer!=nullptr,     _treeNumber );

  _dataWriter->close();
  _writer->writeToFile();
  
  delete _dataWriter;
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {
 
  if ( repositories::plotFilter.plotPatch(marker) and not marker.hasBeenRefined() and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridInitialisation ) {
    int vertexIndices[TwoPowerD];
  
    //const double PatchScaling = 0.95;
    const double PatchScaling = 1.0;

    assertion( _writer!=nullptr );
    assertion( _dataWriter!=nullptr );
  
    const int patchIndex = _writer->plotPatch(
      marker.x() - marker.h() * PatchScaling * 0.5,
      marker.h() * PatchScaling
    );
 
    int cellIndex  = _dataWriter->getFirstCellWithinPatch(patchIndex);
    int currentDoF = 0;
  
    dfor(k,4) {
      double* data = fineGridCellSelfSimilarInfallFVQ.value + currentDoF;
      _dataWriter->plotCell( cellIndex, data );
      cellIndex++;
      currentDoF += 5;
    }
  }

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat5::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


