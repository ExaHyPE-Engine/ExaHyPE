// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiresolutionDetailTest.h"

#include "exahype2/MultiresolutionDetail.h"
#include "exahype2/RefinementCommand.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace {
  TARGET_DECLARE
  constexpr int    NumberOfDofsPerAxis   = 4;
  constexpr int    NumberOfVolumes       = peano4::power(NumberOfDofsPerAxis, DIMENSIONS);
  constexpr int    NumberOfPatchUnknowns = 3;
  constexpr int    NumberOfPatchValues   = NumberOfVolumes * NumberOfPatchUnknowns;
  constexpr double Eps                   = 1.0e-2;
  constexpr double CoarsenSafetyFactor   = 9.0;

  template <typename Value>
  struct Density final {
    INLINE_METHOD GPU_CALLABLE_METHOD double operator()(const Value* const volume) const { return volume[0]; }
  };

  template <typename Value>
  struct Pressure final {
    INLINE_METHOD GPU_CALLABLE_METHOD double operator()(const Value* const volume) const { return volume[2]; }
  };

  struct RefineEveryPatchOnDetail final {
    const double*                patches{nullptr};
    exahype2::RefinementCommand* commands{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int patch) const {
      commands[patch] = exahype2::refineOnDetail(
        patches + static_cast<std::ptrdiff_t>(patch) * NumberOfPatchValues,
        exahype2::getVolumeCentresInUnitInterval<NumberOfDofsPerAxis>(),
        NumberOfPatchUnknowns,
        Eps,
        CoarsenSafetyFactor,
        Density<double>{},
        Pressure<double>{}
      );
    }
  };

  struct ComputeDetailOfEveryPatch final {
    const double* patches{nullptr};
    double*       details{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int patch) const {
      details[patch] = exahype2::computeMultiresolutionDetail(
        patches + static_cast<std::ptrdiff_t>(patch) * NumberOfVolumes,
        exahype2::getVolumeCentresInUnitInterval<NumberOfDofsPerAxis>(),
        1,
        Density<double>{}
      );
    }
  };
  TARGET_DECLARE_END

  constexpr auto VolumeCentres = exahype2::getVolumeCentresInUnitInterval<NumberOfDofsPerAxis>();
  constexpr std::array<double, NumberOfDofsPerAxis> LobattoNodes{0.0, 0.2763932, 0.7236068, 1.0};

  double getValueOf(const double* const volume) { return volume[0]; }

  double getDetailOf(const std::vector<double>& patch) {
    return exahype2::computeMultiresolutionDetail(patch.data(), VolumeCentres, 1, getValueOf);
  }

  double getVolumeCoordinate(const int volume) {
    return (static_cast<double>(volume) + 0.5) / static_cast<double>(NumberOfDofsPerAxis) - 0.5;
  }

  std::vector<double> getAsymmetricPatch() {
    std::vector<double> patch(NumberOfVolumes);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
      double value = 1.0 + std::sin(3.0 * getVolumeCoordinate(counter(0)));
      for (int d = 1; d < DIMENSIONS; d++) {
        value += 0.3 * std::cos(2.0 * getVolumeCoordinate(counter(d)))
          + 0.7 * getVolumeCoordinate(counter(0)) * getVolumeCoordinate(counter(d));
      }
      patch[peano4::dLinearised(counter, NumberOfDofsPerAxis)] = value;
    });
    return patch;
  }

  std::vector<double> getSmoothPatch() {
    std::vector<double> patch(static_cast<std::size_t>(NumberOfPatchValues), 0.0);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * NumberOfPatchUnknowns;
      patch[index]     = 1.0 + 0.25 * volume(0) - 0.5 * volume(1);
      patch[index + 1] = -3.0;
      patch[index + 2] = 2.0;
    });
    return patch;
  }

  std::vector<double> getPatchWithAPressureJump() {
    std::vector<double> patch = getSmoothPatch();
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * NumberOfPatchUnknowns;
      patch[index + 2] = volume(0) < NumberOfDofsPerAxis / 2 ? 2.0 : 20.0;
    });
    return patch;
  }
} // namespace

exahype2::tests::MultiresolutionDetailTest::MultiresolutionDetailTest() :
  TestCase("exahype2::tests::MultiresolutionDetailTest") {}

