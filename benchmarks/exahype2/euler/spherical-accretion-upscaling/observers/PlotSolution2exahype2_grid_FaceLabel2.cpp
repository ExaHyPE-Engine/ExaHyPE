#include "PlotSolution2exahype2_grid_FaceLabel2.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::PlotSolution2exahype2_grid_FaceLabel2(int treeNumber) {
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::~PlotSolution2exahype2_grid_FaceLabel2() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::beginTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::endTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {

  logTraceInWith1Argument( "createPersistent/HangingFace(...)", marker.toString() );
      
  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }
  fineGridFaceSelfSimilarInfallFVFaceLabel.setBoundary( isBoundary );

  fineGridFaceSelfSimilarInfallFVFaceLabel.setUpdatedTimeStamp( 0.0 );
  fineGridFaceSelfSimilarInfallFVFaceLabel.setNewTimeStamp( 0.0 );
  fineGridFaceSelfSimilarInfallFVFaceLabel.setOldTimeStamp( 0.0 );

  logTraceOutWith1Argument( "createPersistent/HangingFace(...)", fineGridFaceSelfSimilarInfallFVFaceLabel.toString() );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {

  fineGridFaceSelfSimilarInfallFVFaceLabel.setUpdated( false );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld& fineGridFaceSelfSimilarInfallFVQOld,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& fineGridFaceSelfSimilarInfallFVQNew,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& fineGridFaceSelfSimilarInfallFVQUpdate,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel& fineGridFaceSelfSimilarInfallFVFaceLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_grid_FaceLabel2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> fineGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> fineGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> fineGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> fineGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& fineGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& fineGridCellSelfSimilarInfallFVCellLabel,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQOld> coarseGridFacesSelfSimilarInfallFVQOld,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew> coarseGridFacesSelfSimilarInfallFVQNew,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate> coarseGridFacesSelfSimilarInfallFVQUpdate,
      peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVFaceLabel> coarseGridFacesSelfSimilarInfallFVFaceLabel,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& coarseGridCellSelfSimilarInfallFVQ,
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVCellLabel& coarseGridCellSelfSimilarInfallFVCellLabel) {


}


