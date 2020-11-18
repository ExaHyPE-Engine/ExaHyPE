#include "TimeStep2exahype2_grid_AMROnPatch3.h"


tarch::logging::Log examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::_log( "examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3");





examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::TimeStep2exahype2_grid_AMROnPatch3(int treeNumber) {
}


examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::~TimeStep2exahype2_grid_AMROnPatch3() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::getGridControlEvents() const {

  return refinementControl.getGridControlEvents();
}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::beginTraversal(
      ) {

  _localRefinementControl.clear();


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::endTraversal(
      ) {

  refinementControl.merge( _localRefinementControl );


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::createCell(
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


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::destroyCell(
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


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {

  if (not marker.isRefined()) {
    logTraceIn( "touchCellFirstTime(...)" );
    double* patchData = fineGridCellAcousticsQ.value;
    
  { 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

    if (tarch::la::max( marker.h() ) > InstanceOfAcoustics.getMaxMeshSize() ) {
      refinementCriterion = ::exahype2::RefinementCommand::Refine;
    } 
    else {
      int index = 0;
      dfor( volume, 11 ) {
        refinementCriterion = refinementCriterion and InstanceOfAcoustics.refinementCriterion(
          fineGridCellAcousticsQ.value + index,
          ::exahype2::getVolumeCentre( marker.x(), marker.h(), 11, volume), 
          ::exahype2::getVolumeSize( marker.h(), 11 ),
          InstanceOfAcoustics.getMinTimeStamp()
        );
        index += 2 + 2;
      }
     
      if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < InstanceOfAcoustics.getMinMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
      else if (refinementCriterion==::exahype2::RefinementCommand::Coarsen and 3.0* tarch::la::max( marker.h() ) > InstanceOfAcoustics.getMaxMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
    }
    
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, false );
  } 
    logTraceOut( "touchCellFirstTime(...)" );
  }

}


void examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_AMROnPatch3::touchCellLastTime(
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


