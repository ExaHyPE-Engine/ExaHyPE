#include "smartmpi.h"

#include <iostream>

namespace {
  std::function<void(int typeNumber)>                receiver;
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
}


void smartmpi::registerReceiver( std::function<void(int typeNumber)> value ) {
  receiver = value;
}


void smartmpi::init(MPI_Comm communicator) {

}

