// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Warning.h"

void tarch::instrumentation::Log::warningWithScopeName(std::string_view scopeName, std::string_view message) {
  const auto context = createContext();
  writeMessage(
    "Warning",
    context.timestampNanoseconds,
    context.rank,
    context.threadId,
    scopeName,
    message,
    ConsoleStream::Cerr,
    true
  );
}
