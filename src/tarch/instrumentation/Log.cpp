// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Log.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/CommandLineSink.h"
#include "tarch/instrumentation/FileSink.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

std::ostream&
tarch::align(std::ostream& out) {
  out.put('\x1f');
  return out;
}

std::chrono::steady_clock::time_point tarch::instrumentation::Log::_startupTime{std::chrono::steady_clock::now()};

// The default configuration and the snapshot store are heap objects that are
// never freed. Destructors of other globals still write log messages, so the
// configuration these pointers name stays valid throughout static destruction.
const tarch::instrumentation::Log::Configuration* const tarch::instrumentation::Log::_defaultConfiguration{
  new tarch::instrumentation::Log::Configuration{}
};

std::vector<std::unique_ptr<const tarch::instrumentation::Log::Configuration>>* const
  tarch::instrumentation::Log::_configurationSnapshots{
    new std::vector<std::unique_ptr<const tarch::instrumentation::Log::Configuration>>()
  };

std::atomic<const tarch::instrumentation::Log::Configuration*> tarch::instrumentation::Log::_configuration{
  tarch::instrumentation::Log::_defaultConfiguration
};

std::once_flag tarch::instrumentation::Log::_debugFilterEnvironmentOnce;

std::atomic<std::string::size_type> tarch::instrumentation::Log::_alignedMessagePrefixColumn{0};

namespace {
  bool isGlobalMasterRankOrMpiUninitialised() {
    const tarch::mpi::Rank& rank = tarch::mpi::Rank::getInstance();
    return not rank.isInitialised() or rank.isGlobalMaster();
  }

  std::string_view trimRightView(std::string_view value) noexcept {
    std::size_t last = value.size();
    while (
      last > 0
      and (value[last - 1] == ' ' or value[last - 1] == '\t' or value[last - 1] == '\n' or value[last - 1] == '\r')) {
      last--;
    }
    return value.substr(0, last);
  }

  std::mutex& getConfigurationMutex() {
    static std::mutex mutex;
    return mutex;
  }
} // namespace

const tarch::instrumentation::Log::Configuration&
tarch::instrumentation::Log::getConfiguration() {
  return *_configuration.load(std::memory_order_acquire);
}

bool
tarch::instrumentation::Log::isInfoOutputEnabled() {
  return getConfiguration().infoOutputEnabled and isGlobalMasterRankOrMpiUninitialised();
}

bool
tarch::instrumentation::Log::isWarningOutputEnabled() {
  return getConfiguration().warningOutputEnabled;
}

void
tarch::instrumentation::Log::updateConfiguration(const std::function<void(Configuration&)>& update) {
  const std::scoped_lock lock(getConfigurationMutex());
  auto                   next = std::make_unique<Configuration>(getConfiguration());
  update(*next);

  const Configuration* configuration = next.get();
  _configurationSnapshots->push_back(std::move(next));
  _configuration.store(configuration, std::memory_order_release);
}

bool
tarch::instrumentation::Log::isWhitespace(char value) noexcept {
  return value == ' ' or value == '\t' or value == '\n' or value == '\r';
}

std::string_view
tarch::instrumentation::Log::trimView(std::string_view value) noexcept {
  std::size_t first = 0;
  while (first < value.size() and isWhitespace(value[first])) {
    first++;
  }

  std::size_t last = value.size();
  while (last > first and isWhitespace(value[last - 1])) {
    last--;
  }

  return value.substr(first, last - first);
}

std::string
tarch::instrumentation::Log::trim(std::string_view value) {
  return std::string(trimView(value));
}

std::string_view
tarch::instrumentation::Log::getFileNameStem(std::string_view fileName) noexcept {
  const std::size_t lastSeparator = fileName.find_last_of("/\\");
  const std::size_t begin         = lastSeparator == std::string_view::npos ? 0 : lastSeparator + 1;
  const std::size_t lastDot       = fileName.find_last_of('.');

  if (lastDot != std::string_view::npos and lastDot > begin) {
    return fileName.substr(begin, lastDot - begin);
  }

  return fileName.substr(begin);
}

