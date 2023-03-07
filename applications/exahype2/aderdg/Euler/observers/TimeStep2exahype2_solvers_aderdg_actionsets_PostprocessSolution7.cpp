#include "TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7.h"


tarch::logging::Log examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::_log( "examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7");





examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7(int treeNumber) {
// @todo Should be overwritten
}


examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::~TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::beginTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::endTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::createPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::destroyPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::createHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::destroyHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::touchFaceFirstTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::touchFaceLastTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::createCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::destroyCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::touchCellFirstTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_PostprocessSolution7::touchCellLastTime(
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


