#include "TimeStep2exahype2_grid_SetLabels5.h"


tarch::logging::Log examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::_log( "examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5");





examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::TimeStep2exahype2_grid_SetLabels5(int treeNumber) {
}


examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::~TimeStep2exahype2_grid_SetLabels5() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::beginTraversal(
      ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::endTraversal(
      ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFaceLabel.setBoundary( isBoundary );

}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFaceLabel.setBoundary( isBoundary );

}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


