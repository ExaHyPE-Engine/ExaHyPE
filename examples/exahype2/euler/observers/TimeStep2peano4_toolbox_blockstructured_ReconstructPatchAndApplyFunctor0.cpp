#include "TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0.h"


tarch::logging::Log examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::_log( "examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0");





examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0(int treeNumber) {
  _treeNumber = treeNumber;
}


examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::~TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::beginTraversal(
      ) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::endTraversal(
      ) {


}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::createPersistentFace(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::destroyPersistentFace(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::createHangingFace(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::destroyHangingFace(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ,
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& coarseGridCellEulerCellSemaphoreLabel) {

  if ( not marker.isRefined() and fineGridFaceLabel.getBoundary() and (InstanceOfEuler.getSolverState()==Euler::SolverState::GridInitialisation or InstanceOfEuler.getSolverState()==Euler::SolverState::Primary or InstanceOfEuler.getSolverState()==Euler::SolverState::PrimaryAfterGridInitialisation) ) {
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), marker.isLocal() );
    
    logDebug( "touchFaceFirstTime(...)", "label=" << fineGridFaceLabel.toString() );
    ::exahype2::fv::applyBoundaryConditions(
      [&](
        double                                       Qinside[],
        double                                       Qoutside[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> void {
        InstanceOfEuler.boundaryConditions( Qinside, Qoutside, faceCentre, volumeH, t, normal );
      },
      marker.x(),
      marker.h(),
      InstanceOfEuler.getMinTimeStamp(),
      1e-06,
      7,
      5+0,
      marker.getSelectedFaceNumber(),
      fineGridFaceEulerQ.value
    );
    logTraceOut( "touchFaceFirstTime(...)" );
  }

}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::touchFaceLastTime(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::createCell(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::destroyCell(
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


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::touchCellFirstTime(
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

  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1*2;
      }
      else {
        base *= 7;
      }
    }
    return result;
  };

  if (not marker.isRefined() and (InstanceOfEuler.getSolverState()==Euler::SolverState::Primary or InstanceOfEuler.getSolverState()==Euler::SolverState::PrimaryAfterGridInitialisation)) {
    logTraceInWith1Argument( "touchCellFirstTime(...)", marker.toString() );

    
    double* reconstructedPatch;
    #if Dimensions==2
    reconstructedPatch = ::tarch::multicore::allocateMemory(405, ::tarch::multicore::MemoryLocation::Heap);
    #elif Dimensions==3
    reconstructedPatch = ::tarch::multicore::allocateMemory(3645, ::tarch::multicore::MemoryLocation::Heap);
    #endif


    //
    // Loop over original patch (k) and copy stuff over.
    //
    dfor(sourceCell,7) {
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>(1);
      int sourceCellSerialised       = peano4::utils::dLinearised(sourceCell,7);
      int destinationCellSerialised  = peano4::utils::dLinearised(destinationCell,7 + 2*1);
      for (int j=0; j<5; j++) {
        reconstructedPatch[destinationCellSerialised*5+j] = fineGridCellEulerQ.value[ sourceCellSerialised*5+j ];
        assertion3( reconstructedPatch[destinationCellSerialised*5+j]==reconstructedPatch[destinationCellSerialised*5+j], sourceCell, j, _treeNumber );
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
      dfore(k,7,d,0) {
        for (int i=0; i<1; i++) {
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>(1);
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,7 + 2*1);
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<5; j++) {
            reconstructedPatch[ destinationCellSerialised*5+j ] = fineGridFacesEulerQ(d).value[ sourceCellSerialised*5+j ];
            assertion6( 
              reconstructedPatch[ destinationCellSerialised*5+j ]==reconstructedPatch[ destinationCellSerialised*5+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber 
            );
          }

          destinationCell(d) = i+7+1;
          sourceCell(d)      = i+1;

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,7 + 2*1);
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<5; j++) {
            reconstructedPatch[ destinationCellSerialised*5+j ] = fineGridFacesEulerQ(d+Dimensions).value[ sourceCellSerialised*5+j ];
            assertion6( 
              reconstructedPatch[ destinationCellSerialised*5+j ]==reconstructedPatch[ destinationCellSerialised*5+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber
            );
          }
        }
      }
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }

    double* originalPatch = fineGridCellEulerQ.value;
    if (marker.isSkeletonCell()) {

    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
  
      [&](
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            
            InstanceOfEuler.flux( Q, faceCentre, volumeH, t, normal, F );
            
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            
            return InstanceOfEuler.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            
          }, 

          QL, QR, x, dx, t, dt, normal, 5, 0, FL, FR
        );

      },
      marker.x(),
      marker.h(),
      InstanceOfEuler.getMinTimeStamp(),
      1e-06,
      7,
      5,
      0,
      reconstructedPatch,
      originalPatch
  );

      tarch::multicore::freeMemory(reconstructedPatch,::tarch::multicore::MemoryLocation::Heap);
    }
    else {
      ::exahype2::EnclaveTask* newEnclaveTask = new ::exahype2::EnclaveTask(
        marker,
        reconstructedPatch,
        #if Dimensions==2
        245,
        #else
        1715,
        #endif
        [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
           ::exahype2::fv::copyPatch(
             reconstructedPatch,
             originalPatch,
             5,
             0,
             7,
             1
           );
           
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
  
      [&](
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            
            InstanceOfEuler.flux( Q, faceCentre, volumeH, t, normal, F );
            
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            
            return InstanceOfEuler.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            
          }, 

          QL, QR, x, dx, t, dt, normal, 5, 0, FL, FR
        );

      },
      marker.x(),
      marker.h(),
      InstanceOfEuler.getMinTimeStamp(),
      1e-06,
      7,
      5,
      0,
      reconstructedPatch,
      originalPatch
  );

        }
      );
      
      fineGridCellEulerCellSemaphoreLabel.setSemaphoreNumber( newEnclaveTask->getTaskId() );
      peano4::parallel::Tasks spawn( 
        newEnclaveTask,
        peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
        peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFV" )
      );      
    }
    
    
    
    
    
    logTraceOut( "touchCellFirstTime(...)" );
  }

}


void examples::exahype2::euler::observers::TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor0::touchCellLastTime(
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


