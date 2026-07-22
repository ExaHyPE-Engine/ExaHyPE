// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace dg {
    namespace tests {
      class CellIntegralTest;
    }
  } // namespace dg
} // namespace exahype2

class exahype2::dg::tests::CellIntegralTest : public tarch::tests::TestCase {
private:
  const double _Order4_QuadraturePoints1d[4 + 1];
  const double _Order4_MassMatrixDiagonal1d[4 + 1];
  const double _Order4_StiffnessOperator1d[(4 + 1) * (4 + 1)];
  const double _Order4_DerivativeOperator1d[(4 + 1) * (4 + 1)];

  const double _Order2_QuadraturePoints1d[2 + 1];
  const double _Order2_MassMatrixDiagonal1d[2 + 1];
  const double _Order2_StiffnessOperator1d[(2 + 1) * (2 + 1)];
  const double _Order2_DerivativeOperator1d[(2 + 1) * (2 + 1)];

  /**
   * Run the order-2 Euler setup that stays stationary for a vanishing inner energy.
   */
  void runEulerOrder2OnStationarySetup();
  void runEulerOrder4OnStationarySetup();
  void runReduceMaxScaledEigenvalue();

public:
  CellIntegralTest();

  virtual ~CellIntegralTest() = default;

  virtual void run() override;
};
