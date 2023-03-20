#include "PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0");





benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0(int treeNumber) {
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::~PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::prepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::unprepareTraversal() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::beginTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::endTraversal(
      ) {


}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::createPersistentFace(
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

  logTraceInWith1Argument( "createPersistentFace(...)", marker.toString() );
  
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      4,
      1,
      5,
      coarseGridCellSelfSimilarInfallFVQ.value,
      coarseGridFacesSelfSimilarInfallFVQOld(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFVQOld.value
  );
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      4,
      1,
      5,
      coarseGridCellSelfSimilarInfallFVQ.value,
      coarseGridFacesSelfSimilarInfallFVQNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFVQNew.value
  );
  
  // It is important that we clear the halo layer. If we have two layers of 
  // AMR, the finest one will restrict into QUpdate (so it has to be properly
  // initialised as 0).
  ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQUpdate.value
  );
  fineGridFaceSelfSimilarInfallFVFaceLabel.setNewTimeStamp(coarseGridCellSelfSimilarInfallFVCellLabel.getTimeStamp());
  fineGridFaceSelfSimilarInfallFVFaceLabel.setOldTimeStamp(coarseGridCellSelfSimilarInfallFVCellLabel.getTimeStamp());
  
  logTraceOutWith1Argument( "createPersistentFace(...)", marker.toString() );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::destroyPersistentFace(
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
 
  if ( not marker.isInteriorFaceWithinPatch() ) {
    logTraceIn( "destroyPersistentFace(...)" );
    
    ::toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQUpdate.value,
      coarseGridFacesSelfSimilarInfallFVQUpdate(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQNew.value,
      coarseGridFacesSelfSimilarInfallFVQNew(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQOld.value,
      coarseGridFacesSelfSimilarInfallFVQOld(marker.getSelectedFaceNumber()).value
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
    coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setBoundary( isBoundary );

    // left and right
    for (int i=0; i<2; i++) {
      double newTimeStamp     = std::max( coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(     i ), fineGridFaceSelfSimilarInfallFVFaceLabel.getNewTimeStamp( i ));
      double oldTimeStamp     = std::max( coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(     i ), fineGridFaceSelfSimilarInfallFVFaceLabel.getOldTimeStamp( i ));
      double updatedTimeStamp = std::max( coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( i ), fineGridFaceSelfSimilarInfallFVFaceLabel.getUpdatedTimeStamp( i ));
      coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setUpdated(         i,true);
      coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setNewTimeStamp(    i,newTimeStamp);
      coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setOldTimeStamp(    i,oldTimeStamp);
      coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i,updatedTimeStamp);
    }
  
    logTraceOut( "destroyPersistentFace(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::createHangingFace(
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

  if ( 
  repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridInitialisation
 ) {
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      4,
      1,
      5,
      coarseGridFacesSelfSimilarInfallFVQNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFVQNew.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      4,
      1,
      5,
      coarseGridFacesSelfSimilarInfallFVQOld(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFVQOld.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      4,
      1,
      5,
      coarseGridFacesSelfSimilarInfallFVQNew(marker.getSelectedFaceNumber()).value,
      fineGridFaceSelfSimilarInfallFVQNew.value
    );

    // It is important that we clear the halo layer. If we have two layers of 
    // AMR, the finest one will restrict into QUpdate (so it has to be properly
    // initialised as 0).
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQUpdate.value
    );
    const int leftRightEntry = marker.getSelectedFaceNumber()<Dimensions ? 0 : 1;
    fineGridFaceSelfSimilarInfallFVFaceLabel.setNewTimeStamp(leftRightEntry,coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry));
    fineGridFaceSelfSimilarInfallFVFaceLabel.setOldTimeStamp(leftRightEntry,coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry));

    logTraceOut( "createHangingFace(...)" );
  }
  else if ( repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::GridInitialisation ) {
    /*
    
     @todo Not required for FV, but compare to FD4 variant what has to be done 
           throughout initialisation.
     
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );
    
    int normal = marker.getSelectedFaceNumber() % Dimensions;
    dfore(i, 4, normal, 0) {
      for( int j=0; j<2*1; j++) {
        tarch::la::Vector<Dimensions, int> dof = i;
        dof(normal) = j;
        
        int serialisedDoF = ::toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          dof,
          4,
          1,
          normal
        );
        
        repositories::InstanceOfSelfSimilarInfallFV.initialCondition(
          fineGridFaceSelfSimilarInfallFVQNew.value + serialisedDoF * 5,
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), 4, 1, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), 4 ),
          true // grid grid is constructed
        );
        repositories::InstanceOfSelfSimilarInfallFV.initialCondition(
          fineGridFaceSelfSimilarInfallFVQOld.value + serialisedDoF * 5,
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), 4, 1, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), 4 ),
          true // grid grid is constructed
        );
        repositories::InstanceOfSelfSimilarInfallFV.initialCondition(
          fineGridFaceSelfSimilarInfallFVQUpdate.value + serialisedDoF * 5,
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), 4, 1, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), 4 ),
          true // grid grid is constructed
        );
      }
    }
    
    logTraceOut( "createHangingFace(...)" );
    */
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::destroyHangingFace(
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

  if ( true ) {
    logTraceInWith4Arguments( "destroyHangingFace(...)", "fineGridFaceSelfSimilarInfallFV", "coarseGridFacesSelfSimilarInfallFV", marker.getSelectedFaceNumber(), marker.toString() );

    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_averaging(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQUpdate.value,
      coarseGridFacesSelfSimilarInfallFVQUpdate(marker.getSelectedFaceNumber()).value
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= Dimensions;
    double newTimeStamp = std::max( coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), fineGridFaceSelfSimilarInfallFVFaceLabel.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
    coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFacesSelfSimilarInfallFVFaceLabel(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
    
    logTraceOut( "destroyHangingFace(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::touchFaceFirstTime(
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

  if ( true ) {
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), "clear halo layer fineGridFaceSelfSimilarInfallFV" );
    
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      4,
      1,
      5,
      fineGridFaceSelfSimilarInfallFVQUpdate.value
    );

    logTraceOut( "touchFaceFirstTime(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::touchFaceLastTime(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::createCell(
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
    
  logTraceIn( "createCell(...)" );

  ::toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
      marker,
      4,
      5,
      coarseGridCellSelfSimilarInfallFVQ.value,
      fineGridCellSelfSimilarInfallFVQ.value
  );

  ::exahype2::fv::validatePatch(
    fineGridCellSelfSimilarInfallFVQ.value,
    5,
    0, // auxiliary values. Not known here
    4,
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 

  fineGridCellSelfSimilarInfallFVCellLabel.setTimeStamp( coarseGridCellSelfSimilarInfallFVCellLabel.getTimeStamp() );
  fineGridCellSelfSimilarInfallFVCellLabel.setTimeStepSize( coarseGridCellSelfSimilarInfallFVCellLabel.getTimeStepSize() );

  logTraceOut( "createCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::destroyCell(
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

  logTraceInWith1Argument( "destroyCell(...)", marker.toString() );

  ::toolbox::blockstructured::restrictCell_AoS_averaging(
      marker,
      4,
      5,
      fineGridCellSelfSimilarInfallFVQ.value,
      coarseGridCellSelfSimilarInfallFVQ.value
  );

  // exploit the fact that we can "reuse" some FV utils here    
  ::exahype2::fv::validatePatch(
    fineGridCellSelfSimilarInfallFVQ.value,
    5,
    0, // auxiliary values. Not known here
    4,
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 
  
  coarseGridCellSelfSimilarInfallFVCellLabel.setTimeStamp( fineGridCellSelfSimilarInfallFVCellLabel.getTimeStamp() );
  coarseGridCellSelfSimilarInfallFVCellLabel.setTimeStepSize( fineGridCellSelfSimilarInfallFVCellLabel.getTimeStepSize() );
  
  logTraceOut( "destroyCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::touchCellFirstTime(
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

  if ( marker.hasBeenRefined() and not marker.willBeRefined() ) {
    logTraceInWith2Arguments( "touchCellFirstTime(...)", marker.toString(), "clear cell fineGridCellSelfSimilarInfallFVQ" );
    
    ::toolbox::blockstructured::clearCell(
      marker,
      4,
      5,
      fineGridCellSelfSimilarInfallFVQ.value
    );

    logTraceOut( "touchCellFirstTime(...)" );
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution2exahype2_solvers_fv_actionsets_DynamicAMR0::touchCellLastTime(
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


