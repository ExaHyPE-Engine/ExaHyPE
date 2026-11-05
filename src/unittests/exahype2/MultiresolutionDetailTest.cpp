// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
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
#include "tarch/testing/UnitTest.h"

namespace {
  TARGET_DECLARE
  constexpr int    NumberOfDofsPerAxis   = 4;
  constexpr int    NumberOfVolumes       = peano4::power(NumberOfDofsPerAxis, DIMENSIONS);
  constexpr int    NumberOfPatchUnknowns = 3;
  constexpr int    NumberOfPatchValues   = NumberOfVolumes * NumberOfPatchUnknowns;
  constexpr double Eps                   = 1.0e-2;
  constexpr double CoarsenSafetyFactor   = 9.0;

  template <typename Value>
  struct PatchState final {
    Value density{};
    Value velocity{};
    Value pressure{};
  };

  struct Density final {
    template <typename Value>
    INLINE_METHOD GPU_CALLABLE_METHOD double operator()(const PatchState<Value>& state) const {
      return state.density;
    }
  };

  struct Pressure final {
    template <typename Value>
    INLINE_METHOD GPU_CALLABLE_METHOD double operator()(const PatchState<Value>& state) const {
      return state.pressure;
    }
  };

  struct RefineEveryPatchOnDetail final {
    const PatchState<double>*    states{nullptr};
    exahype2::RefinementCommand* commands{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int patch) const {
      using CellStates = PatchState<double>[NumberOfVolumes];
      commands[patch]  = exahype2::refineOnDetail(
        reinterpret_cast<const CellStates*>(states + static_cast<std::ptrdiff_t>(patch) * NumberOfVolumes),
        exahype2::getVolumeCentresInUnitInterval<NumberOfDofsPerAxis>(),
        Eps,
        CoarsenSafetyFactor,
        Density{},
        Pressure{}
      );
    }
  };

  /**
   * Returns the density of a volume that stores the density only.
   */
  struct DensityOfVolume final {
    INLINE_METHOD GPU_CALLABLE_METHOD double operator()(const double* const volume) const { return volume[0]; }
  };

  struct ComputeDetailOfEveryPatch final {
    const double* patches{nullptr};
    double*       details{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int patch) const {
      details[patch] = exahype2::computeMultiresolutionDetail(
        patches + static_cast<std::ptrdiff_t>(patch) * NumberOfVolumes,
        exahype2::getVolumeCentresInUnitInterval<NumberOfDofsPerAxis>(),
        1,
        DensityOfVolume{}
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
      for (int axis = 1; axis < DIMENSIONS; axis++) {
        value += 0.3 * std::cos(2.0 * getVolumeCoordinate(counter(axis)))
          + 0.7 * getVolumeCoordinate(counter(0)) * getVolumeCoordinate(counter(axis));
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

  template <typename Value>
  void setStatesOf(const std::vector<double>& patch, PatchState<Value> (&states)[NumberOfVolumes]) {
    for (int volume = 0; volume < NumberOfVolumes; volume++) {
      const int index = volume * NumberOfPatchUnknowns;
      states[volume]  = {
        static_cast<Value>(patch[index]),
        static_cast<Value>(patch[index + 1]),
        static_cast<Value>(patch[index + 2])
      };
    }
  }
} // namespace

UNIT_TEST("The volume centres of the unit interval lie at odd multiples of half the volume width") {
  GIVEN("The unit interval divided into the four volumes of a patch axis") {
    constexpr std::size_t NumberOfVolumesPerAxis = NumberOfDofsPerAxis;

    WHEN("The function getVolumeCentresInUnitInterval() returns the centres of the volumes") {
      constexpr auto Centres = exahype2::getVolumeCentresInUnitInterval<NumberOfVolumesPerAxis>();

      THEN("Every centre lies at an odd multiple of half the volume width") {
        for (std::size_t volume = 0; volume < NumberOfVolumesPerAxis; volume++) {
          VALIDATE_NUMERICAL_EQUALS(Centres[volume], getVolumeCoordinate(static_cast<int>(volume)) + 0.5, volume);
        }
      }
    }
  }
}

UNIT_TEST("A constant field has zero detail in double and in single precision") {
  GIVEN("A patch of the value one in double and in single precision") {
    const std::vector<double> patch(NumberOfVolumes, 1.0);
    const std::vector<float>  singlePrecisionPatch(NumberOfVolumes, 1.0F);

    WHEN("The function computeMultiresolutionDetail() evaluates both patches") {
      const double detail                = getDetailOf(patch);
      const double singlePrecisionDetail = exahype2::computeMultiresolutionDetail(
        singlePrecisionPatch.data(),
        VolumeCentres,
        1,
        [](const float* const volume) { return volume[0]; }
      );

      THEN("Both details are zero") {
        VALIDATE_NUMERICAL_EQUALS(detail, 0.0);
        VALIDATE_NUMERICAL_EQUALS(singlePrecisionDetail, 0.0);
      }
    }
  }
}

UNIT_TEST("An affine field has zero detail") {
  GIVEN("A patch whose value grows affinely along every axis") {
    std::vector<double> patch(NumberOfVolumes);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
      double value = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        value += static_cast<double>(axis + 2) * getVolumeCoordinate(counter(axis));
      }
      patch[peano4::dLinearised(counter, NumberOfDofsPerAxis)] = value;
    });

    WHEN("The function computeMultiresolutionDetail() evaluates the patch") {
      const double detail = getDetailOf(patch);

      THEN("The detail is zero") { VALIDATE_NUMERICAL_EQUALS(detail, 0.0); }
    }
  }
}

UNIT_TEST("A jump between two halves of the patch has a detail far above the one of a ramp over the same values") {
  GIVEN("A patch that jumps from one to two halfway along the first axis and a ramp along the same axis") {
    std::vector<double> step(NumberOfVolumes);
    std::vector<double> ramp(NumberOfVolumes);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
      const int linearised = peano4::dLinearised(counter, NumberOfDofsPerAxis);
      step[linearised]     = (counter(0) < NumberOfDofsPerAxis / 2) ? 1.0 : 2.0;
      ramp[linearised]     = 1.0 + getVolumeCoordinate(counter(0));
    });

