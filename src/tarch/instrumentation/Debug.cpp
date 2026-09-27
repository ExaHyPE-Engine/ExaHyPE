// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Debug.h"

void
tarch::instrumentation::Log::debugWithScopeName(
  [[maybe_unused]] std::string_view scopeName,
  [[maybe_unused]] std::string_view message
) {
#if PEANO_DEBUG
  const auto context = createContext();
  writeMessage(
    "Debug",
    context.timestampNanoseconds,
    context.rank,
    context.threadId,
    scopeName,
    message,
    ConsoleStream::Cout,
    false
  );
#endif
}
