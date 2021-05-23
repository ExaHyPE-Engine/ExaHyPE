#include "EnclaveTask.h"
#include "EnclaveBookkeeping.h"

#include "tarch/multicore/Core.h"

#include "exahype2/fv/Generic.h"


tarch::logging::Log  exahype2::EnclaveTask::_log( "exahype2::EnclaveTask" );


exahype2::EnclaveTask::EnclaveTask(
  int                                            enclaveTaskTypeId,
  const ::peano4::datamanagement::CellMarker&    marker,
  double                                         t,
  double                                         dt,
  double*                                        inputValues,
  int                                            numberOfResultValues,
  Functor                                        functor
):
    // @todo Also Typ gibt es wohl dann doch schon
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),enclaveTaskTypeId,tarch::multicore::Task::DefaultPriority),
  _marker(marker),
  _t(t),
  _dt(dt),
  _inputValues(inputValues),
  _outputValues(nullptr),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor) {
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );
}


bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument( "run()", getTaskId() );
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker,_t,_dt);
  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues);
  logTraceOut( "run()" );
  return false;
}


#ifdef UseSmartMPI
bool exahype2::EnclaveTask::canMigrate() const {
  return true;
}

void exahype2::EnclaveTask::runLocally() {
  run();
}


void exahype2::EnclaveTask::sendTaskInputToRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("have to implement this", false);
//  int numberOfInputValues =
/*
  _numberOfResultValues(numberOfResultValues),
  MPI_Send( _inputValues )


  int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
  _inputValues,_outputValues,_marker,_t,_dt
*/
}


void exahype2::EnclaveTask::receiveTaskInputFromRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("have to implement this", false);
}


void exahype2::EnclaveTask::runLocallyAndSendTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker,_t,_dt);
  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

  assertionMsg("have to implement this", false);
}


void exahype2::EnclaveTask::receiveTaskOutputFromRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("some lines here are not implemented", false);

  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues);
}
#endif