    WHEN("The function computeMultiresolutionDetail() evaluates both patches") {
      const double stepDetail = getDetailOf(step);
      const double rampDetail = getDetailOf(ramp);

      THEN("The detail of the jump exceeds 0.05 and a million times the detail of the ramp") {
        VALIDATE(stepDetail > 0.05);
        VALIDATE(stepDetail > 1.0e6 * (rampDetail + 1.0e-15));
      }
    }
  }
}

UNIT_TEST("Every reflection and axis swap of a patch yields the bitwise-identical detail for both node layouts") {
  GIVEN("A patch without symmetry and two node layouts, the volume centres and the Lobatto nodes") {
    const std::vector<double> patch = getAsymmetricPatch();
    constexpr std::array      NodeLayouts{VolumeCentres, LobattoNodes};

    WHEN("The test computes the details of the patch and of its reflections and axis swaps in both node layouts") {
      std::vector<double>              patchDetails;
      std::vector<std::vector<double>> reflectionDetails;
      std::vector<std::vector<double>> swapDetails;
      std::vector<double>              transformed(NumberOfVolumes);
      for (const auto& nodes : NodeLayouts) {
        patchDetails.push_back(exahype2::computeMultiresolutionDetail(patch.data(), nodes, 1, getValueOf));

        std::vector<double>& reflectionDetailsOfTheLayout = reflectionDetails.emplace_back();
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
            tarch::la::Vector<DIMENSIONS, int> reflected = counter;
            reflected(axis)                              = NumberOfDofsPerAxis - 1 - counter(axis);
            transformed[peano4::dLinearised(counter, NumberOfDofsPerAxis)] =
              patch[peano4::dLinearised(reflected, NumberOfDofsPerAxis)];
          });
          reflectionDetailsOfTheLayout.push_back(
            exahype2::computeMultiresolutionDetail(transformed.data(), nodes, 1, getValueOf)
          );
        }

        std::vector<double>& swapDetailsOfTheLayout = swapDetails.emplace_back();
        for (int firstAxis = 0; firstAxis < DIMENSIONS; firstAxis++) {
          for (int secondAxis = firstAxis + 1; secondAxis < DIMENSIONS; secondAxis++) {
            peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
              tarch::la::Vector<DIMENSIONS, int> swapped = counter;
              swapped(firstAxis)                         = counter(secondAxis);
              swapped(secondAxis)                        = counter(firstAxis);
              transformed[peano4::dLinearised(counter, NumberOfDofsPerAxis)] =
                patch[peano4::dLinearised(swapped, NumberOfDofsPerAxis)];
            });
            swapDetailsOfTheLayout.push_back(
              exahype2::computeMultiresolutionDetail(transformed.data(), nodes, 1, getValueOf)
            );
          }
        }
      }

      THEN("The detail of the patch exceeds 1e-6 and every reflection and axis swap reproduces it bitwise") {
        for (std::size_t layout = 0; layout < NodeLayouts.size(); layout++) {
          VALIDATE(patchDetails[layout] > 1.0e-6, layout);
          for (const double reflectionDetail : reflectionDetails[layout]) {
            VALIDATE_EQUALS(reflectionDetail, patchDetails[layout], layout);
          }
          for (const double swapDetail : swapDetails[layout]) {
            VALIDATE_EQUALS(swapDetail, patchDetails[layout], layout);
          }
        }
      }
    }
  }
}

