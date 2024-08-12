// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"


namespace peano4 {
  namespace utils {
    namespace tests {
      class ParallelDForTest;
    }
  }
}


class peano4::utils::tests::ParallelDForTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testParallelDFor();

  public:
    ParallelDForTest();
    virtual void run() override;
};


