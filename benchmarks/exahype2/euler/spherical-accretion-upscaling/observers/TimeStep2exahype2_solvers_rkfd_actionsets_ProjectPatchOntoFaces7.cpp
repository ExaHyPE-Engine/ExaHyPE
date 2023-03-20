#include "TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::~TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::beginTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::endTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& fineGridFaceSelfSimilarInfallFD4QOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& fineGridFaceSelfSimilarInfallFD4QNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& fineGridFaceSelfSimilarInfallFD4QUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& fineGridFaceSelfSimilarInfallFD4FaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> fineGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> fineGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> fineGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> fineGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& fineGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& fineGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& fineGridCellSelfSimilarInfallFD4CellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld> coarseGridFacesSelfSimilarInfallFD4QOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew> coarseGridFacesSelfSimilarInfallFD4QNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate> coarseGridFacesSelfSimilarInfallFD4QUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel> coarseGridFacesSelfSimilarInfallFD4FaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& coarseGridCellSelfSimilarInfallFD4Q,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& coarseGridCellSelfSimilarInfallFD4QRhsEstimates,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& coarseGridCellSelfSimilarInfallFD4CellLabel) {

  logTraceIn( "touchCellLastTime(...)---ProjectPatchOntoFaces" );
  
  if (not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()==SelfSimilarInfallFD4::SolverState::RungeKuttaSecondarySubStep0) {
    
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables( 1, 5, 0, 5, 0);
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( 1, 5, 0, 5, 0 );

    // Set the variable
    // double timeStepSize
    
  double timeStepSize = repositories::InstanceOfSelfSimilarInfallFD4.getAdmissibleTimeStepSize();
    
    for(int d=0; d<Dimensions; d++) {
      /**
       * d-loop over all dimensions except d. The vector k's entry d is set
       * to 0. We start with the left/bottom face, i.e. the one closer to the 
       * coordinate system's origin.
       */
      dfore(k,5,d,0) {
        for (int i=0; i<3; i++) {
          tarch::la::Vector<Dimensions,int> patchCell   = k;
          tarch::la::Vector<Dimensions,int> overlapCell = k;
          patchCell(d)   = i;
          overlapCell(d) = i+3;
          
          // I could use the enumerator as well. Doesn't really matter here. One 
          // thing is tied to ExaHyPE, the other one to the blockstructured toolbox.
          int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,5,3,d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*(5+0) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<5; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            
              
            
          }

          for (int j=5; j<5+0; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
  
          patchCell(d)   = i+5-3;
          overlapCell(d) = i;
          
          overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,5,3,d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*(5+0) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<5; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d+Dimensions).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            
              
            
          }

          for (int j=5; j<5+0; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d+Dimensions).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
        } // overlap depth loop
      } // dfore, i..e loop over submanifold
    } // loop over dimensions
  } // if predicate holds, i.e. right RK step
  
  if (not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()==SelfSimilarInfallFD4::SolverState::GridInitialisation) {
    
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables( 1, 5, 0, 5, 0);
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( 1, 5, 0, 5, 0 );

    // Set the variable
    // double timeStepSize
    
  double timeStepSize = repositories::InstanceOfSelfSimilarInfallFD4.getAdmissibleTimeStepSize();
    
    for(int d=0; d<Dimensions; d++) {
      /**
       * d-loop over all dimensions except d. The vector k's entry d is set
       * to 0. We start with the left/bottom face, i.e. the one closer to the 
       * coordinate system's origin.
       */
      dfore(k,5,d,0) {
        for (int i=0; i<3; i++) {
          tarch::la::Vector<Dimensions,int> patchCell   = k;
          tarch::la::Vector<Dimensions,int> overlapCell = k;
          patchCell(d)   = i;
          overlapCell(d) = i+3;
          
          // I could use the enumerator as well. Doesn't really matter here. One 
          // thing is tied to ExaHyPE, the other one to the blockstructured toolbox.
          int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,5,3,d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*(5+0) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<5; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            
              
            
          }

          for (int j=5; j<5+0; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
  
          patchCell(d)   = i+5-3;
          overlapCell(d) = i;
          
          overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,5,3,d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*(5+0) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<5; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d+Dimensions).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            
              
            
          }

          for (int j=5; j<5+0; j++) {
            fineGridFacesSelfSimilarInfallFD4QUpdate(d+Dimensions).value[overlapCellSerialised*(5+0)+j] = 
              fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
        } // overlap depth loop
      } // dfore, i..e loop over submanifold
    } // loop over dimensions
  } // if predicate holds, i.e. right RK step
   // all the predicates are done now
    
  for (int d=0; d<Dimensions; d++) {
    fineGridFacesSelfSimilarInfallFD4FaceLabel(d).setUpdated(1,true);
    fineGridFacesSelfSimilarInfallFD4FaceLabel(d).setUpdatedTimeStamp(1,fineGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp());
    fineGridFacesSelfSimilarInfallFD4FaceLabel(d+Dimensions).setUpdated(0,true);
    fineGridFacesSelfSimilarInfallFD4FaceLabel(d+Dimensions).setUpdatedTimeStamp(0,fineGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp());
  }  
  logTraceOut( "touchCellLastTime(...)---ProjectPatchOntoFaces" );
}


