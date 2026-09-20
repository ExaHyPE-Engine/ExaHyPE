// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UpdatePatchTest.h"

#include "exahype2/fv/Solver.h"
#include "exahype2/fv/UpdatePatch.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/tests/ActionSetTestData.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/Constants.h"

namespace {
  constexpr int    PatchSize              = 2;
  constexpr int    NumberOfUnknowns       = 1;
  constexpr int    HaloSize               = 1;
  constexpr double SourceValue            = 0.5;
  constexpr double EigenvalueOfTheKernel  = 2.0;
  constexpr double EigenvalueOfTheSetUp   = 8.0;
  constexpr double TimeStepFactor         = 0.4;
  constexpr double MeshWidthOfTheCell     = 1.0;
  constexpr double MaxAdmissibleMeshWidth = 1.0;
  constexpr double TerminalTime           = 10.0;
  constexpr double TimeStampOfTheCell     = 3.0;
  constexpr double ValueBeforeTheUpdate   = 1.0;

  constexpr double FixedTimeStepSize   = TimeStepFactor * (MeshWidthOfTheCell / PatchSize) / MaxAdmissibleMeshWidth;
  constexpr double ValueAfterTheUpdate = ValueBeforeTheUpdate + FixedTimeStepSize * SourceValue;

  constexpr double AdaptiveTimeStepSize =
    TimeStepFactor * (MeshWidthOfTheCell / PatchSize) / (DIMENSIONS * EigenvalueOfTheSetUp);

  constexpr int volumesPerPatch() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int volumesPerOverlap() {
    int product = 2 * HaloSize;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      product *= PatchSize;
    }
    return product;
  }

  constexpr int PatchValues   = volumesPerPatch() * NumberOfUnknowns;
  constexpr int OverlapValues = volumesPerOverlap() * NumberOfUnknowns;

  template <typename TimeStepping>
  class TestSolver
    : public exahype2::fv::Solver<TestSolver<TimeStepping>, PatchSize, NumberOfUnknowns, 0, TimeStepping> {
  public:
    explicit TestSolver(const double timeStepFactor) :
      exahype2::fv::Solver<TestSolver<TimeStepping>, PatchSize, NumberOfUnknowns, 0, TimeStepping>(timeStepFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}

    std::bitset<DIMENSIONS> PeriodicBoundaries{};

    template <typename ComputePrecision>
    static void sourceTerm(
      [[maybe_unused]] const ComputePrecision* const                state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* const                                       sources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sources[unknown] = static_cast<ComputePrecision>(SourceValue);
      }
    }

    template <typename ComputePrecision>
    static double maxEigenvalue(
      const ComputePrecision* const                                 state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      return std::max(EigenvalueOfTheKernel, static_cast<double>(state[0]));
    }

  protected:
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return MaxAdmissibleMeshWidth; }
  };

  class FixedSolver final : public TestSolver<exahype2::FixedTimeStepping> {
  public:
    FixedSolver() :
      TestSolver(TimeStepFactor) {}
  };

  class AdaptiveSolver final : public TestSolver<exahype2::AdaptiveTimeStepping> {
  public:
    AdaptiveSolver() :
      TestSolver(TimeStepFactor) {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return TerminalTime; }
  };

  struct TestCell final {
    exahype2::fv::tests::CellLabel                                              cellLabel{};
    std::array<exahype2::fv::tests::FaceLabel, peano4::FacesPerCell>            faceLabels{};
    exahype2::fv::tests::Patch<PatchValues>                                     inputPatch{};
    exahype2::fv::tests::Patch<PatchValues>                                     outputPatch{};
    std::array<exahype2::fv::tests::Patch<OverlapValues>, peano4::FacesPerCell> faceInput{};
    std::array<exahype2::fv::tests::Patch<OverlapValues>, peano4::FacesPerCell> faceOutput{};
    int                                                                         postprocessCalls{0};
  };

  TestCell createCell() {
    TestCell cell;
    cell.cellLabel.setTimeStamp(TimeStampOfTheCell);
    cell.inputPatch.fill(ValueBeforeTheUpdate);
    for (auto& face : cell.faceInput) {
      face.fill(ValueBeforeTheUpdate);
    }
    return cell;
  }

  peano4::dataview::CellMarker createMarker() {
    peano4::dataview::CellMarker marker{};
    marker.level = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = MeshWidthOfTheCell;
      marker.centre[axis] = 0.5;
    }
    return marker;
  }

  template <typename SolverType>
  void updateOneCell(SolverType& solver, TestCell& cell, const double postprocessFactor = 1.0) {
    exahype2::fv::updatePatch<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      false,
      false,
      true,
      false,
      false,
      true,
      true,
      double>(
      createMarker(),
      solver,
      cell.cellLabel,
      exahype2::fv::tests::enumerateFaces(cell.faceLabels),
      cell.inputPatch.data(),
      cell.outputPatch.data(),
      exahype2::fv::tests::enumerateFaces(cell.faceInput),
      exahype2::fv::tests::enumerateFaces(cell.faceOutput),
      [&cell, postprocessFactor](
        double* const                 patch,
        [[maybe_unused]] const double timeStamp,
        [[maybe_unused]] const double timeStepSize
      ) {
        cell.postprocessCalls++;
        for (int value = 0; value < PatchValues; value++) {
          patch[value] *= postprocessFactor;
        }
      }
    );
  }

  /**
   * Sets the next time-step bound from one cell's mesh width and wave speed.
   */
  template <typename SolverType>
  void enterTimeStepping(SolverType& solver) {
    solver.startGridInitialisationStep();
    solver.finishGridInitialisationStep();
    solver.startTimeStep();
    solver.update(0.0, 0.0, MeshWidthOfTheCell, MeshWidthOfTheCell);
    if constexpr (SolverType::UsesAdaptiveTimeStepping) {
      solver.setMaxEigenvalue(EigenvalueOfTheSetUp);
    }
    solver.reduceLocalUpdates();
    solver.finishTimeStep();
    solver.startTimeStep();
  }
} // namespace

