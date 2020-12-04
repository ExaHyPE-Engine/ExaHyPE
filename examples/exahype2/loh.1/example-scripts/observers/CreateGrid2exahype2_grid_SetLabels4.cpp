#include "CreateGrid2exahype2_grid_SetLabels4.h"


tarch::logging::Log examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::_log( "examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4");





examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::CreateGrid2exahype2_grid_SetLabels4(int treeNumber) {
}


examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::~CreateGrid2exahype2_grid_SetLabels4() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::getGridControlEvents() {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::beginTraversal(
      ) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::endTraversal(
      ) {

}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {


  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFaceLabel.setBoundary( isBoundary );
}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::destroyPersistentFace(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel) {


  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFaceLabel.setBoundary( isBoundary );
}


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::destroyHangingFace(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::touchFaceFirstTime(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::touchFaceLastTime(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::createCell(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::destroyCell(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::touchCellFirstTime(
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


void examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_SetLabels4::touchCellLastTime(
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


