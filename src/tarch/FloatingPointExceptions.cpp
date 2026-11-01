// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FloatingPointExceptions.h"

#if defined(PEANO_WITH_FPE)

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/mpi/Rank.h"

#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
#include <sys/ucontext.h>
#include <unistd.h>

#if not defined(PEANO_FPE_SYMBOLISER)
#error "A floating-point exception symboliser is required."
#endif
#endif

namespace {
  void terminateAfterTrapSetupFailure(const char* const file, const int line) noexcept {
    std::fprintf(stderr, "Fatal floating-point exception trap setup failure in (%s:%d).\n", file, line);
    std::fflush(stderr);
    tarch::mpi::abort(EXIT_FAILURE);
  }

#if (defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))) or defined(_MSC_VER)
  bool hasException(
    const tarch::FloatingPointExceptions::ExceptionMask candidate,
    const tarch::FloatingPointExceptions::Exception     exception
  ) noexcept {
    return (candidate & static_cast<tarch::FloatingPointExceptions::ExceptionMask>(exception)) != 0U;
  }
#endif

#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
  thread_local volatile std::sig_atomic_t armedExceptions = tarch::FloatingPointExceptions::None;
#else
  thread_local tarch::FloatingPointExceptions::ExceptionMask armedExceptions = tarch::FloatingPointExceptions::None;
#endif

#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
  int toNativeExceptionMask(const tarch::FloatingPointExceptions::ExceptionMask exceptions) noexcept {
    int result = 0;

    if (hasException(exceptions, tarch::FloatingPointExceptions::DivisionByZero)) {
      result |= FE_DIVBYZERO;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Invalid)) {
      result |= FE_INVALID;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Overflow)) {
      result |= FE_OVERFLOW;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Underflow)) {
      result |= FE_UNDERFLOW;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Inexact)) {
      result |= FE_INEXACT;
    }

    return result;
  }
#elif defined(_MSC_VER)
  unsigned int toNativeExceptionMask(const tarch::FloatingPointExceptions::ExceptionMask exceptions) noexcept {
    unsigned int result = 0U;

    if (hasException(exceptions, tarch::FloatingPointExceptions::DivisionByZero)) {
      result |= _EM_ZERODIVIDE;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Invalid)) {
      result |= _EM_INVALID;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Overflow)) {
      result |= _EM_OVERFLOW;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Underflow)) {
      result |= _EM_UNDERFLOW;
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Inexact)) {
      result |= _EM_INEXACT;
    }

    return result;
  }
#endif

#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
  tarch::FloatingPointExceptions::ExceptionMask toExceptionMask(const int signalCode) noexcept {
    using ExceptionMask = tarch::FloatingPointExceptions::ExceptionMask;

    switch (signalCode) {
    case FPE_FLTDIV:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::DivisionByZero);
    case FPE_FLTINV:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::Invalid);
    case FPE_FLTOVF:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::Overflow);
    case FPE_FLTUND:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::Underflow);
    case FPE_FLTRES:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::Inexact);
#if defined(FPE_FLTSUB)
    case FPE_FLTSUB:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::Invalid);