UNIT_TEST("A NaN or an infinite value in a patch yields an infinite detail") {
  GIVEN("A patch of the value one") {
    std::vector<double> patch(NumberOfVolumes, 1.0);

    WHEN("The function computeMultiresolutionDetail() evaluates the patch") {
      const double detail = getDetailOf(patch);

      THEN("The detail is zero") { VALIDATE_NUMERICAL_EQUALS(detail, 0.0); }
    }

    WHEN("A NaN overwrites the middle volume and computeMultiresolutionDetail() evaluates the patch") {
      patch[NumberOfVolumes / 2] = std::numeric_limits<double>::quiet_NaN();
      const double detail        = getDetailOf(patch);

      THEN("The detail is infinite") { VALIDATE(std::isinf(detail)); }
    }

    WHEN("An infinite value overwrites the middle volume and computeMultiresolutionDetail() evaluates the patch") {
      patch[NumberOfVolumes / 2] = std::numeric_limits<double>::infinity();
      const double detail        = getDetailOf(patch);

      THEN("The detail is infinite") { VALIDATE(std::isinf(detail)); }
    }
  }
}

UNIT_TEST("A field affine in the Lobatto nodes fits exactly with these nodes and misfits with the volume centres") {
  GIVEN("A patch whose value grows affinely in the Lobatto nodes along every axis") {
    std::vector<double> patch(NumberOfVolumes, 0.0);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      double value = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        value += (2.0 + axis) * LobattoNodes[static_cast<std::size_t>(node(axis))];
      }
      patch[peano4::dLinearised(node, NumberOfDofsPerAxis)] = value;
    });

    WHEN("The function computeMultiresolutionDetail() evaluates the patch with both node layouts") {
      const double detailAtTheLobattoNodes =
        exahype2::computeMultiresolutionDetail(patch.data(), LobattoNodes, 1, getValueOf);
      const double detailAtTheVolumeCentres = getDetailOf(patch);

      THEN("The detail at the Lobatto nodes is zero and the one at the volume centres exceeds 1e-3") {
        VALIDATE_NUMERICAL_EQUALS(detailAtTheLobattoNodes, 0.0);
        VALIDATE(detailAtTheVolumeCentres > 1e-3);
      }
    }
  }
}

UNIT_TEST("The indicator reads one unknown of an AoS patch and the other unknowns leave the detail unchanged") {
  GIVEN("A patch whose three unknowns store a jump, an affine field and a quadratic field") {
    constexpr int       NumberOfUnknowns = 3;
    std::vector<double> patch(static_cast<std::size_t>(NumberOfVolumes) * NumberOfUnknowns, 0.0);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * NumberOfUnknowns;
      patch[index]     = volume(0) < NumberOfDofsPerAxis / 2 ? -5.0 : 5.0;
      patch[index + 1] = 2.0 + 0.5 * volume(0) + 0.25 * volume(1);
      patch[index + 2] = 1000.0 * volume(0) * volume(0);
    });

    WHEN("The indicators of the affine and of the jumping unknown compute their details") {
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

      THEN("The affine unknown has zero detail and the jumping unknown a detail above 0.1") {
        VALIDATE_NUMERICAL_EQUALS(smoothDetail, 0.0);
        VALIDATE(stepDetail > 1e-1);
      }
    }
  }
}

