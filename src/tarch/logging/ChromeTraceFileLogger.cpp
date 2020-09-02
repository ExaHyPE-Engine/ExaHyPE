#include "tarch/logging/ChromeTraceFileLogger.h"

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"

#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <algorithm>

#include "LogFilter.h"


#include "../mpi/Rank.h"



tarch::logging::Log tarch::logging::ChromeTraceFileLogger::_log( "tarch::logging::ChromeTraceFileLogger" );


tarch::logging::ChromeTraceFileLogger   tarch::logging::ChromeTraceFileLogger::_singleton;


tarch::logging::ChromeTraceFileLogger::ChromeTraceFileLogger():
  _outputStream(nullptr),
  _quitOnError(false),
  _hasWrittenEntry(false) {
  configureOutputStreams();
}


tarch::logging::ChromeTraceFileLogger& tarch::logging::ChromeTraceFileLogger::getInstance() {
  return _singleton;
}


void tarch::logging::ChromeTraceFileLogger::nextEntry() {
  if ( _hasWrittenEntry and _outputStream!=nullptr) {
	*_outputStream << ",\n";
  }
  _hasWrittenEntry = true;
}


void tarch::logging::ChromeTraceFileLogger::configureOutputStreams() {
  if (_outputStream!=nullptr) {
    _outputStream->setf( std::ios_base::scientific, std::ios_base::floatfield );
    _outputStream->precision(20);
  }
  std::cerr.setf( std::ios_base::scientific, std::ios_base::floatfield );
  std::cerr.precision(20);
  std::cout.setf( std::ios_base::scientific, std::ios_base::floatfield );
  std::cout.precision(20);
}


tarch::logging::ChromeTraceFileLogger& tarch::logging::ChromeTraceFileLogger::operator=(const ChromeTraceFileLogger& rhs) {
  return *this;
}


tarch::logging::ChromeTraceFileLogger::ChromeTraceFileLogger(const ChromeTraceFileLogger& param) {
}


tarch::logging::ChromeTraceFileLogger::~ChromeTraceFileLogger() {
  close();
}




std::string tarch::logging::ChromeTraceFileLogger::addSeparators(std::string message) const {
  const int ColumnWidth = 8;
  if ( message.size() > 0 ) {
    while (message.size() < ColumnWidth) {
      message += " ";
    }
  }
  message += " ";

  return message;
}


std::string tarch::logging::ChromeTraceFileLogger::constructMessageString(
  std::string          messageType,
  long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message
) {
  std::string result;

  result += addSeparators(std::to_string(timestampMS));
  result += addSeparators("rank:" + std::to_string(rank) );
  result += addSeparators("core:" + std::to_string(threadId) );
  result += addSeparators(messageType);
  result += message;
  result += "\n";

  return result;
}




std::string tarch::logging::ChromeTraceFileLogger::constructEventEntryInTraceFile(
  std::string          messageType,
  long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message
) {
  std::ostringstream result;
  std::string args = message;
  std::replace( args.begin(), args.end(), '"', ' '); // replace all 'x' to 'y'
  result << "  {"
		 << " \"name\": \"" << trace << "\","
		 << " \"cat\": \"" << messageType << "\","
		 << " \"ph\": \"i\","
		 << " \"ts\": " << timestampMS << ","
		 << " \"pid\": " << rank << ","
		 << " \"tid\": " << threadId << ","
		 << " \"args\": { \"message\": \"" + args + "\"}"
		 " }";
  return result.str();
}


void tarch::logging::ChromeTraceFileLogger::debug(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
  std::string eventEntry = constructEventEntryInTraceFile(
      "debug",
      timestampMS, rank, threadId, trace, message
    );
    tarch::multicore::Lock lockCout( _semaphore );
    if (_outputStream!=nullptr) {
      nextEntry();
      *_outputStream << eventEntry;
      #if PeanoDebug>=4
      _outputStream->flush();
      #endif
    }
}


void tarch::logging::ChromeTraceFileLogger::info(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
    std::string outputMessage = constructMessageString(
      "info",
	  timestampMS, rank, threadId, trace, message
    );
    std::string eventEntry = constructEventEntryInTraceFile(
      "info",
	  timestampMS, rank, threadId, trace, message
    );
    tarch::multicore::Lock lockCout( _semaphore );
    std::cout << outputMessage;
    if (_outputStream!=nullptr) {
      nextEntry();
      *_outputStream << eventEntry;
      #if PeanoDebug>=4
      _outputStream->flush();
      #endif
    }
}


