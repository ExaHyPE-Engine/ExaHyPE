#include "PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3");



tarch::mpi::BooleanSemaphore  benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::_semaphore("benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3");



benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::~PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "./solution-SelfSimilarInfallFD4-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "./solution-SelfSimilarInfallFD4",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())
  );    
      
  #if Dimensions==2
  double* mapping = nullptr;
  #else
  double* mapping = nullptr;
  #endif

  _dataWriter = _writer->createCellDataWriter( "SelfSimilarInfallFD4Q", 5, 5, "rho,j_x,j_y,j_z,E,aux", "", mapping );
  _dataWriter->setPrecision( PlotterPrecision );

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::endTraversal(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {
 
  if ( repositories::plotFilter.plotPatch(marker) and not marker.hasBeenRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridInitialisation ) {
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
  
    dfor(k,5) {
      double* data = fineGridCellSelfSimilarInfallFD4Q.value + currentDoF;
      _dataWriter->plotCell( cellIndex, data );
      cellIndex++;
      currentDoF += 5;
    }
  }

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


