// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting::tests {
  struct StubLayout;
  template <exahype2::TimeStepping::Rule Rule>
  struct StubSolverOf;
  struct RelaxedStubSolver;
  struct RiemannStubSolver;
  struct SubcellProjections;

  using StubSolver          = StubSolverOf<exahype2::TimeStepping::Rule::Adaptive>;
  using FixedStepStubSolver = StubSolverOf<exahype2::TimeStepping::Rule::Fixed>;
} // namespace exahype2::dg::limiting::tests

TARGET_DECLARE
/**
 * The cell layout of the limiter tests.
 */
struct exahype2::dg::limiting::tests::StubLayout final {
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
 * A solver with the first value as its one observable and its wave speed. The first value must be
 * positive.
 */
template <exahype2::TimeStepping::Rule Rule>
struct exahype2::dg::limiting::tests::StubSolverOf {
  static constexpr int DGOrder                    = StubLayout::NodesPerAxis - 1;
  static constexpr int PatchSize                  = StubLayout::SubcellsPerAxis;
  static constexpr int NumberOfUnknowns           = 1;
  static constexpr int NumberOfAuxiliaryVariables = StubLayout::NumberOfData - NumberOfUnknowns;
  static constexpr int NumberOfObservables        = StubLayout::NumberOfObservables;

  static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = Rule;

  using Reconstruction         = exahype2::fv::reconstruction::Constant;
  using FallbackReconstruction = void;

  static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
    double*                                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void mapDmpObservables(double* observables, const double* Q);

  static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const double*                                Q,
    int                                          numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double*                                Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double*                                      F
  );
};

/**
 * The stub solver with a relaxation term that doubles the specified parameter.
 */
struct exahype2::dg::limiting::tests::RelaxedStubSolver final : exahype2::dg::limiting::tests::StubSolver {
  static INLINE_METHOD GPU_CALLABLE_METHOD double dmpRelaxationParameter(
    int    observable,
    double specifiedRelaxationParameter
  );
};

struct exahype2::dg::limiting::tests::RiemannStubSolver final : exahype2::dg::limiting::tests::StubSolver {
  static constexpr double SubcellWaveSpeed = 2.0;

  static INLINE_METHOD GPU_CALLABLE_METHOD double solveSubcellRiemannProblem(
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
};
TARGET_DECLARE_END

/**
 * The projection tables of the two-node Gauss-Legendre basis and three subcells per axis in the
 * layout of the generated tables.
 */
struct exahype2::dg::limiting::tests::SubcellProjections final {
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

#include "exahype2/dg/limiting/tests/LimiterTestData.cpph"
