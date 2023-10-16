// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace dg {
    namespace tests {
      class CellIntegralTest;
    }
  }
}

class exahype2::dg::tests::CellIntegralTest : public tarch::tests::TestCase {
  private:
    const double _Order4_QuadraturePoints1d[4+1];
    const double _Order4_MassMatrixDiagonal1d[4+1];
    const double _Order4_StiffnessOperator1d[(4+1)*(4+1)];
    const double _Order4_DerivativeOperator1d[(4+1)*(4+1)];
    const double _Order4_BasisFunctionValuesLeft1d[4+1];

    const double _Order2_QuadraturePoints1d[2+1];
    const double _Order2_MassMatrixDiagonal1d[2+1];
    const double _Order2_StiffnessOperator1d[(2+1)*(2+1)];
    const double _Order2_DerivativeOperator1d[(2+1)*(2+1)];

    /**
     * I initialise all velocity data with zero, and set density to 1.
     * First, we set the inner energy to zero. After that, we set it to 4.
     *
     * One might think that the result should always be 0 for all components,
     * but the result is not zero if the inner energy is positive. You get
     * a zero update for the density, and you get a zero update for the
     * energy. The velocity however becomes star shaped, i.e.
     * looks like matter pressed out from the centre.
     *
     * This behaviour is due to the internal energy pushing matter out of the
     * domain. If the inner energy equals zero, then nothing moves at all.
     */
    void runEulerOrder2OnStationarySetup();
    void runEulerOrder4OnStationarySetup();
    //void runDGEuler();
    //void runDGElastic();

  public:
    CellIntegralTest();

    virtual ~CellIntegralTest() = default;

    virtual void run() override;
};
