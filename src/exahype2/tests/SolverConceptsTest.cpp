// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverConceptsTest.h"

#include "exahype2/SolverConcepts.h"

#if defined(__NVCOMPILER)
// NVHPC diagnoses members inspected only by requires expressions as unused.
#pragma diag_push
#pragma diag_suppress declared_but_not_referenced
#endif

namespace {
  using Coordinates = exahype2::Coordinates;

  struct Tracer final {};

  struct Complete {
    static void initialCondition(double* const Q, const Coordinates& x, const Coordinates& h);
    static void boundaryConditions(
      const double* const inside,
      double* const       outside,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      int                 normal
    );
    static exahype2::RefinementCommand refinementCriterion(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t
    );
    static double maxEigenvalue(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal
    );
    static void flux(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       F
    );
    static void flux(
      const double* const Q,
      const double* const gradient[DIMENSIONS],
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       F
    );
    static void nonconservativeProduct(
      const double* const Q,
      const double* const deltaQ,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       BTimesDeltaQ
    );
    static void sourceTerm(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      double* const       S
    );
    static void diffusiveSourceTerm(
      const double* const Q,
      const double* const deltaQ,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      double* const       S
    );
    static double solveRiemannProblem(
      const double* const QL,
      const double* const QR,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       FL,
      double* const       FR
    );
    static void riemannSolver(
      double* const       FL,
      double* const       FR,
      const double* const QL,
      const double* const QR,
      double              t,
      double              dt,
      const Coordinates&  x,
      const Coordinates&  h,
      int                 direction,
      bool                isBoundaryFace,
      int                 faceIndex
    );
    static void multiplyMaterialParameterMatrix(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       rhs
    );
    static void pointSource(
      const double* const Q,
      const double* const x,
      double              t,
      double              dt,
      double* const       forceVector,
      int                 n
    );
    static void   initPointSourceLocations(double location[2][DIMENSIONS]);
    static double limitedDifference(
      const double* const QL,
      const double* const Q,
      const double* const QR,
      const Coordinates&  x,
      const Coordinates&  h,
      int                 normal,
      int                 unknown
    );
    static void reconstruct(
      const double* const QL,
      const double* const Q,
      const double* const QR,
      const double* const limitedDifferences,
      const Coordinates&  x,
      const Coordinates&  h,
      int                 normal,
      int                 unknown,
      double&             reconstructedMinus,
      double&             reconstructedPlus
    );
    static bool isPhysicallyAdmissible(
      const double* const Q,
      int                 numberOfStates,
      const Coordinates&  x,
      const Coordinates&  h
    );
    static void   mapDmpObservables(double* const observables, const double* const Q);
    static double dmpRelaxationParameter(int observable, double specifiedRelaxationParameter);
    static void   analyticalSolution(
      const Coordinates&  x,
      double              t,
      double              dt,
      const double* const Q,
      double* const       solution
    );
    static void analyticalSolution(Tracer* const particle, double t, double dt, double* const solution);
  };

