// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Info.h"

void
tarch::instrumentation::Log::infoWithScopeName(std::string_view scopeName, std::string_view message) {
  const auto context = createContext();
  // A redirected stdout buffers fully, so the last argument flushes every info line.
  writeMessage(
    "Info",
    context.timestampNanoseconds,
    context.rank,
    context.threadId,
    scopeName,
    message,
    ConsoleStream::Cout,
    true
  );
}
