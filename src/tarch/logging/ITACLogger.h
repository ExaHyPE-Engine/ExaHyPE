// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef Parallel
#include <mpi.h>
#endif

#include <iostream>
#include <fstream>
#include <set>
#include <stack>
#include <map>
#include <string>

#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Log.h"

namespace tarch {
  namespace logging {
    class ITACLogger;
  }
}

/**
 * Command Line Logger
 *
 * Standard log output device. Implements the LogHandle. Usually there's only
 * one instance of this class, as the Log type (which is implicitly used by all
 * log and trace macros) forwards all data to a singleton.
 *
 * ## Error and warnings
 *
 * All error and warning messages are dumped to \texttt{cerr}. The logger
 * automatically aborts the application if it encounters an error, once the
 * error message is written and \texttt{cerr} is flushed.
 *
 * ## Info and debugging
 *
 * Debug information is piped to \texttt{cout}. The same happens with writes to
 * the \texttt{info} log level.
 *
 * ## Thread safety
 *
 * All logs are protected by a semaphore to avoid race conditions.
 *
 *
 * @author  Tobias Weinzierl, Wolfgang Eckhardt
 */
class tarch::logging::ITACLogger {
  private:
    static Log _log;

    static ITACLogger  _singleton;

    tarch::multicore::MultiReadSingleWriteSemaphore _semaphore;

    std::map< std::string, int > _itacHandles;

    bool _firstTraceWritten;

    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    ITACLogger& operator=(const CommandLineLogger&) = delete;

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    ITACLogger(const ITACLogger&) = delete;

    /**
     * Construct message string
     *
     * !!! Thread Safety
     *
     * The message string relies on the global field _indent. This one might
     * change throughout the execution of this method. However, I accept such a
     * behavior: Changing _indent throughout the message execution makes the
     * method add the wrong number of whitespaces in front of the message. That
     * is a 'bug' we can accept.
     *
     * To satisfy Intel Inspector et al at least slightly, I copy over _indent
     * before I actually construct the message string. So the indent can't change
     * while we add the spaces/tabs to the output.
     */
    std::string constructMessageString(
      std::string          messageType,
      long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message
    );

    /**
     * It's a singleton.
     *
     * According to Intel's documentation, ITAC is automatically initialised by
     * MPI_Init(). However, if we have no MPI, then we have to initialise
     * manually.
     */
    ITACLogger();

    std::string getTimeStampHumanReadable( long int timestampNanoseconds ) const;

  public:
    ~ITACLogger();

    static ITACLogger& getInstance();

    /**
     * Is public as some analysis frameworks check explicitly whether these
     * features are switched on.
     */
    bool        getLogMachineName() const;

    /**
     * Is public as some analysis frameworks check explicitly whether these
     * features are switched on.
     */
    bool        getLogThreadName() const;

    /**
     * Is public as some analysis frameworks check explicitly whether these
     * features are switched on.
     */
    bool        getLogTrace() const;

    /**
     * Is public as some analysis frameworks check explicitly whether these
     * features are switched on.
     */
    bool        getLogTimeStamp() const;

    void debug(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);
    void info(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Write Warning
     *
     * In the implementation, I call a flush on cout before I write to cerr.
     * Otherwise, the cerr messages might overtake cout. Before the operation
     * returns, it does a flush on cerr, too. Otherwise, the message might not
     * occur, i.e. the application might shut down before the message is flushed
     * to the terminal.
     */
    void warning(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Write Error
     *
     * In the implementation, I call a flush on cout before I write to cerr.
     * Otherwise, the cerr messages might overtake cout. Before the operation
     * returns, it does a flush on cerr, too. Otherwise, the message might not
     * occur, i.e. the application might shut down before the message is flushed
     * to the terminal.
     */
    void error(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    void traceIn(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);
    void traceOut(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Tells the logger to increment/decrement the indent.
     *
     * For ITAC, we do not need any indentation. Consequently, we put the
     * nop implementation into the header, so the compiler can inline the
     * call and eventually remove it completely.
     */
    void indent( bool indent, const std::string& trace ) {};

    void close();

    void suspendTrace();
    void continueTrace();
};