#endif
    default:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::None);
    }
  }

  struct FloatingPointEvent final {
    tarch::FloatingPointExceptions::ExceptionMask exceptions;
    std::uintptr_t                                instructionAddress;
    std::uintptr_t                                stackPointer;
    std::uintptr_t                                framePointer;
    std::uintptr_t                                stackReturnAddress;
  };

  struct ProcessMapping final {
    std::uintptr_t mappingStart;
    std::uintptr_t mappingEnd;
    std::uintptr_t fileOffset;
    std::string    objectFile;
    bool           readable;
    bool           executable;
  };

  struct SymbolisedLocation final {
    std::string sourceFile;
    int         sourceLine = 0;
    std::string functionName;
    std::string additionalFrames;
  };

  struct CodeLocation final {
    std::string instruction;
    std::string sourceFile;
    int         sourceLine = 0;
    std::string functionName;
    std::string additionalFrames;
  };

  std::once_flag                  floatingPointSignalHandlerInstallation;
  bool                            floatingPointSignalHandlerInstalled = false;
  struct sigaction                previousFloatingPointSignalAction{};
  volatile std::sig_atomic_t      previousSignalHandlerWasReset = 0;
  volatile std::sig_atomic_t      signalHandlerProcessId        = -1;
  thread_local FloatingPointEvent trappedEvent{};

  void writeSignalMessage(const char* const message, const std::size_t numberOfBytes) noexcept {
    const auto numberOfBytesWritten = write(STDERR_FILENO, message, numberOfBytes);
    static_cast<void>(numberOfBytesWritten);
  }

  std::uintptr_t getInstructionAddress(const void* const context) noexcept {
    if (context == nullptr) {
      return 0;
    }

    const auto* const signalContext = static_cast<const ucontext_t*>(context);
#if defined(__x86_64__)
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_RIP]);
#else
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_EIP]);
#endif
  }

  std::uintptr_t getStackPointer(const void* const context) noexcept {
    if (context == nullptr) {
      return 0;
    }

    const auto* const signalContext = static_cast<const ucontext_t*>(context);
#if defined(__x86_64__)
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_RSP]);
#else
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_ESP]);
#endif
  }

  std::uintptr_t getFramePointer(const void* const context) noexcept {
    if (context == nullptr) {
      return 0;
    }

    const auto* const signalContext = static_cast<const ucontext_t*>(context);
#if defined(__x86_64__)
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_RBP]);
#else
    return static_cast<std::uintptr_t>(signalContext->uc_mcontext.gregs[REG_EBP]);
