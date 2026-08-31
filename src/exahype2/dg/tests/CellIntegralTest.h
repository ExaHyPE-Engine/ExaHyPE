// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class CellIntegralTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::CellIntegralTest final : public tarch::tests::TestCase {
public:
  CellIntegralTest();

  void run() override;

private:
  const double _orderFourQuadraturePoints1D[4 + 1];
  const double _orderFourMassMatrixDiagonal1D[4 + 1];
  const double _orderFourStiffnessOperator1D[(4 + 1) * (4 + 1)];
  const double _orderFourDerivativeOperator1D[(4 + 1) * (4 + 1)];

  const double _orderTwoQuadraturePoints1D[2 + 1];
  const double _orderTwoMassMatrixDiagonal1D[2 + 1];
  const double _orderTwoStiffnessOperator1D[(2 + 1) * (2 + 1)];
  const double _orderTwoDerivativeOperator1D[(2 + 1) * (2 + 1)];

  void runEulerOrder2OnStationarySetup();
  void runEulerOrder4OnStationarySetup();
  void runReduceMaxScaledEigenvalue();
};
