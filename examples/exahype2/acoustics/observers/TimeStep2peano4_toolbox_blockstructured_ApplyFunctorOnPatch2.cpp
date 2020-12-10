#include "TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2.h"


tarch::logging::Log examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::_log( "examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2");





examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2(int treeNumber) {
}


examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::~TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::beginTraversal(
      ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::endTraversal(
      ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ) {


}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::createCell(
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


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::destroyCell(
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


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::touchCellFirstTime(
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
    int index = 0;
    dfor( volume, 11 ) {
      InstanceOfAcoustics.adjustSolution(
        fineGridCellAcousticsQ.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), 11, volume), 
        ::exahype2::getVolumeSize( marker.h(), 11 ),
        InstanceOfAcoustics.getMinTimeStamp()
      );
      index += 2 + 2;
    }
  } 
    logTraceOut( "touchCellFirstTime(...)" );
  }

}


void examples::exahype2::acoustics::observers::TimeStep2peano4_toolbox_blockstructured_ApplyFunctorOnPatch2::touchCellLastTime(
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


