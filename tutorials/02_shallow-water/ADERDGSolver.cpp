// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The function bodies are compilable stubs of the exercises in ShallowWater.ipynb. With the stubs,
// Q keeps its initial state. The documentation pages "Part 2 - Solution to Exercise 1" and
// "Part 2 - Solution to Exercise 2" contain the solutions.
//
// Q.h contains the water height, Q.hu the momentum h*u and Q.b the bathymetry.
#include "ADERDGSolver.h"

void
tutorials::exahype2::swe::ADERDGSolver::initialCondition(
  State&                   Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 1: Set up the dam-break initial state. The stub writes flat water at rest.
  Q   = {};
  Q.h = 1.0;
}

void
tutorials::exahype2::swe::ADERDGSolver::boundaryConditions(
  const State&             Qinside,
  State&                   Qoutside,
  const Coordinates&       faceCentre,
  const Coordinates&       cellSize,
  const double             time,
  const int                normal,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 1: Prescribe the outside state. The stub copies the inside state as a crude
  // outflow condition.
  Qoutside = Qinside;
}

::exahype2::RefinementCommand
tutorials::exahype2::swe::ADERDGSolver::refinementCriterion(
  const CellStates*        Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 2: Refine near the moving front. The stub keeps the mesh.
  return ::exahype2::RefinementCommand::Keep;
}

double
tutorials::exahype2::swe::ADERDGSolver::maxEigenvalue(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 1: Return max(|u_n + sqrt(g h)|, |u_n - sqrt(g h)|) with u_n = Q.hu(normal) / Q.h
  // and g = 9.81. The positive stub value gives a finite time step size.
  return 1.0;
}

void
tutorials::exahype2::swe::ADERDGSolver::flux(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  Flux&                    F,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 1: Implement the shallow-water flux in direction normal.
  F = {};
}

void
tutorials::exahype2::swe::ADERDGSolver::nonconservativeProduct(
  const State&             Q,
  const Difference&        deltaQ,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  NonconservativeProduct&  BTimesDeltaQ,
  const RuntimeParameters& parameters
) {
  // TODO Exercise 2: Implement the bathymetry term B(Q) grad Q. The stub writes zero.
  BTimesDeltaQ = {};
}
