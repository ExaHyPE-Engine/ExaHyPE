// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include "../DGUtils.h"
#include "../CellIntegral.h"
#include "../Riemann.h"
#include "../rusanov/Riemann.h"
#include "../BoundaryConditions.h"
#include "testUtils.h"
#include "exahype2/CellData.h"


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
