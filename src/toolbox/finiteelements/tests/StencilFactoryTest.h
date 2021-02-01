// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_FINITEELEMEMTS_TESTS_STENCIL_FACTORY_TEST_H_
#define _TOOLBOX_FINITEELEMEMTS_TESTS_STENCIL_FACTORY_TEST_H_

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
    void run() override;
};


#endif
