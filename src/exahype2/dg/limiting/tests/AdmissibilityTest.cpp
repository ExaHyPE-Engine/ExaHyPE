// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AdmissibilityTest.h"

#include "exahype2/dg/limiting/Admissibility.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"

namespace {
  using Solver = exahype2::dg::limiting::tests::TestSolver<true>;

  constexpr int NumberOfDofsPerAxis     = Solver::NumberOfDofsPerAxis;
  constexpr int NumberOfSubcellsPerAxis = Solver::NumberOfSubcellsPerAxis;
  constexpr int NumberOfNodes           = Solver::NumberOfNodes;

  constexpr double DgToLobattoMatrix[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{1.0, 0.0, 0.0, 1.0};
  // The middle column drops both nodes, so the middle sample of each axis is zero.
  constexpr double DgToSubcellMatrix[NumberOfDofsPerAxis * NumberOfSubcellsPerAxis]{1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

  void mapFirstUnknown(const double* const state, double* const observables) { observables[0] = state[0]; }
} // namespace

exahype2::dg::limiting::tests::AdmissibilityTest::AdmissibilityTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::AdmissibilityTest") {}

void exahype2::dg::limiting::tests::AdmissibilityTest::run() {
  testMethod(testRelaxationTermReplacesTheSpecifiedParameter);
  testMethod(testSamplingExposesExtremaBetweenTheNodes);
  testMethod(testCandidateInsideTheReferenceRangeIsAccepted);
  testMethod(testCandidateOutsideTheReferenceRangeIsRejected);
  testMethod(testRelaxationWidensTheAcceptedRange);
}

/**
 * Checks that a solver's relaxation term decides the relaxation of one observable.
 */
void exahype2::dg::limiting::tests::AdmissibilityTest::testRelaxationTermReplacesTheSpecifiedParameter() {
  validateNumericalEquals((exahype2::dg::limiting::relaxationParameterOfObservable<Solver, false>(0, 0.25)), 0.25);
  validateNumericalEquals((exahype2::dg::limiting::relaxationParameterOfObservable<Solver, true>(0, 0.25)), 0.5);
}

/**
 * Checks that the projected samples widen the extrema beyond the range of the storage nodes.
 */
void exahype2::dg::limiting::tests::AdmissibilityTest::testSamplingExposesExtremaBetweenTheNodes() {
  double nodalState[NumberOfNodes];
  for (int node = 0; node < NumberOfNodes; node++) {
    nodalState[node] = static_cast<double>(node + 1);
  }
  double minimum[1];
  double maximum[1];

  exahype2::dg::limiting::reduceSampledObservableExtrema<1, NumberOfDofsPerAxis, NumberOfSubcellsPerAxis, 1>(
    minimum,
    maximum,
    nodalState,
    DgToLobattoMatrix,
    DgToSubcellMatrix,
    mapFirstUnknown
  );

  validateNumericalEquals(minimum[0], 0.0);
  validateNumericalEquals(maximum[0], static_cast<double>(NumberOfNodes));
}

/**
 * Checks that a candidate range within the reference range passes the principle.
 */
void exahype2::dg::limiting::tests::AdmissibilityTest::testCandidateInsideTheReferenceRangeIsAccepted() {
  const double candidateMinimum[2]{1.0, 2.0};
  const double candidateMaximum[2]{9.0, 8.0};
  const double referenceMinimum[2]{0.0, 0.0};
  const double referenceMaximum[2]{10.0, 10.0};

  validateTrue((exahype2::dg::limiting::satisfiesRelaxedMaximumPrinciple<Solver, 2, false>(
    candidateMinimum,
    candidateMaximum,
    referenceMinimum,
    referenceMaximum,
    0.0,
    0.0
  )));
}

/**
 * Checks that one observable outside the reference range rejects the whole candidate.
 */
void exahype2::dg::limiting::tests::AdmissibilityTest::testCandidateOutsideTheReferenceRangeIsRejected() {
  const double candidateMinimum[2]{1.0, -0.5};
  const double candidateMaximum[2]{9.0, 8.0};
  const double referenceMinimum[2]{0.0, 0.0};
  const double referenceMaximum[2]{10.0, 10.0};

  validateFalse((exahype2::dg::limiting::satisfiesRelaxedMaximumPrinciple<Solver, 2, false>(
    candidateMinimum,
    candidateMaximum,
    referenceMinimum,
    referenceMaximum,
    0.0,
    0.0
  )));
}

/**
 * Checks that the relaxation term accepts a candidate the specified parameter alone rejects.
 */
void exahype2::dg::limiting::tests::AdmissibilityTest::testRelaxationWidensTheAcceptedRange() {
  const double     candidateMinimum[1]{-0.5};
  const double     candidateMaximum[1]{9.0};
  const double     referenceMinimum[1]{0.0};
  const double     referenceMaximum[1]{10.0};
  constexpr double SpecifiedRelaxation = 0.25;

  validateFalse((exahype2::dg::limiting::satisfiesRelaxedMaximumPrinciple<Solver, 1, false>(
    candidateMinimum,
    candidateMaximum,
    referenceMinimum,
    referenceMaximum,
    SpecifiedRelaxation,
    0.0
  )));
  validateTrue((exahype2::dg::limiting::satisfiesRelaxedMaximumPrinciple<Solver, 1, true>(
    candidateMinimum,
    candidateMaximum,
    referenceMinimum,
    referenceMaximum,
    SpecifiedRelaxation,
    0.0
  )));
}
