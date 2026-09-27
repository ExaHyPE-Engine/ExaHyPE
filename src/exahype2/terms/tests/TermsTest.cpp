// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TermsTest.h"

#include "exahype2/RefinementCommand.h"
#include "exahype2/dg/ader/Solver.h"
#include "exahype2/dg/rk/Solver.h"
#include "exahype2/fv/Solver.h"
#include "exahype2/terms/AnalyticalSolution.h"
#include "exahype2/terms/BoundaryConditions.h"
#include "exahype2/terms/CellPredicate.h"
#include "exahype2/terms/DiffusiveFlux.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/DmpObservables.h"
#include "exahype2/terms/DmpRelaxationParameter.h"
#include "exahype2/terms/FacePredicate.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/InitialCondition.h"
#include "exahype2/terms/InitialisesOnHost.h"
#include "exahype2/terms/InitialisesPatches.h"
#include "exahype2/terms/LimitedDifference.h"
#include "exahype2/terms/MaterialParameters.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/MeasuredUnknowns.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/ParticleAnalyticalSolution.h"
#include "exahype2/terms/PhysicalAdmissibility.h"
#include "exahype2/terms/PointSource.h"
#include "exahype2/terms/PointSourceLocations.h"
#include "exahype2/terms/PostprocessUpdatedPatch.h"
#include "exahype2/terms/Reconstruction.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "exahype2/terms/SubcellRiemannSolver.h"
#include "tarch/la/Vector.h"

struct exahype2::terms::tests::TermsTest::Tracer final {};

struct exahype2::terms::tests::TermsTest::Complete {
  static void initialCondition(
    double*                                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );
  static void boundaryConditions(
    const double*                                inside,
    double*                                      outside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    int                                          normal
  );
  static exahype2::RefinementCommand refinementCriterion(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t
  );
  static double maxEigenvalue(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal
  );
  static void flux(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      F
  );
  static void flux(
    const double*                                Q,
    const double* const                          gradient[DIMENSIONS],
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      F
  );
  static void nonconservativeProduct(
    const double*                                Q,
    const double*                                deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      BTimesDeltaQ
  );
  static void sourceTerm(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    double*                                      S
  );
  static void diffusiveSourceTerm(
    const double*                                Q,
    const double*                                deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    double*                                      S
  );
  static double solveRiemannProblem(
    const double*                                QL,
    const double*                                QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex,
    double*                                      FL,
    double*                                      FR
  );
  static double solveSubcellRiemannProblem(
    const double*                                QL,
    const double*                                QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex,
    double*                                      FL,
    double*                                      FR
  );
  static void multiplyMaterialParameterMatrix(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      rhs
  );
  static void pointSource(const double* Q, const double* x, double t, double dt, double* forceVector, int n);
  static void initPointSourceLocations(double location[2][DIMENSIONS]);
  static double limitedDifference(
    const double*                                QL,
    const double*                                Q,
    const double*                                QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal,
    int                                          unknown
  );
  static void reconstruct(
    const double*                                QL,
    const double*                                Q,
    const double*                                QR,
    const double*                                limitedDifferences,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal,
    int                                          unknown,
    double&                                      reconstructedMinus,
    double&                                      reconstructedPlus
  );
  static bool isPhysicallyAdmissible(
    const double*                                Q,
    int                                          numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );
  static void mapDmpObservables(double* observables, const double* Q);
  static double dmpRelaxationParameter(int observable, double specifiedRelaxationParameter);
  static void analyticalSolution(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    const double*                                Q,
    double*                                      solution
  );
  static void analyticalSolution(Tracer* particle, double t, double dt, double* solution);
  static constexpr std::array<int, 2> MeasuredUnknowns{0, 1};
  static void postprocessUpdatedPatch(
    double*                                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt
  );
  static bool ownsCell(const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& h);
  static bool plotsFace(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal
  );
  static constexpr bool InitialisesOnHost  = true;
  static constexpr bool InitialisesPatches = true;
};

struct exahype2::terms::tests::TermsTest::FiniteVolumeSolver final : exahype2::terms::tests::TermsTest::Complete {
  static constexpr int                     PatchSize                  = 4;
  static constexpr int                     NumberOfUnknowns           = 2;
  static constexpr int                     NumberOfAuxiliaryVariables = 1;
  static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
};