std::string
tarch::instrumentation::Log::extractQualifiedFunctionName(std::string_view functionSignature) {
  functionSignature = trimView(functionSignature);

  const std::size_t templateSuffix = functionSignature.find(" [");
  if (templateSuffix != std::string_view::npos) {
    functionSignature = functionSignature.substr(0, templateSuffix);
  }

  const std::size_t argumentList = functionSignature.rfind('(');
  if (argumentList == std::string_view::npos) {
    return functionSignature.find("::") == std::string_view::npos ? std::string{} : std::string(functionSignature);
  }

  std::string_view beforeArguments = trimView(functionSignature.substr(0, argumentList));
  std::size_t      nameStart       = std::string_view::npos;
  int              parenDepth      = 0;
  int              angleDepth      = 0;
  int              bracketDepth    = 0;
  int              braceDepth      = 0;

  for (std::size_t i = beforeArguments.size(); i > 0; i--) {
    const char character = beforeArguments[i - 1];
    switch (character) {
    case ')':
      parenDepth++;
      break;
    case '(':
      if (parenDepth > 0) {
        parenDepth--;
      }
      break;
    case '>':
      angleDepth++;
      break;
    case '<':
      if (angleDepth > 0) {
        angleDepth--;
      }
      break;
    case ']':
      bracketDepth++;
      break;
    case '[':
      if (bracketDepth > 0) {
        bracketDepth--;
      }
      break;
    case '}':
      braceDepth++;
      break;
    case '{':
      if (braceDepth > 0) {
        braceDepth--;
      }
      break;
    default:
      if (isWhitespace(character) and parenDepth == 0 and angleDepth == 0 and bracketDepth == 0 and braceDepth == 0) {
        nameStart = i;
        i         = 1;
      }
      break;
    }
  }

  if (nameStart != std::string_view::npos) {
    beforeArguments = beforeArguments.substr(nameStart);
  }

  return std::string(beforeArguments);
}

/**
 * Returns a scope label such as namespace::Class::method(), derived from the
 * compiler function signature the macro call site passes in.
 */
std::string
tarch::instrumentation::Log::makeScopeName(
  std::string_view functionSignature,
  std::string_view functionName,
  std::string_view fileName
) {
  std::string qualifiedFunctionName = extractQualifiedFunctionName(functionSignature);

  if (qualifiedFunctionName.empty()) {
    const std::string_view fileStem = getFileNameStem(fileName);
    qualifiedFunctionName.reserve(fileStem.size() + functionName.size() + 2);
    if (not fileStem.empty()) {
      qualifiedFunctionName.append(fileStem.data(), fileStem.size());
      qualifiedFunctionName += "::";
    }
    qualifiedFunctionName.append(functionName.data(), functionName.size());
  }

  qualifiedFunctionName += "()";
  return qualifiedFunctionName;
}

void
tarch::instrumentation::Log::appendColumn(
  std::string&           result,
  const std::string&     separator,
  std::string::size_type width,
  std::string_view       value
) {
  if (value.empty()) {
    return;
  }

  result += separator;
  result.append(value.data(), value.size());
  if (value.size() < width) {
    result.append(width - value.size(), ' ');
  }
}

void
tarch::instrumentation::Log::appendMessageColumn(
  std::string&           result,
  const std::string&     separator,
  std::string::size_type width,
  std::string::size_type indent,
  std::string_view       message
) {
  result += separator;
  result.append(indent, ' ');
  result.append(message.data(), message.size());

  const auto paddedSize = indent + message.size();
  if (paddedSize < width) {
    result.append(width - paddedSize, ' ');
  }
}

std::string
tarch::instrumentation::Log::getTimeStampHumanReadable(long int timestampNanoseconds) {
  long int      timestampSeconds = timestampNanoseconds / 1000 / 1000 / 1000;
  constexpr int SecondsPerHour   = 60 * 60;
  long int      hours            = timestampSeconds / SecondsPerHour;
  timestampSeconds               = timestampSeconds - SecondsPerHour * hours;

  constexpr int SecondsPerMinute = 60;
  long int      minutes          = timestampSeconds / SecondsPerMinute;
  timestampSeconds               = timestampSeconds - SecondsPerMinute * minutes;

  const long int seconds = timestampSeconds;

  std::string result;
  result.reserve(8);
  if (hours < 10) {
    result += "0";
  }
  result += std::to_string(hours);
  result += ":";
  if (minutes < 10) {
    result += "0";
  }
  result += std::to_string(minutes);
  result += ":";
  if (seconds < 10) {
    result += "0";
  }
  result += std::to_string(seconds);
  return result;
}

