#include "smartmpi.h"
#include "GlobalStatistics.h"

#include <iostream>

namespace {
  std::function<void(int typeNumber)>                receiver;

  MPI_Comm                                           smartMPICommunicator;

  smartmpi::GlobalStatistics                         globalStatistics;
}


smartmpi::SmartMPITask::SmartMPITask( int typeNumber ):
  TypeNumber(typeNumber) {
}


std::string smartmpi::SmartMPITask::toString() const {
  return "(SmartMPI-task:" + std::to_string(TypeNumber) + ")";
}


void smartmpi::spawn(SmartMPITask* task) {
  #if SmartMPIDebug>=4
  std::cout << SmartMPIPrefix << "spawn task " << task->toString() << " locally" << std::endl;
  #endif
  task->runLocally();
  delete task;
}


void smartmpi::registerReceiver( std::function<void(int typeNumber)> value ) {
  receiver = value;
}


void smartmpi::init(MPI_Comm communicator, bool createSubcommunicator) {
  if (createSubcommunicator) {
    int world_rank, world_size;
    MPI_Comm_rank(communicator, &world_rank);
    MPI_Comm_size(communicator, &world_size);

    MPI_Comm_dup(communicator, & smartMPICommunicator);
  }
  else {
    smartMPICommunicator = communicator;
  }


  globalStatistics.setCommunicator( smartMPICommunicator );
}


void smartmpi::shutdown() {

}


void smartmpi::reportMPIWaitTime(double time) {
  globalStatistics.reportMPIWaitTime(time);
}


void smartmpi::tick() {
  globalStatistics.gatherWaitTimes();
}