void tarch::logging::ChromeTraceFileLogger::warning(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
    std::string outputMessage = constructMessageString(
      "warning",
	  timestampMS, rank, threadId, trace, message
    );
    std::string eventEntry = constructEventEntryInTraceFile(
      "warning",
	  timestampMS, rank, threadId, trace, message
    );
    tarch::multicore::Lock lockCout( _semaphore );
    std::cout << outputMessage;
    if (_outputStream!=nullptr) {
    	nextEntry();
    	*_outputStream << eventEntry;
      #if PeanoDebug>=4
      _outputStream->flush();
      #endif
    }
}


void tarch::logging::ChromeTraceFileLogger::traceIn(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
	if (_outputStream!=nullptr) {
      std::string args = message;
	  std::replace( args.begin(), args.end(), '"', ' '); // replace all 'x' to 'y'
      tarch::multicore::Lock lockCout( _semaphore );
      nextEntry();
      *_outputStream << "  {"
			 << " \"name\": \"" << trace << "\","
			 << " \"cat\": \"trace\","
			 << " \"ph\": \"B\","
			 << " \"ts\": " << timestampMS << ","
			 << " \"pid\": " << rank << ","
			 << " \"tid\": " << threadId << ","
			 << " \"args\": { \"params\": \"" + args + "\"}"
			 " }";
    #if PeanoDebug>=4
    _outputStream->flush();
    #endif
  }
}


void tarch::logging::ChromeTraceFileLogger::traceOut(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
  if (_outputStream!=nullptr) {
    std::string args = message;
	std::replace( args.begin(), args.end(), '"', ' '); // replace all 'x' to 'y'
    tarch::multicore::Lock lockCout( _semaphore );
    nextEntry();
	*_outputStream << "  {"
			 << " \"name\": \"" << trace << "\","
			 << " \"cat\": \"trace\","
			 << " \"ph\": \"E\","
			 << " \"ts\": " << timestampMS << ","
			 << " \"pid\": " << rank << ","
			 << " \"tid\": " << threadId << ","
			 << " \"args\": { \"result\": \"" + args + "\"}"
			 " }";
    #if PeanoDebug>=4
    _outputStream->flush();
    #endif
  }
}


void tarch::logging::ChromeTraceFileLogger::error(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message) {
    std::string outputMessage = constructMessageString(
      "error",
	  timestampMS, rank, threadId, trace, message
    );
    std::string eventEntry = constructEventEntryInTraceFile(
      "error",
	  timestampMS, rank, threadId, trace, message
    );
    tarch::multicore::Lock lockCout( _semaphore );
    std::cerr << outputMessage;
    if (_outputStream!=nullptr) {
      nextEntry();
      *_outputStream << eventEntry;
    }

  if (_quitOnError) {
    tarch::mpi::Rank::abort(-1);
  }
}




void tarch::logging::ChromeTraceFileLogger::indent( bool indent, const std::string& trace, const std::string& message ) {
}


void tarch::logging::ChromeTraceFileLogger::setOutputFile( const std::string&  outputLogFileName ) {
  std::ostringstream myOutputFileName;
  #ifdef Parallel
  if (!outputLogFileName.empty()) {
    myOutputFileName << "rank-" << tarch::mpi::Rank::getInstance().getRank() << "-" << outputLogFileName;
  }
  #else
  myOutputFileName << outputLogFileName;
  #endif

  if (_outputStream!=nullptr) {
    _outputStream->flush();
    delete _outputStream;
    _outputStream            = nullptr;
  }

  _outputStream = new std::ofstream( myOutputFileName.str().c_str() );

  configureOutputStreams();

  *_outputStream << "[\n";
}


void tarch::logging::ChromeTraceFileLogger::setQuitOnError(bool value) {
  _quitOnError = value;
}


void tarch::logging::ChromeTraceFileLogger::close() {
  std::cout.flush();
  std::cerr.flush();
  if (_outputStream!=nullptr) {
	*_outputStream << "]";
    delete _outputStream;
    _outputStream = nullptr;
  }
  else {
	std::cerr << "Warning: ChromeTraceFileLogger used, but no trace file set. Use singleton's setOuputFile(). No trace information dumped" << std::endl;
  }
}