std::string
tarch::instrumentation::Log::alignMessage(std::string_view message, std::string::size_type messageStartColumn) {
  const std::size_t marker = message.find(AlignmentMarker);
  if (marker == std::string_view::npos) {
    return std::string(message);
  }

  const std::string_view prefix    = message.substr(0, marker);
  const std::string_view suffix    = message.substr(marker + 1);
  const auto requestedPrefixColumn = messageStartColumn + std::max(MinimumAlignedMessagePrefixWidth, prefix.size());

  auto prefixColumn = _alignedMessagePrefixColumn.load(std::memory_order_relaxed);
  while (prefixColumn < requestedPrefixColumn
         and not _alignedMessagePrefixColumn
                   .compare_exchange_weak(prefixColumn, requestedPrefixColumn, std::memory_order_relaxed)) {
  }
  prefixColumn = std::max(prefixColumn, requestedPrefixColumn);

  const auto prefixEndColumn = messageStartColumn + prefix.size();
  const auto separatorSpaces = prefixColumn - prefixEndColumn + 1;

  std::string result;
  result.reserve(prefix.size() + separatorSpaces + suffix.size());
  result.append(prefix.data(), prefix.size());
  result.append(separatorSpaces, ' ');
  result.append(suffix.data(), suffix.size());
  return result;
}

std::string
tarch::instrumentation::Log::constructMessageString(
  std::string_view     messageType,
  long int             timestampNanoseconds,
  int                  rank,
  int                  threadId,
  std::string_view     scopeName,
  std::string_view     message,
  const Configuration& configuration
) {
  std::string result;
  result.reserve(
    message.size() + MinimumAlignedMessagePrefixWidth + scopeName.size() + messageType.size()
    + configuration.logColumnSeparator.size() * 7 + NumberOfScopeColumnSpaces + NumberOfStandardColumnSpaces * 5 + 1
  );

  if (configuration.logTimeStamp) {
    const std::string timestamp = std::to_string(timestampNanoseconds);
    appendColumn(result, configuration.logColumnSeparator, NumberOfStandardColumnSpaces, timestamp);
  }

  if (configuration.logTimeStampHumanReadable) {
    const std::string timestamp = getTimeStampHumanReadable(timestampNanoseconds);
    appendColumn(result, configuration.logColumnSeparator, NumberOfStandardColumnSpaces, timestamp);
  }

  if (configuration.logMachineName) {
    const std::string machine = "Rank:" + std::to_string(rank);
    appendColumn(result, configuration.logColumnSeparator, NumberOfStandardColumnSpaces, machine);
  }

  if (configuration.logThreadName) {
    const std::string thread = "Core:" + std::to_string(threadId);
    appendColumn(result, configuration.logColumnSeparator, NumberOfStandardColumnSpaces, thread);
  }

  if (configuration.logMessageType) {
    appendColumn(result, configuration.logColumnSeparator, NumberOfStandardColumnSpaces, messageType);
  }

  if (configuration.logScopeName) {
    appendColumn(result, configuration.logColumnSeparator, NumberOfScopeColumnSpaces, scopeName);
  }

  if (message.find(AlignmentMarker) == std::string_view::npos) {
    appendMessageColumn(result, configuration.logColumnSeparator, 0, 0, trimRightView(message));
  } else {
    const std::string alignedMessage = alignMessage(message, result.size() + configuration.logColumnSeparator.size());
    appendMessageColumn(result, configuration.logColumnSeparator, 0, 0, trimRightView(alignedMessage));
  }
  result += '\n';

  return result;
}

void
tarch::instrumentation::Log::configureDebugFilter() {
  std::call_once(_debugFilterEnvironmentOnce, []() {
    bool                     debugEnabledByDefault = false;
    std::vector<std::string> debugScopePrefixes;

    const char* rawFilter = std::getenv("PEANO_LOG_DEBUG");
    if (rawFilter != nullptr) {
      std::string_view remaining(rawFilter);
      while (not remaining.empty()) {
        const std::size_t delimiter = remaining.find(',');
        const std::string token =
          trim(delimiter == std::string_view::npos ? remaining : remaining.substr(0, delimiter));

        if (token == "1" or token == "true" or token == "on" or token == "yes" or token == "all" or token == "*") {
          debugEnabledByDefault = true;
        } else if (not token.empty() and token != "0" and token != "false" and token != "off" and token != "no") {
          debugScopePrefixes.push_back(token);
        }

        if (delimiter == std::string_view::npos) {
          remaining = {};
        } else {
          remaining.remove_prefix(delimiter + 1);
        }
      }
    }

    updateConfiguration([&](Configuration& configuration) {
      if (configuration.debugFilterConfigured) {
        return;
      }

      configuration.debugFilterConfigured = true;
      configuration.debugEnabledByDefault = debugEnabledByDefault;
      configuration.debugScopePrefixes    = debugScopePrefixes;
    });
  });
}

