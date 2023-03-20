#include "SelfSimilarInfallFD4EnclaveTask.h"

// user includes

#include "exahype2/CellData.h"

#include "exahype2/fd/fd4/FD4.h"


#include "config.h"


#include "exahype2/CellData.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "exahype2/enumerator/Enumerator.h"

#include "exahype2/fd/PatchUtils.h"

#include "peano4/parallel/parallel.h"

#include "peano4/utils/Loop.h"

#include "tarch/compiler/CompilerSpecificSettings.h"

#include "tarch/multicore/smartScheduling.h"
#include "tarch/multicore/otter.h"

#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#if defined(UseSmartMPI)
#include "communication/Tags.h"
#endif

#include <string.h>
#include <memory>

tarch::logging::Log                benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask" );
int                                benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::_enclaveTaskTypeId( peano4::parallel::getTaskType("benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask") );


int benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        targetPatch
) {
  ::exahype2::CellData  patchData( reconstructedPatch, marker.x(), marker.h(), t, dt, targetPatch );

  

  ::exahype2::fd::fd4::timeStep_patchwise_heap_functors(
  patchData,
  5,
  3,
  5,
  0,
  8.0,
   true ,
   false ,
   true ,
  false, // Runge-Kutta, so no need to copy old data over or to take dt into account
  ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection,
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  )->void {
    
    repositories::InstanceOfSelfSimilarInfallFD4.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    
  },
  [&](
    const double * __restrict__                  Q,
    const double * __restrict__     deltaQ,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ BTimesDeltaQ
  )->void {
    
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  volumeX,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    double * __restrict__ S
  )->void {
    
    repositories::InstanceOfSelfSimilarInfallFD4.sourceTerm( Q, volumeX, volumeH, t, dt, S );
    
  }
);
  

  ::exahype2::fd::validatePatch(
    targetPatch,
    5,
    0,
    5,
    0, // halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // outcome has to be valid

  tarch::freeMemory(reconstructedPatch,tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );

  
  return patchData.maxEigenvalue[0];
  
}


//_inputValues,_outputValues,_marker,_t,_dt,_maxEigenvalue

benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::SelfSimilarInfallFD4EnclaveTask(
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
    (5+2*3) * (5+2*3) * (5+0),
    5 * 5 * 5,
    #else
    (5+2*3) * (5+2*3) * (5+2*3) * (5+0),
    5 * 5 * 5 * 5,
    #endif
    [&]() -> void {
      _maxEigenvalue = applyKernelToCell(
        _marker,
        _t,
        _dt,
        _inputValues,
        _outputValues
      );
    }
  )
  #ifdef UseSmartMPI
  , smartmpi::Task(_enclaveTaskTypeId)
  #endif
{}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::isSmartMPITask() const {
  #ifdef UseSmartMPI
  return true;
  #else
  return false;
  #endif
}


#ifdef UseSmartMPI
void benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::runLocally() {
  computeTask();
  if (_remoteTaskId != -1) {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(_remoteTaskId,_numberOfResultValues,_outputValues,_maxEigenvalue);
  } else {
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::moveTask(int rank, int tag, MPI_Comm communicator) {
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


smartmpi::Task* benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::receiveTask(int rank, int tag, MPI_Comm communicator) {
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfInputValues =
    #if Dimensions==2
    {{ NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D }};
    #else
    {{ NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D }};
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

  SelfSimilarInfallFD4EnclaveTask* result = new SelfSimilarInfallFD4EnclaveTask(
    markerMessage,
    tMessage.getValue(),
    dtMessage.getValue(),
    inputValues
  );
  result->_remoteTaskId = taskIdMessage.getValue();
  return result;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::runLocallyAndSendTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

//  _functor(_inputValues,_outputValues,_marker,_t,_dt,_maxEigenvalue);
//  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );
  _functor();

  logInfo(
    "runLocallyAndSendTaskOutputToRank(...)",
    "executed remote task on this rank. Will start to send result back"
  );

  forwardTaskOutputToRank(rank, tag, communicator);
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::forwardTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
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


smartmpi::Task* benchmarks::exahype2::euler::sphericalaccretionupscaling::tasks::SelfSimilarInfallFD4EnclaveTask::receiveOutcome(int rank, int tag, MPI_Comm communicator, const bool intentionToForward) {
  logInfo( "receiveOutcome(...)", "rank=" << rank << ", tag=" << tag );
  peano4::grid::GridTraversalEvent dummyEvent;
  const int NumberOfResultValues =
    #if Dimensions==2
    {{ NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D }};
    #else
    {{ NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D }};
    #endif

  ::tarch::mpi::DoubleMessage tMessage;
  ::tarch::mpi::DoubleMessage dtMessage;
  ::tarch::mpi::DoubleMessage eValueMessage;
  ::tarch::mpi::IntegerMessage taskIdMessage;
  ::peano4::datamanagement::CellMarker markerMessage(dummyEvent);
  double* outputValues = tarch::allocateMemory( NumberOfResultValues, tarch::MemoryLocation::Heap );

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

    SelfSimilarInfallFD4EnclaveTask* result = new SelfSimilarInfallFD4EnclaveTask(
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


