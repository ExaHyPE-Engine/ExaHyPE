// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include <bitset>
#include <cmath>

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/ader/Solver.h"
#include "exahype2/fv/reconstruction/WENO.h"
#include "exahype2/fv/weno/NonlinearWeights.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"

namespace {
  TARGET_DECLARE
  constexpr double TimeStepFactor = 0.5;
  constexpr double Pi             = 3.14159265358979323846;

  /**
   * The advection velocity along an axis.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr double getVelocity(const int axis) { return 1.0 / (1 << axis); }

  /**
   * The average of 1 + Amplitude sin(2 pi (x_0 + ... + x_d)) over the box of size h around x at time t
   * of an advection with getVelocity(). Amplitude zero gives a uniform state.
   */
  template <bool Smooth>
  INLINE_METHOD GPU_CALLABLE_METHOD double getAverage(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t
  ) {
    if constexpr (not Smooth) {
      return 1.0;
    } else {
      double phase  = 0.0;
      double factor = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        phase += x(axis) - getVelocity(axis) * t;
        factor *= std::sin(Pi * h(axis)) / (Pi * h(axis));
      }
      return 1.0 + 0.5 * std::sin(2.0 * Pi * phase) * factor;
    }
  }
  TARGET_DECLARE_END

  /**
   * Linear advection of one unknown with getVelocity() under ADER-WENO of order Order on patches of
   * Volumes volumes per axis.
   */
  template <int Order, int Volumes, bool Smooth>
  struct AdvectionSolver final : exahype2::fv::ader::Solver<AdvectionSolver<Order, Volumes, Smooth>, double, double> {
    using Base = exahype2::fv::ader::Solver<AdvectionSolver<Order, Volumes, Smooth>, double, double>;

    static constexpr int PatchSize                  = Volumes;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using Reconstruction = exahype2::fv::reconstruction::WENO<Order, exahype2::fv::weno::JiangShu<8>>;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Adaptive;

    AdvectionSolver() :
      Base(TimeStepFactor, "Advection", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = getAverage<Smooth>(x, h, 0.0);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* const                                           inside,
      double* const                                                 outside,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal
    ) {
      outside[0] = inside[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t
    ) {
      return exahype2::RefinementCommand::Keep;
    }

    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const P* const                               Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      const int                                                     normal
    ) {
      return getVelocity(normal);
    }

    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const P* const                                                Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      const int                                                     normal,
      P* const                                                      F
    ) {
      F[0] = static_cast<P>(getVelocity(normal)) * Q[0];
    }
  };

  TARGET_DECLARE
  /**
   * Adds the integral of the unknown over the patch of one cell to the accumulator, or with Error the
   * integral of its distance to the exact average at timeStamp.
   */
  template <typename Solver, bool Smooth, bool Error>
  struct IntegrateOverCells final {
    peano4::Partition                    partition{};
    typename Solver::Patches::ConstView solution{};
    double                               timeStamp{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, double& accumulator) const {
      constexpr int Volumes = Solver::PatchSize;
      const int     cell    = partition.getCells()[entry];
      const auto    centre  = partition.getCellCentre(cell);
      const auto    width   = partition.getCellWidth(cell);
      const auto    size    = exahype2::fv::getVolumeSize(width, Volumes);
      double        volume  = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        volume *= size(axis);
      }
      const double* const patch = solution.getChunk(cell);
      for (int linear = 0; linear < peano4::power(Volumes, DIMENSIONS); linear++) {
        const auto   index = peano4::dDelinearised(linear, Volumes);
        const double value = patch[linear];
        if constexpr (Error) {
          const auto x = exahype2::fv::getVolumeCentre(centre, width, Volumes, index);
          accumulator += std::abs(value - getAverage<Smooth>(x, size, timeStamp)) * volume;
        } else {
          accumulator += value * volume;
        }
      }
    }
  };
  TARGET_DECLARE_END

  template <typename Solver, bool Smooth, bool Error>
  double integrate(const Solver& solver, const peano4::Partition& partition) {
    double result = 0.0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      IntegrateOverCells<Solver, Smooth, Error>{
        partition.onDevice(),
        solver.getSolution().getView(),
        solver.getTimeStepping().getTimeStamp()
      },
      tarch::accelerator::Sum<double>{result}
    );
    double globalResult = 0.0;
    tarch::mpi::allreduce(
      &result,
      &globalResult,
      1,
      MPI_DOUBLE,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    return globalResult;
  }

  exahype2::Limits getLimits(const double terminalTime) {
    exahype2::Limits limits;
    limits.terminalTime           = terminalTime;
    limits.minAdmissiblePatchSize = 0.0;
    limits.maxAdmissiblePatchSize = 1.0;
    return limits;
  }

  const peano4::Partition& buildPeriodicGrid(const int depth) {
    peano4::tests::buildRegularGrid(depth, std::bitset<DIMENSIONS>((1ULL << DIMENSIONS) - 1));
    return peano4::Spacetree::getInstance().getPartition();
  }

  /**
   * Reduces the values of solver over all ranks in the one allreduce that ends a step of a run.
   */
  template <typename Solver>
  void finishStep(Solver& solver) {
    tarch::mpi::ReductionRecord record;
    solver.contributeTo(record);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    solver.adoptFrom(record);
  }

  template <typename Solver>
  void startOn(Solver& solver, const peano4::Partition& partition, const double terminalTime) {
    solver.setLimits(getLimits(terminalTime));
    solver.allocate(partition);
    exahype2::RefinementRequests requests;
    requests.clear(partition.getNumberOfSlots());
    solver.initialise(partition, requests);
    finishStep(solver);
  }

  template <typename Solver>
  void advance(Solver& solver, const peano4::Partition& partition, const int steps) {
    exahype2::RefinementRequests requests;
    for (int step = 0; step < steps; step++) {
      requests.clear(partition.getNumberOfSlots());
      solver.performTimeStep(partition, requests);
      finishStep(solver);
    }
  }

  /**
   * Returns the L1 error of smooth advection of order Order on patches of Volumes volumes per axis on
   * the periodic regular grid of depth after the time endTime.
   */
  template <int Order, int Volumes>
  double getAdvectionError(const int depth, const double endTime) {
    using Solver          = AdvectionSolver<Order, Volumes, true>;
    const auto& partition = buildPeriodicGrid(depth);
    Solver      solver;
    startOn(solver, partition, endTime);
    exahype2::RefinementRequests requests;
    for (int step = 0; step < 100000 and solver.getTimeStepping().getTimeStamp() < endTime; step++) {
      requests.clear(partition.getNumberOfSlots());
      solver.performTimeStep(partition, requests);
      finishStep(solver);
    }
    const double error = integrate<Solver, true, true>(solver, partition);
    solver.release();
    return error;
  }
} // namespace

