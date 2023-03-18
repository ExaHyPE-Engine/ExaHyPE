#include "TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0(int treeNumber) {
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::~TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::beginTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::endTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::createPersistentFace(
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

  logTraceInWith1Argument( "createPersistentFace(...)", marker.toString() );
  
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      coarseGridCellSelfSimilarInfallFD4Q.value,
      coarseGridFacesSelfSimilarInfallFD4QOld(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFD4QOld.value
  );
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      coarseGridCellSelfSimilarInfallFD4Q.value,
      coarseGridFacesSelfSimilarInfallFD4QNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFD4QNew.value
  );
  
  // It is important that we clear the halo layer. If we have two layers of 
  // AMR, the finest one will restrict into QUpdate (so it has to be properly
  // initialised as 0).
  ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QUpdate.value
  );
  fineGridFaceSelfSimilarInfallFD4FaceLabel.setNewTimeStamp(coarseGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp());
  fineGridFaceSelfSimilarInfallFD4FaceLabel.setOldTimeStamp(coarseGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp());
  
  logTraceOutWith1Argument( "createPersistentFace(...)", marker.toString() );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::destroyPersistentFace(
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
   
  if ( not marker.isInteriorFaceWithinPatch() ) {
    logTraceIn( "destroyPersistentFace(...)" );
    
    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QUpdate.value,
      coarseGridFacesSelfSimilarInfallFD4QUpdate(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QNew.value,
      coarseGridFacesSelfSimilarInfallFD4QNew(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QOld.value,
      coarseGridFacesSelfSimilarInfallFD4QOld(marker.getSelectedFaceNumber()).value
    );
    
    // A coarse face might have been non-persistent before. So it might
    // not carry a valid boundary flag, and we have to re-analyse it and
    // set it accordingly.    
    tarch::la::Vector<Dimensions, double> offset(DomainOffset);
    tarch::la::Vector<Dimensions, double> size(DomainSize);
    bool isBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
      isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
    }
    coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setBoundary( isBoundary );

    // left and right
    for (int i=0; i<2; i++) {
      double newTimeStamp     = std::max( coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(     i ), fineGridFaceSelfSimilarInfallFD4FaceLabel.getNewTimeStamp( i ));
      double oldTimeStamp     = std::max( coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(     i ), fineGridFaceSelfSimilarInfallFD4FaceLabel.getOldTimeStamp( i ));
      double updatedTimeStamp = std::max( coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( i ), fineGridFaceSelfSimilarInfallFD4FaceLabel.getUpdatedTimeStamp( i ));
      coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setUpdated(         i,true);
      coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setNewTimeStamp(    i,newTimeStamp);
      coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setOldTimeStamp(    i,oldTimeStamp);
      coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i,updatedTimeStamp);
    }
  
    logTraceOut( "destroyPersistentFace(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::createHangingFace(
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

  if ( 
  repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridInitialisation
 ) {
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      coarseGridFacesSelfSimilarInfallFD4QNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFD4QNew.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      coarseGridFacesSelfSimilarInfallFD4QOld(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFD4QOld.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      coarseGridFacesSelfSimilarInfallFD4QNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFD4QNew.value
    );

    // It is important that we clear the halo layer. If we have two layers of 
    // AMR, the finest one will restrict into QUpdate (so it has to be properly
    // initialised as 0).
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QUpdate.value
    );
    const int leftRightEntry = marker.getSelectedFaceNumber()<Dimensions ? 0 : 1;
    fineGridFaceSelfSimilarInfallFD4FaceLabel.setNewTimeStamp(leftRightEntry,coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry));
    fineGridFaceSelfSimilarInfallFD4FaceLabel.setOldTimeStamp(leftRightEntry,coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry));

    logTraceOut( "createHangingFace(...)" );
  }
  else if ( repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()==SelfSimilarInfallFD4::SolverState::GridInitialisation ) {
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );
    
    int normal = marker.getSelectedFaceNumber() % Dimensions;
    dfore(i, 5, normal, 0) {
      for( int j=0; j<2*3; j++) {
        tarch::la::Vector<Dimensions, int> dof = i;
        dof(normal) = j;
        
        int serialisedDoF = ::toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          dof,
          5,
          3,
          normal
        );
        
        repositories::InstanceOfSelfSimilarInfallFD4.initialCondition(
          fineGridFaceSelfSimilarInfallFD4QNew.value + serialisedDoF * 5,
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), 5, 3, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), 5 ),
          true // grid grid is constructed
        );
        repositories::InstanceOfSelfSimilarInfallFD4.initialCondition(
          fineGridFaceSelfSimilarInfallFD4QOld.value + serialisedDoF * 5,
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), 5, 3, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), 5 ),
          true // grid grid is constructed
        );
        repositories::InstanceOfSelfSimilarInfallFD4.initialCondition(
          fineGridFaceSelfSimilarInfallFD4QUpdate.value + serialisedDoF * 5,
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), 5, 3, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), 5 ),
          true // grid grid is constructed
        );
      }
    }
    
    logTraceOut( "createHangingFace(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::destroyHangingFace(
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

  if ( true ) {
    logTraceInWith4Arguments( "destroyHangingFace(...)", "fineGridFaceSelfSimilarInfallFD4", "coarseGridFacesSelfSimilarInfallFD4", marker.getSelectedFaceNumber(), marker.toString() );

    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QUpdate.value,
      coarseGridFacesSelfSimilarInfallFD4QUpdate(marker.getSelectedFaceNumber()).value
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= Dimensions;
    double newTimeStamp = std::max( coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), fineGridFaceSelfSimilarInfallFD4FaceLabel.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
    coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFacesSelfSimilarInfallFD4FaceLabel(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
    
    logTraceOut( "destroyHangingFace(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::touchFaceFirstTime(
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

  if ( true ) {
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), "clear halo layer fineGridFaceSelfSimilarInfallFD4" );
    
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      5,
      3,
      5,
      fineGridFaceSelfSimilarInfallFD4QUpdate.value
    );

    logTraceOut( "touchFaceFirstTime(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::createCell(
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
    
  logTraceIn( "createCell(...)" );

  ::toolbox::blockstructured::interpolateCell_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      5,
      coarseGridCellSelfSimilarInfallFD4Q.value,
      fineGridCellSelfSimilarInfallFD4Q.value
  );

  ::exahype2::fv::validatePatch(
    fineGridCellSelfSimilarInfallFD4Q.value,
    5,
    0, // auxiliary values. Not known here
    5,
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 

  fineGridCellSelfSimilarInfallFD4CellLabel.setTimeStamp( coarseGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp() );
  fineGridCellSelfSimilarInfallFD4CellLabel.setTimeStepSize( coarseGridCellSelfSimilarInfallFD4CellLabel.getTimeStepSize() );

  logTraceOut( "createCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::destroyCell(
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

  logTraceInWith1Argument( "destroyCell(...)", marker.toString() );

  ::toolbox::blockstructured::restrictCell_AoS_tensor_product<SelfSimilarInfallFD4>(
      marker,
      5,
      5,
      fineGridCellSelfSimilarInfallFD4Q.value,
      coarseGridCellSelfSimilarInfallFD4Q.value
  );

  // exploit the fact that we can "reuse" some FV utils here    
  ::exahype2::fv::validatePatch(
    fineGridCellSelfSimilarInfallFD4Q.value,
    5,
    0, // auxiliary values. Not known here
    5,
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 
  
  coarseGridCellSelfSimilarInfallFD4CellLabel.setTimeStamp( fineGridCellSelfSimilarInfallFD4CellLabel.getTimeStamp() );
  coarseGridCellSelfSimilarInfallFD4CellLabel.setTimeStepSize( fineGridCellSelfSimilarInfallFD4CellLabel.getTimeStepSize() );
  
  logTraceOut( "destroyCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::touchCellFirstTime(
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

  if ( marker.hasBeenRefined() and not marker.willBeRefined() ) {
    logTraceInWith2Arguments( "touchCellFirstTime(...)", marker.toString(), "clear cell fineGridCellSelfSimilarInfallFD4Q" );
    
    ::toolbox::blockstructured::clearCell(
      marker,
      5,
      5,
      fineGridCellSelfSimilarInfallFD4Q.value
    );

    logTraceOut( "touchCellFirstTime(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::touchCellLastTime(
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