void
exahype2::tests::MultiresolutionDetailTest::run() {
  testMethod(testVolumeCentresAreEquidistant);
  testMethod(testConstantFieldHasZeroDetail);
  testMethod(testAffineFieldHasZeroDetail);
  testMethod(testStepFieldHasLargeDetail);
  testMethod(testDetailIsBitwiseInvariantUnderReflectionsAndSwaps);
  testMethod(testNonFiniteValueRefines);
  testMethod(testCustomNodesFitTheirOwnAffineField);
  testMethod(testStridedPatchWithIndicator);
  testMethod(testSingleNodePatchHasZeroDetail);
  testMethod(testRefinementCommandFromDetail);
  testMethod(testRefineOnDetail);
  testMethod(testRefineOnDetailRunsOnTheDevice);
  testMethod(testDetailIsBitwiseInvariantUnderSwapsOnTheDevice);
}

void
exahype2::tests::MultiresolutionDetailTest::testVolumeCentresAreEquidistant() {
  for (int volume = 0; volume < NumberOfDofsPerAxis; volume++) {
    validateNumericalEqualsWithParams1(
      VolumeCentres[static_cast<std::size_t>(volume)],
      getVolumeCoordinate(volume) + 0.5,
      volume
    );
  }
}

void
exahype2::tests::MultiresolutionDetailTest::testConstantFieldHasZeroDetail() {
  const std::vector<double> patch(NumberOfVolumes, 1.0);
  validateNumericalEquals(getDetailOf(patch), 0.0);

  const std::vector<float> singlePrecisionPatch(NumberOfVolumes, 1.0F);
  validateNumericalEquals(
    exahype2::computeMultiresolutionDetail(
      singlePrecisionPatch.data(),
      VolumeCentres,
      1,
      [](const float* const volume) { return volume[0]; }
    ),
    0.0
  );
}

void
exahype2::tests::MultiresolutionDetailTest::testAffineFieldHasZeroDetail() {
  std::vector<double> patch(NumberOfVolumes);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    double value = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      value += static_cast<double>(d + 2) * getVolumeCoordinate(counter(d));
    }
    patch[peano4::dLinearised(counter, NumberOfDofsPerAxis)] = value;
  });
  validateTrue(std::abs(getDetailOf(patch)) < 1.0e-12);
}

/**
 * A jump between two halves of the patch has a detail far above the one of a ramp over the same values.
 */
void
exahype2::tests::MultiresolutionDetailTest::testStepFieldHasLargeDetail() {
  std::vector<double> step(NumberOfVolumes);
  std::vector<double> ramp(NumberOfVolumes);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    const int linearised = peano4::dLinearised(counter, NumberOfDofsPerAxis);
    step[linearised]     = (counter(0) < NumberOfDofsPerAxis / 2) ? 1.0 : 2.0;
    ramp[linearised]     = 1.0 + getVolumeCoordinate(counter(0));
  });
  const double stepDetail = getDetailOf(step);
  const double rampDetail = getDetailOf(ramp);
  validateTrue(stepDetail > 0.05);
  validateTrue(stepDetail > 1.0e6 * (rampDetail + 1.0e-15));
}

void
exahype2::tests::MultiresolutionDetailTest::testDetailIsBitwiseInvariantUnderReflectionsAndSwaps() {
  const std::vector<double> patch = getAsymmetricPatch();
  std::vector<double>       transformed(NumberOfVolumes);
  for (const auto& nodes : {VolumeCentres, LobattoNodes}) {
    const double patchDetail = exahype2::computeMultiresolutionDetail(patch.data(), nodes, 1, getValueOf);
    validateTrue(patchDetail > 1.0e-6);

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
        tarch::la::Vector<DIMENSIONS, int> reflected = counter;
        reflected(axis)                              = NumberOfDofsPerAxis - 1 - counter(axis);
        transformed[peano4::dLinearised(counter, NumberOfDofsPerAxis)] =
          patch[peano4::dLinearised(reflected, NumberOfDofsPerAxis)];
      });
      validateEquals(exahype2::computeMultiresolutionDetail(transformed.data(), nodes, 1, getValueOf), patchDetail);
    }

    for (int firstAxis = 0; firstAxis < DIMENSIONS; firstAxis++) {
      for (int secondAxis = firstAxis + 1; secondAxis < DIMENSIONS; secondAxis++) {
        peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
          tarch::la::Vector<DIMENSIONS, int> swapped = counter;
          swapped(firstAxis)                         = counter(secondAxis);
          swapped(secondAxis)                        = counter(firstAxis);
          transformed[peano4::dLinearised(counter, NumberOfDofsPerAxis)] =
            patch[peano4::dLinearised(swapped, NumberOfDofsPerAxis)];
        });
        validateEquals(exahype2::computeMultiresolutionDetail(transformed.data(), nodes, 1, getValueOf), patchDetail);
      }
    }
  }
}

