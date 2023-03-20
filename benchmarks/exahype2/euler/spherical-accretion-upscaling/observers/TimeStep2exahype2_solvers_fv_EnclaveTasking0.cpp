#include "TimeStep2exahype2_solvers_fv_EnclaveTasking0.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::TimeStep2exahype2_solvers_fv_EnclaveTasking0(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::~TimeStep2exahype2_solvers_fv_EnclaveTasking0() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::beginTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::endTraversal(
      ) {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::createPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::destroyPersistentFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::createHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::destroyHangingFace(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::touchFaceFirstTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::createCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::destroyCell(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::touchCellFirstTime(
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
    not marker.hasBeenRefined()
    and 
    true
    and
    repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Secondary 
  ) {
    const int taskNumber = fineGridCellSelfSimilarInfallFVCellLabel.getSemaphoreNumber();
    if ( marker.hasBeenEnclaveCell() and taskNumber>=0 ) {
      double maxEigenvalue;
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCellSelfSimilarInfallFVQ.value, maxEigenvalue );

      ::exahype2::fv::validatePatch(
        fineGridCellSelfSimilarInfallFVQ.value,
        5,
        0,
        4,
        0,
        std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
      );
      
      
    repositories::InstanceOfSelfSimilarInfallFV.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    const double newTimeStepSize = repositories::InstanceOfSelfSimilarInfallFV.getAdmissibleTimeStepSize();

      fineGridCellSelfSimilarInfallFVCellLabel.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      fineGridCellSelfSimilarInfallFVCellLabel.setHasUpdated(true);
      fineGridCellSelfSimilarInfallFVCellLabel.setTimeStepSize(newTimeStepSize);
    }
    
    if ( fineGridCellSelfSimilarInfallFVCellLabel.getHasUpdated() ) {
      double*  newQ = fineGridCellSelfSimilarInfallFVQ.value;
      
      

      repositories::InstanceOfSelfSimilarInfallFV.update(
        fineGridCellSelfSimilarInfallFVCellLabel.getTimeStepSize(),
        fineGridCellSelfSimilarInfallFVCellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
    else {
      repositories::InstanceOfSelfSimilarInfallFV.update(
        0.0,
        fineGridCellSelfSimilarInfallFVCellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_fv_EnclaveTasking0::touchCellLastTime(
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