exahype2::fv::tests::UpdatePatchTest::UpdatePatchTest() :
  tarch::tests::TestCase("exahype2::fv::tests::UpdatePatchTest") {}

void exahype2::fv::tests::UpdatePatchTest::run() {
  testMethod(testUpdateAdvancesThePatchAndTheLabels);
  testMethod(testUpdateReportsTheEigenvalueOfAnAdaptiveSolver);
  testMethod(testPostprocessedStateBoundsTheTimeStep);
  testMethod(testCellOutsideATimeStepOnlyAddsItsStatistics);
}

/**
 * Checks that the patch and its labels advance together.
 */
void exahype2::fv::tests::UpdatePatchTest::testUpdateAdvancesThePatchAndTheLabels() {
  FixedSolver solver;
  enterTimeStepping(solver);
  validateNumericalEquals(solver.getTimeStepSize(), FixedTimeStepSize);

  TestCell cell = createCell();
  updateOneCell(solver, cell);

  for (int value = 0; value < PatchValues; value++) {
    validateNumericalEqualsWithParams1(cell.outputPatch.data()[value], ValueAfterTheUpdate, value);
  }
  validateEquals(cell.postprocessCalls, 1);

  validateEquals(cell.cellLabel.getHasUpdated(), true);
  validateNumericalEquals(cell.cellLabel.getTimeStepSize(), FixedTimeStepSize);
  validateNumericalEquals(cell.cellLabel.getTimeStamp(), TimeStampOfTheCell + FixedTimeStepSize);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateEqualsWithParams1(cell.faceLabels[axis].getUpdated(1), true, axis);
    validateEqualsWithParams1(cell.faceLabels[axis + DIMENSIONS].getUpdated(0), true, axis);
    validateNumericalEqualsWithParams1(
      cell.faceLabels[axis].getUpdatedTimeStamp(1),
      TimeStampOfTheCell + FixedTimeStepSize,
      axis
    );
    validateNumericalEqualsWithParams1(
      cell.faceLabels[axis + DIMENSIONS].getUpdatedTimeStamp(0),
      TimeStampOfTheCell + FixedTimeStepSize,
      axis
    );
    validateEqualsWithParams1(cell.faceLabels[axis].getUpdated(0), false, axis);
  }

  solver.reduceLocalUpdates();
  validateNumericalEquals(solver.getMinTimeStamp(), TimeStampOfTheCell + FixedTimeStepSize);
  validateNumericalEquals(solver.getMinTimeStepSize(), FixedTimeStepSize);
}

/**
 * Checks that the kernel's wave speed bounds the next time step.
 */
void exahype2::fv::tests::UpdatePatchTest::testUpdateReportsTheEigenvalueOfAnAdaptiveSolver() {
  AdaptiveSolver solver;
  enterTimeStepping(solver);
  validateNumericalEquals(solver.getTimeStepSize(), AdaptiveTimeStepSize);

  TestCell cell = createCell();
  updateOneCell(solver, cell);

  validateNumericalEquals(cell.cellLabel.getTimeStamp(), TimeStampOfTheCell + AdaptiveTimeStepSize);

  solver.reduceLocalUpdates();
  solver.finishTimeStep();
  validateNumericalEquals(solver.getMaxEigenvalue(), EigenvalueOfTheKernel);
}

/**
 * Checks that postprocessing contributes to the next time-step bound.
 */
void exahype2::fv::tests::UpdatePatchTest::testPostprocessedStateBoundsTheTimeStep() {
  constexpr double PostprocessFactor = 8.0;
  AdaptiveSolver   solver;
  enterTimeStepping(solver);
  TestCell cell = createCell();

  updateOneCell(solver, cell, PostprocessFactor);
  solver.reduceLocalUpdates();
  solver.finishTimeStep();

  const double expectedValue = PostprocessFactor * (ValueBeforeTheUpdate + AdaptiveTimeStepSize * SourceValue);
  validateNumericalEquals(solver.getMaxEigenvalue(), expectedValue);
}

/**
 * Checks that an inactive cell keeps its patch and labels.
 */
void exahype2::fv::tests::UpdatePatchTest::testCellOutsideATimeStepOnlyAddsItsStatistics() {
  FixedSolver solver;
  enterTimeStepping(solver);
  solver.startPlottingStep();

  TestCell cell = createCell();
  cell.cellLabel.setTimeStepSize(FixedTimeStepSize);
  updateOneCell(solver, cell);

  for (int value = 0; value < PatchValues; value++) {
    validateNumericalEqualsWithParams1(cell.outputPatch.data()[value], 0.0, value);
  }
  validateEquals(cell.postprocessCalls, 0);
  validateEquals(cell.cellLabel.getHasUpdated(), false);
  validateNumericalEquals(cell.cellLabel.getTimeStamp(), TimeStampOfTheCell);

  solver.reduceLocalUpdates();
  validateNumericalEquals(solver.getMinTimeStamp(), TimeStampOfTheCell);
}
