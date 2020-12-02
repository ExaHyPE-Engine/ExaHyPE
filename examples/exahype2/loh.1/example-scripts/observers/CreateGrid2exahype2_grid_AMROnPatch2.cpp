#include "CreateGrid2exahype2_grid_AMROnPatch2.h"


tarch::logging::Log examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::_log( "examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2");





examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::CreateGrid2exahype2_grid_AMROnPatch2(int treeNumber) {
}


examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::~CreateGrid2exahype2_grid_AMROnPatch2() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::getGridControlEvents() {

  return refinementControl.getGridControlEvents();
}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::beginTraversal(
      ) {

  _localRefinementControl.clear();

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::endTraversal(
      ) {

  refinementControl.merge( _localRefinementControl );

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

  if (not marker.isRefined()) {
    logTraceIn( "touchCellFirstTime(...)" );
    double* patchData = fineGridCellLOH1Q.value;
    
  { 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();
    int index = 0;
    dfor( volume, 5 ) {
      refinementCriterion = refinementCriterion and InstanceOfLOH1.refinementCriterion(
        fineGridCellLOH1Q.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), 5, volume), 
        ::exahype2::getVolumeSize( marker.h(), 5 ),
        InstanceOfLOH1.getMinTimeStamp()
      );
      index += 9 + 4;
    }
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, true );
  } 

    logTraceOut( "touchCellFirstTime(...)" );
  }
}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_AMROnPatch2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {

}


