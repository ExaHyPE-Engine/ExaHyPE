#include "tarch/logging/ITTLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"

#include "LogFilter.h"

#include <sstream>
#include <stdlib.h>
#include <chrono>

#include "../mpi/Rank.h"

#include "config.h"


tarch::logging::Log tarch::logging::ITTLogger::_log( "tarch::logging::ITTLogger" );


tarch::logging::ITTLogger  tarch::logging::ITTLogger::_singleton;


tarch::logging::ITTLogger::ITTLogger():
  _firstTraceWritten(false) {
}


tarch::logging::ITTLogger& tarch::logging::ITTLogger::getInstance() {
  return _singleton;
}


tarch::logging::ITTLogger::~ITTLogger() {
  close();
}


void tarch::logging::ITTLogger::suspendTrace() {
  #ifdef UseITT
  __itt_pause();
  #endif
}


void tarch::logging::ITTLogger::continueTrace() {
  #ifdef UseITT
  __itt_resume();
  #endif
}


void tarch::logging::ITTLogger::indent( bool, const std::string&, const std::string& ) {}


std::string tarch::logging::ITTLogger::getTimeStampHumanReadable( long int timestampNanoseconds ) const {
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


std::string tarch::logging::ITTLogger::constructMessageString(
  [[maybe_unused]] std::string messageType,
  [[maybe_unused]] long int timestampNanoseconds,
  [[maybe_unused]] int rank,
  [[maybe_unused]] int threadId,
  [[maybe_unused]] const std::string& trace,
  [[maybe_unused]] const std::string& message
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


void tarch::logging::ITTLogger::debug(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
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


void tarch::logging::ITTLogger::info(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetInfo,
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cout << outputMessage;
}


void tarch::logging::ITTLogger::warning(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    "warning",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();
}


void tarch::logging::ITTLogger::error(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
     "error",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();

  close();
  tarch::mpi::Rank::abort(-1);
}


void tarch::logging::ITTLogger::traceIn(
  [[maybe_unused]] long int timestampNanoseconds,
  [[maybe_unused]] int rank,
  [[maybe_unused]] int threadId,
  [[maybe_unused]] const std::string& trace,
  [[maybe_unused]] const std::string& message
) {
  #ifdef UseITT
  if (not _firstTraceWritten) {
    _firstTraceWritten = true;
    continueTrace();
  }

  tarch::multicore::Lock lock(_semaphore);

  if (_ittHandles.count(trace)==0) {
    __itt_event event = __itt_event_create(trace.c_str(), trace.size() );
    _ittHandles.insert( std::pair<std::string, __itt_event>(trace,event) );
  }

  __itt_event_start(_ittHandles[trace.c_str()]);
  #endif
}

void tarch::logging::ITTLogger::traceOut(
  [[maybe_unused]] long int timestampNanoseconds,
  [[maybe_unused]] int rank,
  [[maybe_unused]] int threadId,
  [[maybe_unused]] const std::string& trace,
  [[maybe_unused]] const std::string& message
) {
  #ifdef UseITT
  __itt_event_end(_ittHandles[trace.c_str()]);
  #endif
}

void tarch::logging::ITTLogger::close() {
  std::cout.flush();
  std::cerr.flush();
}
