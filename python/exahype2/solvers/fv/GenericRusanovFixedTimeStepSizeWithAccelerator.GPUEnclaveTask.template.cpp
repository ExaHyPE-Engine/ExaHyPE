#include "{{CLASSNAME}}.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"

#include <algorithm>

#include "exahype2/fv/Rusanov.h"

#include "peano4/parallel/Tasks.h"

#ifdef UseNVIDIA
#include <nvToolsExt.h>
#endif

{{INCLUDES}}


tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_gpuEnclaveTaskId( peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}") );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        targetPatch
) {
  {{PREPROCESS_RECONSTRUCTED_PATCH}}
  #if Dimensions==2
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(//_SplitLoop(
  #else
  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(//_SplitLoop(
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
        [] (
          const double* __restrict__                   Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       BgradQ[]
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
          {% endif %}
        },
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
        FL,FR,
       {{SKIP_FLUX}},
       {{SKIP_NCP}}
      );
    },
    [] (
      const double* __restrict__                   Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double* __restrict__                         S
    ) -> void {
      repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeCentre, volumeH, t, dt, S);
    },
    marker.x(),
    marker.h(),
    t,
    dt,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    reconstructedPatch,
    targetPatch
  );

  {{FREE_SKELETON_MEMORY}}
  {{POSTPROCESS_UPDATED_PATCH}}
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker&    marker,
  double                                         t,
  double                                         dt,
  double* __restrict__                           reconstructedValues
):
  tarch::multicore::Task(
    tarch::multicore::reserveTaskNumber(),
    _gpuEnclaveTaskId,
    tarch::multicore::Task::DefaultPriority
  ),
  _marker(marker),
  _t(t),
  _dt(dt),
  _reconstructedValues(reconstructedValues) {
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::run() {
  logTraceIn( "run()" );
  // @todo Holger: We should just run the single task here that's represented by this task.
  //               Can you please add it?
  logTraceOut( "run()" );
  return false;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse( const std::list<Task*>& otherTasks ) {
  // @todo Debug
  logInfo( "fuse(...)", "asked to fuse " << otherTasks.size() << " tasks into one large GPU task" );
  /*
  // @todo Holger: This patch container also has to store the timestamp and time step size per patch
  PatchContainer patchkeeper;
  for (auto p: otherTask) {
    {{CLASSNAME}}* currentTask = static_cast<{{CLASSNAME}}*>(p);
    #if Dimensions==2
    patchkeeper.push_back({currentTask->_reconstructedPatch, currentTask->_timeStamp, currentTask->getTaskId(), currentTask->_marker.x()[0], currentTask->_marker.h()[0], currentTask->_marker.x()[1], currentTask->_marker.h()[1]});
    #elif Dimensions==3
    patchkeeper.push_back({currentTask->_reconstructedPatch, currentTask->_timeStamp, currentTask->getTaskId(), currentTask->_marker.x()[0], currentTask->_marker.h()[0], currentTask->_marker.x()[1], currentTask->_marker.h()[1], currentTask->_marker.x()[2] , currentTask->_marker.h()[2]});
    #endif
  }

  // You don't need any of these. Just process the argument as it is.

  const int Nremain = _patchkeeper.size();
  const int Nmax = {{NGRABMAX}}; // This needs to be a template argument --- this (among other things) depends on the patchsize and the machine so GPU memory needs to be taken into account

  if (Nmax==0)  localwork = std::move(_patchkeeper);
  else
  {
     if (_patchkeeper.size()>0)
     {
        const int maxwork = std::min(Nremain, Nmax); // Don't request more work than there is
        for (int i=1;i<maxwork+1;i++)
        {
           localwork.push_back(_patchkeeper[Nremain-i]);
        }
        _patchkeeper.resize(Nremain-maxwork);
     }
  }
  myLock.free();

  //
  // If the user specifies some pre-processing, it is inserted here. If
  // the preprocessing is empty, this loop should be removed by the
  // compiler.
  //
  for (int i=0;i<static_cast<int>(localwork.size());i++) {
    // const auto& marker =
    // PREPROCESS_RECONSTRUCTED_PATCH has to go in here, but the marker can't be copied yet
  }

  if (localwork.size() >0)
  {
     double* destinationPatchOnCPU = ::tarch::allocateMemory(_destinationPatchSize*localwork.size(), ::tarch::MemoryLocation::Heap);
#if Dimensions==2
        ::exahype2::fv::Fusanov_2D<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},{{SOLVER_NAME}}>
#elif Dimensions==3
        ::exahype2::fv::Fusanov_3D<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},{{SOLVER_NAME}}>
#endif
           (1,{{TIME_STEP_SIZE}},  localwork, destinationPatchOnCPU, _sourcePatchSize, _destinationPatchSize, {{SKIP_FLUX}}, {{SKIP_NCP}});

     for (int i=0;i<static_cast<int>(localwork.size());i++)
     {
        const int taskid = std::get<2>(localwork[i]);
        double* outpatch = ::tarch::allocateMemory(_destinationPatchSize, ::tarch::MemoryLocation::Heap);
        std::copy(destinationPatchOnCPU + i*_destinationPatchSize, destinationPatchOnCPU + (i+1) * _destinationPatchSize, outpatch);
        {{POSTPROCESS_UPDATED_PATCH}}
        ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskid, _destinationPatchSize, outpatch);
        ::tarch::freeMemory(std::get<0>(localwork[i]), ::tarch::MemoryLocation::Heap);
     }
     ::tarch::freeMemory(destinationPatchOnCPU, ::tarch::MemoryLocation::Heap);
  }

  */
  return true;
}

