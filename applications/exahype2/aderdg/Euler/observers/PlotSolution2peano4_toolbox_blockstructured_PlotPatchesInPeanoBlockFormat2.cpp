#include "PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2.h"


tarch::logging::Log examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::_log( "examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2");



tarch::mpi::BooleanSemaphore  examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::_semaphore("examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2");



examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2(int treeNumber) {

  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
}


examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::~PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "./solution-euler2D-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "./solution-euler2D",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    repositories::getMinTimeStamp()
  );    
      
  #if Dimensions==2
  double mapping[32] = { 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875
, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875
, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875
, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875
, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125
, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125
, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125
, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125
, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875
, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875
, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875
, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875
, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625
, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625
, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625
, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625
};
  #else
  double mapping[192] = { 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875
, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875
, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875
, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875
, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875
, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875
, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875
, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875
, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875
, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875
, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875
, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875
, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875
, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875
, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875
, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875
, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125
, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125
, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125
, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125
, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125
, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125
, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125
, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125
, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125
, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125
, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125
, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125
, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125
, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125
, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125
, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125
, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875
, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875
, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875
, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875
, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875
, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875
, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875
, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875
, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875
, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875
, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875
, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875
, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875
, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875
, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875
, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875
, 0.069431844202973713731097404888714663684368133544921875, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625
, 0.33000947820757187134432797392946667969226837158203125, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625
, 0.66999052179242812865567202607053332030773162841796875, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625
, 0.9305681557970262307577513638534583151340484619140625, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625
, 0.069431844202973713731097404888714663684368133544921875, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625
, 0.33000947820757187134432797392946667969226837158203125, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625
, 0.66999052179242812865567202607053332030773162841796875, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625
, 0.9305681557970262307577513638534583151340484619140625, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625
, 0.069431844202973713731097404888714663684368133544921875, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625
, 0.33000947820757187134432797392946667969226837158203125, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625
, 0.66999052179242812865567202607053332030773162841796875, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625
, 0.9305681557970262307577513638534583151340484619140625, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625
, 0.069431844202973713731097404888714663684368133544921875, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625
, 0.33000947820757187134432797392946667969226837158203125, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625
, 0.66999052179242812865567202607053332030773162841796875, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625
, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625, 0.9305681557970262307577513638534583151340484619140625
};
  #endif

  _dataWriter = _writer->createVertexDataWriter( "euler2DQ", 4, 4, "", "", mapping );
  _dataWriter->setPrecision( PlotterPrecision );

}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::endTraversal(
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


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {
 
  if ( repositories::plotFilter.plotPatch(marker) and not marker.hasBeenRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridInitialisation ) {
    int vertexIndices[TwoPowerD];
  
    //const double PatchScaling = 0.95;
    const double PatchScaling = 1.0;

    assertion( _writer!=nullptr );
    assertion( _dataWriter!=nullptr );
  
    const int patchIndex = _writer->plotPatch(
      marker.x() - marker.h() * PatchScaling * 0.5,
      marker.h() * PatchScaling
    );
 
    int vertexIndex  = _dataWriter->getFirstVertexWithinPatch(patchIndex);
    int currentDoF = 0;
  
    dfor(k,4) {
      double* data = fineGridCelleuler2DQ.value + currentDoF;
      _dataWriter->plotVertex( vertexIndex, data );
      vertexIndex++;
      currentDoF += 4;
    }
  }

}


void examples::exahype2::euler2D::observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


}


