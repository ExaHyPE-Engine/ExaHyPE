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

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Log.h"

#ifdef UseITT
#include <ittnotify.h>
#endif

namespace tarch {
  namespace logging {
    class ITTLogger;
  }
}

/**
 * Command Line Logger
 *
 *
 *Details on this can be found on <a href="https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2023-0/collection-control-api.html">Intel's collection control</a> pages.
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
class tarch::logging::ITTLogger {
  private:
    static Log _log;

    static ITTLogger _singleton;

    tarch::multicore::BooleanSemaphore _semaphore;

    #ifdef UseITT
    std::map< std::string, __itt_event > _ittHandles;
    #endif

    bool _firstTraceWritten;

    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    ITTLogger& operator=(const CommandLineLogger&) = delete;

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    ITTLogger(const ITTLogger&) = delete;

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
    ITTLogger();

    std::string getTimeStampHumanReadable( long int timestampNanoseconds ) const;

  public:
    ~ITTLogger();

    static ITTLogger& getInstance();

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

    /**
     * https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2023-0/event-api.html
     */
    void traceIn(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);
    void traceOut(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Tells the logger to increment/decrement the indent.
     *
     * !!! Thread Safety
     *
     * _indent is a global static field shared by all threads. If we increment
     * or decrement it, this is first of all a read followed by a write.
     * Consequently data races could occur and the counter could become smaller
     * than zero. This ain't possible in the sequential code as each increment
     * is accompanied by a decrement. The following table illustrates the race:
     *
     * || value of _indent  || Thread 1 || Thread 2
     * |  2                 |  initial condition   |  initial condition
     * |                    |  enter indent(false) |  enter indent(true)
     * |                    |  fetch indent into register |  fetch indent into register
     * |                    |  register value -= 2 |  register value += 2
     * |  4                 |  is a little bit slower |  write back new value of indent
     * |  0                 |  write back new value of indent |
     *
     * To avoid this data race, I introduced a semaphore. This one could also
     * be implemented with TBB's atomic construct, e.g., but I prefer the
     * semaphor / critical section technique.
     *
     * @param trace    Needed in debug mode to be able to find out who called indent(false) without an indent(true)
     * @param message  Needed in debug mode to be able to find out who called indent(false) without an indent(true)
     */
    void indent( bool indent, const std::string& trace, const std::string& message );

    void close();

    void suspendTrace();
    void continueTrace();
};
