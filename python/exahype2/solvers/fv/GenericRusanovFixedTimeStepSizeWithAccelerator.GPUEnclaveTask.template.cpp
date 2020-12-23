#include "{{CLASSNAME}}.h"
#include "tarch/multicore/Core.h"

#include <algorithm>


tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runComputeKernelsOnSkeletonCell(double* __restrict__  reconstructedPatch, const ::peano4::datamanagement::CellMarker& marker, double* __restrict__  targetPatch) {
  {% if USE_SPLIT_LOOP %}
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
  #endif
  {% else %}
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
  #endif
  {% endif %}
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
         double * __restrict__ Q,
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
          observers::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
          {% endif %}
        },
        {% if NCP_IMPLEMENTATION!="<none>" %}
        [] (
          double                                       Q[],
          double                                       gradQ[][Dimensions],
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       BgradQ[]
        ) -> void {
          observers::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
        },
        {% endif %}
        [] (
          double                                       Q[],
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return observers::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
        },
        QL, QR, x, dx, t, dt, normal,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        FL,FR
      );
    },
    marker.x(),
    marker.h(),
    observers::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
    observers::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
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
  double* destinationPatchOnGPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Accelerator);

  const double timeStamp = observers::{{SOLVER_INSTANCE}}.getMinTimeStamp();

  #if defined(GPUOffloading)
  #pragma omp target map(from:destinationPatchOnGPU[0:destinationPatchSize]) map(to:reconstructedPatch[0:sourcePatchSize])
  {
  #endif
  ::exahype2::fv::copyPatch(
    _reconstructedPatch,
    destinationPatchOnGPU,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1 // halo size
  );

  {% if USE_SPLIT_LOOP %}
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
  #endif
  {% else %}
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
  #endif
  {% endif %}
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
          double * __restrict__ Q,
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
          {{SOLVER_NAME}}::flux( Q, faceCentre, volumeH, t, normal, F );
          {% endif %}
        },
        {% if NCP_IMPLEMENTATION!="<none>" %}
        [] (
          double                                       Q[],
          double                                       gradQ[][Dimensions],
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       BgradQ[]
        ) -> void {
          {{SOLVER_NAME}}::nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
        },
        {% endif %}
        [] (
          double                                       Q[],
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return {{SOLVER_NAME}}::maxEigenvalue( Q, faceCentre, volumeH, t, normal);
        },
        QL, QR, x, dx, t, dt, normal,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        FL,FR
      );
    },
    _marker.x(),
    _marker.h(),
    timeStamp,
    {{TIME_STEP_SIZE}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    _reconstructedPatch,
    destinationPatchOnGPU
  );
  #if defined(GPUOffloading)
  }
  #endif


  // get stuff explicitly back from GPU, as it will be stored
  // locally for a while
  double* destinationPatchOnCPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Heap);
  std::copy_n(destinationPatchOnGPU,destinationPatchSize,destinationPatchOnCPU);

  // Free data given in from calling routine (requires pointer name without underscore)
  double* reconstructedPatch = _reconstructedPatch;
  {{FREE_SKELETON_MEMORY}}

  ::tarch::multicore::freeMemory(destinationPatchOnGPU, ::tarch::multicore::MemoryLocation::Accelerator);
  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),destinationPatchSize,destinationPatchOnCPU);
  logTraceOut( "run()" );
  return false;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::prefetch() {
}

