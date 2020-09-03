#include "tarch/logging/Log.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"

#include "config.h"


#include "tarch/logging/LogFilter.h"
#include "tarch/logging/CommandLineLogger.h"


#if UseLogService==ChromeTraceFileLogger
#include "ChromeTraceFileLogger.h"
#endif

#if UseLogService==NVTXLogger
#include "NVTXLogger.h"
#endif

#if UseLogService==ITACLogger
#include "ITACLogger.h"
#endif



/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef CompilerHasUTSName
#include <sys/utsname.h>
#endif

#include "tarch/mpi/Rank.h"

#include <time.h>


#include "tarch/compiler/CompilerSpecificSettings.h"


tarch::logging::Log::Log(const std::string& className):
  _className( className ) {
  _startupTime = std::chrono::system_clock::now();
}


tarch::logging::Log::~Log() {
}


#if PeanoDebug>=4
void tarch::logging::Log::debug(const std::string& methodName, const std::string& message) {
  if (LogFilter::getInstance().writeDebug( _className )) {
    UseLogService::getInstance().debug(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
  }
}
#endif


void tarch::logging::Log::info(const std::string& methodName, const std::string& message) {
  if (LogFilter::getInstance().writeInfo( _className )) {
    UseLogService::getInstance().info(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
  }
}


void tarch::logging::Log::warning(const std::string& methodName, const std::string& message) {
  UseLogService::getInstance().warning(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
}


void tarch::logging::Log::error(const std::string& methodName, const std::string& message) {
  UseLogService::getInstance().error(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
}


#if PeanoDebug>=1
void tarch::logging::Log::traceIn(const std::string& methodName, const std::string& message) {
  if (LogFilter::getInstance().writeTrace( _className )) {
    UseLogService::getInstance().traceIn(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
  }
}


void tarch::logging::Log::traceOut(const std::string& methodName, const std::string& message) {
  if (LogFilter::getInstance().writeTrace( _className )) {
    UseLogService::getInstance().traceOut(getTimeStamp(),tarch::mpi::Rank::getInstance().getRank(),tarch::multicore::Core::getInstance().getCoreNumber(),getTraceInformation(methodName),message);
  }
}
#else
void tarch::logging::Log::traceIn(const std::string& methodName, const std::string& message) {
}


void tarch::logging::Log::traceOut(const std::string& methodName, const std::string& message) {
}
#endif


void tarch::logging::Log::indent( bool indent, const std::string& trace, const std::string& message ) const {
  UseLogService::getInstance().indent( indent, trace, message );
}


std::string tarch::logging::Log::getMachineInformation() {
  std::ostringstream message;

  #ifdef CompilerHasUTSName
  utsname* utsdata = new utsname();
  assertion( utsdata!=NULL );
  uname(utsdata);

  message << "[" << utsdata->nodename << "]";

  #ifdef Parallel
  message << ",";
  #endif

  delete utsdata;
  #endif

  #ifdef Parallel
  if (tarch::mpi::Rank::getInstance().isInitialised()) {
    message << "rank:" << tarch::mpi::Rank::getInstance().getRank();
  }
  else {
    message << "rank:not-initialised-yet";
  }
  #else
  message << "rank:0";
  #endif

  return message.str();
}


void tarch::logging::Log::flushBeforeAssertion() {
  UseLogService::getInstance().close();
}


long int tarch::logging::Log::getTimeStamp() const {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  long int result = std::chrono::duration_cast<std::chrono::nanoseconds>(now - _startupTime).count();
  assertion1(result>=0,result);
  return result;
}


std::string tarch::logging::Log::getTraceInformation( const std::string& methodName ) const {
  return _className + "::" + methodName;
}

