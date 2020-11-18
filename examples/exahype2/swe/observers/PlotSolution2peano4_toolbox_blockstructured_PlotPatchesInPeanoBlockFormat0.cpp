#include "PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0.h"


tarch::logging::Log examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::_log( "examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0");





examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::~PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::beginTraversal(
      ) {

  static int rankLocalCounter = 0;
  static tarch::mpi::BooleanSemaphore booleanSemaphore("solution-SWE");
  static bool calledBefore = false;
  
  int counter;
  {
    tarch::mpi::Lock lock(booleanSemaphore);
    counter = rankLocalCounter;
    rankLocalCounter++;
  }
  std::ostringstream snapshotFileName;
  snapshotFileName << "solution-SWE" << "-tree-" << _treeNumber << "-" << counter;

  if ( _treeNumber==0 and not calledBefore ) {
    calledBefore = true;
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "solution-SWE",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::CreateNew
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("solution-SWE");
  }
  else if ( _treeNumber==0 ) {
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "solution-SWE",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewDataSet
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("solution-SWE");
  }
  else {
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("solution-SWE");
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "solution-SWE",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData
    );
  }

  _dataWriter = _writer->createCellDataWriter( "SWEQ", 11, 4, "" );

}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::endTraversal(
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


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {
 
  int vertexIndices[TwoPowerD];
  
  const double PatchScaling = 0.95;

  assertion( _writer!=nullptr );
  assertion( _dataWriter!=nullptr );
  
  const int patchIndex = _writer->plotPatch(
    marker.x() - marker.h() * PatchScaling * 0.5,
    marker.h() * PatchScaling
  );
 
  int cellIndex  = _dataWriter->getFirstCellWithinPatch(patchIndex);
  int currentDoF = 0;
  
  dfor(k,11) {
    double* data = fineGridCellSWEQ.value + currentDoF;
    _dataWriter->plotCell( cellIndex, data );
    cellIndex++;
    currentDoF += 4;
  }

}


void examples::exahype2::swe::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat0::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


