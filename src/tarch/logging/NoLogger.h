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

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Log.h"

namespace tarch {
  namespace logging {
    class NoLogger;
  }
}

/**
 * Command Line Logger
 *
 * Standard log output device. Implements the LogHandle. Usually there's only
 * one instance of this class for only the Log type aggregates it in a static
 * way. For a detailed description of this compiler flags please browse through
 * the static consts of the class or the howto page.
 *
 * An interesting issue is the behaviour of the command line logger in the
 * parallel mode if debug is switched on. In this case the number of messages
 * soon becomes unreadable for any human. Thus, the command line logger
 * internally creates one output file rank.log for each rank and writes all the
 * information into this file. info, warnings and errors are written (in short form)
 * to std::cout / std::cerr, too.
 *
 * If the debug mode is not set, all information is written to std::cout.
 *
 * Note, that if multithreading is switched on (TBB-flag), the access to std::cout or the
 * logfile respectively is synchronized. The other methods are not thread-safe and should
 * be called only in safe context for defined behaviour.
 *
 * @version $Revision: 1.19 $
 * @author  Tobias Weinzierl, Wolfgang Eckhardt
 */
class tarch::logging::NoLogger {
  private:
    static Log _log;

    static NoLogger _singleton;

    tarch::multicore::BooleanSemaphore _semaphore;

    bool           _quitOnError;

    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    NoLogger& operator=(const NoLogger&) = delete;

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    NoLogger(const NoLogger&) = delete;

    /**
     * It's a singleton.
     */
    NoLogger();

    std::string constructMessageString(
      std::string          messageType,
      long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message
    );
  public:
    ~NoLogger() = default;

    static NoLogger& getInstance();

    /**
     * Nop
     */
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
     * Nop
     */
    void traceIn(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {}

    /**
     * Nop
     */
    void traceOut(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message) {}

    /**
     * Nop
     */
    void indent( bool indent, const std::string& trace ) {}

    void setQuitOnError(bool value);
};
