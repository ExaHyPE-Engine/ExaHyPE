// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LOGGING_CHROME_TRACE_FILE_LOGGER_H_
#define _TARCH_LOGGING_CHROME_TRACE_FILE_LOGGER_H_


#ifdef Parallel
#include <mpi.h>
#endif
#include <iostream>
#include <fstream>
#include <set>
#include <stack>


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Log.h"




namespace tarch {
  namespace logging {
    class ChromeTraceFileLogger;
  }
}

/**
 * Chrome Trace File Logger
 *
 * To visualise the output files, either use Google's Chrome or the trace file
 * viewer from The Chromium Projects. In Ubuntu, simply install the Chromium
 * web browser. Once launched, type in chrome://tracing/ and press enter. After
 * that, you can load the trace file.
 *
 * @author  Tobias Weinzierl
 */
class tarch::logging::ChromeTraceFileLogger {
  private:
    static Log _log;

    static ChromeTraceFileLogger   _singleton;

    tarch::multicore::BooleanSemaphore _semaphore;

    /**
     * Test for the column separator of a string output.
     */
    std::ostream*  _outputStream;
    bool           _quitOnError;
    bool           _hasWrittenEntry;


    void nextEntry();

    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    ChromeTraceFileLogger& operator=(const ChromeTraceFileLogger& rhs);

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    ChromeTraceFileLogger(const ChromeTraceFileLogger& param);


    /**
     * Ensures each column has same length
     */
    std::string addSeparators(std::string  message) const;

    /**
     * Construct message string for output to terminal. I pipe data to the
     * terminal for error messages, warnings and infos.
     */
    std::string constructMessageString(
      std::string          messageType,
	   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message
    );


    std::string constructEventEntryInTraceFile(
      std::string          messageType,
	   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message
    );

    /**
     * Configures the output streams
     */
    void configureOutputStreams();

    /**
     * It's a singleton.
     */
    ChromeTraceFileLogger();
  public:
    ~ChromeTraceFileLogger();

    static ChromeTraceFileLogger& getInstance();

    void debug(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);
    void info(    long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);
    void warning( long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);
    void error(   long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);
    void traceIn( long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);
    void traceOut(long int timestampMS, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Tells the logger to increment/decrement the indent.
     *
     * Implemented for compatibility reasons
     */
    void indent( bool indent, const std::string& trace, const std::string& message );

    /**
     * Is redundant, as you could use setLogFormat() instead. However, it is
     * tedious to specify all of these parameters if the only thing you want
     * is to establish an output file.
     */
    void setOutputFile( const std::string&  outputLogFileName );

    void setQuitOnError(bool value);

    void close();
};

#endif
