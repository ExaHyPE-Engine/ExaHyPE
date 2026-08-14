#include "tarch/logging/NoLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"

#include "LogFilter.h"

#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <chrono>

#include "../mpi/Rank.h"



tarch::logging::Log tarch::logging::NoLogger::_log( "tarch::logging::NoLogger" );

tarch::logging::NoLogger  tarch::logging::NoLogger::_singleton;


tarch::logging::NoLogger::NoLogger():
  _quitOnError(false) {
}


tarch::logging::NoLogger& tarch::logging::NoLogger::getInstance() {
  return _singleton;
}


std::string tarch::logging::NoLogger::constructMessageString(
  std::string          messageType,
  long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message
) {
  std::ostringstream result;
  result << timestampNanoseconds << "\t"
         << ", rank:" << rank << "\t"
         << ", core:" << threadId << "\t"
         << messageType << "\t"
         << trace << ": "
         << message;

  return result.str();
}

void tarch::logging::NoLogger::info(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  tarch::multicore::Lock lockCout( _semaphore );

  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetInfo,
    timestampNanoseconds, rank, threadId, trace, message
  );

  std::cout << outputMessage << std::endl;
}


void tarch::logging::NoLogger::warning(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  tarch::multicore::Lock lockCout( _semaphore );

  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetWarning,
    timestampNanoseconds, rank, threadId, trace, message
  );

  std::cerr << outputMessage << std::endl;
  std::cerr << outputMessage;
  std::cerr.flush();
}


void tarch::logging::NoLogger::error(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  tarch::multicore::Lock lockCout( _semaphore );

  std::string outputMessage = constructMessageString(
    "error",
    timestampNanoseconds, rank, threadId, trace, message
  );

  std::cerr << outputMessage << std::endl;
  std::cerr << outputMessage;
  std::cerr.flush();

  if (_quitOnError) {
    tarch::mpi::Rank::abort(-1);
  }
}