#endif
  }

  std::string trimTrailingWhitespace(std::string value) {
    while (not value.empty()
           and (value.back() == '\n' or value.back() == '\r' or value.back() == ' ' or value.back() == '\t')) {
      value.pop_back();
    }

    return value;
  }

  std::string trimLeadingWhitespace(std::string value) {
    const auto firstNonWhitespace = value.find_first_not_of(" \t");
    if (firstNonWhitespace == std::string::npos) {
      return "";
    }

    value.erase(0, firstNonWhitespace);
    return value;
  }

  std::string quoteForShell(std::string value) {
    std::string result = "'";
    for (const char character : value) {
      if (character == '\'') {
        result += "'\\''";
      } else {
        result += character;
      }
    }
    result += "'";
    return result;
  }

  bool parseNonNegativeInteger(const std::string_view value, int& result) noexcept {
    const auto [parsedEnd, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc() and parsedEnd == value.data() + value.size() and result >= 0;
  }

  bool parseSourceLocation(std::string value, std::string& sourceFile, int& sourceLine) {
    const auto discriminator = value.find(" (discriminator ");
    if (discriminator != std::string::npos) {
      value.erase(discriminator);
    }
    if (value.find("??") != std::string::npos) {
      return false;
    }

    const auto lastSeparator = value.rfind(':');
    if (lastSeparator == std::string::npos or lastSeparator == 0) {
      return false;
    }

    int lastNumber = 0;
    if (not parseNonNegativeInteger(std::string_view(value).substr(lastSeparator + 1), lastNumber)) {
      return false;
    }

    const auto previousSeparator = value.rfind(':', lastSeparator - 1);
    int        possibleLine      = 0;
    if (
      previousSeparator != std::string::npos
      and parseNonNegativeInteger(
        std::string_view(value).substr(previousSeparator + 1, lastSeparator - previousSeparator - 1),
        possibleLine
      )
      and possibleLine > 0
    ) {
      sourceFile = value.substr(0, previousSeparator);
      sourceLine = possibleLine;
    } else if (lastNumber > 0) {
      sourceFile = value.substr(0, lastSeparator);
      sourceLine = lastNumber;
    } else {
      return false;
    }

    return not sourceFile.empty();
  }

  bool isSymbolisedLocationValid(const SymbolisedLocation& location) noexcept {
    return not location.sourceFile.empty() and location.sourceLine > 0;
  }

  SymbolisedLocation describeCodeLocationWithSymboliser(const char* const objectFile, const std::uintptr_t address) {
    if (objectFile == nullptr or objectFile[0] == '\0') {
      return {};
    }

    std::ostringstream command;
    command << quoteForShell(PEANO_FPE_SYMBOLISER) << " -C -f -i -e " << quoteForShell(objectFile) << " 0x" << std::hex
            << address << std::dec << " 2>/dev/null";

    FILE* const symboliserOutput = popen(command.str().c_str(), "r");
    if (symboliserOutput == nullptr) {
      return {};
    }

    std::array<char, 1024> buffer{};
    std::string            output;
    while (fgets(buffer.data(), buffer.size(), symboliserOutput) != nullptr) {
      output += buffer.data();
    }

    pclose(symboliserOutput);
    output = trimTrailingWhitespace(output);
    if (output.empty()) {
      return {};
    }

    std::istringstream lines(output);
    SymbolisedLocation result;
    std::string        functionName;
    std::string        sourceDescription;
    while (std::getline(lines, functionName) and std::getline(lines, sourceDescription)) {
      std::string sourceFile;
      int         sourceLine = 0;
      if (not parseSourceLocation(trimTrailingWhitespace(sourceDescription), sourceFile, sourceLine)) {
        continue;
      }
      functionName = trimTrailingWhitespace(functionName);

      if (not isSymbolisedLocationValid(result)) {
        result.sourceFile   = sourceFile;
        result.sourceLine   = sourceLine;
        result.functionName = functionName;
      } else {
        if (not result.additionalFrames.empty()) {
          result.additionalFrames += "; ";
        }
        result.additionalFrames += functionName + " at " + sourceFile + ":" + std::to_string(sourceLine);
      }
    }
    return result;
  }

  bool findProcessMapping(const std::uintptr_t address, ProcessMapping& result) {
    std::ifstream maps("/proc/self/maps");
    std::string   line;
    while (std::getline(maps, line)) {
      unsigned long long mappingStart       = 0;
      unsigned long long mappingEnd         = 0;
      unsigned long long fileOffset         = 0;
      char               permissions[5]     = {};
      int                parsedPrefixLength = 0;

      const auto numberOfParsedFields = std::sscanf(
        line.c_str(),
        "%llx-%llx %4s %llx %*s %*s %n",
        &mappingStart,
        &mappingEnd,
        permissions,
        &fileOffset,
        &parsedPrefixLength
      );
      if (numberOfParsedFields != 4) {
        continue;
      }
      if (address < mappingStart or address >= mappingEnd) {
        continue;
      }

      auto objectFile = trimLeadingWhitespace(line.substr(parsedPrefixLength));
      if (not objectFile.empty() and objectFile.front() == '[') {
        objectFile.clear();
      }
      result = ProcessMapping{
        .mappingStart = static_cast<std::uintptr_t>(mappingStart),
        .mappingEnd   = static_cast<std::uintptr_t>(mappingEnd),
        .fileOffset   = static_cast<std::uintptr_t>(fileOffset),
        .objectFile   = objectFile,
        .readable     = permissions[0] == 'r',
        .executable   = permissions[2] == 'x',
      };
      return true;
    }
    return false;
  }

  CodeLocation describeCodeLocation(const std::uintptr_t instructionAddress) {
    CodeLocation       result;
    std::ostringstream instruction;
    instruction << "0x" << std::hex << instructionAddress << std::dec;

    ProcessMapping mapping{};
    if (findProcessMapping(instructionAddress, mapping) and not mapping.objectFile.empty()) {
      instruction << " in " << mapping.objectFile;
      const auto objectRelativeAddress = instructionAddress - mapping.mappingStart + mapping.fileOffset;
      auto symbolisedLocation = describeCodeLocationWithSymboliser(mapping.objectFile.c_str(), objectRelativeAddress);
      if (not isSymbolisedLocationValid(symbolisedLocation) and objectRelativeAddress != instructionAddress) {
        symbolisedLocation = describeCodeLocationWithSymboliser(mapping.objectFile.c_str(), instructionAddress);
      }
      result.sourceFile       = symbolisedLocation.sourceFile;
      result.sourceLine       = symbolisedLocation.sourceLine;
      result.functionName     = symbolisedLocation.functionName;
      result.additionalFrames = symbolisedLocation.additionalFrames;
    }
    result.instruction = instruction.str();
    return result;
  }

  bool containsReadableWords(
    const ProcessMapping& mapping,
    const std::uintptr_t  address,
    const std::size_t     numberOfWords
  ) noexcept {
    if (not mapping.readable or address < mapping.mappingStart or address > mapping.mappingEnd) {
      return false;
    }
    return numberOfWords <= (mapping.mappingEnd - address) / sizeof(std::uintptr_t);
  }

  std::vector<std::uintptr_t> collectCallerAddresses(const FloatingPointEvent& event) {
    constexpr std::size_t MaximumNumberOfCallers = 16;

    std::vector<std::uintptr_t> callers;
    callers.reserve(MaximumNumberOfCallers);

    ProcessMapping faultMapping{};
    if (findProcessMapping(event.instructionAddress, faultMapping) and faultMapping.objectFile.empty()) {
      ProcessMapping returnAddressMapping{};
      if (
        event.stackReturnAddress > 0 and findProcessMapping(event.stackReturnAddress, returnAddressMapping)
        and returnAddressMapping.executable
      ) {
        callers.push_back(event.stackReturnAddress - 1);
      }
    }

    ProcessMapping stackMapping{};
    if (not findProcessMapping(event.stackPointer, stackMapping) or not stackMapping.readable) {
      return callers;
    }

    auto framePointer = event.framePointer;
    while (callers.size() < MaximumNumberOfCallers and containsReadableWords(stackMapping, framePointer, 2)) {
      const auto* const frame         = reinterpret_cast<const std::uintptr_t*>(framePointer);
      const auto        nextFrame     = frame[0];
      const auto        returnAddress = frame[1];
      ProcessMapping    returnMapping{};
      if (returnAddress > 0 and findProcessMapping(returnAddress, returnMapping) and returnMapping.executable) {
        const auto callerAddress = returnAddress - 1;
        if (callers.empty() or callers.back() != callerAddress) {
          callers.push_back(callerAddress);
        }
      }
      if (nextFrame <= framePointer) {
        break;
      }
      framePointer = nextFrame;
    }
    return callers;
  }

  std::string toString(const tarch::FloatingPointExceptions::ExceptionMask exceptions) {
    std::string result;
    const auto  append = [&result](const char* const entry) -> void {
      if (not result.empty()) {
        result += ", ";
      }
      result += entry;
    };

    if (hasException(exceptions, tarch::FloatingPointExceptions::DivisionByZero)) {
      append("division-by-zero");
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Invalid)) {
      append("invalid");
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Overflow)) {
      append("overflow");
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Underflow)) {
      append("underflow");
    }
    if (hasException(exceptions, tarch::FloatingPointExceptions::Inexact)) {
      append("inexact");
    }
    return result;
  }

  void reportFloatingPointEvent(const FloatingPointEvent& event) {
    const auto faultLocation = describeCodeLocation(event.instructionAddress);

    std::ostringstream report;
    report << "Fatal floating-point exception";
    if (not faultLocation.sourceFile.empty()) {
      report << " in (" << faultLocation.sourceFile << ":" << faultLocation.sourceLine << ")";
    }
    report << "\nException=" << toString(event.exceptions) << "\nFault=" << faultLocation.instruction;
    if (not faultLocation.sourceFile.empty()) {
      report << " [" << faultLocation.sourceFile << ":" << faultLocation.sourceLine << "]";
    }
    if (not faultLocation.functionName.empty()) {
      report << "\nFunction=" << faultLocation.functionName;
    }
    if (not faultLocation.additionalFrames.empty()) {
      report << "\nInlinedFrom=" << faultLocation.additionalFrames;
    }

    const auto callerAddresses = collectCallerAddresses(event);
    for (std::size_t callerIndex = 0; callerIndex < callerAddresses.size(); callerIndex++) {
      const auto callerLocation = describeCodeLocation(callerAddresses[callerIndex]);
      report << (callerIndex == 0 ? "\nCallers=" : "; ") << "#" << callerIndex + 1 << " " << callerLocation.instruction;
      if (not callerLocation.sourceFile.empty()) {
        report << " [" << callerLocation.sourceFile << ":" << callerLocation.sourceLine << "]";
      }
      if (not callerLocation.functionName.empty()) {
        report << " " << callerLocation.functionName;
      }
    }
    if (not faultLocation.sourceFile.empty()) {
      report << "\nSource=" << faultLocation.sourceFile << ":" << faultLocation.sourceLine;
    } else {
      report << "\nSource=unresolved";
    }
    report << "\n";
    const auto output = report.str();
    static_cast<void>(std::fwrite(output.data(), 1, output.size(), stderr));
    std::fflush(stderr);
  }

  ALIGN_STACK_ON_ENTRY void terminateAfterFloatingPointSignal() noexcept {
    try {
      reportFloatingPointEvent(trappedEvent);
    } catch (...) {
      static constexpr char FailureMessage[] = "Fatal floating-point exception reporting failure.\n";
      writeSignalMessage(FailureMessage, sizeof(FailureMessage) - 1);
    }
    tarch::mpi::abort(128 + SIGFPE);
  }

  bool maskExceptionTrapsInSignalContext(void* const context) noexcept {
    if (context == nullptr) {
      return false;
    }

    auto* const signalContext        = static_cast<ucontext_t*>(context);
    auto*       floatingPointContext = signalContext->uc_mcontext.fpregs;
#if defined(__x86_64__)
    if (floatingPointContext == nullptr) {
      floatingPointContext = &signalContext->__fpregs_mem;
    }
#endif
    if (floatingPointContext == nullptr) {
      return false;
    }

    constexpr unsigned int SseExceptionStatusBits = 0x3FU;
    constexpr unsigned int SseExceptionMaskShift  = 7U;

    const auto nativeExceptions = toNativeExceptionMask(tarch::FloatingPointExceptions::AllExceptions);
    floatingPointContext->cwd |= static_cast<decltype(floatingPointContext->cwd)>(nativeExceptions);
    floatingPointContext->swd &= static_cast<decltype(floatingPointContext->swd)>(~nativeExceptions);
    floatingPointContext->mxcsr &= ~SseExceptionStatusBits;
    floatingPointContext->mxcsr |=
      static_cast<decltype(floatingPointContext->mxcsr)>(nativeExceptions << SseExceptionMaskShift);
    return true;
  }

  [[noreturn]] void raiseWithDefaultSignalAction(const int signalNumber) noexcept {
    struct sigaction signalAction{};
    signalAction.sa_handler = SIG_DFL;
    sigemptyset(&signalAction.sa_mask);
    static_cast<void>(sigaction(signalNumber, &signalAction, nullptr));

    sigset_t signalSet;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, signalNumber);
    static_cast<void>(sigprocmask(SIG_UNBLOCK, &signalSet, nullptr));
    static_cast<void>(raise(signalNumber));
    std::_Exit(128 + signalNumber);
  }

  void invokePreviousSignalHandler(
    const int        signalNumber,
    siginfo_t* const signalInformation,
    void* const      context
  ) noexcept {
    const auto signalAction = previousFloatingPointSignalAction;
    if (previousSignalHandlerWasReset != 0 or signalAction.sa_handler == SIG_DFL) {
      raiseWithDefaultSignalAction(signalNumber);
    }
    if (signalAction.sa_handler == SIG_IGN) {
      return;
    }

    sigset_t originalSignalMask;
    static_cast<void>(sigprocmask(SIG_SETMASK, nullptr, &originalSignalMask));
    auto delegatedSignalMask = originalSignalMask;
    for (int blockedSignal = 1; blockedSignal < NSIG; blockedSignal++) {
      if (sigismember(&signalAction.sa_mask, blockedSignal) == 1) {
        sigaddset(&delegatedSignalMask, blockedSignal);
      }
    }
    if ((signalAction.sa_flags & SA_NODEFER) != 0) {
      sigdelset(&delegatedSignalMask, signalNumber);
    } else {
      sigaddset(&delegatedSignalMask, signalNumber);
    }
    static_cast<void>(sigprocmask(SIG_SETMASK, &delegatedSignalMask, nullptr));

    if ((signalAction.sa_flags & SA_RESETHAND) != 0) {
      previousSignalHandlerWasReset = 1;
    }
    if ((signalAction.sa_flags & SA_SIGINFO) != 0) {
      signalAction.sa_sigaction(signalNumber, signalInformation, context);
    } else {
      signalAction.sa_handler(signalNumber);
    }
    static_cast<void>(sigprocmask(SIG_SETMASK, &originalSignalMask, nullptr));
  }

  void handleFloatingPointSignal(
    const int        signalNumber,
    siginfo_t* const signalInformation,
    void* const      signalContext
  ) noexcept {
    const auto signalledExceptions = toExceptionMask(signalInformation != nullptr ? signalInformation->si_code : 0);
    const auto localExceptions     = static_cast<tarch::FloatingPointExceptions::ExceptionMask>(armedExceptions);

    const auto handledExceptions = localExceptions == tarch::FloatingPointExceptions::None
      ? signalledExceptions
      : signalledExceptions & localExceptions;
    if (signalNumber != SIGFPE or handledExceptions == tarch::FloatingPointExceptions::None) {
      invokePreviousSignalHandler(signalNumber, signalInformation, signalContext);
      return;
    }

    if (getpid() != signalHandlerProcessId) {
      static constexpr char ForkedProcessMessage[] = "Fatal floating-point exception in a forked process.\n";
      writeSignalMessage(ForkedProcessMessage, sizeof(ForkedProcessMessage) - 1);
      std::_Exit(128 + SIGFPE);
    }

    const auto stackPointer = getStackPointer(signalContext);

    trappedEvent = FloatingPointEvent{
      .exceptions         = handledExceptions,
      .instructionAddress = getInstructionAddress(signalContext),
      .stackPointer       = stackPointer,
      .framePointer       = getFramePointer(signalContext),
      .stackReturnAddress = stackPointer == 0 ? 0 : *reinterpret_cast<const std::uintptr_t*>(stackPointer),
    };
    if (not maskExceptionTrapsInSignalContext(signalContext)) {
      static constexpr char ContextFailureMessage[] = "Fatal floating-point exception context failure.\n";
      writeSignalMessage(ContextFailureMessage, sizeof(ContextFailureMessage) - 1);
      std::_Exit(128 + SIGFPE);
    }
    armedExceptions = tarch::FloatingPointExceptions::None;

    // The handler does async-signal-safe work only. The report follows in
    // terminateAfterFloatingPointSignal().
    auto* const machineContext = static_cast<ucontext_t*>(signalContext);
#if defined(__x86_64__)
    machineContext->uc_mcontext.gregs[REG_RIP] =
      static_cast<greg_t>(reinterpret_cast<std::uintptr_t>(&terminateAfterFloatingPointSignal));
#else
    machineContext->uc_mcontext.gregs[REG_EIP] =
      static_cast<greg_t>(reinterpret_cast<std::uintptr_t>(&terminateAfterFloatingPointSignal));
#endif
  }

  void installFloatingPointSignalHandler() {
    struct sigaction signalAction{};
    signalAction.sa_sigaction = handleFloatingPointSignal;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags  = SA_SIGINFO;
    signalHandlerProcessId = static_cast<std::sig_atomic_t>(getpid());
    if (sigaction(SIGFPE, &signalAction, &previousFloatingPointSignalAction) != 0) {
      signalHandlerProcessId = -1;
      return;
    }
    floatingPointSignalHandlerInstalled = true;
  }
