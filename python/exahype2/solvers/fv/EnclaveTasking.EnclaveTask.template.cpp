#include "{{CLASSNAME}}.h"

// user includes
{{SOLVER_INCLUDES}}

#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/utils/Loop.h"

#include "tarch/multicore/SmartScheduler.h"
#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#if defined(UseSmartMPI)
#include "communication/Tags.h"
#endif

#include <string.h>

tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
{% if USE_GPU %}
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_gpuEnclaveTaskId( peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}", false) );
{% endif %}

#if defined(UseSmartMPI)
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(
  tarch::multicore::registerSmartMPITask(
    peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"),
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
  peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}")
);
#endif


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        targetPatch
) {
  {{PREPROCESS_RECONSTRUCTED_PATCH_IN_SKELETON_CELL}}

  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    targetPatch,
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

  tarch::freeMemory(reconstructedPatch,tarch::MemoryLocation::Heap );

  {{POSTPROCESS_UPDATED_PATCH_IN_SKELETON_CELL}}
}


{% if USE_GPU %}
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
{% else %}
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
    [&](double* reconstructedPatch, double* targetPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
          {{PREPROCESS_RECONSTRUCTED_PATCH_IN_ENCLAVE_TASK}}

          ::exahype2::fv::copyPatch(
            reconstructedPatch,
            targetPatch,
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

          {{POSTPROCESS_UPDATED_PATCH_IN_ENCLAVE_TASK}}
        }
  )
  #ifdef UseSmartMPI
  , smartmpi::Task(_enclaveTaskTypeId)
  #endif
{}
{% endif %}

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

{% if USE_GPU %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::run() {
  logTraceIn( "run()" );

  double* outputValues = tarch::allocateMemory(_destinationPatchSize,tarch::MemoryLocation::Heap);

  applyKernelToCell(
    _marker,
    _t,
    _dt,
    _reconstructedValues,
    outputValues
  );

  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_destinationPatchSize,outputValues);

  logTraceOut( "run()" );
  return false;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::canFuse() const {
  #if GPUOffloading
  return true;
  #else
  return false;
  #endif
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse( const std::list<Task*>& otherTasks ) {
  // @todo Debug
  logInfo( "fuse(...)", "asked to fuse " << otherTasks.size() << " tasks into one large GPU task" );

  assertion( not otherTasks.empty() );


  #if Dimensions==2
  ::exahype2::fv::patchData2d pD;
  #endif
  #if Dimensions==3
  ::exahype2::fv::patchData3d pD;
  #endif

  const int NPT =otherTasks.size();
  const int LR = _sourcePatchSize*NPT;
  const int LTOT = _destinationPatchSize*NPT;
  pD.npatches=NPT;

  pD.reco     = new double[LR];
  pD.id       = new    int[NPT];
  pD.x0       = new double[NPT];
  pD.h0       = new double[NPT];
  pD.x1       = new double[NPT];
  pD.h1       = new double[NPT];
  #if Dimensions==3
  pD.x2       = new double[NPT];
  pD.h2       = new double[NPT];
  #endif
  pD.t        = new double[NPT];
  pD.dt       = new double[NPT];
  pD.result   = new double[LTOT];
  memset(pD.result, 0, LTOT*sizeof(double));


  int i=0;
  for (auto p: otherTasks) {
    {{CLASSNAME}}* currentTask = static_cast<{{CLASSNAME}}*>(p);
    for (size_t j=0;j<_sourcePatchSize;j++) 
    {
      pD.reco[i*_sourcePatchSize + j] = currentTask->_reconstructedValues[j];
    }
    pD.id[i]  = currentTask->getTaskId();
    pD.x0[i]  = currentTask->_marker.x()[0];
    pD.h0[i]  = currentTask->_marker.h()[0];
    pD.x1[i]  = currentTask->_marker.x()[1];
    pD.h1[i]  = currentTask->_marker.h()[1];
  #if Dimensions==3
    pD.x2[i]  = currentTask->_marker.x()[2];
    pD.h2[i]  = currentTask->_marker.h()[2];
  #endif
    pD.t[i]   = _t;
    pD.dt[i]  = _dt;
    i++;
  }
  

  //
  // If the user specifies some pre-processing, it is inserted here. If
  // the preprocessing is empty, this loop should be removed by the
  // compiler.
  //
  //for (int i=0;i<static_cast<int>(localwork.size());i++) {
    // const auto& marker =
    // PREPROCESS_RECONSTRUCTED_PATCH has to go in here, but the marker can't be copied yet
  //}

  //double* destinationPatchOnCPU = ::tarch::allocateMemory(
    //_destinationPatchSize*otherTasks.size(),
    //::tarch::MemoryLocation::Heap
  //);
  //for (size_t i = 0;i<_destinationPatchSize*otherTasks.size();i++) *(destinationPatchOnCPU + i) =0;
  
  {{FUSED_RIEMANN_SOLVER_CALL}}
  (
    1,
    pD,
    _sourcePatchSize,
    _destinationPatchSize
  );
  for (int i=0;i<pD.npatches;i++) {
    const int taskid = pD.id[i];
    double* targetPatch = ::tarch::allocateMemory(_destinationPatchSize, ::tarch::MemoryLocation::Heap);
    //for (int j=0;j<_destinationPatchSize;j++)
    //{
       //targetPatch[j] = pD.result[j + i*_destinationPatchSize];
    //}
    for (int j=0;j<_destinationPatchSize;j++) targetPatch[j] = *(pD.result + j + i*_destinationPatchSize);
    //{{ POSTPROCESS_UPDATED_PATCH }}
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskid, _destinationPatchSize, targetPatch);
  }

  return true;
}
{% endif %}
