#include "SmartEnclaveTask.h"
#include "EnclaveTask.h"


void exahype2::initSmartMPIForEnclaveTasks() {
  #ifdef UseSmartMPI
  smartmpi::init( tarch::mpi::Rank::getInstance().getCommunicator() );
  #endif
}


#if defined(UseSmartMPI)

#include "SmartEnclaveTask.h"
#include "EnclaveBookkeeping.h"

#include "tarch/multicore/Core.h"
#include "peano4/parallel/Tasks.h"

tarch::logging::Log  exahype2::SmartEnclaveTask::_log( "exahype2::SmartEnclaveTask" );




exahype2::SmartEnclaveTask::SmartEnclaveTask(
  const ::peano4::datamanagement::CellMarker&    marker,
  double*                                        inputValues,
  int                                            numberOfResultValues,
  std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >  functor,
  int solverNumber
):
  smartmpi::SmartMPITask(solverNumber),
  _wrappedTask(nullptr) {
  logTraceIn( "SmartEnclaveTask(...)" );

  _wrappedTask = new EnclaveTask( marker, inputValues, numberOfResultValues, functor );

  logTraceOut( "SmartEnclaveTask(...)" );
}


int exahype2::SmartEnclaveTask::getTaskId() const {
  assertion( _wrappedTask!=nullptr );
  return _wrappedTask->getTaskId();
}


void exahype2::SmartEnclaveTask::runLocally() {
  logTraceIn( "runLocally()" );

  peano4::parallel::Tasks spawn(
    _wrappedTask,
    peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
    peano4::parallel::Tasks::getLocationIdentifier( "exahype2::SmartEnclaveTask" )
  );
  _wrappedTask = nullptr;

  logTraceOut( "runLocally()" );
}


void exahype2::SmartEnclaveTask::sendTaskToRank(int rank, int tag, MPI_Comm communicator) {

}


void exahype2::SmartEnclaveTask::runLocallyAndSendResultToRank(int rank, int tag, MPI_Comm communicator) {

}


#endif