#endif
} // namespace

bool
tarch::FloatingPointExceptions::enableExceptionTraps(const ExceptionMask exceptions) noexcept {
#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
  std::call_once(floatingPointSignalHandlerInstallation, installFloatingPointSignalHandler);
  if (not floatingPointSignalHandlerInstalled) {
    return false;
  }

  const auto supportedExceptions = exceptions & AllExceptions;
  const auto nativeExceptions    = toNativeExceptionMask(supportedExceptions);
  if (nativeExceptions == 0) {
    disableExceptionTraps();
    return true;
  }

  std::feclearexcept(FE_ALL_EXCEPT);
  if (feenableexcept(nativeExceptions) == -1) {
    return false;
  }

  armedExceptions = static_cast<ExceptionMask>(armedExceptions) | supportedExceptions;
  return true;
#elif defined(_MSC_VER)
  const auto supportedExceptions = exceptions & AllExceptions;
  const auto nativeExceptions    = toNativeExceptionMask(supportedExceptions);
  if (nativeExceptions == 0U) {
    disableExceptionTraps();
    return true;
  }

  static_cast<void>(_clearfp());
  unsigned int currentControlWord = 0U;
  if (_controlfp_s(&currentControlWord, 0U, 0U) != 0) {
    return false;
  }

  const auto updatedControlWord = currentControlWord & ~nativeExceptions;
  if (_controlfp_s(&currentControlWord, updatedControlWord, _MCW_EM) != 0) {
    return false;
  }
  armedExceptions |= supportedExceptions;
  return true;
#else
  static_cast<void>(exceptions);
  return false;
#endif
}

