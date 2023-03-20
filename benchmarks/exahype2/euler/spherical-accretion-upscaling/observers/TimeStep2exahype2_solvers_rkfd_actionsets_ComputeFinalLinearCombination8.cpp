#include "TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::~TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::beginTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::endTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::createPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::destroyPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::createHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::destroyHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::touchFaceFirstTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::createCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::destroyCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::touchCellFirstTime(
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

  if (not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction and (repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()==SelfSimilarInfallFD4::SolverState::RungeKuttaSecondarySubStep0)) {
    
  double timeStepSize = repositories::InstanceOfSelfSimilarInfallFD4.getAdmissibleTimeStepSize();

    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1,            5, 0, 5, 0);
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( 1, 5, 0, 5, 0 );
    
    dfor( dof, 5 ) {
      for (int unknown=0; unknown<5; unknown++) {
        
        fineGridCellSelfSimilarInfallFD4Q.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] += 
          timeStepSize * 1 * fineGridCellSelfSimilarInfallFD4QRhsEstimates.value[ enumeratorWithoutAuxiliaryVariables(0,dof,unknown) ];
        
       }
    }

    const double timeStamp = fineGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp();

    double* newQ = fineGridCellSelfSimilarInfallFD4Q.value;
    
    

    ::exahype2::fd::validatePatch(
      newQ,
      5,
      0,
      5,
      0,  // we do not have an overlap of 3 here within the cell data, only within reconstructed values
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // outcome has to be valid

    
    ::exahype2::CellData  patchData( 
      nullptr,      // QIn
      marker.x(), 
      marker.h(), 
      timeStamp, 
      timeStepSize, 
      fineGridCellSelfSimilarInfallFD4Q.value 
    );

    ::exahype2::fd::reduceMaxEigenvalue_patchwise_functors(
      patchData,
      5,
      0,                            // only reconstructed data have overlap of 3, not the real data
      5,
      0,
      [&](
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  gridCellCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
      )->double {
        return repositories::InstanceOfSelfSimilarInfallFD4.maxEigenvalue( Q, gridCellCentre, gridCellH, t, dt, normal );
      }
    );

    const double maxEigenvalue = patchData.maxEigenvalue[0];
    
    
    // Set the variable
    // double newTimeStepSize
    
    repositories::InstanceOfSelfSimilarInfallFD4.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    const double newTimeStepSize = repositories::InstanceOfSelfSimilarInfallFD4.getAdmissibleTimeStepSize();
     
    fineGridCellSelfSimilarInfallFD4CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCellSelfSimilarInfallFD4CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCellSelfSimilarInfallFD4CellLabel.setHasUpdated(true);
    repositories::InstanceOfSelfSimilarInfallFD4.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
    
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8::touchCellLastTime(
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