struct exahype2::terms::tests::TermsTest::ADERDGSolver final : exahype2::terms::tests::TermsTest::Complete {
  static constexpr int                     DGOrder                    = 3;
  static constexpr int                     NumberOfUnknowns           = 2;
  static constexpr int                     NumberOfAuxiliaryVariables = 1;
  static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
};

struct exahype2::terms::tests::TermsTest::RungeKuttaDGSolver final : exahype2::terms::tests::TermsTest::Complete {
  static constexpr int                     DGOrder                    = 3;
  static constexpr int                     RungeKuttaOrder            = 2;
  static constexpr int                     NumberOfUnknowns           = 2;
  static constexpr int                     NumberOfAuxiliaryVariables = 1;
  static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
};

template <unsigned long long PeriodicAxes>
struct exahype2::terms::tests::TermsTest::FiniteVolumeSolverWithoutBoundaryConditions final {
  static void initialCondition(
    double*                                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );
  static exahype2::RefinementCommand refinementCriterion(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t
  );
  static constexpr int                     PatchSize                  = 4;
  static constexpr int                     NumberOfUnknowns           = 2;
  static constexpr int                     NumberOfAuxiliaryVariables = 1;
  static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{PeriodicAxes};
};

struct exahype2::terms::tests::TermsTest::Defective final {
  void flux(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      F
  );
  static void maxEigenvalue(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal
  );
  static void sourceTerm(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    double*                                      S
  );
  static void solveRiemannProblem(
    double*                                      FL,
    double*                                      FR,
    const double*                                QL,
    const double*                                QR,
    double                                       t,
    double                                       dt,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex
  );
  static constexpr bool InitialisesOnHost = false;
};

exahype2::terms::tests::TermsTest::TermsTest() :
  tarch::tests::TestCase("exahype2::terms::tests::TermsTest") {}

void
exahype2::terms::tests::TermsTest::run() {
  testMethod(testEveryTermOfACompleteSolverSatisfiesItsConcept);
  testMethod(testAMissingOrMistypedTermFailsItsConcept);
  testMethod(testASolverSatisfiesTheConceptOfItsSolverFamily);
  testMethod(testASolverWithoutTheConstantsOfItsSolverFamilyFailsItsConcept);
  testMethod(testOnlyADomainPeriodicAlongEveryAxisOmitsTheBoundaryConditions);
}

void
exahype2::terms::tests::TermsTest::testEveryTermOfACompleteSolverSatisfiesItsConcept() {
  validateEquals((exahype2::terms::HasInitialCondition<Complete, double>), true);
  validateEquals((exahype2::terms::HasBoundaryConditions<Complete, double>), true);
  validateEquals((exahype2::terms::HasRefinementCriterion<Complete, double>), true);
  validateEquals((exahype2::terms::HasMaxEigenvalue<Complete, double>), true);
  validateEquals((exahype2::terms::HasFlux<Complete, double>), true);
  validateEquals((exahype2::terms::HasDiffusiveFlux<Complete, double>), true);
  validateEquals((exahype2::terms::HasNonconservativeProduct<Complete, double>), true);
  validateEquals((exahype2::terms::HasSourceTerm<Complete, double>), true);
  validateEquals((exahype2::terms::HasDiffusiveSourceTerm<Complete, double>), true);
  validateEquals((exahype2::terms::HasRiemannSolver<Complete, double>), true);
  validateEquals((exahype2::terms::HasSubcellRiemannSolver<Complete, double>), true);
  validateEquals((exahype2::terms::HasMaterialParameters<Complete, double>), true);
  validateEquals((exahype2::terms::HasPointSource<Complete, double>), true);
  validateEquals((exahype2::terms::HasPointSourceLocations<Complete, 2>), true);
  validateEquals((exahype2::terms::HasLimitedDifference<Complete, double>), true);
  validateEquals((exahype2::terms::HasReconstruction<Complete, double>), true);
  validateEquals((exahype2::terms::HasPhysicalAdmissibility<Complete, double>), true);
  validateEquals((exahype2::terms::HasDmpObservables<Complete, double>), true);
  validateEquals((exahype2::terms::HasDmpRelaxationParameter<Complete, double>), true);
  validateEquals((exahype2::terms::HasAnalyticalSolution<Complete>), true);
  validateEquals((exahype2::terms::HasMeasuredUnknowns<Complete>), true);
  validateEquals((exahype2::terms::HasPostprocessUpdatedPatch<Complete, double>), true);
  validateEquals((exahype2::terms::HasParticleAnalyticalSolution<Complete, Tracer>), true);
  validateEquals((exahype2::terms::HasCellPredicate<Complete>), true);
  validateEquals((exahype2::terms::HasFacePredicate<Complete>), true);
  validateEquals((exahype2::terms::InitialisesOnHost<Complete>), true);
  validateEquals((exahype2::terms::InitialisesPatches<Complete>), true);
  // A term written for double is not the term for float.
  validateEquals((exahype2::terms::HasFlux<Complete, float>), false);
  validateEquals((exahype2::terms::HasRiemannSolver<Complete, float>), false);
  validateEquals((exahype2::terms::HasPostprocessUpdatedPatch<Complete, float>), false);
}