void
tarch::FloatingPointExceptions::disableExceptionTraps() noexcept {
#if defined(__linux__) and not defined(_MSC_VER) and (defined(__x86_64__) or defined(__i386__))
  fedisableexcept(FE_ALL_EXCEPT);
  armedExceptions = None;
#elif defined(_MSC_VER)
  unsigned int currentControlWord = 0U;
  static_cast<void>(_controlfp_s(&currentControlWord, _MCW_EM, _MCW_EM));
  armedExceptions = None;
#endif
}

tarch::FloatingPointExceptions::ScopedTrap::ScopedTrap(
  const char* const   file,
  const int           line,
  const ExceptionMask exceptions
) :
  _previousExceptions(armedExceptions),
  _previousEnvironment{} {
  if (std::fegetenv(&_previousEnvironment) != 0) {
    terminateAfterTrapSetupFailure(file, line);
  }
  const auto combinedExceptions = _previousExceptions | exceptions;
  if (combinedExceptions != _previousExceptions and not enableExceptionTraps(combinedExceptions)) {
    terminateAfterTrapSetupFailure(file, line);
  }
}

tarch::FloatingPointExceptions::ScopedTrap::~ScopedTrap() {
  std::fesetenv(&_previousEnvironment);
  armedExceptions = _previousExceptions;
}

#else

bool
tarch::FloatingPointExceptions::enableExceptionTraps(const ExceptionMask exceptions) noexcept {
  static_cast<void>(exceptions);
  return false;
}

void
tarch::FloatingPointExceptions::disableExceptionTraps() noexcept {}

tarch::FloatingPointExceptions::ScopedTrap::ScopedTrap(
  const char* const   file,
  const int           line,
  const ExceptionMask exceptions
) :
  _previousExceptions(None),
  _previousEnvironment{} {
  static_cast<void>(file);
  static_cast<void>(line);
  static_cast<void>(exceptions);
}

tarch::FloatingPointExceptions::ScopedTrap::~ScopedTrap() = default;

#endif
