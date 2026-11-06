// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Error.h"

void
tarch::instrumentation::Log::errorWithScopeName(std::string_view scopeName, std::string_view message) {
  const auto context = createContext();
  writeMessage(
    "Error",
    context.timestampNanoseconds,
    context.rank,
    context.threadId,
    scopeName,
    message,
    ConsoleStream::Cerr,
    true
  );
}
