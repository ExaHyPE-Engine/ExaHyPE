#include "tarch/logging/NVTXLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"

#include "LogFilter.h"

#include <sstream>
#include <stdlib.h>
#include <chrono>

#include "../mpi/Rank.h"

#include "../config.h"

#ifdef UseNVIDIA
#include <nvToolsExt.h>
#endif


tarch::logging::Log tarch::logging::NVTXLogger::_log( "tarch::logging::NVTXLogger" );


tarch::logging::NVTXLogger  tarch::logging::NVTXLogger::_singleton;


tarch::logging::NVTXLogger::NVTXLogger() {
}


tarch::logging::NVTXLogger& tarch::logging::NVTXLogger::getInstance() {
  return _singleton;
}


tarch::logging::NVTXLogger::~NVTXLogger() {
  close();
}


void tarch::logging::NVTXLogger::indent( bool indent, const std::string& trace, const std::string& message ) {
}


std::string tarch::logging::NVTXLogger::getTimeStampHumanReadable( long int timestampNanoseconds ) const {
  long int timestampSeconds = timestampNanoseconds / 1000 / 1000 / 1000;
  const int HourScaling = 60 * 60;
  long int hours = timestampSeconds / HourScaling;
  timestampSeconds = timestampSeconds - HourScaling * hours;

  const int MinutesScaling = 60;
  long int minutes = timestampSeconds / MinutesScaling;
  timestampSeconds = timestampSeconds - MinutesScaling * minutes;

  const int SecondsScaling = 1;
  long int seconds = timestampSeconds / SecondsScaling;

  std::stringstream result;
  if (hours<10) {
    result << "0";
  }
  result << hours << ":";
  if (minutes<10) {
    result << "0";
  }
  result << minutes << ":";
  if (seconds<10) {
    result << "0";
  }
  result << seconds;
  return result.str();
}


std::string tarch::logging::NVTXLogger::constructMessageString(
  std::string          messageType,
  long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message
) {
  std::ostringstream result;
  result << getTimeStampHumanReadable(timestampNanoseconds)
         << "\trank:" << rank
         << "\t" << trace
         << "\t" << messageType
         << "\t" << message
         << "\n";
  return result.str();
}


void tarch::logging::NVTXLogger::debug(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #if !defined(PeanoDebug) || PeanoDebug<1
  assertion(false);
  #endif

  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetDebug,
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cout << outputMessage;
}


void tarch::logging::NVTXLogger::info(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetInfo,
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cout << outputMessage;
}


void tarch::logging::NVTXLogger::warning(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    "warning",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();
}


void tarch::logging::NVTXLogger::error(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
     "error",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();

  close();
  exit(-1);
}


void tarch::logging::NVTXLogger::traceIn(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseNVIDIA
  nvtxRangePush(trace.c_str());
  #endif
}


void tarch::logging::NVTXLogger::traceOut(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseNVIDIA
  nvtxRangePop();
  #endif
}


void tarch::logging::NVTXLogger::close() {
  std::cout.flush();
  std::cerr.flush();
}
