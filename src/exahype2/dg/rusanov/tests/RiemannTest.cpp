// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannTest.h"

#include "exahype2/Enumeration.h"
#include "exahype2/dg/rusanov/Rusanov.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int    Order              = 1;
  constexpr int    Unknowns           = 1;
  constexpr int    AuxiliaryVariables = 0;
  constexpr int    FaceNumber         = 0;
  constexpr int    BufferEntries      = 64;
  constexpr double Unwritten          = -1.0e30;
  constexpr double FluxFactor         = 2.0;
  constexpr double Eigenvalue         = 3.0;

  exahype2::dg::Flux linearFlux() {
    return [](
             const double* NOALIAS                                         Q,
             [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
             [[maybe_unused]] double                                       t,
             [[maybe_unused]] double                                       dt,
             [[maybe_unused]] int                                          normal,
             double* NOALIAS                                               F
           ) { F[0] = FluxFactor * Q[0]; };
  }

  exahype2::dg::MaxEigenvalue constantEigenvalue() {
    return [](
             [[maybe_unused]] const double* NOALIAS                        Q,
             [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
             [[maybe_unused]] double                                       t,
             [[maybe_unused]] double                                       dt,
             [[maybe_unused]] int                                          normal
           ) { return Eigenvalue; };
  }

  /**
   * Runs the pointwise Rusanov solve on one face and returns the number of entries it wrote.
   *
   * The caller fills projectedValues; every entry of solution the solve leaves alone keeps
   * Unwritten, so a test checks exactly the entries the solve produced.
   */
  int solveOneFace(const double* projectedValues, double* solution) {
    const double quadraturePoints[2] = {0.5 - 0.5 / std::sqrt(3.0), 0.5 + 0.5 / std::sqrt(3.0)};

    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    tarch::la::Vector<DIMENSIONS, double>       faceCentre(0.5);
    faceCentre(FaceNumber % DIMENSIONS) = 0.0;

    for (int entry = 0; entry < BufferEntries; entry++) {
      solution[entry] = Unwritten;
    }

    exahype2::dg::rusanov::solveRiemannProblemPointwiseInSitu(
      linearFlux(),
      exahype2::dg::NonConservativeProduct(),
      constantEigenvalue(),
      faceCentre,
      cellSize,
      0.0,
      0.1,
      Order,
      Unknowns,
      AuxiliaryVariables,
      FaceNumber,
      quadraturePoints,
      true,
      false,
      projectedValues,
      solution
    );

    int writtenEntries = 0;
    for (int entry = 0; entry < BufferEntries; entry++) {
      if (solution[entry] != Unwritten) {
        writtenEntries++;
      }
    }
    return writtenEntries;
  }
} // namespace

exahype2::dg::rusanov::tests::RiemannTest::RiemannTest() :
  TestCase("exahype2::dg::rusanov::tests::RiemannTest") {}

void exahype2::dg::rusanov::tests::RiemannTest::run() {
  testMethod(testSolveRiemannProblemForEqualStates);
  testMethod(testSolveRiemannProblemAddsJumpTerm);
}

void exahype2::dg::rusanov::tests::RiemannTest::testSolveRiemannProblemForEqualStates() {
  const double state = 0.75;
  double       projectedValues[BufferEntries];
  double       solution[BufferEntries];
  for (int entry = 0; entry < BufferEntries; entry++) {
    projectedValues[entry] = state;
  }

  const int writtenEntries = solveOneFace(projectedValues, solution);
  validateWithParams1(writtenEntries > 0, writtenEntries);
  for (int entry = 0; entry < BufferEntries; entry++) {
    if (solution[entry] != Unwritten) {
      validateNumericalEqualsWithParams2(solution[entry], FluxFactor * state, entry, solution[entry]);
    }
  }
}

void exahype2::dg::rusanov::tests::RiemannTest::testSolveRiemannProblemAddsJumpTerm() {
  const double leftState  = 0.25;
  const double rightState = 1.25;
  const int    normal     = FaceNumber % DIMENSIONS;

  double projectedValues[BufferEntries];
  double solution[BufferEntries];
  for (int entry = 0; entry < BufferEntries; entry++) {
    projectedValues[entry] = leftState;
  }
  peano4::dForEachExclusive(Order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    tarch::la::Vector<DIMENSIONS, int> rightDoF = dof;
    rightDoF(normal)                            = 1;
    projectedValues
      [static_cast<std::size_t>(exahype2::serialiseVoxelIndexInOverlap(rightDoF, Order + 1, 1, normal))
       * (Unknowns + AuxiliaryVariables)] = rightState;
  });

  const int    writtenEntries = solveOneFace(projectedValues, solution);
  const double expected = 0.5 * FluxFactor * (leftState + rightState) - 0.5 * Eigenvalue * (rightState - leftState);
  validateWithParams1(writtenEntries > 0, writtenEntries);
  for (int entry = 0; entry < BufferEntries; entry++) {
    if (solution[entry] != Unwritten) {
      validateNumericalEqualsWithParams2(solution[entry], expected, entry, solution[entry]);
    }
  }
}