void
exahype2::tests::MultiresolutionDetailTest::testNonFiniteValueRefines() {
  std::vector<double> patch(NumberOfVolumes, 1.0);
  validateNumericalEqualsWithEps(getDetailOf(patch), 0.0, 1e-12);

  patch[NumberOfVolumes / 2] = std::numeric_limits<double>::quiet_NaN();
  validateTrue(std::isinf(getDetailOf(patch)));

  patch[NumberOfVolumes / 2] = std::numeric_limits<double>::infinity();
  validateTrue(std::isinf(getDetailOf(patch)));
}

void
exahype2::tests::MultiresolutionDetailTest::testCustomNodesFitTheirOwnAffineField() {
  std::vector<double> patch(NumberOfVolumes, 0.0);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    double value = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      value += (2.0 + d) * LobattoNodes[static_cast<std::size_t>(node(d))];
    }
    patch[peano4::dLinearised(node, NumberOfDofsPerAxis)] = value;
  });

  validateNumericalEqualsWithEps(
    exahype2::computeMultiresolutionDetail(patch.data(), LobattoNodes, 1, getValueOf),
    0.0,
    1e-12
  );
  validate(getDetailOf(patch) > 1e-3);
}

/**
 * The indicator reads one unknown of an AoS patch and the other unknowns leave the detail unchanged.
 */
void
exahype2::tests::MultiresolutionDetailTest::testStridedPatchWithIndicator() {
  constexpr int       NumberOfUnknowns = 3;
  std::vector<double> patch(static_cast<std::size_t>(NumberOfVolumes) * NumberOfUnknowns, 0.0);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * NumberOfUnknowns;
    patch[index]     = volume(0) < NumberOfDofsPerAxis / 2 ? -5.0 : 5.0;
    patch[index + 1] = 2.0 + 0.5 * volume(0) + 0.25 * volume(1);
    patch[index + 2] = 1000.0 * volume(0) * volume(0);
  });

  const double smoothDetail = exahype2::computeMultiresolutionDetail(
    patch.data(),
    VolumeCentres,
    NumberOfUnknowns,
    [](const double* const volume) { return volume[1]; }
  );
  const double stepDetail = exahype2::computeMultiresolutionDetail(
    patch.data(),
    VolumeCentres,
    NumberOfUnknowns,
    [](const double* const volume) { return volume[0]; }
  );

  validateNumericalEqualsWithEps(smoothDetail, 0.0, 1e-12);
  validate(stepDetail > 1e-1);
}

void
exahype2::tests::MultiresolutionDetailTest::testSingleNodePatchHasZeroDetail() {
  const double value = 42.0;
  validateEquals(
    exahype2::computeMultiresolutionDetail(&value, exahype2::getVolumeCentresInUnitInterval<1>(), 1, getValueOf),
    0.0
  );
}

/**
 * The command refines above eps, erases below eps over the safety factor and keeps in between.
 */
void
exahype2::tests::MultiresolutionDetailTest::testRefinementCommandFromDetail() {
  validateTrue(
    exahype2::getRefinementCommandFromDetail(2.0e-2, Eps, CoarsenSafetyFactor) == exahype2::RefinementCommand::Refine
  );
  validateTrue(
    exahype2::getRefinementCommandFromDetail(1.0e-2, Eps, CoarsenSafetyFactor) == exahype2::RefinementCommand::Keep
  );
  validateTrue(
    exahype2::getRefinementCommandFromDetail(5.0e-3, Eps, CoarsenSafetyFactor) == exahype2::RefinementCommand::Keep
  );
  validateTrue(
    exahype2::getRefinementCommandFromDetail(Eps / CoarsenSafetyFactor, Eps, CoarsenSafetyFactor)
    == exahype2::RefinementCommand::Keep
  );
  validateTrue(
    exahype2::getRefinementCommandFromDetail(1.0e-4, Eps, CoarsenSafetyFactor) == exahype2::RefinementCommand::Erase
  );
  validateTrue(
    exahype2::getRefinementCommandFromDetail(0.0, Eps, CoarsenSafetyFactor) == exahype2::RefinementCommand::Erase
  );
}

