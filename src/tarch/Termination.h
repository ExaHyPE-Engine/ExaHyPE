// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  [[noreturn]] void terminateWithError(
    const std::string_view     message,
    const int                  errorCode = -1,
    const std::source_location location  = std::source_location::current()
  );
} // namespace tarch
