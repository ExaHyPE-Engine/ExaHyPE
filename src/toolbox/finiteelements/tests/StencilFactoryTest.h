// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


namespace toolbox {
  namespace finiteelements {
    namespace tests {
      class StencilFactoryTest;
    }
  }
}


class toolbox::finiteelements::tests::StencilFactoryTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testIntegrationWithN1();
  public:
    StencilFactoryTest();
    virtual void run() override;
};


