// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef Parallel
#include <mpi.h>
#endif
#include <iostream>
#include <sstream>
#include <functional>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/clock.h>
#endif

#include <ctime>
#include <chrono>

#include "config.h"

namespace tarch {
  namespace logging {
    class Log;
    class CommandLineLogger;
    class ChromeTraceFileLogger;
    class NVTXLogger;
    class ITACLogger;
    class ITTLogger;
    class ScorePLogger;
  }
}

#if PeanoDebug>=4
/**
 * @see logInfo() macro
 */
#define logDebug(methodName, logMacroMessageStream) \
   { \
      auto logMacroMessage = [&](void) -> std::string { \
        std::ostringstream conv; \
        conv << logMacroMessageStream; \
        conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
        return conv.str(); \
      }; \
      _log.debug (methodName, logMacroMessage); \
   }
#else
#define logDebug(methodName, logMacroMessageStream)
#endif

#if PeanoDebug>=1
#define logTraceIn(methodName) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
      std::ostringstream conv; \
      conv << "in (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
      return conv.str(); \
    }; \
    _log.traceIn(methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith1Argument(methodName,argument0) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
      std::ostringstream conv; \
      conv << "in:" << #argument0 << ":" << argument0; \
      conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
      return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true, _log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith2Arguments(methodName,argument0,argument1) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true, _log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith3Arguments(methodName,argument0,argument1,argument2) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith4Arguments(methodName,argument0,argument1,argument2,argument3) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith5Arguments(methodName,argument0,argument1,argument2,argument3,argument4) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith6Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith7Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << "," << #argument6 << ":" << argument6; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith8Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << "," << #argument6 << ":" << argument6; \
     conv << "," << #argument7 << ":" << argument7; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceInWith9Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7,argument8) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "in:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << "," << #argument6 << ":" << argument6; \
     conv << "," << #argument7 << ":" << argument7; \
     conv << "," << #argument8 << ":" << argument8; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.traceIn (methodName, logMacroMessage); \
    _log.indent(true,_log.getTraceInformation(methodName), logMacroMessage); \
  }

#define logTraceOut(methodName) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName),logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith1Argument(methodName,argument0) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
      std::ostringstream conv; \
      conv << "out:" << #argument0 << ":" << argument0; \
      conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
      return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith2Arguments(methodName,argument0,argument1) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith3Arguments(methodName,argument0,argument1,argument2) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith4Arguments(methodName,argument0,argument1,argument2,argument3) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith5Arguments(methodName,argument0,argument1,argument2,argument3,argument4) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith6Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith7Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << "," << #argument6 << ":" << argument6; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName), logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }


