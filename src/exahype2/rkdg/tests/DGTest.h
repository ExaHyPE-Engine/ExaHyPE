// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include "../../rkdg/CellIntegral.h"
#include "../../rkdg/DGUtils.h"
#include "../../rkdg/Riemann.h"
#include "../../rkdg/tests/testUtils.h"

namespace exahype2 {
  namespace dg {
    namespace tests {
      class DGTest;
    }
  }
}


class exahype2::dg::tests::DGTest : public tarch::tests::TestCase {
  private:
    std::ofstream file_stream;
    const double QuadraturePoints[3+1];
    const double QuadratureWeights[3+1];
    const double MassMatrix[(3+1)*(3+1)];
    const double StiffnessOperator[(3+1)*(3+1)];
    const double DerivativeOperator[(3+1)*(3+1)];
    const double BasisFunctionValuesLeft[(3+1)];
    const double BasisFunctionValuesRight[(3+1)];

    void UtilsTest();
    void runDGEuler();
    void runDGElastic();

  public:
    DGTest();

    virtual ~DGTest() = default;

    void run() override;
};
