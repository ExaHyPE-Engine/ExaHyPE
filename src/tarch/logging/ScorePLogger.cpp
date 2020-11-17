#include "tarch/logging/ScorePLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"

#include "LogFilter.h"

#include <sstream>
#include <stdlib.h>
#include <chrono>

#include "../mpi/Rank.h"

#include "../config.h"

#ifdef UseScoreP
#include <scorep/SCOREP_User.h>
#endif


tarch::logging::Log tarch::logging::ScorePLogger::_log( "tarch::logging::ScorePLogger" );


tarch::logging::ScorePLogger  tarch::logging::ScorePLogger::_singleton;


tarch::logging::ScorePLogger::ScorePLogger() {
}


tarch::logging::ScorePLogger& tarch::logging::ScorePLogger::getInstance() {
  return _singleton;
}


tarch::logging::ScorePLogger::~ScorePLogger() {
  close();
}


void tarch::logging::ScorePLogger::indent( bool indent, const std::string& trace, const std::string& message ) {
}


std::string tarch::logging::ScorePLogger::getTimeStampHumanReadable( long int timestampNanoseconds ) const {
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


std::string tarch::logging::ScorePLogger::constructMessageString(
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


void tarch::logging::ScorePLogger::debug(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
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


void tarch::logging::ScorePLogger::info(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    LogFilter::FilterListEntry::TargetInfo,
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cout << outputMessage;
}


void tarch::logging::ScorePLogger::warning(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string outputMessage = constructMessageString(
    "warning",
    timestampNanoseconds, rank, threadId, trace, message
  );

  tarch::multicore::Lock lockCout( _semaphore );
  std::cerr << outputMessage;
  std::cerr.flush();
}


void tarch::logging::ScorePLogger::error(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
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


void tarch::logging::ScorePLogger::traceIn(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseITAC
  tarch::multicore::Lock lock(_semaphore);

  if (_scorePHandles.count(trace)==0) {
    _scorePHandles.insert( std::pair<std::string, int>(trace,SCOREP_USER_INVALID_REGION) );
  }

  SCOREP_USER_REGION_ENTER(_scorePHandles[trace]);
  #endif
}


void tarch::logging::ScorePLogger::traceOut(long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {
  #ifdef UseITAC
  SCOREP_USER_REGION_LEAVE(_scorePHandles[trace]);
  #endif
}


void tarch::logging::ScorePLogger::close() {
  std::cout.flush();
  std::cerr.flush();
}
