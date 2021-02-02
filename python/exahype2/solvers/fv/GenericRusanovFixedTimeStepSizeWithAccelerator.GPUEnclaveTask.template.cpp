#include "{{CLASSNAME}}.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"

#include <algorithm>

#include "exahype2/fv/Rusanov.h"

#ifdef UseNVIDIA
#include <nvToolsExt.h>
#endif


tarch::multicore::BooleanSemaphore {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_patchsema;
//std::vector<std::tuple<const ::peano4::datamanagement::CellMarker&, double*, const double, int> > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_patchkeeper;
tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

#if Dimensions==2
std::vector<std::tuple<double*, const double, int, double, double, double, double> > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_patchkeeper;
#elif Dimensions==3
std::vector<std::tuple<double*, const double, int, double, double, double, double, double, double> > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_patchkeeper;
#endif

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
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       BgradQ[]
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
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
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),tarch::multicore::Task::DefaultPriority)
{
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );

  const double timeStamp = repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp();
  tarch::multicore::Lock myLock( _patchsema );
#if Dimensions==2
  _patchkeeper.push_back({reconstructedPatch, timeStamp, getTaskId(), marker.x()[0], marker.h()[0], marker.x()[1], marker.h()[1]});
#elif Dimensions==3
  _patchkeeper.push_back({reconstructedPatch, timeStamp, getTaskId(), marker.x()[0], marker.h()[0], marker.x()[1], marker.h()[1], marker.x()[2] , marker.h()[2]});
#endif
  myLock.free();
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::run()
{

  logTraceIn( "run()" );
  tarch::multicore::Lock myLock( _patchsema );
  auto localwork = std::move(_patchkeeper);
  myLock.free();

  if (localwork.size() >0)
  {
     double* destinationPatchOnCPU = ::tarch::allocateMemory(_destinationPatchSize*localwork.size(), ::tarch::MemoryLocation::Heap);
        ::exahype2::fv::Fusanov_2D<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>
           (1,{{TIME_STEP_SIZE}},  localwork, destinationPatchOnCPU, _sourcePatchSize, _destinationPatchSize);

     for (int i=0;i<localwork.size();i++)
     {
        const int taskid = std::get<2>(localwork[i]);
        double* outpatch = ::tarch::allocateMemory(_destinationPatchSize, ::tarch::MemoryLocation::Heap);
        std::copy(destinationPatchOnCPU + i*_destinationPatchSize, destinationPatchOnCPU + (i+1) * _destinationPatchSize, outpatch);
        ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskid, _destinationPatchSize, outpatch);
        ::tarch::freeMemory(std::get<0>(localwork[i]), ::tarch::MemoryLocation::Heap);
     }
     ::tarch::freeMemory(destinationPatchOnCPU, ::tarch::MemoryLocation::Heap);
  }

  logTraceOut( "run()" );
  return false;
}

//bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::run()
//{
  //logTraceIn( "run()" );
  //tarch::multicore::Lock myLock( _patchsema );
  //auto localwork = std::move(_patchkeeper);
  ////myLock.free();
  //if (localwork.size()==0) abort();
 
  //for (auto lw : localwork)
  //{
     //double * reconstructedPatch = std::get<0>(lw);
     //double            timeStamp = std::get<1>(lw);
     //int                  taskId = std::get<2>(lw);
     //double x0 = std::get<3>(lw);
     //double h0 = std::get<4>(lw);
     //double x1 = std::get<5>(lw);
     //double h1 = std::get<6>(lw);
//#if Dimensions==3
     //double x2 = std::get<7>(lw);
     //double h2 = std::get<8>(lw);
//# endif


     //double destinationPatch[_destinationPatchSize];

//#pragma omp target enter data map(alloc:destinationPatch[0:_destinationPatchSize]) map(to:reconstructedPatch[0:_sourcePatchSize])
  
//#ifdef UseNVIDIA
     //nvtxRangePop();
     //nvtxRangePushA("Rusanov");
//#endif

//#if Dimensions==2
     ////::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop_Rusanov<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>
     //::exahype2::fv::Fusanov_2D<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>
        //({x0,x1}, {h0,h1}, 1,
//#elif Dimensions==3
     //::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop_Rusanov<{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},EulerOnGPU>
        //({x0,x1,x2}, {h0,h1,h2},
//#endif
        //timeStamp,
       //{{TIME_STEP_SIZE}},
        //reconstructedPatch,
        //destinationPatch
        //);
//#ifdef UseNVIDIA
        //nvtxRangePop();
//#endif
//#pragma omp target exit data map(from:destinationPatch[0:_destinationPatchSize]) map(delete:reconstructedPatch[0:_sourcePatchSize])
        //::tarch::freeMemory(reconstructedPatch, ::tarch::MemoryLocation::Heap);

        //// get stuff explicitly back from GPU, as it will be stored
        //// locally for a while
        //double* destinationPatchOnCPU = ::tarch::allocateMemory(_destinationPatchSize, ::tarch::MemoryLocation::Heap);
        //std::copy_n(destinationPatch,_destinationPatchSize,destinationPatchOnCPU);

        //::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_destinationPatchSize,destinationPatchOnCPU);
  //}


  //logTraceOut( "run()" );
  //return false;
//}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::prefetch() {
}

