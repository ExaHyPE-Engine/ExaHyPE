#include "TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6(int treeNumber) {
  _treeNumber = treeNumber;
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::~TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::beginTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::endTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::createPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::destroyPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::createHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::destroyHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::touchFaceFirstTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::createCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::destroyCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::touchCellFirstTime(
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

  if (
    repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Primary or 
    repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PrimaryAfterGridInitialisation
  ) {
    fineGridCellSelfSimilarInfallFVCellLabel.setHasUpdated(false);
  }

  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1.0*2;
      }
      else {
        base *= 4;
      }
    }
    return result;
  };
  
  if (not marker.hasBeenRefined() and (repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Primary or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PrimaryAfterGridInitialisation)) {
    logTraceInWith1Argument( "touchCellFirstTime(...)", marker.toString() );

    
    double* oldQWithHalo;
    #if Dimensions==2
    oldQWithHalo = ::tarch::allocateMemory(180, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    #elif Dimensions==3
    oldQWithHalo = ::tarch::allocateMemory(1080, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    #endif


    //
    // Loop over original patch (k) and copy stuff over.
    //
    dfor(sourceCell,4) {
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>(1.0);
      int sourceCellSerialised       = peano4::utils::dLinearised(sourceCell,4);
      int destinationCellSerialised  = peano4::utils::dLinearised(destinationCell,4 + 2*1.0);
      for (int j=0; j<5; j++) {
        oldQWithHalo[destinationCellSerialised*5+j] = fineGridCellSelfSimilarInfallFVQ.value[ sourceCellSerialised*5+j ];
        assertion4( oldQWithHalo[destinationCellSerialised*5+j]==oldQWithHalo[destinationCellSerialised*5+j], sourceCell, j, _treeNumber, marker.toString() );
      }
    }

    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {
      logTraceInWith1Argument( "touchCellFirstTime(...)::loopOverFace", d );
      //
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the 
      // coordinate system's origin.
      //
      dfore(k,4,d,0) {
        for (int i=0; i<1.0; i++) {
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>(1.0);
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,4 + 2*1.0);
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<5; j++) {
            oldQWithHalo[ destinationCellSerialised*5+j ] = fineGridFacesSelfSimilarInfallFVQNew(d).value[ sourceCellSerialised*5+j ];
            assertion7( 
              false or 
              oldQWithHalo[ destinationCellSerialised*5+j ]==oldQWithHalo[ destinationCellSerialised*5+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }

          destinationCell(d) = i+4+1.0;
          sourceCell(d)      = i+1.0;

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,4 + 2*1.0);
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<5; j++) {
            oldQWithHalo[ destinationCellSerialised*5+j ] = fineGridFacesSelfSimilarInfallFVQNew(d+Dimensions).value[ sourceCellSerialised*5+j ];
            assertion7( 
              false or 
              oldQWithHalo[ destinationCellSerialised*5+j ]==oldQWithHalo[ destinationCellSerialised*5+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }
        }
      }
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }

    double* newQ = fineGridCellSelfSimilarInfallFVQ.value;
    
  double timeStamp    = fineGridCellSelfSimilarInfallFVCellLabel.getTimeStamp();

  // Set the following two paramters
  // double timeStepSize
  
  double timeStepSize = repositories::InstanceOfSelfSimilarInfallFV.getAdmissibleTimeStepSize();
    
  
    
  assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStepSize, timeStamp );
  assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStepSize, timeStamp );

  ::exahype2::fv::validatePatch(
      oldQWithHalo,
      5,
      0,
      4,
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid

  if (marker.willBeSkeletonCell()) {
    const double maxEigenvalue = tasks::SelfSimilarInfallFVEnclaveTask::applyKernelToCell(
      marker,
      timeStamp,
      timeStepSize,
      oldQWithHalo,
      newQ
    );
    
    
    repositories::InstanceOfSelfSimilarInfallFV.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    const double newTimeStepSize = repositories::InstanceOfSelfSimilarInfallFV.getAdmissibleTimeStepSize();

    fineGridCellSelfSimilarInfallFVCellLabel.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::SkeletonTask );
    fineGridCellSelfSimilarInfallFVCellLabel.setHasUpdated(true);
    fineGridCellSelfSimilarInfallFVCellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCellSelfSimilarInfallFVCellLabel.setTimeStepSize(newTimeStepSize);
  }
  else { // is an enclave cell
    assertion( marker.willBeEnclaveCell() );
    assertion( not marker.willBeRefined() );
    auto newEnclaveTask = new tasks::SelfSimilarInfallFVEnclaveTask(
      marker,
      timeStamp,
      timeStepSize,
      oldQWithHalo
    );
    fineGridCellSelfSimilarInfallFVCellLabel.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    fineGridCellSelfSimilarInfallFVCellLabel.setTimeStamp(timeStamp + timeStepSize);

    tarch::multicore::spawnTask( newEnclaveTask );
  }
  
    
    

    logTraceOut( "touchCellFirstTime(...)" );
  }

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor6::touchCellLastTime(
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


