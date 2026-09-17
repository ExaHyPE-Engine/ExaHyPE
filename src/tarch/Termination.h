// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  [[noreturn]] void terminateWithError(
    std::string_view     message,
    int                  errorCode = -1,
    std::source_location location  = std::source_location::current()
  );
} // namespace tarch
