#include "TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4.h"


tarch::logging::Log examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::_log( "examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4");





examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4(int treeNumber) {
}


examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::~TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::beginTraversal(
      ) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::endTraversal(
      ) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {

    if (not marker.isRefined() and (InstanceOfEuler.getSolverState()==Euler::SolverState::Secondary or InstanceOfEuler.getSolverState()==Euler::SolverState::GridInitialisation )) {
      logTraceIn( "--backup patch overlap from fineGridFaceEulerQNew into fineGridFaceEulerQ--(...)" );
      int counter = 0;
      dfore(k,7,0,0) {
        for (int i=0; i<1.0*2; i++) {
          for (int j=0; j<5; j++) {
            fineGridFaceEulerQ.value[counter] = fineGridFaceEulerQNew.value[counter];
            counter++;
          }
        }
      }
      logTraceOut( "--backup patch overlap from fineGridFaceEulerQNew into fineGridFaceEulerQ--(...)" );
    }

}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& fineGridCellEulerCellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& fineGridCellEulerCellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& fineGridCellEulerCellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_BackupPatchOverlap4::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& fineGridCellEulerCellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {


}


