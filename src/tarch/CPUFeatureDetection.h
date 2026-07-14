// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  struct CPUFeatures {
    bool sse2   = false;
    bool avx    = false;
    bool avx2   = false;
    bool avx512 = false;
    bool neon   = false;
    bool sve    = false;

    static CPUFeatures detect();
  };

  std::string detectCPUFeatures();
} // namespace tarch
