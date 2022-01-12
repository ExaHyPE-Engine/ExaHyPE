#include "{{CLASSNAME}}.h"

// user includes
{{SOLVER_INCLUDES}}

#include "config.h"

#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/parallel/parallel.h"

#include "peano4/utils/Loop.h"

#include "tarch/compiler/CompilerSpecificSettings.h"

#include "tarch/multicore/SmartScheduler.h"

#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#if defined(UseSmartMPI)
#include "communication/Tags.h"
#endif

#include <string.h>
#include <memory>

tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
{% if STATELESS_PDE_TERMS %}
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_gpuEnclaveTaskId( peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}") );
{% endif %}

#if defined(UseSmartMPI)
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(
  tarch::multicore::registerSmartMPITask(
    peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"),
    [](smartmpi::ReceiverCallType type, int rank, int tag, MPI_Comm communicator) -> smartmpi::Task* {
      if (type==smartmpi::ReceiverCallType::ReceiveTask) {
        return receiveTask( rank, tag, communicator );
      } else if (type==smartmpi::ReceiverCallType::ReceivedTaskOutcomeForFowarding) {
        static constexpr bool isForwarding = true;
        return receiveOutcome( rank, tag, communicator, isForwarding);
      } else {
        assert(type==smartmpi::ReceiverCallType::ReceiveOutcome);
        static constexpr bool isForwarding = false;
        return receiveOutcome( rank, tag, communicator, isForwarding );
      }
    }
  )
);
#else
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(
  peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}")
);
#endif


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        targetPatch
) {
  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    targetPatch,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1 // halo size
  );

  {{COMPUTE_KERNEL_CALL}}
  (
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
      {{RIEMANN_SOLVER_CALL}}
    },
    [&](
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      double * __restrict__                        S
    ) -> void {
      {{SOURCE_TERM_CALL}}
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

  ::exahype2::fv::validatePatch(
    targetPatch,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    0, // halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // outcome has to be valid

  tarch::freeMemory(reconstructedPatch,tarch::MemoryLocation::Heap );

  {{POSTPROCESS_UPDATED_PATCH}}


  {% if COMPUTE_MAX_EIGENVALUE %}
  const double timeStamp    = t;
  const double timeStepSize = dt;

  const double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
    [] (
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) -> double {
      return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
    },
    marker.x(),
    marker.h(),
    timeStamp,
    timeStamp + timeStepSize,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    targetPatch
  );

  return maxEigenvalue;
  {% else %}
  return 0.0;
  {% endif %}
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch
):
  ::exahype2::EnclaveTask(
    _enclaveTaskTypeId,
    marker,
    t,
    dt,
    reconstructedPatch,
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    #endif
    [&](double* reconstructedPatch, double* targetPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt, double& maxEigenvalue ) -> void {
          ::exahype2::fv::copyPatch(
            reconstructedPatch,
            targetPatch,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{NUMBER_OF_VOLUMES_PER_AXIS}},
            1 // halo size
          );

          {{COMPUTE_KERNEL_CALL}}
          (
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
              {{RIEMANN_SOLVER_CALL}}
            },
          [&](
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       dx,
            double                                       t,
            double                                       dt,
            double * __restrict__                        S
          ) -> void {
            {{SOURCE_TERM_CALL}}
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

          {{POSTPROCESS_UPDATED_PATCH}}

          {% if COMPUTE_MAX_EIGENVALUE %}
          const double timeStamp    = t;
          const double timeStepSize = dt;

          maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
            [] (
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal
            ) -> double {
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            },
            marker.x(),
            marker.h(),
            timeStamp,
            timeStamp + timeStepSize,
            {{NUMBER_OF_VOLUMES_PER_AXIS}},
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            targetPatch
          );
          {% else %}
          maxEigenvalue = 0.0;
          {% endif %}
        }
  )
  #ifdef UseSmartMPI
  , smartmpi::Task(_enclaveTaskTypeId)
  #endif
{}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSmartMPITask() const {
  #ifdef UseSmartMPI
  return true;
  #else
  return false;
  #endif
}


