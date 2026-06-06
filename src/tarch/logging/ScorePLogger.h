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
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/multicore/multicore.h"
#include "tarch/logging/Log.h"

#ifdef UseScoreP
#include <scorep/SCOREP_User_Types.h>
#endif

namespace tarch {
  namespace logging {
    class ScorePLogger;
  }
}

/**
 * Command Line Logger
 *
 * Log output device connecting to Score-P. Implements the LogHandle. Usually
 * there's only one instance of this class for only the Log type aggregates it
 * in a static way.
 *
 * The logger accepts trace and log statements and forwards them to Score-P.
 * Information messages, warning and errors are also piped onto the terminal.
 * We follow the Score-P documentation from
 *
 * https://www.vi-hps.org/projects/score-p/documentation/documentation.html
 *
 * ## Implementation
 *
 * The idea is that the tracing holds one region handle per traced function.
 * Per handle, we have to call
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * SCOREP_USER_REGION_DEFINE
 * SCOREP_USER_REGION_INIT
 * SCOREP_USER_REGION_ENTER
 * SCOREP_USER_REGION_END
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * in line with https://perftools.pages.jsc.fz-juelich.de/cicd/scorep/tags/latest/html/group__SCOREP__User.html#ga8c8f772145f95dce57dcf86b6998365c.
 * Obviously, define and init should be called only once, while we have an
 * arbitrary number of enter-end calls.
 *
 * The logger works with lazy generation of regions.
 *
 * Obviously, we cannot use the define macro, as it creates a static instance
 * and we work with lookup tables here instead of static attributes. Therefore,
 * we do not work with the SCOREP_USER_REGION_DEFINE macro but instead directly
 * with the underlying data type.
 *
 *
 * ## Relation to Score-P instrumentation
 *
 * All of the Score-P labels here logical. That is, they are tied to the
 * logical function name and not the real one. In practice, this is not true,
 * as Peano uses a lot of trace in and trace out macros to annotate specific
 * regions. Anyway, we kind of replicate the Score-P instrumentation and
 * both the instrumentation and the ScorePLogger can be used in tandem.
 *
 * @version $Revision: 1.19 $
 * @author  Tobias Weinzierl
 */
class tarch::logging::ScorePLogger {
  private:
    static Log _log;

    static ScorePLogger _singleton;

    tarch::multicore::MultiReadSingleWriteSemaphore _semaphore;

    #ifdef UseScoreP
    std::map< std::string, SCOREP_User_RegionHandle > _scorePHandles;
    #endif

    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    ScorePLogger& operator=(const CommandLineLogger&) = delete;

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    ScorePLogger(const ScorePLogger&) = delete;

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
     */
    ScorePLogger();

    std::string getTimeStampHumanReadable( long int timestampNanoseconds ) const;

  public:
    ~ScorePLogger();

    static ScorePLogger& getInstance();

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

    /**
     * Log trace out event
     *
     * This event forwards the call to a call of the
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * SCOREP_USER_REGION_END
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * macro. In earlier versions, it used to delegate to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * SCOREP_USER_REGION_LEAVE
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * but that macro seems to be discontinued in Score-P 6.
     */
    void traceOut(   long int timestampNanoseconds, int rank, int threadId, const std::string& trace, const std::string& message);

    /**
     * Tells the logger to increment/decrement the indent.
     *
     * For Score-P, we do not need any indentation. Consequently, we put the
     * nop implementation into the header, so the compiler can inline the
     * call and eventually remove it completely.
     */
    void indent( bool indent, const std::string& trace ) {};

    void close();
};
