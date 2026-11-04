// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting::unittests {
  struct RuntimeParameters;
  struct StubLayout;
  template <exahype2::TimeStepping::Rule Rule>
  struct StubSolverOf;
  struct RelaxedStubSolver;
  struct SubcellProjections;

  using StubSolver          = StubSolverOf<exahype2::TimeStepping::Rule::Adaptive>;
  using FixedStepStubSolver = StubSolverOf<exahype2::TimeStepping::Rule::Fixed>;
} // namespace exahype2::dg::limiting::unittests

struct exahype2::dg::limiting::unittests::RuntimeParameters final {};

TARGET_DECLARE
/**
 * The cell layout of the limiter tests.
 */
struct exahype2::dg::limiting::unittests::StubLayout final {
  static constexpr int    NodesPerAxis         = 2;
  static constexpr int    SubcellsPerAxis      = 3;
  static constexpr int    NumberOfData         = 2;
  static constexpr int    NumberOfObservables  = 1;
  static constexpr int    NodesPerCell         = peano4::power(NodesPerAxis, DIMENSIONS);
  static constexpr int    SubcellsPerCell      = peano4::power(SubcellsPerAxis, DIMENSIONS);
  static constexpr int    ValuesPerCell        = NodesPerCell * NumberOfData;
  static constexpr int    SubcellValuesPerCell = SubcellsPerCell * NumberOfData;
  static constexpr int    ExtremaPerCell       = 2 * NumberOfObservables;
  static constexpr double TimeStamp            = 0.75;
  static constexpr double TimeStepSize         = 0.125;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * A solver whose observable and wave speed are the first value. The solver admits only states with
 * a positive first value.
 */
template <exahype2::TimeStepping::Rule Rule>
struct exahype2::dg::limiting::unittests::StubSolverOf {
  static constexpr int DGOrder                    = StubLayout::NodesPerAxis - 1;
  static constexpr int PatchSize                  = StubLayout::SubcellsPerAxis;
  static constexpr int NumberOfUnknowns           = 1;
  static constexpr int NumberOfAuxiliaryVariables = StubLayout::NumberOfData - NumberOfUnknowns;
  static constexpr int NumberOfObservables        = StubLayout::NumberOfObservables;

  static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = Rule;

  using RuntimeParameters      = exahype2::dg::limiting::unittests::RuntimeParameters;
  using Reconstruction         = exahype2::fv::reconstruction::Constant;
  using FallbackReconstruction = void;
  using TimeIntegration        = exahype2::fv::timestepping::SingleStep;

  static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
    double*                                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const RuntimeParameters&                     parameters
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void mapDmpObservables(
    double*                  observables,
    const double*            Q,
    const RuntimeParameters& parameters
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const double*                                Q,
    int                                          numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const RuntimeParameters&                     parameters
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const RuntimeParameters&                     parameters
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      F,
    const RuntimeParameters&                     parameters
  );
};

/**
 * The stub solver with a relaxation term that doubles the specified parameter.
 */
struct exahype2::dg::limiting::unittests::RelaxedStubSolver final : exahype2::dg::limiting::unittests::StubSolver {
  static INLINE_METHOD GPU_CALLABLE_METHOD double dmpRelaxationParameter(
    int                      observable,
    double                   specifiedRelaxationParameter,
    const RuntimeParameters& parameters
  );
};
TARGET_DECLARE_END

/**
 * The projection tables of the two-node Gauss-Legendre basis and three subcells per axis in the
 * layout of the generated tables.
 */
struct exahype2::dg::limiting::unittests::SubcellProjections final {
  static constexpr std::size_t ProjectionEntries =
    static_cast<std::size_t>(StubLayout::NodesPerAxis) * StubLayout::SubcellsPerAxis;
  static constexpr std::size_t LobattoEntries =
    static_cast<std::size_t>(StubLayout::NodesPerAxis) * StubLayout::NodesPerAxis;

  std::vector<double> quadraturePoints = std::vector<double>(StubLayout::NodesPerAxis);
  std::vector<double> nodesToSubcells  = std::vector<double>(ProjectionEntries);
  std::vector<double> subcellsToNodes  = std::vector<double>(ProjectionEntries);
  std::vector<double> nodesToLobatto   = std::vector<double>(LobattoEntries);

  SubcellProjections();
};

#include "unittests/exahype2/dg/limiting/LimiterTestData.cpph"
