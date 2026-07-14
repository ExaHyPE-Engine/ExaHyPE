// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FloatingPointExceptions.h"

#if defined(PEANO_WITH_FPE)

#include "tarch/NonCriticalAssertions.h"

#if defined(__linux__) && !defined(_MSC_VER)
#include <cfenv>
#include <csignal>
#include <cxxabi.h>
#include <dlfcn.h>
#include <fenv.h>
#include <sys/ucontext.h>
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#include <float.h>
#endif

namespace {
  constexpr bool hasException(
    tarch::FloatingPointExceptions::ExceptionMask candidate,
    tarch::FloatingPointExceptions::Exception     exception
  ) noexcept {
    return (candidate & static_cast<tarch::FloatingPointExceptions::ExceptionMask>(exception)) != 0u;
  }

#if defined(__linux__) && !defined(_MSC_VER)
  int toNativeExceptionMask(tarch::FloatingPointExceptions::ExceptionMask exceptions) noexcept {
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

  tarch::FloatingPointExceptions::ExceptionMask toExceptionMask(int signalCode) noexcept {
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
    default:
      return static_cast<ExceptionMask>(tarch::FloatingPointExceptions::None);
    }
  }

  std::once_flag                                             fpeHandlerInstalled;
  thread_local tarch::FloatingPointExceptions::ExceptionMask armedExceptions   = tarch::FloatingPointExceptions::None;
  thread_local volatile std::sig_atomic_t                    trappedExceptions = 0;
  thread_local void*                                         trappedInstructionPointer = nullptr;
  thread_local void*                                         trappedAddress            = nullptr;
#elif defined(_MSC_VER)
  unsigned int toNativeExceptionMask(tarch::FloatingPointExceptions::ExceptionMask exceptions) noexcept {
    unsigned int result = 0;

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

  std::string toString(tarch::FloatingPointExceptions::ExceptionMask exceptions) {
    if (exceptions == tarch::FloatingPointExceptions::None) {
      return "none";
    }

    std::string result;
    auto        append = [&result](const char* entry) -> void {
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

#if defined(__linux__) && !defined(_MSC_VER)
  void* getInstructionPointer(void* context) noexcept {
#if defined(__x86_64__) || defined(__i386__)
    if (context == nullptr) {
      return nullptr;
    }

    auto* signalContext = reinterpret_cast<ucontext_t*>(context);

#if defined(__x86_64__)
    return reinterpret_cast<void*>(signalContext->uc_mcontext.gregs[REG_RIP]);
#else
    return reinterpret_cast<void*>(signalContext->uc_mcontext.gregs[REG_EIP]);
#endif
#else
    static_cast<void>(context);
    return nullptr;
#endif
  }

  std::string demangleSymbolName(const char* symbolName) {
    if (symbolName == nullptr) {
      return "";
    }

    int   status          = 0;
    char* demangledSymbol = abi::__cxa_demangle(symbolName, nullptr, nullptr, &status);
    if (status == 0 && demangledSymbol != nullptr) {
      std::string result(demangledSymbol);
      std::free(demangledSymbol);
      return result;
    }

    std::free(demangledSymbol);
    return symbolName;
  }

  std::string trimTrailingWhitespace(std::string value) {
    while (
      not value.empty()
      and (value.back() == '\n' or value.back() == '\r' or value.back() == ' ' or value.back() == '\t')
    ) {
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
    for (const char ch : value) {
      if (ch == '\'') {
        result += "'\\''";
      } else {
        result += ch;
      }
    }
    result += "'";
    return result;
  }

  struct ProcMapsEntry {
    std::string    objectFile;
    std::uintptr_t objectRelativeAddress = 0;

    bool isValid() const noexcept { return not objectFile.empty(); }
  };

  ProcMapsEntry describeCodeLocationFromProcMaps(const void* address) {
    ProcMapsEntry result;
    if (address == nullptr) {
      return result;
    }

    std::ifstream maps("/proc/self/maps");
    if (not maps.good()) {
      return result;
    }

    const auto  instructionAddress = reinterpret_cast<std::uintptr_t>(address);
    std::string line;
    while (std::getline(maps, line)) {
      unsigned long long mappingStart       = 0;
      unsigned long long mappingEnd         = 0;
      unsigned long long fileOffset         = 0;
      int                parsedPrefixLength = 0;

      if (std::sscanf(
            line.c_str(),
            "%llx-%llx %*4s %llx %*s %*s %n",
            &mappingStart,
            &mappingEnd,
            &fileOffset,
            &parsedPrefixLength
          )
          != 3) {
        continue;
      }

      if (instructionAddress < mappingStart || instructionAddress >= mappingEnd) {
        continue;
      }

      std::string objectFile = trimLeadingWhitespace(line.substr(parsedPrefixLength));
      if (objectFile.empty() || objectFile.front() == '[') {
        continue;
      }

      result.objectFile            = objectFile;
      result.objectRelativeAddress = static_cast<std::uintptr_t>(instructionAddress - mappingStart + fileOffset);
      return result;
    }

    return result;
  }

  std::string describeCodeLocationWithAddr2Line(const char* objectFile, std::uintptr_t address) {
    if (objectFile == nullptr || objectFile[0] == '\0') {
      return "";
    }

    std::ostringstream command;
    command
      << "addr2line -C -f -p -e " << quoteForShell(objectFile) << " 0x" << std::hex << address << std::dec
      << " 2>/dev/null";

    FILE* pipe = popen(command.str().c_str(), "r");
    if (pipe == nullptr) {
      return "";
    }

    std::array<char, 1024> buffer{};
    std::string            result;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      result += buffer.data();
    }

    pclose(pipe);
    result = trimTrailingWhitespace(result);

    if (result.empty() or result == "?? ??:0" or result == "?? at ??:?") {
      return "";
    }

    return result;
  }

  std::string describeCodeLocation(const void* address) {
    std::ostringstream result;
    result << address;

    if (address == nullptr) {
      return result.str();
    }

    const auto     instructionAddress    = reinterpret_cast<std::uintptr_t>(address);
    const char*    objectFile            = nullptr;
    std::uintptr_t objectRelativeAddress = instructionAddress;
    std::string    resolvedObjectFile;

    const ProcMapsEntry procMapsEntry = describeCodeLocationFromProcMaps(address);
    if (procMapsEntry.isValid()) {
      resolvedObjectFile    = procMapsEntry.objectFile;
      objectFile            = resolvedObjectFile.c_str();
      objectRelativeAddress = procMapsEntry.objectRelativeAddress;
    }

    Dl_info symbolInformation{};
    if (dladdr(address, &symbolInformation) != 0) {
      if (resolvedObjectFile.empty() && symbolInformation.dli_fname != nullptr) {
        resolvedObjectFile = symbolInformation.dli_fname;
        objectFile         = resolvedObjectFile.c_str();
      }

      if (symbolInformation.dli_sname != nullptr) {
        result << ", symbol " << demangleSymbolName(symbolInformation.dli_sname);

        if (symbolInformation.dli_saddr != nullptr) {
          const auto symbolAddress = reinterpret_cast<std::uintptr_t>(symbolInformation.dli_saddr);

          result << "+0x" << std::hex << (instructionAddress - symbolAddress) << std::dec;
        }
      }

      if (objectRelativeAddress == instructionAddress && symbolInformation.dli_fbase != nullptr) {
        const auto objectBaseAddress = reinterpret_cast<std::uintptr_t>(symbolInformation.dli_fbase);
        if (instructionAddress >= objectBaseAddress) {
          objectRelativeAddress = instructionAddress - objectBaseAddress;
        }
      }
    }

    if (not resolvedObjectFile.empty()) {
      result << " in " << resolvedObjectFile;
    }

    if (objectFile == nullptr) {
      objectFile = "/proc/self/exe";
    }

    const std::string addr2LineDescription = describeCodeLocationWithAddr2Line(objectFile, objectRelativeAddress);
    if (not addr2LineDescription.empty()) {
      result << " [" << addr2LineDescription << "]";
    }

    return result.str();
  }

  bool maskExceptionTrapsInSignalContext(void* context, int nativeExceptions) noexcept {
#if defined(__x86_64__) || defined(__i386__)
    if (context == nullptr) {
      return false;
    }

    auto* signalContext        = reinterpret_cast<ucontext_t*>(context);
    auto* floatingPointContext = signalContext->uc_mcontext.fpregs;

#if defined(__x86_64__)
    if (floatingPointContext == nullptr) {
      floatingPointContext = &signalContext->__fpregs_mem;
    }
#endif

    if (floatingPointContext == nullptr) {
      return false;
    }

    constexpr unsigned int SseExceptionStatusBits = 0x3fu;
    constexpr unsigned int SseExceptionMaskShift  = 7u;

    floatingPointContext->cwd |= static_cast<decltype(floatingPointContext->cwd)>(nativeExceptions);
    floatingPointContext->swd &= static_cast<decltype(floatingPointContext->swd)>(~nativeExceptions);
    floatingPointContext->mxcsr &= ~SseExceptionStatusBits;
    floatingPointContext->mxcsr |= static_cast<decltype(floatingPointContext->mxcsr)>(
      nativeExceptions << SseExceptionMaskShift
    );

    return true;
#else
    static_cast<void>(context);
    static_cast<void>(nativeExceptions);
    return false;
#endif
  }

  void handleFloatingPointSignal(int signalNumber, siginfo_t* signalInformation, void* context) noexcept {
    if (signalNumber != SIGFPE || armedExceptions == tarch::FloatingPointExceptions::None) {
      std::_Exit(128 + SIGFPE);
    }

    const int nativeExceptions = toNativeExceptionMask(armedExceptions);
    if (trappedExceptions == 0) {
      trappedInstructionPointer = getInstructionPointer(context);
      trappedAddress            = signalInformation != nullptr ? signalInformation->si_addr : nullptr;
      trappedExceptions         = static_cast<std::sig_atomic_t>(
        toExceptionMask(signalInformation != nullptr ? signalInformation->si_code : 0)
      );
      if (trappedExceptions == 0) {
        trappedExceptions = static_cast<std::sig_atomic_t>(armedExceptions);
      }
    }

    if (nativeExceptions == 0 || not maskExceptionTrapsInSignalContext(context, nativeExceptions)) {
      std::_Exit(128 + SIGFPE);
    }

    armedExceptions = tarch::FloatingPointExceptions::None;
  }

  void installFloatingPointSignalHandler() {
    struct sigaction signalAction{};
    signalAction.sa_sigaction = handleFloatingPointSignal;
    signalAction.sa_flags     = SA_SIGINFO;
    sigemptyset(&signalAction.sa_mask);

    sigaction(SIGFPE, &signalAction, nullptr);
  }
#endif
} // namespace

bool tarch::FloatingPointExceptions::areExceptionTrapsSupported() noexcept {
#if defined(__linux__) && !defined(_MSC_VER)
  return true;
#elif defined(_MSC_VER)
  return true;
#else
  return false;
#endif
}

bool tarch::FloatingPointExceptions::enableExceptionTraps(ExceptionMask exceptions) noexcept {
#if defined(__linux__) && !defined(_MSC_VER)
  std::call_once(fpeHandlerInstalled, installFloatingPointSignalHandler);

  const int nativeExceptions = toNativeExceptionMask(exceptions);
  if (nativeExceptions == 0) {
    disableExceptionTraps();
    trappedExceptions         = 0;
    trappedInstructionPointer = nullptr;
    trappedAddress            = nullptr;
    return true;
  }

  trappedExceptions         = 0;
  trappedInstructionPointer = nullptr;
  trappedAddress            = nullptr;
  std::feclearexcept(FE_ALL_EXCEPT);
  if (feenableexcept(nativeExceptions) == -1) {
    armedExceptions = None;
    return false;
  }

  armedExceptions = exceptions;
  return true;
#elif defined(_MSC_VER)
  const unsigned int nativeExceptions = toNativeExceptionMask(exceptions);
  if (nativeExceptions == 0u) {
    return true;
  }

  _clearfp();

  unsigned int currentControlWord = 0u;
  if (_controlfp_s(&currentControlWord, 0u, 0u) != 0) {
    return false;
  }

  const unsigned int updatedControlWord = currentControlWord & ~nativeExceptions;
  return _controlfp_s(&currentControlWord, updatedControlWord, _MCW_EM) == 0;
#else
  static_cast<void>(exceptions);
  return false;
#endif
}

void tarch::FloatingPointExceptions::disableExceptionTraps() noexcept {
#if defined(__linux__) && !defined(_MSC_VER)
  fedisableexcept(FE_ALL_EXCEPT);
  armedExceptions = None;
#elif defined(_MSC_VER)
  unsigned int currentControlWord = 0u;
  _controlfp_s(&currentControlWord, _MCW_EM, _MCW_EM);
#endif
}

tarch::FloatingPointExceptions::ScopedTrap::ScopedTrap(const char* file, int line, ExceptionMask exceptions):
  _file(file),
  _line(line),
  _trapEnabled(false) {
#if defined(__linux__) && !defined(_MSC_VER)
  if (tarch::hasNonCriticalAssertionBeenViolated()) {
    disableExceptionTraps();
    return;
  }

  _trapEnabled = enableExceptionTraps(exceptions);
#else
  static_cast<void>(file);
  static_cast<void>(line);
  static_cast<void>(exceptions);
#endif
}

tarch::FloatingPointExceptions::ScopedTrap::~ScopedTrap() {
#if defined(__linux__) && !defined(_MSC_VER)
  if (_trapEnabled) {
    disableExceptionTraps();
  }

  const auto raisedExceptions = static_cast<ExceptionMask>(trappedExceptions);
  if (raisedExceptions != None) {
    std::ostringstream details;
    details << "Exception=" << ::toString(raisedExceptions);
    if (trappedInstructionPointer != nullptr) {
      details << "; Location=" << describeCodeLocation(trappedInstructionPointer);
    } else if (trappedAddress != nullptr) {
      details << "; Location=" << describeCodeLocation(trappedAddress);
    }

    tarch::triggerNonCriticalAssertion(_file, _line, "FloatingPointException", details.str());

    trappedExceptions         = 0;
    trappedInstructionPointer = nullptr;
    trappedAddress            = nullptr;
  }
#endif
}

#endif
