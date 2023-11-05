// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace fv {
    namespace tests {
      class InterpolationRestrictionTest;
    }
  }
}


class exahype2::fv::tests::InterpolationRestrictionTest : public tarch::tests::TestCase {
  private:
    void testPiecewiseConstantInterpolationWithTensorProduct1();

    void testPiecewiseConstantInterpolationWithTensorProduct2();

    /**
     * Simple test that Han and me set up for FD4 with SSInfall. We use the
     * following operators:
     *
static constexpr double TangentialRestrictionMatrix1d[] = {
  0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 ,0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};

static constexpr double NormalRestrictionMatrix1d[] = {
  0.0, 0.0, 0.0, 0.3333333333333333, 0.3333333333333333, 0.3333333333333333,
  0.0, 0.0, 0.0, 0.0, -2.0, 3.0,
  0.0, 0.0, 0.0, 0.0, -5.0, 6.0};
     *
     * This is some kind of injection along the normal, and averaging along
     * the normal with some extrapolation. These operators did yield zero or
     * negative densities in the Euler SSInfall setup with a patch size of 5:
     *
     *
assertion in file observers/TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0.cpp, line 265 failed: coarseGridFacesSelfSimilarInfallFD4QUpdate(marker.getSelectedFaceNumber()).value[i*5+0] > 0.0
parameter coarseGridFacesSelfSimilarInfallFD4QUpdate(marker.getSelectedFaceNumber()).value[i*5+0]: 0.00000000000000000000e+00
parameter i: 2
parameter marker.toString(): (x=[0.185185,0.185185,0.37037],h=[0.037037,0.037037,0.037037],select=5,is-cell-local=1,is-face-local=1,has-face-been-refined=0,will-face-be-refined=0,rel-pos=[0,0,2])
benchmark-plot-3-no-opt-asserts-FD4-5: observers/TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0.cpp:265: void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR0::destroyHangingFace(const peano4::datamanagement::FaceMarker &, benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld &, benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew &, benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate &, benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel &, peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>, peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>, peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>, peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>, benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q &, benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates &, benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel &): Assertion `false' failed.
Aborted (core dumped)
     *
     */
    void testInjectionExtrapolationRestrictionWithTensorProduct();

    /**
     * Simple averaging.
     *
     * I test a patch size of 5, work with an overlap of 3 and assume that
     * we have two unknowns per finite differences point.
     *
     * The tangential restriction matrix thus is a 5 times 15 matrix, as we
     * take the five volumes that are tangential to a face in one direction.
     * We assume that the normal points in the x-direction and we speak about
     * the right face of a cell, i.e. the face index equals Dimensions.
     *
     * As we restrict, the right half of the fine grid face (inside) has to
     * be restricted to the right half of the coarse grid face (outside).
     */
    void testAverageRestrictionWithTensorProduct();

  public:
    InterpolationRestrictionTest();
    
    virtual void run();
};