exahype2::fv::ader::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::fv::ader::tests::SolverTest") {}

void
exahype2::fv::ader::tests::SolverTest::run() {
  testMethod(testAUniformStateStaysUniform);
  testMethod(testPeriodicAdvectionConservesMass);
  testMethod(testALevelJumpConservesMass);
  testMethod(testSmoothAdvectionConvergesAtHighOrder);
  testMethod(testReleaseFreesEveryPool);
}

void
exahype2::fv::ader::tests::SolverTest::testAUniformStateStaysUniform() {
  using Solver          = AdvectionSolver<2, 4, false>;
  const auto& partition = buildPeriodicGrid(2);
  Solver      solver;
  startOn(solver, partition, 1.0);
  advance(solver, partition, 4);
  const double error = integrate<Solver, false, true>(solver, partition);
  validateWithParams1(error < 1.0e-12, error);
  validate(solver.getTimeStepping().getTimeStamp() > 0.0);
  solver.release();
}

void
exahype2::fv::ader::tests::SolverTest::testPeriodicAdvectionConservesMass() {
  using Solver          = AdvectionSolver<3, 6, true>;
  const auto& partition = buildPeriodicGrid(2);
  Solver      solver;
  startOn(solver, partition, 1.0);
  const double massBefore = integrate<Solver, true, false>(solver, partition);
  advance(solver, partition, 4);
  const double massAfter = integrate<Solver, true, false>(solver, partition);
  validateWithParams2(std::abs(massAfter - massBefore) <= 1.0e-12 * std::abs(massBefore), massBefore, massAfter);
  solver.release();
}

/**
 * The coarse cells beside a refined block add the restricted fluxes of the fine faces, so the
 * total mass stays.
 */
void
exahype2::fv::ader::tests::SolverTest::testALevelJumpConservesMass() {
  using Solver          = AdvectionSolver<2, 4, true>;
  constexpr int Depth   = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  const auto&   initial = buildPeriodicGrid(Depth);
  Solver        solver;
  startOn(solver, initial, 1.0);
  advance(solver, initial, 1);

  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.markPersistentDataLive();
  peano4::Regrid regrid;
  if (not spacetree.regrid({peano4::tests::getRefinementOfCentreCell(Depth)}, regrid)) {
    tarch::terminateWithError("The regrid of the test grid must change the grid.");
  }
  const auto&                  partition = spacetree.getPartition();
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.regrid(partition, regrid, requests);
  spacetree.finishRegrid();
  finishStep(solver);
  validate(partition.getMaximumLevel() > partition.getMinimumLevel());

  const double massBefore = integrate<Solver, true, false>(solver, partition);
  advance(solver, partition, 4);
  const double massAfter = integrate<Solver, true, false>(solver, partition);
  validateWithParams2(std::abs(massAfter - massBefore) <= 1.0e-12 * std::abs(massBefore), massBefore, massAfter);
  solver.release();
}

/**
 * The predictor lifts the order in time to the order of the reconstruction, so a refinement by the
 * subdivision factor shrinks the error by about its power order + 1.
 */
void
exahype2::fv::ader::tests::SolverTest::testSmoothAdvectionConvergesAtHighOrder() {
#if DIMENSIONS == 2
  constexpr double EndTime = 0.12;
  const double     factor  = std::log(static_cast<double>(peano4::SubdivisionFactor));

  const double secondOrderCoarseError = getAdvectionError<2, 8>(1, EndTime);
  const double secondOrderFineError   = getAdvectionError<2, 8>(2, EndTime);
  const double secondOrder            = std::log(secondOrderCoarseError / secondOrderFineError) / factor;
  validateWithParams3(secondOrder > 2.7, secondOrderCoarseError, secondOrderFineError, secondOrder);

  const double thirdOrderCoarseError = getAdvectionError<3, 8>(1, EndTime);
  const double thirdOrderFineError   = getAdvectionError<3, 8>(2, EndTime);
  const double thirdOrder            = std::log(thirdOrderCoarseError / thirdOrderFineError) / factor;
  validateWithParams3(thirdOrder > 3.7, thirdOrderCoarseError, thirdOrderFineError, thirdOrder);
#endif
}

void
exahype2::fv::ader::tests::SolverTest::testReleaseFreesEveryPool() {
  using Solver          = AdvectionSolver<2, 4, true>;
  const auto& partition = buildPeriodicGrid(1);
  Solver      solver;
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  startOn(solver, partition, 1.0);
  advance(solver, partition, 1);
  validate(solver.getAllocatedBytes().getTotal() > solver.getSolution().getAllocatedBytes().getTotal());
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
}
