// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{{SOLVER_INCLUDES}}

#include "exahype2/dg/CellIntegral.h"
#include "exahype2/CellData.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/parallel/parallel.h"

#include "peano4/utils/Loop.h"

#include "tarch/CompilerSpecificSettings.h"

#include "tarch/multicore/smartScheduling.h"

#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#include <string.h>
#include <memory>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));


int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      timeStamp,
  double                                      timeStepSize,
  double* NOALIAS                        QIn,
  double* NOALIAS                        QOut
) {
  ::exahype2::CellData cellData(
    QIn,
    marker.getX(),
    marker.getH(),
    timeStamp,
    timeStepSize,
    QOut
  );

  {% if STATELESS_PDE_TERMS %}
  if (repositories::{{SOLVER_INSTANCE}}.cellCanUseStatelessPDETerms(
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
  )) {
    ::exahype2::dg::{{VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS}}
  } else
  {% endif %}

  ::exahype2::dg::{{VOLUMETRIC_COMPUTE_KERNEL_CALL}}
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  const double* NOALIAS                  linearCombinationOfPreviousShots
):
  ::exahype2::EnclaveTask<double>(
    _enclaveTaskTypeId,
    marker,
    t,
    dt,
    #if DIMENSIONS==2
    tarch::allocateMemory<double>({{NUMBER_OF_DOFS_PER_CELL_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}), tarch::MemoryLocation::Heap),
    nullptr,
    {{NUMBER_OF_DOFS_PER_CELL_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
    {{NUMBER_OF_DOFS_PER_CELL_2D}}* {{NUMBER_OF_UNKNOWNS}},
    #else
    tarch::allocateMemory<double>({{NUMBER_OF_DOFS_PER_CELL_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}), tarch::MemoryLocation::Heap),
    nullptr,
    {{NUMBER_OF_DOFS_PER_CELL_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
    {{NUMBER_OF_DOFS_PER_CELL_3D}}* {{NUMBER_OF_UNKNOWNS}},
    #endif
    [&]() -> void {
      applyKernelToCell(
        _marker,
        _t,
        _dt,
        _inputValues,
        _outputValues
      );

      tarch::freeMemory( _inputValues, tarch::MemoryLocation::Heap );
    }
  ),
  _linearCombinationOfPreviousShots(nullptr) {
  // @todo Name is not good. Should be SOLVE_VOLUME_INTEGRAL_ENCLAVE_TASK, so we can
  //       distinguish different task types once we map other steps onto tasks, too
  setPriority({{ENCLAVE_TASK_PRIORITY}});
  std::copy_n(linearCombinationOfPreviousShots, _numberOfInputValues, _inputValues);
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  std::shared_ptr< double[] >                 linearCombinationOfPreviousShots,
  double* NOALIAS                        output
):
  ::exahype2::EnclaveTask<double>(
    _enclaveTaskTypeId,
    marker,
    t,
    dt,
    linearCombinationOfPreviousShots.get(),
    output,
    #if DIMENSIONS==2
    {{NUMBER_OF_DOFS_PER_CELL_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
    {{NUMBER_OF_DOFS_PER_CELL_2D}}* {{NUMBER_OF_UNKNOWNS}},
    #else
    {{NUMBER_OF_DOFS_PER_CELL_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
    {{NUMBER_OF_DOFS_PER_CELL_3D}}* {{NUMBER_OF_UNKNOWNS}},
    #endif
    [&]() -> void {
      applyKernelToCell(
        _marker,
        _t,
        _dt,
        _inputValues,
        _outputValues
      );
    }
  ),
  _linearCombinationOfPreviousShots(linearCombinationOfPreviousShots) {
  // @todo Name is not good. Should be SOLVE_VOLUME_INTEGRAL_ENCLAVE_TASK, so we can
  //       distinguish different task types once we map other steps onto tasks, too
  setPriority({{ENCLAVE_TASK_PRIORITY}});
}


#ifdef UseSmartMPI
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSmartMPITask() const {
#ifdef UseSmartMPI
  return true;
#else
  return false;
#endif
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocally() {
  computeTask();
  if (_remoteTaskId != -1) {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(_remoteTaskId,_numberOfResultValues,_outputValues,_maxEigenvalue);
  } else {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::moveTask(int rank, int tag, MPI_Comm communicator) {
  ::tarch::mpi::DoubleMessage  tMessage(_t);
  ::tarch::mpi::DoubleMessage  dtMessage(_dt);
  ::tarch::mpi::IntegerMessage taskIdMessage;

  if ( tag != smartmpi::communication::MoveTaskToMyServerForEvaluationTag &&
       tag != smartmpi::communication::MoveTaskToComputesComputeRankTag ) {
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
    #if DIMENSIONS==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* inputValues = tarch::allocateMemory<double>( NumberOfInputValues, tarch::MemoryLocation::Heap );

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
  _outputValues = tarch::allocateMemory<double>( _numberOfResultValues, tarch::MemoryLocation::Heap );

//  _functor(_inputValues,_outputValues,_marker,_t,_dt,_maxEigenvalue);
//  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );
  _functor();

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
  ::tarch::mpi::DoubleMessage  eValueMessage(_maxEigenvalue);
  ::tarch::mpi::IntegerMessage taskIdMessage(_remoteTaskId);

  ::peano4::datamanagement::CellMarker::send( _marker, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( dtMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::send( eValueMessage, rank, tag, communicator );
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
    #if DIMENSIONS==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}};
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::DoubleMessage eValueMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* outputValues = tarch::allocateMemory<double>( NumberOfResultValues, tarch::MemoryLocation::Heap );

  ::peano4::datamanagement::CellMarker::receive( markerMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( tMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( dtMessage, rank, tag, communicator );
  ::tarch::mpi::DoubleMessage::receive( eValueMessage, rank, tag, communicator );
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
    result->_maxEigenvalue = eValueMessage.getValue();
    return result;
  }
  logInfo(
    "receiveOutcome(...)",
    "bookmark outcome of task " << taskIdMessage.getValue()
  );
  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(taskIdMessage.getValue(),NumberOfResultValues,outputValues,eValueMessage.getValue());
  return nullptr;
}
#endif


{% if STATELESS_PDE_TERMS %}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::canFuse() const {
  return repositories::{{SOLVER_INSTANCE}}.cellCanUseStatelessPDETerms(
    _marker.getX(),
    _marker.getH(),
    _t,
    _dt
  );
}


/**
 * Also merge the current task
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse( const std::list<Task*>& otherTasks, int targetDevice ) {
  logDebug("fuse(...)", "rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << (otherTasks.size() + 1) << " tasks into one large GPU task on device " << targetDevice);

  ::exahype2::CellData cellData(otherTasks.size() + 1, tarch::MemoryLocation::Heap, targetDevice);
  int currentTask = 0;
  for (auto& p: otherTasks) {
    cellData.QIn[currentTask]         = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_inputValues;
    cellData.cellCentre[currentTask]  = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.getX();
    cellData.cellSize[currentTask]    = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.getH();
    cellData.t[currentTask]           = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_t;
    cellData.dt[currentTask]          = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_dt;
    cellData.id[currentTask]          = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_taskNumber;
    cellData.QOut[currentTask]        = tarch::allocateMemory<double>(_numberOfResultValues, tarch::MemoryLocation::Heap, targetDevice);
    delete p;
    currentTask++;
  }
  cellData.QIn[currentTask]         = _inputValues;
  cellData.cellCentre[currentTask]  = _marker.getX();
  cellData.cellSize[currentTask]    = _marker.getH();
  cellData.t[currentTask]           = _t;
  cellData.dt[currentTask]          = _dt;
  cellData.id[currentTask]          = _taskNumber;
  cellData.QOut[currentTask]        = tarch::allocateMemory<double>(_numberOfResultValues, tarch::MemoryLocation::Heap, targetDevice);

  //
  // ==============
  // Invoke kernels
  // ==============
  //
  bool foundOffloadingBranch = false;

  #if defined(WITH_GPU_SYCL)
  if (targetDevice>=0 or targetDevice==Host) {
    foundOffloadingBranch = true;
    ::exahype2::dg::{{KERNEL_NAMESPACE}}::sycl::{{FUSED_VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS_GPU}}
  }
  #endif

  if (not foundOffloadingBranch) {
    logDebug("fuse(...)", "rank " << tarch::mpi::Rank::getInstance().getRank()
      << " cannot find offloading branch for device " << targetDevice << ". Process fused tasks on the CPU.");
    ::exahype2::dg::{{KERNEL_NAMESPACE}}::{{FUSED_VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS_CPU}}
  }

  //
  // ==================
  // Bring back results
  // ==================
  //
  for (int i=0; i<cellData.numberOfCells; i++) {
    //tarch::freeMemory(cellData.QIn[i], tarch::MemoryLocation::Heap, targetDevice);
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(cellData.id[i], _numberOfResultValues, cellData.QOut[i], cellData.maxEigenvalue[i]);
  }

  // Don't rerun, so return false.
  return false;
}

{% endif %}