bool
tarch::instrumentation::Log::debugMessagesAreConfigured() {
  if (not isGlobalMasterRankOrMpiUninitialised()) {
    return false;
  }

  configureDebugFilter();
  const Configuration& configuration = getConfiguration();
  return configuration.debugEnabledByDefault or not configuration.debugScopePrefixes.empty();
}

bool
tarch::instrumentation::Log::debugMessagesAreEnabled(std::string_view scopeName) {
  if (not debugMessagesAreConfigured()) {
    return false;
  }

  const Configuration& configuration = getConfiguration();
  if (configuration.debugEnabledByDefault) {
    return true;
  }

  for (const std::string& prefix : configuration.debugScopePrefixes) {
    if (scopeName.substr(0, prefix.size()) == prefix) {
      return true;
    }
  }

  return false;
}

void
tarch::instrumentation::Log::writeFormattedLine(
  std::string_view     line,
  ConsoleStream        consoleStream,
  bool                 flushConsole,
  std::string_view     messageType,
  const Configuration& configuration
) {
  if (not configuration.outputFileName.empty()) {
    FileSink::getInstance().write(line, configuration.outputFileName);
  }

  switch (consoleStream) {
  case ConsoleStream::Cout:
    CommandLineSink::getInstance().writeToStdOut(line, flushConsole);
    break;
  case ConsoleStream::Cerr:
    if (messageType == "Warning") {
      CommandLineSink::getInstance().writeToStdErr(line, flushConsole, CommandLineSink::TerminalStyle::Warning);
    } else if (messageType == "Error") {
      CommandLineSink::getInstance().writeToStdErr(line, flushConsole, CommandLineSink::TerminalStyle::Error);
    } else {
      CommandLineSink::getInstance().writeToStdErr(line, flushConsole);
    }
    break;
  case ConsoleStream::None:
    break;
  }
}

void
tarch::instrumentation::Log::writeMessage(
  std::string_view messageType,
  long int         timestampNanoseconds,
  int              rank,
  int              threadId,
  std::string_view scopeName,
  std::string_view message,
  ConsoleStream    consoleStream,
  bool             flushConsole
) {
  if (consoleStream == ConsoleStream::Cout and not isGlobalMasterRankOrMpiUninitialised()) {
    return;
  }

  const Configuration& configuration = getConfiguration();
  const std::string    outputMessage =
    constructMessageString(messageType, timestampNanoseconds, rank, threadId, scopeName, message, configuration);
  writeFormattedLine(outputMessage, consoleStream, flushConsole, messageType, configuration);
}

tarch::instrumentation::Log::Context
tarch::instrumentation::Log::createContext() {
  return {
    getTimeStamp(),
    tarch::mpi::Rank::getInstance().getRank(),
    tarch::tasking::Runtime::getInstance().getCoreNumber()
  };
}

void
tarch::instrumentation::Log::flushBeforeAssertion() {
  CommandLineSink::getInstance().close();
  FileSink::getInstance().close();
}

long int
tarch::instrumentation::Log::getTimeStamp() {
  const auto now    = std::chrono::steady_clock::now();
  long int   result = std::chrono::duration_cast<std::chrono::nanoseconds>(now - _startupTime).count();
  assertion(result >= 0, result);
  return result;
}

void
tarch::instrumentation::Log::setLogTimeStamp(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logTimeStamp = value; });
}

void
tarch::instrumentation::Log::setLogTimeStampHumanReadable(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logTimeStampHumanReadable = value; });
}

void
tarch::instrumentation::Log::setLogMachineName(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logMachineName = value; });
}

void
tarch::instrumentation::Log::setLogThreadName(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logThreadName = value; });
}

void
tarch::instrumentation::Log::setLogMessageType(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logMessageType = value; });
}

void
tarch::instrumentation::Log::setLogScopeName(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.logScopeName = value; });
}

void
tarch::instrumentation::Log::setInfoOutputEnabled(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.infoOutputEnabled = value; });
}

void
tarch::instrumentation::Log::setWarningOutputEnabled(bool value) {
  updateConfiguration([&](Configuration& configuration) { configuration.warningOutputEnabled = value; });
}

void
tarch::instrumentation::Log::setOutputFile(const std::string& outputLogFileName) {
  updateConfiguration([&](Configuration& configuration) { configuration.outputFileName = outputLogFileName; });
  FileSink::getInstance().setOutputFile(outputLogFileName);
}

void
tarch::instrumentation::Log::close() {
  CommandLineSink::getInstance().close();
  FileSink::getInstance().close();
}
