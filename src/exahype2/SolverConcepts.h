// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "tarch/la/Vector.h"

/**
 * Declares one concept per PDE term of a generated solver. Every term is a static function of
 * the solver type with the argument list the kernels call.
 */
namespace exahype2 {
  using Coordinates = tarch::la::Vector<DIMENSIONS, double>;

  template <typename SolverType, typename Precision>
  concept HasInitialCondition = requires(Precision* const Q, const Coordinates& x, const Coordinates& h) {
    { SolverType::initialCondition(Q, x, h) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasBoundaryConditions = requires(
    const Precision* const inside,
    Precision* const       outside,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const int              normal
  ) {
    { SolverType::boundaryConditions(inside, outside, x, h, t, normal) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasRefinementCriterion =
    requires(const Precision* const Q, const Coordinates& x, const Coordinates& h, const double t) {
      { SolverType::refinementCriterion(Q, x, h, t) } -> std::same_as<exahype2::RefinementCommand>;
    };

  template <typename SolverType, typename Precision>
  concept HasMaxEigenvalue = requires(
    const Precision* const Q,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal
  ) {
    { SolverType::maxEigenvalue(Q, x, h, t, dt, normal) } -> std::same_as<double>;
  };

  template <typename SolverType, typename Precision>
  concept HasFlux = requires(
    const Precision* const Q,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal,
    Precision* const       F
  ) {
    { SolverType::flux(Q, x, h, t, dt, normal, F) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasDiffusiveFlux = requires(
    const Precision* const Q,
    const Precision* const gradient[DIMENSIONS],
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal,
    Precision* const       F
  ) {
    { SolverType::flux(Q, gradient, x, h, t, dt, normal, F) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasNonconservativeProduct = requires(
    const Precision* const Q,
    const Precision* const deltaQ,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal,
    Precision* const       BTimesDeltaQ
  ) {
    { SolverType::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasSourceTerm = requires(
    const Precision* const Q,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    Precision* const       S
  ) {
    { SolverType::sourceTerm(Q, x, h, t, dt, S) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasDiffusiveSourceTerm = requires(
    const Precision* const Q,
    const Precision* const deltaQ,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    Precision* const       S
  ) {
    { SolverType::diffusiveSourceTerm(Q, deltaQ, x, h, t, dt, S) } -> std::same_as<void>;
  };

  /**
   * The finite-volume Riemann solver returns the maximum eigenvalue it computes and writes the
   * left and right fluxes.
   */
  template <typename SolverType, typename Precision>
  concept HasFiniteVolumeRiemannSolver = requires(
    const Precision* const QL,
    const Precision* const QR,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal,
    Precision* const       FL,
    Precision* const       FR
  ) {
    { SolverType::solveRiemannProblem(QL, QR, x, h, t, dt, normal, FL, FR) } -> std::same_as<double>;
  };

  /**
   * The discontinuous Galerkin Riemann solver writes the left and right fluxes of one face
   * quadrature point.
   */
  template <typename SolverType, typename Precision>
  concept HasGalerkinRiemannSolver = requires(
    Precision* const       FL,
    Precision* const       FR,
    const Precision* const QL,
    const Precision* const QR,
    const double           t,
    const double           dt,
    const Coordinates&     x,
    const Coordinates&     h,
    const int              direction,
    const bool             isBoundaryFace,
    const int              faceIndex
  ) {
    {
      SolverType::riemannSolver(FL, FR, QL, QR, t, dt, x, h, direction, isBoundaryFace, faceIndex)
    } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasMaterialParameters = requires(
    const Precision* const Q,
    const Coordinates&     x,
    const Coordinates&     h,
    const double           t,
    const double           dt,
    const int              normal,
    Precision* const       rhs
  ) {
    { SolverType::multiplyMaterialParameterMatrix(Q, x, h, t, dt, normal, rhs) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasPointSource = requires(
    const Precision* const Q,
    const double* const    x,
    const double           t,
    const double           dt,
    Precision* const       forceVector,
    const int              n
  ) {
    { SolverType::pointSource(Q, x, t, dt, forceVector, n) } -> std::same_as<void>;
  };

  template <typename SolverType, int NumberOfPointSources>
  concept HasPointSourceLocations = requires(double location[NumberOfPointSources][DIMENSIONS]) {
    { SolverType::initPointSourceLocations(location) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasLimitedDifference = requires(
    const Precision* const QL,
    const Precision* const Q,
    const Precision* const QR,
    const Coordinates&     x,
    const Coordinates&     h,
    const int              normal,
    const int              unknown
  ) {
    { SolverType::limitedDifference(QL, Q, QR, x, h, normal, unknown) } -> std::same_as<Precision>;
  };

  template <typename SolverType, typename Precision>
  concept HasReconstruction = requires(
    const Precision* const QL,
    const Precision* const Q,
    const Precision* const QR,
    const Precision* const limitedDifferences,
    const Coordinates&     x,
    const Coordinates&     h,
    const int              normal,
    const int              unknown,
    Precision&             reconstructedMinus,
    Precision&             reconstructedPlus
  ) {
    {
      SolverType::reconstruct(
        QL,
        Q,
        QR,
        limitedDifferences,
        x,
        h,
        normal,
        unknown,
        reconstructedMinus,
        reconstructedPlus
      )
    } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasPhysicalAdmissibility =
    requires(const Precision* const Q, const int numberOfStates, const Coordinates& x, const Coordinates& h) {
      { SolverType::isPhysicallyAdmissible(Q, numberOfStates, x, h) } -> std::same_as<bool>;
    };

  template <typename SolverType, typename Precision>
  concept HasDmpObservables = requires(Precision* const observables, const Precision* const Q) {
    { SolverType::mapDmpObservables(observables, Q) } -> std::same_as<void>;
  };

  template <typename SolverType, typename Precision>
  concept HasDmpRelaxationParameter = requires(const int observable, const Precision specifiedRelaxationParameter) {
    { SolverType::dmpRelaxationParameter(observable, specifiedRelaxationParameter) } -> std::same_as<Precision>;
  };

  template <typename SolverType>
  concept HasAnalyticalSolution =
    requires(const Coordinates& x, const double t, const double dt, const double* const Q, double* const solution) {
      { SolverType::analyticalSolution(x, t, dt, Q, solution) } -> std::same_as<void>;
    };

  /**
   * The analytical solution of a tracer writes the particle's data and position.
   */
  template <typename SolverType, typename Particle>
  concept HasParticleAnalyticalSolution =
    requires(Particle* const particle, const double t, const double dt, double* const solution) {
      { SolverType::analyticalSolution(particle, t, dt, solution) } -> std::same_as<void>;
    };
} // namespace exahype2

namespace exahype2::fv {
  /**
   * A finite-volume solver declares the terms every patch update calls and the constants the
   * patch loops and the boundary update read.
   */
  template <typename SolverType, typename Storage>
  concept SolverConcept =
    exahype2::HasInitialCondition<SolverType, Storage> and exahype2::HasBoundaryConditions<SolverType, Storage>
    and exahype2::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::NumberOfFiniteVolumesPerAxisPerPatch } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::fv

namespace exahype2::dg::ader {
  /**
   * An ADER-DG solver declares the terms every predictor and corrector sweep calls and the
   * constants the node loops and the boundary update read. Its boundary conditions run in the
   * compute precision.
   */
  template <typename SolverType, typename Storage, typename Compute>
  concept SolverConcept =
    exahype2::HasInitialCondition<SolverType, Storage> and exahype2::HasBoundaryConditions<SolverType, Compute>
    and exahype2::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::DGOrder } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::dg::ader

namespace exahype2::dg::rk {
  /**
   * A Runge-Kutta DG solver declares the terms every stage calls and the constants the node
   * loops, the stage loop and the boundary update read. Its boundary conditions run in the
   * compute precision.
   */
  template <typename SolverType, typename Storage, typename Compute>
  concept SolverConcept =
    exahype2::HasInitialCondition<SolverType, Storage> and exahype2::HasBoundaryConditions<SolverType, Compute>
    and exahype2::HasRefinementCriterion<SolverType, Storage> and requires {
          { SolverType::DGOrder } -> std::convertible_to<int>;
          { SolverType::NumberOfRungeKuttaStages } -> std::convertible_to<int>;
          { SolverType::NumberOfUnknowns } -> std::convertible_to<int>;
          { SolverType::NumberOfAuxiliaryVariables } -> std::convertible_to<int>;
          { SolverType::PeriodicBoundaries[0] } -> std::convertible_to<bool>;
        };
} // namespace exahype2::dg::rk
