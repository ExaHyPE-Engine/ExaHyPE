// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
  inline std::ostream& align(std::ostream& out) {
    out.put('\x1f');
    return out;
  }
} // namespace tarch

namespace tarch::multicore {
  class BooleanSemaphore;
} // namespace tarch::multicore

namespace tarch::instrumentation {
  class Log;
} // namespace tarch::instrumentation

#define TARCH_LOGGING_FUNCTION_NAME std::string_view(__func__)

#if defined(__GNUC__) or defined(__clang__) or defined(__NVCOMPILER) or defined(__PGI) or defined(__CUDACC__) or \
  defined(__NVCC__)
#define TARCH_LOGGING_FUNCTION_SIGNATURE std::string_view(__PRETTY_FUNCTION__)
#elif defined(_MSC_VER)
#define TARCH_LOGGING_FUNCTION_SIGNATURE std::string_view(__FUNCSIG__)
#else
#define TARCH_LOGGING_FUNCTION_SIGNATURE TARCH_LOGGING_FUNCTION_NAME
#endif

#define TARCH_INSTRUMENTATION_SCOPE_NAME \
  tarch::instrumentation::Log::makeScopeName(TARCH_LOGGING_FUNCTION_SIGNATURE, TARCH_LOGGING_FUNCTION_NAME, __FILE__)

class tarch::instrumentation::Log {
private:
  enum class ConsoleStream { None, Cout, Cerr };

  static constexpr std::string::size_type NumberOfStandardColumnSpaces     = 12;
  static constexpr std::string::size_type NumberOfScopeColumnSpaces        = 55;
  static constexpr std::string::size_type MinimumAlignedMessagePrefixWidth = 24;
  static constexpr char                   AlignmentMarker                  = '\x1f';

  struct Configuration {
    std::string              logColumnSeparator        = " ";
    bool                     logTimeStamp              = true;
    bool                     logTimeStampHumanReadable = true;
    bool                     logMachineName            = true;
    bool                     logThreadName             = true;
    bool                     logMessageType            = true;
    bool                     logScopeName              = true;
    std::string              outputFileName            = "";
    bool                     debugFilterConfigured     = false;
    bool                     debugEnabledByDefault     = false;
    std::vector<std::string> debugScopePrefixes;
  };

  struct Context {
    long int timestampNanoseconds;
    int      rank;
    int      threadId;
  };

  /**
   * Returns the time stamp in nanoseconds.
   */
  static long int getTimeStamp();

  static Context createContext();

  static tarch::multicore::BooleanSemaphore& getSemaphore();

  static tarch::multicore::BooleanSemaphore& getConfigurationSemaphore();

  static bool isWhitespace(char value) noexcept;

  static std::string_view trimView(std::string_view value) noexcept;

  static std::string trim(std::string_view value);

  static std::string_view getFileNameStem(std::string_view fileName) noexcept;

  static std::string extractQualifiedFunctionName(std::string_view functionSignature);

  static void
  appendColumn(std::string& result, const std::string& separator, std::string::size_type width, std::string_view value);

  static void appendMessageColumn(
    std::string&           result,
    const std::string&     separator,
    std::string::size_type width,
    std::string::size_type indent,
    std::string_view       message
  );

  static std::string getTimeStampHumanReadable(long int timestampNanoseconds);

  static std::string alignMessage(std::string_view message, std::string::size_type messageStartColumn);

  static std::string constructMessageString(
    std::string_view     messageType,
    long int             timestampNanoseconds,
    int                  rank,
    int                  threadId,
    std::string_view     scopeName,
    std::string_view     message,
    const Configuration& configuration
  );

  static void configureDebugFilter();
  static bool debugMessagesAreConfigured();

  static const Configuration& getConfiguration();

  static void updateConfiguration(const std::function<void(Configuration&)>& update);

  static void writeFormattedLine(
    std::string_view     line,
    ConsoleStream        consoleStream,
    bool                 flushConsole,
    std::string_view     messageType,
    const Configuration& configuration
  );

  static void writeMessage(
    std::string_view messageType,
    long int         timestampNanoseconds,
    int              rank,
    int              threadId,
    std::string_view scopeName,
    std::string_view message,
    ConsoleStream    consoleStream,
    bool             flushConsole
  );

#if PEANO_TRACE
  static void closeTrace(bool useCollective);
#endif

  static std::chrono::steady_clock::time_point             _startupTime;
  static const Configuration* const                               _defaultConfiguration;
  static std::vector<std::unique_ptr<const Configuration>>* const _configurationSnapshots;
  static std::atomic<const Configuration*>                        _configuration;
  static std::once_flag                                    _debugFilterEnvironmentOnce;
  static std::atomic<std::string::size_type>               _alignedMessagePrefixColumn;

public:
  Log()                      = delete;
  Log(const Log&)            = delete;
  Log& operator=(const Log&) = delete;

  /**
   * Derive a scope label such as namespace::Class::method() from the compiler
   * function signature available at the macro call site.
   */
  static std::string
  makeScopeName(std::string_view functionSignature, std::string_view functionName, std::string_view fileName);

  static bool debugMessagesAreEnabled(std::string_view scopeName);

  template <typename MessageFactory>
  static void debugIfEnabled(
    std::string_view functionSignature,
    std::string_view functionName,
    std::string_view fileName,
    MessageFactory   messageFactory
  ) {
    if (debugMessagesAreConfigured()) {
      const std::string scopeName = makeScopeName(functionSignature, functionName, fileName);
      if (debugMessagesAreEnabled(scopeName)) {
        debugWithScopeName(scopeName, messageFactory());
      }
    }
  }

  static void debugWithScopeName(std::string_view scopeName, std::string_view message);
  static void infoWithScopeName(std::string_view scopeName, std::string_view message);
  static bool isInfoOutputEnabled();
  static void warningWithScopeName(std::string_view scopeName, std::string_view message);
  static void errorWithScopeName(std::string_view scopeName, std::string_view message);

#if PEANO_TRACE
  static std::string
  makeTraceScopeName(std::string_view functionSignature, std::string_view functionName, std::string_view fileName);
  static std::string makeTraceScopeName(
    std::string_view functionSignature,
    std::string_view functionName,
    std::string_view fileName,
    std::string_view traceScopeName
  );
  static void enterTimedScope(std::string scopeName);
  static void leaveTimedScope(std::string_view scopeName);
#endif

  static void setLogColumnSeparator(const std::string& separator = " ");
  static void setLogTimeStamp(bool value = true);
  static void setLogTimeStampHumanReadable(bool value = true);
  static void setLogMachineName(bool value = true);
  static void setLogThreadName(bool value = true);
  static void setLogMessageType(bool value = true);
  static void setLogScopeName(bool value = true);

  static void setLogFormat(
    const std::string& columnSeparator,
    bool               logTimeStamp,
    bool               logTimeStampHumanReadable,
    bool               logMachineName,
    bool               logThreadName,
    bool               logMessageType,
    bool               logScopeName,
    const std::string& outputLogFileName
  );

  static void setOutputFile(const std::string& outputLogFileName);

  static void setDebug(bool enabled);
  static void enableDebugScopePrefix(const std::string& scopePrefix);
  static void clearDebugFilter();

  static std::string getScopeName(std::string_view className, std::string_view methodName);

  static void close();

  static void flushBeforeAssertion();
};

#include "Debug.h"
#include "Error.h"
#include "Info.h"
#include "Trace.h"
#include "Warning.h"