#ifdef UseSmartMPI
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocally() {
  run();
  if (_remoteTaskId != -1) {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(_remoteTaskId,_numberOfResultValues,_outputValues);
  } else {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues);
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::moveTask(int rank, int tag, MPI_Comm communicator) {
  ::tarch::mpi::DoubleMessage  tMessage(_t);
  ::tarch::mpi::DoubleMessage  dtMessage(_dt);
  ::tarch::mpi::IntegerMessage taskIdMessage;

  if ( tag != smartmpi::communication::MoveTaskToMyServerForEvaluationTag ) {
    taskIdMessage.setValue(_remoteTaskId);
  } else {
    taskIdMessage.setValue(getTaskId());
  }

  ::peano4::datamanagement::CellMarker::send( _marker, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::send( taskIdMessage, rank, tag, communicator );

  MPI_Request request;
  MPI_Isend( _inputValues, _numberOfInputValues, MPI_DOUBLE, rank, tag, communicator, &request );

  logInfo(
    "moveTask(...)",
    "sent (" << _marker.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << _numberOfInputValues <<
    "," << taskIdMessage.toString() << ") to rank " << rank <<
    " via tag " << tag
  );
}


smartmpi::Task* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveTask(int rank, int tag, MPI_Comm communicator) {
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfInputValues =
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* inputValues = tarch::allocateMemory( NumberOfInputValues, tarch::MemoryLocation::Heap );

  ::peano4::datamanagement::CellMarker::receive( markerMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::receive( taskIdMessage, rank, tag, communicator );

  logInfo(
    "receiveTask(...)",
    "received (" << markerMessage.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << taskIdMessage.toString() << ") from rank " << rank <<
    " via tag " << tag << " and will now receive " << NumberOfInputValues << " doubles"
  );

  MPI_Recv( inputValues, NumberOfInputValues, MPI_DOUBLE, rank, tag, communicator,
    MPI_STATUS_IGNORE
  );

  {{CLASSNAME}}* result = new {{CLASSNAME}}(
    markerMessage,
    tMessage.getValue(),
    dtMessage.getValue(),
    inputValues
  );
  result->_remoteTaskId = taskIdMessage.getValue();
  return result;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocallyAndSendTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker,_t,_dt);
  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

  logInfo(
    "runLocallyAndSendTaskOutputToRank(...)",
    "executed remote task on this rank. Will start to send result back"
  );

  forwardTaskOutputToRank(rank, tag, communicator);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::forwardTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  logInfo(
    "forwardTaskOutputToRank(...)",
    "will start to forward task output (which has already been computed)"
  );

  ::tarch::mpi::DoubleMessage  tMessage(_t);
  ::tarch::mpi::DoubleMessage  dtMessage(_dt);
  ::tarch::mpi::IntegerMessage taskIdMessage(_remoteTaskId);

  ::peano4::datamanagement::CellMarker::send( _marker, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::send( taskIdMessage, rank, tag, communicator );

  MPI_Request request;
  MPI_Isend( _outputValues, _numberOfResultValues, MPI_DOUBLE, rank, tag, communicator, &request );

  logInfo(
    "forwardTaskOutputToRank(...)",
    "sent (" << _marker.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << _numberOfResultValues <<
    "," << taskIdMessage.toString() << ") to rank " << rank <<
    " via tag " << tag
  );

  // tarch::freeMemory(_outputValues,tarch::MemoryLocation::Heap );
}


smartmpi::Task* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveOutcome(int rank, int tag, MPI_Comm communicator, const bool intentionToForward) {
  logInfo( "receiveOutcome(...)", "rank=" << rank << ", tag=" << tag );
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfResultValues =
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* outputValues = tarch::allocateMemory( NumberOfResultValues, tarch::MemoryLocation::Heap );

  ::peano4::datamanagement::CellMarker::receive( markerMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( dtMessage, rank, tag, communicator );
  ::tarch::mpi::IntegerMessage::receive( taskIdMessage, rank, tag, communicator );

  logInfo(
    "receiveOutcome(...)",
    "received (" << markerMessage.toString() << "," << tMessage.toString() << "," << dtMessage.toString() << "," << taskIdMessage.toString() << ") from rank " << rank <<
    " via tag " << tag << " and will now receive " << NumberOfResultValues << " doubles"
  );

  MPI_Recv( outputValues, NumberOfResultValues, MPI_DOUBLE, rank, tag, communicator,
    MPI_STATUS_IGNORE
  );

  /**
   * Having received the output there are two further options:
   * we may need to forward it yet again to another rank - in this case
   * we need a pointer to the task which contains the output;
   * alternatively we bookmark the output and return a nullptr
   */
  if(intentionToForward) {
    double* inputValues = nullptr; // no input as already computed

    {{CLASSNAME}}* result = new {{CLASSNAME}}(
      markerMessage,
      tMessage.getValue(),
      dtMessage.getValue(),
      inputValues
    );
    result->_remoteTaskId = taskIdMessage.getValue();
    result->_outputValues = outputValues;
    return result;
  }
  logInfo(
    "receiveOutcome(...)",
    "bookmark outcome of task " << taskIdMessage.getValue()
  );
  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskIdMessage.getValue(),NumberOfResultValues,outputValues);
  return nullptr;
}
#endif

{% if STATELESS_PDE_TERMS %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::canFuse() const {
  return true;
}


/**
 * Also merge the current task
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse( const std::list<Task*>& otherTasks, int targetDevice ) {
  logDebug( "fuse(...)", "asked to fuse " << (otherTasks.size()+1) << " tasks into one large GPU task" );

  assertion( not otherTasks.empty() );

  #if Dimensions==2
  ::exahype2::fv::patchData2d pD;
  #endif
  #if Dimensions==3
  ::exahype2::fv::patchData3d pD;
  #endif

  const int NPT  = otherTasks.size()+1;
  const int LR   = _numberOfInputValues*NPT;
  const int LTOT = _numberOfResultValues*NPT;
  pD.npatches=NPT;

  pD.reco     = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[LR];
  pD.id       = new (std::align_val_t(ExaHyPEFusedKernelAlignment))   int[NPT];
  pD.x0       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.h0       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.x1       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.h1       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  #if Dimensions==3
  pD.x2       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.h2       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  #endif
  pD.t        = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.dt       = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  pD.result   = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[LTOT];
  pD.maxEigenvalue    = new (std::align_val_t(ExaHyPEFusedKernelAlignment)) double[NPT];
  memset(pD.result, 0, LTOT*sizeof(double));


  int i=0;
  for (auto p: otherTasks) {
    {{CLASSNAME}}* currentTask = static_cast<{{CLASSNAME}}*>(p);
    for (size_t j=0;j<_numberOfInputValues;j++) {
      pD.reco[i*_numberOfInputValues + j] = currentTask->_inputValues[j];
    }
    pD.id[i]    = currentTask->getTaskId();
    pD.x0[i]    = currentTask->_marker.x()[0];
    pD.h0[i]    = currentTask->_marker.h()[0];
    pD.x1[i]    = currentTask->_marker.x()[1];
    pD.h1[i]    = currentTask->_marker.h()[1];
    #if Dimensions==3
    pD.x2[i]    = currentTask->_marker.x()[2];
    pD.h2[i]    = currentTask->_marker.h()[2];
    #endif
    pD.t[i]     = currentTask->_t;
    pD.dt[i]    = currentTask->_dt;
    pD.maxEigenvalue[i] = -1.0;
    delete[]  currentTask->_inputValues;
    i++;

    delete currentTask;
  }
  //
  // Now add the data from the task from which this whole thing has been called
  //
  for (size_t j=0;j<_numberOfInputValues;j++) {
    pD.reco[i*_numberOfInputValues + j] = _inputValues[j];
  }
  pD.id[i]  = getTaskId();
  pD.x0[i]  = _marker.x()[0];
  pD.h0[i]  = _marker.h()[0];
  pD.x1[i]  = _marker.x()[1];
  pD.h1[i]  = _marker.h()[1];
  #if Dimensions==3
  pD.x2[i]  = _marker.x()[2];
  pD.h2[i]  = _marker.h()[2];
  #endif
  pD.t[i]   = _t;
  pD.dt[i]  = _dt;
  pD.maxEigenvalue[i] = -1.0;
  
  delete[] _inputValues;

  //
  // ==============
  // Invoke kernels
  // ==============
  //
  bool foundOffloadingBranch = false;

  #if OpenMPGPUOffloading>0
  if (targetDevice>=0) {
    foundOffloadingBranch = true;
    #if Dimensions==2
    ::exahype2::fv::Fusanov_2D_with_OpenMP_offloading
    #elif Dimensions==3
    ::exahype2::fv::Fusanov_3D_with_OpenMP_offloading
    #endif
    {{FUSED_RIEMANN_SOLVER_TEMPLATE_ARGUMENTS}}
    (
      1,
      pD,
      _numberOfInputValues,
      _numberOfResultValues,
      targetDevice
    );
  }
  #endif

  #if SYCLGPUOffloading>0
  if (targetDevice>=0) {
    static std::vector< sycl::queue* >  gpuQueues;

    if (targetDevice>=gpuQueues.size()) {
      // @todo Holger, I don't know how to assign a queue to a GPU
      // -> Ask Andy
      gpuQueues.push_back( new sycl::queue(gpu_selector{}) );
    }

    foundOffloadingBranch = true;

    #if Dimensions==2
    ::exahype2::fv::Fusanov_2D_with_SYCL_offloading
    #elif Dimensions==3
    ::exahype2::fv::Fusanov_3D_with_SYCL_offloading
    #endif
    {{FUSED_RIEMANN_SOLVER_TEMPLATE_ARGUMENTS}}
    // @todo First template arguments are generic (also used in other branches).
    // The second level of template arguments, i.e. the <1,1,1> are the ones that
    // describe the workgroup sizes, and we'll have to tune them later
    <1,1,1>
    (
      1,
      pD,
      _numberOfInputValues,
      _numberOfResultValues,
      *gpuQueues[targetDevice]
    );
  }
  #endif

  if (not foundOffloadingBranch) {
    #if Dimensions==2
    ::exahype2::fv::Fusanov_2D_with_batching
    #elif Dimensions==3
    ::exahype2::fv::Fusanov_3D_with_batching
    #endif
    {{FUSED_RIEMANN_SOLVER_TEMPLATE_ARGUMENTS}}
    (
      1,
      pD,
      _numberOfInputValues,
      _numberOfResultValues
    );
  }

  //
  // ==================
  // Bring back results
  // ==================
  //
  for (int i=0;i<pD.npatches;i++) {
    const int taskid = pD.id[i];
    double* targetPatch = ::tarch::allocateMemory(_numberOfResultValues, ::tarch::MemoryLocation::Heap);
    for (int j=0;j<_numberOfResultValues;j++) {
      targetPatch[j] = *(pD.result + j + i*_numberOfResultValues);
    }
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskid, _numberOfResultValues, targetPatch, pD.maxEigenvalue[i]);
  }

  return false;
}
{% endif %}
