// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LOGGING_COMMANDLINELOGGER_H_
#define _TARCH_LOGGING_COMMANDLINELOGGER_H_


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
    class CommandLineLogger;
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
class tarch::logging::CommandLineLogger {
  private:
    static Log _log;

    static CommandLineLogger  _singleton;

    tarch::multicore::BooleanSemaphore _semaphore;

    /**
     * Test for the column separator of a string output.
     */
    std::string    _logColumnSeparator;
    bool           _logTimeStamp;
    bool           _logTimeStampHumanReadable;
    bool           _logMachineName;
    bool           _logThreadName;
    bool           _logMessageType;
    bool           _logTrace;
    std::ostream*  _outputStream;
    bool           _hasWrittenToOuputStream;
    int            _iterationCounter;
    std::string    _outputFileName;
    bool           _quitOnError;


    /**
     * Declared private since assignment does not make sense for an output
     * class (output information mismatch).
     */
    CommandLineLogger& operator=(const CommandLineLogger& rhs);

    /**
     * Declared private since copying does not make sense for an output
     * class (output information mismatch).
     */
    CommandLineLogger(const CommandLineLogger& param);

    /**
     * Indent is supported only in debug mode.
     */
    static std::string::size_type _indent;

    /**
     * @todo
     */
    static const std::string::size_type NumberOfIndentSpaces;

    /**
     * @todo
     */
    static const std::string::size_type NumberOfStandardColumnSpaces;

    /**
     * The trace column is not formatted using only tabulators, but it uses
     * spaces to create a unique column size: First, TRACE_SPACE_NUMBER-n
     * spaces are added if n is the length of the original string, then a \\t
     * is appended. This message augmentation is done only, if the Debug-mode
     * is set.
     */
    static const std::string::size_type NumberOfTraceColumnSpaces;

    static const int                    DigitsInFilenamesIterationNumer;

    /**
     * Takes the message and adds spaces such that the entries are aligned like
     * in a table. Should either be passed NumberOfIndentSpaces or
     * NumberOfTraceSpaces.
     */
    std::string addSeparators(std::string::size_type spaces, std::string message) const;

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
     * @param timestampNanoseconds Time stamp given in milliseconds
     */
    static std::string getTimeStampHumanReadable( long int timestampNanoseconds );

    /**
     * Configures the output streams
     */
    void configureOutputStreams();

    /**
     * It's a singleton.
     */
    CommandLineLogger();

    std::string getLogColumnSeparator() const;
    bool        getLogTimeStampHumanReadable() const;
    bool        getLogMessageType() const;

    std::ostream& out();

    void reopenOutputStream();

    std::stack< std::string >  _indentTraces;
  public:
    ~CommandLineLogger();

    static CommandLineLogger& getInstance();

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

    void setLogColumnSeparator( const std::string& separator = " ");
    void setLogTimeStamp( bool value = true );
    void setLogTimeStampHumanReadable( bool value = true );
    void setLogMachineName( bool value = true );
    void setLogThreadName( bool value = true );
    void setLogMessageType( bool value = true );
    void setLogTrace( bool value = true );

    /**
     * @param outputLogFileName If this argument is not the empty string, the
     *        logger pipes all debug data into a file and not to the terminal
     *        anymore.
     *
     * @see closeAllOutputFilesAndReopenNewOnes
     */
    void setLogFormat(
      const std::string& columnSeparator,
      bool logTimeStamp,
      bool logTimeStampHumanReadable,
      bool logMachineName,
      bool logThreadName,
      bool logMessageType,
      bool logTrace,
      const std::string&  outputLogFileName
    );

    /**
     * Is redundant, as you could use setLogFormat() instead. However, it is
     * tedious to specify all of these parameters if the only thing you want
     * is to establish an output file.
     */
    void setOutputFile( const std::string&  outputLogFileName );

    void setQuitOnError(bool value);

    /**
     * The command line logger can pipe debug data into an output file instead
     * of piping everything to the terminal. This happens if you call
     * setLogFormat() with a non-empty log file name. This operation closes the
     * output file, and opens a new one. Many uses, e.g., prefer one debug
     * output file per traversal.
     *
     * <h2> Debugging </h2>
     *
     * If you PeanoDebug symbol is bigger than 0, the routine gives you some hint
     * which trace objects are still on the stack, i.e. which traceIn has not yet
     * been matched by a traceOut.
     */
    void closeOutputStreamAndReopenNewOne();

    void close();
};

#endif
