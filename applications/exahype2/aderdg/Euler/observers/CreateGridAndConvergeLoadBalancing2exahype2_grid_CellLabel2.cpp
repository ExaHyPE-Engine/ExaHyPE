#include "CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2.h"


tarch::logging::Log examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::_log( "examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2");





examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2(int treeNumber) {
}


examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::~CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::beginTraversal(
      ) {


}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::endTraversal(
      ) {


}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::createPersistentFace(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::destroyPersistentFace(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::createHangingFace(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::destroyHangingFace(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::touchFaceFirstTime(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::touchFaceLastTime(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::createCell(
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


  fineGridCelleuler2DCellLabel.setSemaphoreNumber(  ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
  if ( tarch::la::equals(repositories::getMinTimeStamp(),0.0) ) {
    fineGridCelleuler2DCellLabel.setTimeStamp(  0.0 );
    fineGridCelleuler2DCellLabel.setTimeStepSize( 0.0 );
  }
  else {
    fineGridCelleuler2DCellLabel.setTimeStamp(  coarseGridCelleuler2DCellLabel.getTimeStamp() );
    fineGridCelleuler2DCellLabel.setTimeStepSize( coarseGridCelleuler2DCellLabel.getTimeStepSize() );
  }
  fineGridCelleuler2DCellLabel.setHasUpdated( false );

}


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::destroyCell(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::touchCellFirstTime(
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


void examples::exahype2::euler2D::observers::CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2::touchCellLastTime(
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


