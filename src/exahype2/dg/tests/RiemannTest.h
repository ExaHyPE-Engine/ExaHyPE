// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace dg {
    namespace tests {
      class RiemannTest;
    }
  } // namespace dg
} // namespace exahype2

class exahype2::dg::tests::RiemannTest : public tarch::tests::TestCase {
private:
  /**
   * Project a constant order-3 solution onto the faces.
   */
  void testProjectVolumetricDataOntoFacesForConstantSolutionOrder3();

  /**
   * Project a constant solution onto the faces for the order-0 Euler setup.
   */
  void testProjectVolumetricDataOntoFacesForConstantSolutionOrder0();

  /**
   * The coarse-to-fine trace interpolation reproduces the coarse polynomial.
   */
  void testInterpolateProjectedFacePolynomialReproducesPolynomials();

  /**
   * The fine-to-coarse trace restriction reproduces the coarse polynomial.
   */
  void testRestrictAndAccumulateProjectedFacePolynomialReproducesPolynomials();

  /**
   * The transition flux a fine cell consumes reproduces the coarse polynomial.
   */
  void testInterpolateRiemannSolutionReproducesPolynomials();

public:
  RiemannTest();

  virtual ~RiemannTest() = default;

  virtual void run() override;
};