UNIT_TEST("A patch of one node has zero detail") {
  GIVEN("A patch of the one value 42") {
    constexpr double SingleValue = 42.0;

    WHEN("The function computeMultiresolutionDetail() evaluates the patch at its one volume centre") {
      const double detail = exahype2::computeMultiresolutionDetail(
        &SingleValue,
        exahype2::getVolumeCentresInUnitInterval<1>(),
        1,
        getValueOf
      );

      THEN("The detail is zero") { VALIDATE_EQUALS(detail, 0.0); }
    }
  }
}

UNIT_TEST("The command of a detail refines above eps, erases below eps over the safety factor and keeps otherwise") {
  GIVEN("The details 2e-2, 1e-2, 5e-3, eps over the safety factor, 1e-4 and zero") {
    constexpr double DetailAboveEps     = 2.0e-2;
    constexpr double DetailAtEps        = 1.0e-2;
    constexpr double DetailBelowEps     = 5.0e-3;
    constexpr double DetailAtEraseBound = Eps / CoarsenSafetyFactor;
    constexpr double SmallDetail        = 1.0e-4;
    constexpr double ZeroDetail         = 0.0;

    WHEN("The function getRefinementCommandFromDetail() derives the command of every detail") {
      const exahype2::RefinementCommand commandAboveEps =
        exahype2::getRefinementCommandFromDetail(DetailAboveEps, Eps, CoarsenSafetyFactor);
      const exahype2::RefinementCommand commandAtEps =
        exahype2::getRefinementCommandFromDetail(DetailAtEps, Eps, CoarsenSafetyFactor);
      const exahype2::RefinementCommand commandBelowEps =
        exahype2::getRefinementCommandFromDetail(DetailBelowEps, Eps, CoarsenSafetyFactor);
      const exahype2::RefinementCommand commandAtEraseBound =
        exahype2::getRefinementCommandFromDetail(DetailAtEraseBound, Eps, CoarsenSafetyFactor);
      const exahype2::RefinementCommand commandOfSmallDetail =
        exahype2::getRefinementCommandFromDetail(SmallDetail, Eps, CoarsenSafetyFactor);
      const exahype2::RefinementCommand commandOfZeroDetail =
        exahype2::getRefinementCommandFromDetail(ZeroDetail, Eps, CoarsenSafetyFactor);

      THEN("The command refines above eps, erases for 1e-4 and for zero and keeps for the other details") {
        VALIDATE(commandAboveEps == exahype2::RefinementCommand::Refine);
        VALIDATE(commandAtEps == exahype2::RefinementCommand::Keep);
        VALIDATE(commandBelowEps == exahype2::RefinementCommand::Keep);
        VALIDATE(commandAtEraseBound == exahype2::RefinementCommand::Keep);
        VALIDATE(commandOfSmallDetail == exahype2::RefinementCommand::Erase);
        VALIDATE(commandOfZeroDetail == exahype2::RefinementCommand::Erase);
      }
    }
  }
}

UNIT_TEST("The function refineOnDetail() keeps a null cell, erases a smooth cell and refines a pressure jump") {
  GIVEN("A null cell, the states of a smooth cell and the states of a cell with a pressure jump in both precisions") {
    const PatchState<double>(*const nullCell)[NumberOfVolumes] = nullptr;
    PatchState<double> smoothStates[NumberOfVolumes];
    PatchState<double> jumpStates[NumberOfVolumes];
    PatchState<float>  singlePrecisionJumpStates[NumberOfVolumes];
    setStatesOf(getSmoothPatch(), smoothStates);
    setStatesOf(getPatchWithAPressureJump(), jumpStates);
    setStatesOf(getPatchWithAPressureJump(), singlePrecisionJumpStates);

    WHEN("The function refineOnDetail() evaluates the states with the density or with the density and the pressure") {
      const exahype2::RefinementCommand commandOfTheNullCell =
        exahype2::refineOnDetail(nullCell, VolumeCentres, Eps, CoarsenSafetyFactor, Density{});
      const exahype2::RefinementCommand commandOfTheSmoothStates =
        exahype2::refineOnDetail(&smoothStates, VolumeCentres, Eps, CoarsenSafetyFactor, Density{}, Pressure{});
      const exahype2::RefinementCommand commandOfTheDensityOfTheJumpStates =
        exahype2::refineOnDetail(&jumpStates, VolumeCentres, Eps, CoarsenSafetyFactor, Density{});
      const exahype2::RefinementCommand commandOfTheJumpStates =
        exahype2::refineOnDetail(&jumpStates, VolumeCentres, Eps, CoarsenSafetyFactor, Density{}, Pressure{});
      const exahype2::RefinementCommand commandOfTheSinglePrecisionJumpStates = exahype2::refineOnDetail(
        &singlePrecisionJumpStates,
        VolumeCentres,
        Eps,
        CoarsenSafetyFactor,
        Density{},
        Pressure{}
      );

      THEN("The null cell keeps, the smooth indicators erase and the pressure jump refines in both precisions") {
        VALIDATE(commandOfTheNullCell == exahype2::RefinementCommand::Keep);
        VALIDATE(commandOfTheSmoothStates == exahype2::RefinementCommand::Erase);
        VALIDATE(commandOfTheDensityOfTheJumpStates == exahype2::RefinementCommand::Erase);
        VALIDATE(commandOfTheJumpStates == exahype2::RefinementCommand::Refine);
        VALIDATE(commandOfTheSinglePrecisionJumpStates == exahype2::RefinementCommand::Refine);
      }
    }
  }
}