#define logTraceOutWith8Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1 << ":" << argument1; \
     conv << "," << #argument2 << ":" << argument2; \
     conv << "," << #argument3 << ":" << argument3; \
     conv << "," << #argument4 << ":" << argument4; \
     conv << "," << #argument5 << ":" << argument5; \
     conv << "," << #argument6 << ":" << argument6; \
     conv << "," << #argument7 << ":" << argument7; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
    }; \
    _log.indent(false,_log.getTraceInformation(methodName),logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#define logTraceOutWith12Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7,argument8,argument9,argument10,argument11) \
  { \
    auto logMacroMessage = [&](void) -> std::string { \
     std::ostringstream conv; \
     conv << "out:" << #argument0 << ":" << argument0; \
     conv << "," << #argument1  << ":" << argument1; \
     conv << "," << #argument2  << ":" << argument2; \
     conv << "," << #argument3  << ":" << argument3; \
     conv << "," << #argument4  << ":" << argument4; \
     conv << "," << #argument5  << ":" << argument5; \
     conv << "," << #argument6  << ":" << argument6; \
     conv << "," << #argument7  << ":" << argument7; \
     conv << "," << #argument8  << ":" << argument8; \
     conv << "," << #argument9  << ":" << argument9; \
     conv << "," << #argument10 << ":" << argument10; \
     conv << "," << #argument11 << ":" << argument11; \
     conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
     return conv.str(); \
   }; \
    _log.indent(false,_log.getTraceInformation(methodName),logMacroMessage); \
    _log.traceOut (methodName, logMacroMessage); \
  }

#else
#define logTraceIn(methodName)
#define logTraceInWith1Argument(methodName,argument0)
#define logTraceInWith2Arguments(methodName,argument0,argument1)
#define logTraceInWith3Arguments(methodName,argument0,argument1,argument2)
#define logTraceInWith4Arguments(methodName,argument0,argument1,argument2,argument3)
#define logTraceInWith5Arguments(methodName,argument0,argument1,argument2,argument3,argument4)
#define logTraceInWith6Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5)
#define logTraceInWith7Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6)
#define logTraceInWith8Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7)
#define logTraceInWith9Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7,argument8)
#define logTraceOut(methodName)
#define logTraceOutWith1Argument(methodName,argument0)
#define logTraceOutWith2Arguments(methodName,argument0,argument1)
#define logTraceOutWith3Arguments(methodName,argument0,argument1,argument2)
#define logTraceOutWith4Arguments(methodName,argument0,argument1,argument2,argument3)
#define logTraceOutWith5Arguments(methodName,argument0,argument1,argument2,argument3,argument4)
#define logTraceOutWith6Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5)
#define logTraceOutWith7Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6)
#define logTraceOutWith8Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7)
#define logTraceOutWith12Arguments(methodName,argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7,argument8,argument9,argument10,argument11)
#endif

/**
 * @brief Wrapper macro around tarch::tarch::logging::Log to improve logging.
 *
 * A Log object with name _log has to be defined at the place of calling this
 * macro.
 *
 * This macro allows to combine strings and variables arbitrarily
 * in an efficient way (only one ostringstream object has to be created per
 * usage of logInfo).
 *
 * Usage:
 * logInfo( "myOperation()", "anyText" << myVar << ",anotherText" << myVar2 );
 *
 * !!! Hint
 *
 * Never use the + operator to concatenate data as this is error-prone.
 * If you use always the << operator, you are on the safe side, as the +
 * operator works only for strings properly. If you use it with a string and
 * another data type, it might be that the string is assigned an invalid length.
 */
#define logInfo(methodName, logMacroMessageStream) \
   { \
     auto logMacroMessage = [&](void) -> std::string { \
      std::ostringstream conv; \
      conv << logMacroMessageStream; \
      return conv.str(); \
    }; \
    _log.info (methodName, logMacroMessage); \
   }

#define logExceptionAndQuit(exception) \
  { \
    std::cerr << std::string("caught exception (file:)") << __FILE__ << std::string(", line:") << __LINE__ << std::string("): ") << std::string(exception.what()); \
    exit(-1); \
  }

/**
 * @brief Wrapper macro around tarch::tarch::logging::Log to improve logging.
 *
 * A Log object with name _log has to be defined at the place of calling this
 * macro.
 *
 * This macro allows to combine strings and variables arbitrarily
 * in an efficient way (only one ostringstream object has to be created per
 * usage of logWarning).
 *
 * Usage:
 * logWarning( "myOperation()", "anyText" << myVar << ",anotherText" << myVar2 );
 */
#define logWarning(methodName, logMacroMessageStream) \
   { \
     auto logMacroMessage = [&](void) -> std::string { \
       std::ostringstream conv; \
       conv << logMacroMessageStream; \
       conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
       return conv.str(); \
     }; \
     _log.warning(methodName, logMacroMessage); \
   }

/**
 * @brief Wrapper macro around tarch::tarch::logging::Log to improve logging.
 *
 * A Log object with name _log has to be defined at the place of calling this
 * macro.
 *
 * This macro allows to combine strings and variables arbitrarily
 * in an efficient way (only one ostringstream object has to be created per
 * usage of logError).
 *
 * Usage:
 * logInfo( "myOperation()", "anyText" << myVar << ",anotherText" << myVar2 );
 */
#define logError(methodName, logMacroMessageStream) \
   { \
     auto logMacroMessage = [&](void) -> std::string { \
       std::ostringstream conv; \
       conv << logMacroMessageStream; \
       conv << " (file:" << __FILE__ << ",line:" << __LINE__ << ")"; \
       return conv.str(); \
     }; \
      _log.error (methodName, logMacroMessage); \
   }

/**
 * Log Device
 *
 * Log is the class all logging classes should use. To use the logging API they
 * have to create an instance by their own. It is suggested to hold this
 * instance static for the constructor of the Log class has to be given the
 * class name of the logging class. The Log class itself is stateless.
 * The log requests on this instance are processed here and forwarded to the
 * assigned logger (an internal attribute).
 *
 * Which concrete implementation has to be used for logging is switched using
 * a compiler attribute. Since the logging is used extremely often, this is
 * better than dynamic binding.
 *
 * There are five different log levels, the user may write any output:
 *
 * - error: Here only errors have to be written to. Error logMacroMessages may never
 *          be oppressed.
 * - warning:
 * - debug: Debug information that is switched off normally. Only available if
 *          you use Debug
 * - info:  Statistical information, copyright and similar information. Should
 *          be used rather seldom.
 *
 * \section  Runtime
 *
 * The underlying log device (like the CommandlineLogger) has to offer
 * synchronized output methods. Thus, calls to logging methods in
 * multithreaded environments mark synchronization points of your programme
 * and will change timing behaviour!
 *
 * \section  Log output format
 *
 * You can switch the log output format by setting -DUseLogService=xxx. At
 * the moment, I support two different formats for xxx:
 *
 * - CommandLineLogger       This is the default.
 * - ChromeTraceFileLogger   For the Chrome tracing API.
 *
 * @author  Tobias Weinzierl
 */
class tarch::logging::Log {
  private:
    #if !defined(UseLogService)
    typedef CommandLineLogger     UseLogService;
    #endif

    /**
     * Returns the time stamp in nanoseconds.
     */
    long int getTimeStamp() const;

    /**
     * Name of the class that is using the interface.
     */
    std::string  _className;

    std::chrono::system_clock::time_point  _startupTime;

    #ifdef __APPLE__
    clock_serv_t cclock;
    #endif
  public:
    /**
     * Writes information about the computer the output is written from.
     * The information string contains the operation system name, the computer
     * name and the cpu name.
     */
    static std::string getMachineInformation();

    /**
     * Constructor
     *
     * @param className Name of the class that is using the logging component.
     *                  Please specify both class name and namespace using the
     *                  format namespace::classname
     */
    Log(const std::string& className);

    /**
     * Destructor
     */
    virtual ~Log();

    /**
     * Logs and exception and stops the application. I recommend to use the
     * corresponding macro instead.
     */
    static void exception( const std::bad_alloc& exception, const std::string& file, const int lineNumber );

    /**
     * We could make the logMacroMessage passed into a log statement a string. However,
     * many codes write rather complicated strings and therefore spend a lot of
     * time to construct the logMacroMessage. This is fine in release mode, e.g., where
     * the number of strings is small. It is problematic in trace mode or debug,
     * where we have a lot of strings, and most of them won't be used, as we
     * filter them out later anyway.
     *
     * Therefore, I switch to lazy evaluation: a logMacroMessage string is constructed
     * if and only if the logMacroMessage will be written. The construction is realised
     * through a (lambda) function modelled via Message.
     */
    typedef std::function< std::string(void) >  Message;

    /**
     * Log Debug Information
     *
     * Remark: Please do not use this operation directly, but use the macros
     * above instead.
     *
     * The method has to be given the method name. Often it
     * is useful to pass the whole method signature, that means e.g. if the
     * method info itself would log, the method invocation would look like this:
     * <pre>
     *   info("info(std::string,std::string)", "just an example text");
     * </pre>
     * So you are able to identify methods in your log although you use
     * overloading.
     *
     * @param methodName method name
     * @param logMacroMessage    log logMacroMessage
     */
    #if PeanoDebug>=4
      void debug(const std::string& methodName, Message logMacroMessage);
    #else
      inline void debug([[maybe_unused]] const std::string& methodName, [[maybe_unused]] Message logMacroMessage) const {}
    #endif

    /**
     * Log Information
     *
     * The method has to be given the method name. Often it
     * is useful to pass the whole method signature, that means e.g. if the
     * method info itself would log, the method invocation would look like this:
     * <pre>
     *   info("info(std::string,std::string)", "just an example text");
     * </pre>
     * So you are able to identify methods in your log although you use
     * overloading.
     *
     * @param methodName method name
     * @param logMacroMessage    log logMacroMessage
     */
    void info(const std::string& methodName, Message logMacroMessage);

    /**
     * Log a Warning
     *
     * The method has to be given the method name. Often it
     * is useful to pass the whole method signature, that means e.g. if the
     * method info itself would log, the method invocation would look like this:
     * <pre>
     *   info("info(std::string,std::string)", "just an example text");
     * </pre>
     * So you are able to identify methods in your log although you use
     * overloading.
     *
     * @param methodName method name
     * @param logMacroMessage    log logMacroMessage
     */
    void warning(const std::string& methodName, Message logMacroMessage);

    /**
     * Log an Error
     *
     * The method has to be given the method name. Often it
     * is useful to pass the whole method signature, that means e.g. if the
     * method info itself would log, the method invocation would look like this:
     * <pre>
     *   info("info(std::string,std::string)", "just an example text");
     * </pre>
     * So you are able to identify methods in your log although you use
     * overloading.
     *
     * @param methodName method name
     * @param logMacroMessage    log logMacroMessage
     */
    void error(const std::string& methodName, Message logMacroMessage);

    #if PeanoDebug>=1
    void traceIn(const std::string& methodName, Message logMacroMessage);
    void traceOut(const std::string& methodName, Message logMacroMessage);
    #else
    void traceIn(const std::string& methodName, Message logMacroMessage);
    void traceOut(const std::string& methodName, Message logMacroMessage);
    #endif

    /**
     * Indent the Subsequent Messages
     *
     * Depending on indent the operation increments or decrements the indent.
     * The call is forwarded to the logging device.
     */
    void indent( bool indent, const std::string& trace, Message logMacroMessage ) const;

    std::string getTraceInformation( const std::string& methodName ) const;

    static void flushBeforeAssertion();
};
