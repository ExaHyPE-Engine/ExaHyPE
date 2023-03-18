#include "InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::~InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::beginTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::endTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::createPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::destroyPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::createHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::destroyHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::touchFaceFirstTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::createCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::destroyCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::touchCellFirstTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::touchCellLastTime(
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

  if (
    not marker.hasBeenRefined() 
    and 
    
repositories::InstanceOfSelfSimilarInfallFD4.isLastGridSweepOfTimeStep() 
//and 
//not repositories::InstanceOfSelfSimilarInfallFD4.patchCanUseStatelessPDETerms(marker.x(), marker.h(), timeStamp, timeStepSize)
  ) { 
    logTraceIn( "touchCellFirstTime(...)" );
    int index = 0;
    dfor( volume, 5 ) {
      double* value   = fineGridCellSelfSimilarInfallFD4Q.value + index;
      auto    volumeX = ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), 5, volume);
      auto    volumeH = ::exahype2::fv::getVolumeSize( marker.h(), 5);
      
      
    repositories::InstanceOfSelfSimilarInfallFD4.addDensity(volumeX,volumeH,value[0]);  
  
      
      index += 5 + 0;
    }
    logTraceOut( "touchCellFirstTime(...)" );
  } 
}