UNIT_TEST("A device kernel derives the same refinement commands from states in device memory as the host") {
  GIVEN("The states of a smooth cell and of a cell with a pressure jump in device memory") {
    constexpr int      NumberOfPatches = 2;
    PatchState<double> smoothStates[NumberOfVolumes];
    PatchState<double> jumpStates[NumberOfVolumes];
    setStatesOf(getSmoothPatch(), smoothStates);
    setStatesOf(getPatchWithAPressureJump(), jumpStates);
    std::vector<PatchState<double>> states(std::begin(smoothStates), std::end(smoothStates));
    states.insert(states.end(), std::begin(jumpStates), std::end(jumpStates));
    tarch::accelerator::Buffer<PatchState<double>> deviceStates(states.size(), tarch::MemoryLocation::Device);
    deviceStates.setValues(states);

    WHEN("A device kernel refines every cell on the detail of the density and the pressure") {
      tarch::accelerator::Buffer<exahype2::RefinementCommand> commands(NumberOfPatches, tarch::MemoryLocation::Device);
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(NumberOfPatches),
        RefineEveryPatchOnDetail{deviceStates.getData(), commands.getData()}
      );
      const std::vector<exahype2::RefinementCommand> deviceCommands = commands.getValues();

      THEN("The smooth patch erases and the patch with the jump refines") {
        VALIDATE(deviceCommands[0] == exahype2::RefinementCommand::Erase);
        VALIDATE(deviceCommands[1] == exahype2::RefinementCommand::Refine);
      }
    }
  }
}

UNIT_TEST("A device kernel derives the bitwise-identical detail from a patch and from its first two axes swapped") {
  GIVEN("A patch without symmetry and the same patch with its first two axes swapped in device memory") {
    constexpr int             NumberOfPatches = 2;
    const std::vector<double> patch           = getAsymmetricPatch();
    std::vector<double>       patches(NumberOfPatches * NumberOfVolumes);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
      tarch::la::Vector<DIMENSIONS, int> swapped = counter;
      swapped(0)                                 = counter(1);
      swapped(1)                                 = counter(0);
      const int volume                           = peano4::dLinearised(counter, NumberOfDofsPerAxis);
      patches[volume]                            = patch[volume];
      patches[NumberOfVolumes + volume]          = patch[peano4::dLinearised(swapped, NumberOfDofsPerAxis)];
    });
    tarch::accelerator::Buffer<double> devicePatches(patches.size(), tarch::MemoryLocation::Device);
    devicePatches.setValues(patches);

    WHEN("A device kernel computes the detail of the density of both patches") {
      tarch::accelerator::Buffer<double> details(NumberOfPatches, tarch::MemoryLocation::Device);
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(NumberOfPatches),
        ComputeDetailOfEveryPatch{devicePatches.getData(), details.getData()}
      );
      const std::vector<double> deviceDetails = details.getValues();

      THEN("The detail of the patch is positive and the swapped patch has the same detail bitwise") {
        VALIDATE(deviceDetails[0] > 1.0e-6, deviceDetails[0]);
        VALIDATE_EQUALS(deviceDetails[1], deviceDetails[0]);
      }
    }
  }
}
