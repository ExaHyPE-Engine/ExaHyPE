#include "{{CLASSNAME}}.h"
#include "tarch/multicore/Core.h"

#include <algorithm>

#include "exahype2/fv/Rusanov.h"

#ifdef UseNVIDIA
#include <nvToolsExt.h>
#endif


tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runComputeKernelsOnSkeletonCell(double* __restrict__  reconstructedPatch, const ::peano4::datamanagement::CellMarker& marker, double* __restrict__  targetPatch) {
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
  #endif
    [&](
      const double* __restrict__                   QL,
      const double* __restrict__                   QR,
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
         const double* __restrict__                   Q,
         const tarch::la::Vector<Dimensions,double>&  faceCentre,
         const tarch::la::Vector<Dimensions,double>&  volumeH,
         double                                       t,
         double                                       dt,
         int                                          normal,
         double * __restrict__                        F
        ) -> void {
          {% if FLUX_IMPLEMENTATION=="<none>" %}
          for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
          {% else %}
          repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
          {% endif %}
        },
        {% if NCP_IMPLEMENTATION!="<none>" %}
        [] (
          const double* __restrict__                   Q,
          const double * __restrict__                  dQdn,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       BgradQ[]
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, dQdn, faceCentre, volumeH, t, normal, BgradQ );
        },
        {% endif %}
        [] (
          const double* __restrict__                   Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
        },
        QL, QR, x, dx, t, dt, normal,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        FL,FR
      );
    },
    marker.x(),
    marker.h(),
    repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
    repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    reconstructedPatch,
    targetPatch
  );

  {{FREE_SKELETON_MEMORY}}
}





{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker&    marker,
  double* __restrict__                           reconstructedPatch
):
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),tarch::multicore::Task::DefaultPriority),
  _marker(marker),
  _reconstructedPatch(reconstructedPatch) {
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::run() {
  logTraceIn( "run()" );

  #if Dimensions==2
  const int destinationPatchSize = {{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
  const int sourcePatchSize      = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
  #elif Dimensions==3
  const int destinationPatchSize = {{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
  const int sourcePatchSize      = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
  #endif
  
  double destinationPatchOnGPU[destinationPatchSize]; // This works now since we know the array size at compile time (maybe later this needs to be on Heap)
  double * reconstructedPatch = _reconstructedPatch; // Fixes omp restrictions

  const double timeStamp = repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp();


#ifdef UseNVIDIA
  nvtxRangePushA("copyPatch");
#endif

// the first one should be alloc: not to:
#pragma omp target enter data map(alloc:destinationPatchOnGPU[0:destinationPatchSize]) map(to:reconstructedPatch[0:sourcePatchSize])

  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    destinationPatchOnGPU,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1 // halo size
  );

#ifdef UseNVIDIA
  nvtxRangePushA("Rusanov");
#endif
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop_Rusanov<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>(
  #elif Dimensions==3
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop_Rusanov<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>(
  #endif
    _marker.x(),
    _marker.h(),
    timeStamp,
    {{TIME_STEP_SIZE}},
    reconstructedPatch,
    destinationPatchOnGPU
    );

#ifdef UseNVIDIA
  nvtxRangePop();
#endif

  // Ab hier ist run , ^^^ alles in ctor
#pragma omp target exit data map(from:destinationPatchOnGPU[0:destinationPatchSize]) map(delete:reconstructedPatch[0:sourcePatchSize])


  // get stuff explicitly back from GPU, as it will be stored
  // locally for a while
  double* destinationPatchOnCPU = ::tarch::allocateMemory(destinationPatchSize, ::tarch::MemoryLocation::Heap);
  std::copy_n(destinationPatchOnGPU,destinationPatchSize,destinationPatchOnCPU);

  {{FREE_SKELETON_MEMORY}}

  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),destinationPatchSize,destinationPatchOnCPU);
  logTraceOut( "run()" );
  return false;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::prefetch() {
}

