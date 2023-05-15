// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"

#include <cmath>
#include <iostream>
#include <fstream>

namespace exahype2 {
  namespace dg {
    namespace tests {
      class RiemannTest;
    }
  }
}


class exahype2::dg::tests::RiemannTest : public tarch::tests::TestCase {
  private:
    /**
     * Testing projection to faces by creating fully constant
     * cell and ensuring that the
     * projected values are also the same constant.
     * We construct a cell patch which hosts one unknown and one
     * auxiliary variable per degree of freedom. The unknown is
     * set to 1 and the auxiliary variable to -1. We then project
     * onto the 2d faces and look at their values. All is hard-coded
     * to order 3, i.e. we need four quadrature points per coordinate
     * axis.
     *
     * For the left face of the cell, we have
     *
     *         ... | ...         ... | ...
     *         4,5 | 6,7    ->   0,0 | 1,-1
     *         0,1 | 2,3         0,0 | 1,-1
     *
     * where the comma separate the unknown from the auxiliary variable.
     * For the right face, we have the same enumeration, but the values
     * are mirrored along the vertical line.
     *
     * For the bottom face, we have the enumeration
     *
     *          8,9   10,11  12,13  14,15
     *         --------------------------
     *          0,1    2,3    4,5    6,7
     *
     * which translates into values
     *
     *         1,-1   1,-1   1,-1   1,-1
     *         --------------------------
     *          0,0    0,0    0,0    0,0
     *
     * In a 3d setup, both patterns are repeated in the z-direction, so
     * the same pattern restarts at index 16.
     *
     * The 3d case means, that the 4*4*2=32 first entries of the left size are
     * all 0. The remainig entries then exhibit the 1,-1 pattern.
     *
     */
    void testProjectVolumetricDataOntoFacesForConstantSolutionOrder3();

    /**
     * Projection that's used if DG degenerates to a Finite Volumes scheme
     * for the Euler equation, i.e. for 5 unknowns.
     */
    void testProjectVolumetricDataOntoFacesForConstantSolutionOrder0();

  public:
    RiemannTest();

    virtual ~RiemannTest() = default;

    virtual void run() override;
};


