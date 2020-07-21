#include "tarch/logging/ITACLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"

#include "LogFilter.h"

#include <sstream>
#include <stdlib.h>
#include <chrono>

#include "../mpi/Rank.h"

#include "../config.h"

#ifdef UseITAC
#include <VT.h>
#endif


tarch::logging::Log tarch::logging::ITACLogger::_log( "tarch::logging::ITACLogger" );


tarch::logging::ITACLogger  tarch::logging::ITACLogger::_singleton;


tarch::logging::ITACLogger::ITACLogger() {
}


tarch::logging::ITACLogger& tarch::logging::ITACLogger::getInstance() {
  return _singleton;
}


tarch::logging::ITACLogger::~ITACLogger() {
  close();
}


void tarch::logging::ITACLogger::indent( bool indent, const std::string& trace, const std::string& message ) {
}


std::string tarch::logging::ITACLogger::getTimeStampHumanReadable( long int timestampNanoseconds ) const {
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


std::string tarch::logging::ITACLogger::constructMessageString(
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


void tarch::logging::ITACLogger::debug(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
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


void tarch::logging::ITACLogger::info(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetInfo,
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cout << outputMessage;
}


void tarch::logging::ITACLogger::warning(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    "warning",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();
}


void tarch::logging::ITACLogger::error(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
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


void tarch::logging::ITACLogger::traceIn(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseITAC
  tarch::multicore::Lock lock(_semaphore);

  if (_itacHandles.count(trace)) {
	int newHandle;
    VT_funcdef( trace , VT_NOCLASS, &newHandle );
	_itaceHandles.insert( std::pair<std::string, int>(trace,newHandle) );
  }

  VT_begin(_itaceHandles[trace].c_str());
  #endif
}


void tarch::logging::ITACLogger::traceOut(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseITAC
  VT_end(_itaceHandles[trace].c_str());
  #endif
}


void tarch::logging::ITACLogger::close() {
  std::cout.flush();
  std::cerr.flush();
}
