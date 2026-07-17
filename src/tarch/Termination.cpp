// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Termination.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"

namespace {
  enum class TerminationOutputState { Pending, Writing };
} // namespace

/**
 * Logs the first fatal message with its caller location, flushes pending output, and invokes the MPI-aware abort.
 * One atomic writer prevents concurrent failure paths from interleaving diagnostics or racing the abort; later
 * callers yield until the process ends.
 */
[[noreturn]] void
tarch::terminateWithError(const std::string_view message, const int errorCode, const std::source_location location) {
  static std::atomic<TerminationOutputState> outputState{TerminationOutputState::Pending};
  auto                                       expectedState = TerminationOutputState::Pending;
  if (outputState.compare_exchange_strong(expectedState, TerminationOutputState::Writing)) {
    try {
      std::cout.flush();
      std::cerr.flush();
      const auto scopeName = tarch::instrumentation::Log::makeScopeName(
        location.function_name(),
        location.function_name(),
        location.file_name()
      );
      tarch::instrumentation::Log::errorWithScopeName(scopeName, message);
      tarch::instrumentation::Log::flushBeforeAssertion();
    } catch (...) {
      std::cerr << message << '\n';
      std::cerr.flush();
    }
    tarch::mpi::abort(errorCode);
  }

  while (true) {
    std::this_thread::yield();
  }
}
