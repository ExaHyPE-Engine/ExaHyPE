// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

namespace tarch {
  struct StackSizeLimit {
    bool               detected  = false;
    bool               unlimited = false;
    unsigned long long bytes     = 0;

    static StackSizeLimit detect();
  };

  std::string detectStackSizeLimit();
} // namespace tarch