void
exahype2::terms::tests::TermsTest::testAMissingOrMistypedTermFailsItsConcept() {
  validateEquals((exahype2::terms::HasFlux<Defective, double>), false);
  validateEquals((exahype2::terms::HasMaxEigenvalue<Defective, double>), false);
  validateEquals((exahype2::terms::HasSourceTerm<Defective, double>), false);
  validateEquals((exahype2::terms::HasRiemannSolver<Defective, double>), false);
  validateEquals((exahype2::terms::HasSubcellRiemannSolver<Defective, double>), false);
  validateEquals((exahype2::terms::HasInitialCondition<Defective, double>), false);
  validateEquals((exahype2::terms::HasMeasuredUnknowns<Defective>), false);
  validateEquals((exahype2::terms::HasPostprocessUpdatedPatch<Defective, double>), false);
  validateEquals((exahype2::terms::HasCellPredicate<Defective>), false);
  validateEquals((exahype2::terms::HasFacePredicate<Defective>), false);
  validateEquals((exahype2::terms::InitialisesOnHost<Defective>), false);
  validateEquals((exahype2::terms::InitialisesPatches<Defective>), false);
}

void
exahype2::terms::tests::TermsTest::testASolverSatisfiesTheConceptOfItsSolverFamily() {
  validateEquals((exahype2::fv::SolverConcept<FiniteVolumeSolver, double>), true);
  validateEquals((exahype2::dg::ader::SolverConcept<ADERDGSolver, double, double>), true);
  validateEquals((exahype2::dg::rk::SolverConcept<RungeKuttaDGSolver, double, double>), true);
}

void
exahype2::terms::tests::TermsTest::testASolverWithoutTheConstantsOfItsSolverFamilyFailsItsConcept() {
  validateEquals((exahype2::fv::SolverConcept<Complete, double>), false);
  validateEquals((exahype2::dg::ader::SolverConcept<Complete, double, double>), false);
  validateEquals((exahype2::dg::rk::SolverConcept<ADERDGSolver, double, double>), false);
  // The boundary conditions of a discontinuous Galerkin solver run in the compute precision.
  validateEquals((exahype2::dg::ader::SolverConcept<ADERDGSolver, double, float>), false);
}

/** A solver without boundary conditions satisfies its family concept only on a domain periodic along every axis. */
void
exahype2::terms::tests::TermsTest::testOnlyADomainPeriodicAlongEveryAxisOmitsTheBoundaryConditions() {
  constexpr unsigned long long EveryAxis = (1ULL << DIMENSIONS) - 1;
  using PeriodicAlongEveryAxis           = FiniteVolumeSolverWithoutBoundaryConditions<EveryAxis>;
  using PeriodicAlongTheFirstAxis        = FiniteVolumeSolverWithoutBoundaryConditions<1ULL>;
  using PeriodicAlongNoAxis              = FiniteVolumeSolverWithoutBoundaryConditions<0ULL>;
  validateEquals((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongEveryAxis>), true);
  validateEquals((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongTheFirstAxis>), false);
  validateEquals((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongNoAxis>), false);
  validateEquals((exahype2::fv::SolverConcept<PeriodicAlongEveryAxis, double>), true);
  validateEquals((exahype2::fv::SolverConcept<PeriodicAlongTheFirstAxis, double>), false);
  validateEquals((exahype2::fv::SolverConcept<PeriodicAlongNoAxis, double>), false);
}