/**
 * refineOnDetail() keeps the cell of a null patch, erases an affine patch and refines a jump in one indicator.
 */
void
exahype2::tests::MultiresolutionDetailTest::testRefineOnDetail() {
  const double* const noSolution = nullptr;
  validateTrue(
    exahype2::refineOnDetail(
      noSolution,
      VolumeCentres,
      NumberOfPatchUnknowns,
      Eps,
      CoarsenSafetyFactor,
      Density<double>{}
    )
    == exahype2::RefinementCommand::Keep
  );

  const std::vector<double> smoothPatch = getSmoothPatch();
  validateTrue(
    exahype2::refineOnDetail(
      smoothPatch.data(),
      VolumeCentres,
      NumberOfPatchUnknowns,
      Eps,
      CoarsenSafetyFactor,
      Density<double>{},
      Pressure<double>{}
    )
    == exahype2::RefinementCommand::Erase
  );

  const std::vector<double> jumpPatch = getPatchWithAPressureJump();
  validateTrue(
    exahype2::refineOnDetail(
      jumpPatch.data(),
      VolumeCentres,
      NumberOfPatchUnknowns,
      Eps,
      CoarsenSafetyFactor,
      Density<double>{}
    )
    == exahype2::RefinementCommand::Erase
  );
  validateTrue(
    exahype2::refineOnDetail(
      jumpPatch.data(),
      VolumeCentres,
      NumberOfPatchUnknowns,
      Eps,
      CoarsenSafetyFactor,
      Density<double>{},
      Pressure<double>{}
    )
    == exahype2::RefinementCommand::Refine
  );

  const std::vector<float> singlePrecisionPatch(jumpPatch.begin(), jumpPatch.end());
  validateTrue(
    exahype2::refineOnDetail(
      singlePrecisionPatch.data(),
      VolumeCentres,
      NumberOfPatchUnknowns,
      Eps,
      CoarsenSafetyFactor,
      Density<float>{},
      Pressure<float>{}
    )
    == exahype2::RefinementCommand::Refine
  );
}

void
exahype2::tests::MultiresolutionDetailTest::testRefineOnDetailRunsOnTheDevice() {
  const std::vector<double> smoothPatch = getSmoothPatch();
  const std::vector<double> jumpPatch   = getPatchWithAPressureJump();
  std::vector<double>       patches(smoothPatch);
  patches.insert(patches.end(), jumpPatch.begin(), jumpPatch.end());
  constexpr int NumberOfPatches = 2;

  tarch::accelerator::Buffer<double> devicePatches(patches.size(), tarch::MemoryLocation::Device);
  devicePatches.setValues(patches);
  tarch::accelerator::Buffer<exahype2::RefinementCommand> commands(NumberOfPatches, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(NumberOfPatches),
    RefineEveryPatchOnDetail{devicePatches.getData(), commands.getData()}
  );

  const std::vector<exahype2::RefinementCommand> deviceCommands = commands.getValues();
  validateTrue(deviceCommands[0] == exahype2::RefinementCommand::Erase);
  validateTrue(deviceCommands[1] == exahype2::RefinementCommand::Refine);
}

void
exahype2::tests::MultiresolutionDetailTest::testDetailIsBitwiseInvariantUnderSwapsOnTheDevice() {
  const std::vector<double> patch = getAsymmetricPatch();
  std::vector<double>       patches(2 * NumberOfVolumes);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    tarch::la::Vector<DIMENSIONS, int> swapped = counter;
    swapped(0)                                 = counter(1);
    swapped(1)                                 = counter(0);
    const int volume                           = peano4::dLinearised(counter, NumberOfDofsPerAxis);
    patches[volume]                            = patch[volume];
    patches[NumberOfVolumes + volume]          = patch[peano4::dLinearised(swapped, NumberOfDofsPerAxis)];
  });

  constexpr int                      NumberOfPatches = 2;
  tarch::accelerator::Buffer<double> devicePatches(patches.size(), tarch::MemoryLocation::Device);
  devicePatches.setValues(patches);
  tarch::accelerator::Buffer<double> details(NumberOfPatches, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(NumberOfPatches),
    ComputeDetailOfEveryPatch{devicePatches.getData(), details.getData()}
  );

  const std::vector<double> deviceDetails = details.getValues();
  validateTrue(deviceDetails[0] > 1.0e-6);
  validateEquals(deviceDetails[1], deviceDetails[0]);
}
