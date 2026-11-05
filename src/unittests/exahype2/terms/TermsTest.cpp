// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
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
#include "exahype2/terms/ReconstructionSupport.h"
#include "exahype2/terms/ReconstructionWeights.h"
#include "exahype2/terms/RefinementCriterion.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "exahype2/terms/SubcellRiemannSolver.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

namespace exahype2::terms::unittests {
  struct Tracer final {};

  struct RuntimeParameters final {};

  struct Complete {
    using RuntimeParameters = exahype2::terms::unittests::RuntimeParameters;

    static void initialCondition(
      double*                                      Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const RuntimeParameters&                     parameters
    );
    static void boundaryConditions(
      const double*                                inside,
      double*                                      outside,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      int                                          normal,
      const RuntimeParameters&                     parameters
    );
    static exahype2::RefinementCommand refinementCriterion(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      const RuntimeParameters&                     parameters
    );
    static double maxEigenvalue(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      const RuntimeParameters&                     parameters
    );
    static void flux(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double*                                      F,
      const RuntimeParameters&                     parameters
    );
    static void flux(
      const double*                                Q,
      const double* const                          gradient[DIMENSIONS],
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double*                                      F,
      const RuntimeParameters&                     parameters
    );
    static void nonconservativeProduct(
      const double*                                Q,
      const double*                                deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double*                                      BTimesDeltaQ,
      const RuntimeParameters&                     parameters
    );
    static void sourceTerm(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      double*                                      S,
      const RuntimeParameters&                     parameters
    );
    static void diffusiveSourceTerm(
      const double*                                Q,
      const double*                                deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      double*                                      S,
      const RuntimeParameters&                     parameters
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
      double*                                      FR,
      const RuntimeParameters&                     parameters
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
      double*                                      FR,
      const RuntimeParameters&                     parameters
    );
    static void multiplyMaterialParameterMatrix(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double*                                      rhs,
      const RuntimeParameters&                     parameters
    );
    static void pointSource(
      const double*            Q,
      const double*            x,
      double                   t,
      double                   dt,
      double*                  forceVector,
      int                      n,
      const RuntimeParameters& parameters
    );
    static void initPointSourceLocations(double location[2][DIMENSIONS], const RuntimeParameters& parameters);
    static double limitedDifference(
      const double*                                QL,
      const double*                                Q,
      const double*                                QR,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      int                                          normal,
      int                                          unknown,
      const RuntimeParameters&                     parameters
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
      double&                                      reconstructedPlus,
      const RuntimeParameters&                     parameters
    );
    static bool isPhysicallyAdmissible(
      const double*                                Q,
      int                                          numberOfStates,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const RuntimeParameters&                     parameters
    );
    static bool supportsHighOrderReconstruction(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const RuntimeParameters&                     parameters
    );
    static int reconstructionWeightsOf(int entry);
    static void mapDmpObservables(double* observables, const double* Q, const RuntimeParameters& parameters);
    static double dmpRelaxationParameter(
      int                      observable,
      double                   specifiedRelaxationParameter,
      const RuntimeParameters& parameters
    );
    static void analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      const double*                                Q,
      double*                                      solution,
      const RuntimeParameters&                     parameters
    );
    static void analyticalSolution(
      exahype2::terms::unittests::Tracer* particle,
      double                              t,
      double                              dt,
      double*                             solution,
      const RuntimeParameters&            parameters
    );
    static constexpr std::array<int, 2> MeasuredUnknowns{0, 1};
    static void postprocessUpdatedPatch(
      double*                                      Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      const RuntimeParameters&                     parameters
    );
    static bool ownsCell(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const RuntimeParameters&                     parameters
    );
    static bool plotsFace(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      int                                          normal,
      const RuntimeParameters&                     parameters
    );
    static constexpr bool InitialisesPatches = true;
  };

  struct FiniteVolumeSolver final : exahype2::terms::unittests::Complete {
    static constexpr int                     PatchSize                  = 4;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  struct ADERDGSolver final : exahype2::terms::unittests::Complete {
    static constexpr int                     DGOrder                    = 3;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  struct RungeKuttaDGSolver final : exahype2::terms::unittests::Complete {
    static constexpr int                     DGOrder                    = 3;
    static constexpr int                     RungeKuttaOrder            = 2;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  template <unsigned long long PeriodicAxes>
  struct FiniteVolumeSolverWithoutBoundaryConditions final {
    using RuntimeParameters = exahype2::terms::unittests::RuntimeParameters;

    static void initialCondition(
      double*                                      Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const RuntimeParameters&                     parameters
    );
    static exahype2::RefinementCommand refinementCriterion(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      const RuntimeParameters&                     parameters
    );
    static constexpr int                     PatchSize                  = 4;
    static constexpr int                     NumberOfUnknowns           = 2;
    static constexpr int                     NumberOfAuxiliaryVariables = 1;
    static constexpr std::bitset<DIMENSIONS> PeriodicBoundaries{PeriodicAxes};
  };

  struct Defective final {
    using RuntimeParameters = exahype2::terms::unittests::RuntimeParameters;

    void flux(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double*                                      F,
      const RuntimeParameters&                     parameters
    );
    static void maxEigenvalue(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      const RuntimeParameters&                     parameters
    );
    static void sourceTerm(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      double*                                      S,
      const RuntimeParameters&                     parameters
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
      int                                          faceIndex,
      const RuntimeParameters&                     parameters
    );
    static exahype2::RefinementCommand refinementCriterion(
      const double*                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t
    );
  };
} // namespace exahype2::terms::unittests

namespace {
  using Tracer             = exahype2::terms::unittests::Tracer;
  using Complete           = exahype2::terms::unittests::Complete;
  using FiniteVolumeSolver = exahype2::terms::unittests::FiniteVolumeSolver;
  using ADERDGSolver       = exahype2::terms::unittests::ADERDGSolver;
  using RungeKuttaDGSolver = exahype2::terms::unittests::RungeKuttaDGSolver;
  using Defective          = exahype2::terms::unittests::Defective;

  template <unsigned long long PeriodicAxes>
  using FiniteVolumeSolverWithoutBoundaryConditions =
    exahype2::terms::unittests::FiniteVolumeSolverWithoutBoundaryConditions<PeriodicAxes>;
} // namespace

UNIT_TEST("Every term of a complete solver satisfies its concept and a term for double fails its concept for float") {
  GIVEN("The solver Complete with every term for double and the particle type Tracer") {
    THEN("Complete satisfies every term concept in double") {
      VALIDATE_EQUALS((exahype2::terms::HasInitialCondition<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasBoundaryConditions<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasRefinementCriterion<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasMaxEigenvalue<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasFlux<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasDiffusiveFlux<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasNonconservativeProduct<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasSourceTerm<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasDiffusiveSourceTerm<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasRiemannSolver<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasSubcellRiemannSolver<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasMaterialParameters<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasPointSource<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasPointSourceLocations<Complete, 2>), true);
      VALIDATE_EQUALS((exahype2::terms::HasLimitedDifference<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasReconstruction<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasPhysicalAdmissibility<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasReconstructionSupport<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasReconstructionWeights<Complete>), true);
      VALIDATE_EQUALS((exahype2::terms::HasDmpObservables<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasDmpRelaxationParameter<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasAnalyticalSolution<Complete>), true);
      VALIDATE_EQUALS((exahype2::terms::HasMeasuredUnknowns<Complete>), true);
      VALIDATE_EQUALS((exahype2::terms::HasPostprocessUpdatedPatch<Complete, double>), true);
      VALIDATE_EQUALS((exahype2::terms::HasParticleAnalyticalSolution<Complete, Tracer>), true);
      VALIDATE_EQUALS((exahype2::terms::HasCellPredicate<Complete>), true);
      VALIDATE_EQUALS((exahype2::terms::HasFacePredicate<Complete>), true);
      VALIDATE_EQUALS((exahype2::terms::InitialisesPatches<Complete>), true);
    }

    THEN("The flux, the Riemann solver and the patch postprocessing of Complete fail their concepts for float") {
      VALIDATE_EQUALS((exahype2::terms::HasFlux<Complete, float>), false);
      VALIDATE_EQUALS((exahype2::terms::HasRiemannSolver<Complete, float>), false);
      VALIDATE_EQUALS((exahype2::terms::HasPostprocessUpdatedPatch<Complete, float>), false);
    }
  }
}

UNIT_TEST("A missing or mistyped term fails its concept") {
  GIVEN("The solver Defective with missing terms or terms of the wrong signature or value") {
    THEN("Every term concept fails for Defective") {
      VALIDATE_EQUALS((exahype2::terms::HasFlux<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasMaxEigenvalue<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasSourceTerm<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasRiemannSolver<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasSubcellRiemannSolver<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasInitialCondition<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasMeasuredUnknowns<Defective>), false);
      VALIDATE_EQUALS((exahype2::terms::HasPostprocessUpdatedPatch<Defective, double>), false);
      VALIDATE_EQUALS((exahype2::terms::HasCellPredicate<Defective>), false);
      VALIDATE_EQUALS((exahype2::terms::HasFacePredicate<Defective>), false);
      VALIDATE_EQUALS((exahype2::terms::InitialisesPatches<Defective>), false);
    }

    THEN("The refinement criterion of Defective without the runtime parameters fails its concept") {
      VALIDATE_EQUALS((exahype2::terms::HasRefinementCriterion<Defective, double>), false);
    }
  }
}

UNIT_TEST("A solver satisfies the concept of its solver family") {
  GIVEN("A finite-volume, an ADER-DG and a Runge-Kutta DG solver, each with the constants of its solver family") {
    THEN("Every solver satisfies the concept of its solver family") {
      VALIDATE_EQUALS((exahype2::fv::SolverConcept<FiniteVolumeSolver, double>), true);
      VALIDATE_EQUALS((exahype2::dg::ader::SolverConcept<ADERDGSolver, double, double>), true);
      VALIDATE_EQUALS((exahype2::dg::rk::SolverConcept<RungeKuttaDGSolver, double, double>), true);
    }
  }
}

UNIT_TEST("A solver without the constants of a solver family fails the concept of that solver family") {
  GIVEN("The solver Complete without solver family constants and the ADER-DG solver without a Runge-Kutta order") {
    THEN("Complete fails the finite-volume and the ADER-DG concept, the ADER-DG solver the Runge-Kutta DG concept") {
      VALIDATE_EQUALS((exahype2::fv::SolverConcept<Complete, double>), false);
      VALIDATE_EQUALS((exahype2::dg::ader::SolverConcept<Complete, double, double>), false);
      VALIDATE_EQUALS((exahype2::dg::rk::SolverConcept<ADERDGSolver, double, double>), false);
    }

    THEN("The ADER-DG solver fails its concept for the compute precision float, in which its boundary conditions run") {
      VALIDATE_EQUALS((exahype2::dg::ader::SolverConcept<ADERDGSolver, double, float>), false);
    }
  }
}

UNIT_TEST("A solver without boundary conditions satisfies its concept only on a domain periodic along every axis") {
  GIVEN("Finite-volume solvers without boundary conditions, periodic along every axis, the first axis and zero axes") {
    constexpr unsigned long long EveryAxis = (1ULL << DIMENSIONS) - 1;
    using PeriodicAlongEveryAxis           = FiniteVolumeSolverWithoutBoundaryConditions<EveryAxis>;
    using PeriodicAlongTheFirstAxis        = FiniteVolumeSolverWithoutBoundaryConditions<1ULL>;
    using PeriodicAlongNoAxis              = FiniteVolumeSolverWithoutBoundaryConditions<0ULL>;

    THEN("Only the solver periodic along every axis is periodic everywhere and satisfies the finite-volume concept") {
      VALIDATE_EQUALS((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongEveryAxis>), true);
      VALIDATE_EQUALS((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongTheFirstAxis>), false);
      VALIDATE_EQUALS((exahype2::terms::IsPeriodicAlongEveryAxis<PeriodicAlongNoAxis>), false);
      VALIDATE_EQUALS((exahype2::fv::SolverConcept<PeriodicAlongEveryAxis, double>), true);
      VALIDATE_EQUALS((exahype2::fv::SolverConcept<PeriodicAlongTheFirstAxis, double>), false);
      VALIDATE_EQUALS((exahype2::fv::SolverConcept<PeriodicAlongNoAxis, double>), false);
    }
  }
}