  struct FiniteVolumeSolver final : Complete {
    static constexpr int                     NumberOfFiniteVolumesPerAxisPerPatch = 4;
    static constexpr int                     NumberOfUnknowns                     = 2;
    static constexpr int                     NumberOfAuxiliaryVariables           = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  struct AderDGSolver final : Complete {
    static constexpr int                     DGOrder                    = 3;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  struct RungeKuttaDGSolver final : Complete {
    static constexpr int                     DGOrder                    = 3;
    static constexpr int                     NumberOfRungeKuttaStages   = 2;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  struct Defective final {
    void flux(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal,
      double* const       F
    );
    static void maxEigenvalue(
      const double* const Q,
      const Coordinates&  x,
      const Coordinates&  h,
      double              t,
      double              dt,
      int                 normal
    );
    static void sourceTerm(const double* const Q, const Coordinates& x, double t, double dt, double* const S);
  };
} // namespace

#if defined(__NVCOMPILER)
#pragma diag_pop
#endif

exahype2::tests::SolverConceptsTest::SolverConceptsTest() :
  tarch::tests::TestCase("exahype2::tests::SolverConceptsTest") {}

void exahype2::tests::SolverConceptsTest::run() {
  testMethod(testEveryTermOfACompleteSolverSatisfiesItsConcept);
  testMethod(testAMissingOrMistypedTermFailsItsConcept);
  testMethod(testAFamilySolverSatisfiesTheConceptOfItsFamily);
  testMethod(testASolverWithoutTheConstantsOfItsFamilyFailsItsConcept);
}

void exahype2::tests::SolverConceptsTest::testEveryTermOfACompleteSolverSatisfiesItsConcept() {
  validateEquals((exahype2::HasInitialCondition<Complete, double>), true);
  validateEquals((exahype2::HasBoundaryConditions<Complete, double>), true);
  validateEquals((exahype2::HasRefinementCriterion<Complete, double>), true);
  validateEquals((exahype2::HasMaxEigenvalue<Complete, double>), true);
  validateEquals((exahype2::HasFlux<Complete, double>), true);
  validateEquals((exahype2::HasDiffusiveFlux<Complete, double>), true);
  validateEquals((exahype2::HasNonconservativeProduct<Complete, double>), true);
  validateEquals((exahype2::HasSourceTerm<Complete, double>), true);
  validateEquals((exahype2::HasDiffusiveSourceTerm<Complete, double>), true);
  validateEquals((exahype2::HasFiniteVolumeRiemannSolver<Complete, double>), true);
  validateEquals((exahype2::HasGalerkinRiemannSolver<Complete, double>), true);
  validateEquals((exahype2::HasMaterialParameters<Complete, double>), true);
  validateEquals((exahype2::HasPointSource<Complete, double>), true);
  validateEquals((exahype2::HasPointSourceLocations<Complete, 2>), true);
  validateEquals((exahype2::HasLimitedDifference<Complete, double>), true);
  validateEquals((exahype2::HasReconstruction<Complete, double>), true);
  validateEquals((exahype2::HasPhysicalAdmissibility<Complete, double>), true);
  validateEquals((exahype2::HasDmpObservables<Complete, double>), true);
  validateEquals((exahype2::HasDmpRelaxationParameter<Complete, double>), true);
  validateEquals((exahype2::HasAnalyticalSolution<Complete>), true);
  validateEquals((exahype2::HasParticleAnalyticalSolution<Complete, Tracer>), true);
  // A term written for double is not the term for float.
  validateEquals((exahype2::HasFlux<Complete, float>), false);
}

void exahype2::tests::SolverConceptsTest::testAMissingOrMistypedTermFailsItsConcept() {
  validateEquals((exahype2::HasFlux<Defective, double>), false);
  validateEquals((exahype2::HasMaxEigenvalue<Defective, double>), false);
  validateEquals((exahype2::HasSourceTerm<Defective, double>), false);
  validateEquals((exahype2::HasInitialCondition<Defective, double>), false);
}

void exahype2::tests::SolverConceptsTest::testAFamilySolverSatisfiesTheConceptOfItsFamily() {
  validateEquals((exahype2::fv::SolverConcept<FiniteVolumeSolver, double>), true);
  validateEquals((exahype2::dg::ader::SolverConcept<AderDGSolver, double, double>), true);
  validateEquals((exahype2::dg::rk::SolverConcept<RungeKuttaDGSolver, double, double>), true);
}

void exahype2::tests::SolverConceptsTest::testASolverWithoutTheConstantsOfItsFamilyFailsItsConcept() {
  validateEquals((exahype2::fv::SolverConcept<Complete, double>), false);
  validateEquals((exahype2::dg::ader::SolverConcept<Complete, double, double>), false);
  validateEquals((exahype2::dg::rk::SolverConcept<AderDGSolver, double, double>), false);
  // The boundary conditions of a discontinuous Galerkin solver run in the compute precision.
  validateEquals((exahype2::dg::ader::SolverConcept<AderDGSolver, double, float>), false);
}
