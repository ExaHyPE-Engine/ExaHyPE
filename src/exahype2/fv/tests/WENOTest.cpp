// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "WENOTest.h"

#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/WENO.h"
#include "exahype2/fv/reconstruction/weno/NonlinearWeights.h"
#include "peano4/Constants.h"

namespace {
  constexpr int NumberOfData = 3;

  template <int VolumesPerAxis>
  constexpr int numberOfVolumesInPatch() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      product *= VolumesPerAxis;
    }
    return product;
  }

  template <int VolumesPerAxis, int HaloWidth>
  constexpr int faceOverlapStorageSize() {
    int cells = 2 * HaloWidth;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      cells *= VolumesPerAxis;
    }
    return cells * NumberOfData;
  }

  template <int VolumesPerAxis>
  tarch::la::Vector<DIMENSIONS, int> volumeIndexOf(int linearVolume) {
    tarch::la::Vector<DIMENSIONS, int> volumeIndex;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      volumeIndex(dimension) = linearVolume % VolumesPerAxis;
      linearVolume /= VolumesPerAxis;
    }
    return volumeIndex;
  }


  template <int VolumesPerAxis, int HaloWidth, typename Functor>
  void forEachOverlapVoxel(Functor functor) {
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      tarch::la::Vector<DIMENSIONS, int> overlapCell(0);
      while (true) {
        for (int side = 0; side < 2; side++) {
          auto volumeIndex = overlapCell;
          volumeIndex(normal) =
            side == 0 ? overlapCell(normal) - HaloWidth : VolumesPerAxis - HaloWidth + overlapCell(normal);
          const int face         = side == 0 ? normal : normal + DIMENSIONS;
          const int overlapIndex = exahype2::fv::getFaceVolumeIndex(overlapCell, VolumesPerAxis, HaloWidth, normal);
          functor(volumeIndex, face, overlapIndex);
        }
        int axis = 0;
        while (axis < DIMENSIONS) {
          const int limit = axis == normal ? 2 * HaloWidth : VolumesPerAxis;
          overlapCell(axis)++;
          if (overlapCell(axis) < limit) {
            break;
          }
          overlapCell(axis) = 0;
          axis++;
        }
        if (axis == DIMENSIONS) {
          break;
        }
      }
    }
  }


  template <int Order>
  using WENO = exahype2::fv::reconstruction::WENO<Order, exahype2::fv::reconstruction::weno::JiangShu<8>>;

  // The polynomial sums x^k / (k + 1) up to the given order along every axis.
  template <int Order>
  double polynomialValue(const double x) {
    double value = 0.0;
    for (int power = 0; power <= Order; power++) {
      value += std::pow(x, power) / (power + 1);
    }
    return value;
  }

  template <int Order>
  double polynomialAverage(const double lower, const double upper) {
    double integral = 0.0;
    for (int power = 0; power <= Order; power++) {
      integral += (std::pow(upper, power + 1) - std::pow(lower, power + 1)) / ((power + 1) * (power + 1));
    }
    return integral / (upper - lower);
  }

  // Reconstructs every volume of the patch, and every volume next to or across a face of the
  // patch restricted to that face, from the exact averages of a tensor-product polynomial of the
  // reconstruction's order and returns the largest error against its values at the nodes. The
  // volumes near edges, corners and shared faces reconstruct with fewer stencils.
  template <int Order, int VolumesPerAxis>
  double wenoPolynomialReconstructionError() {
    using Reconstruction    = WENO<Order>;
    using Basis             = typename Reconstruction::Basis;
    constexpr int HaloWidth = Reconstruction::HaloSize;
    const double  h         = 1.0 / VolumesPerAxis;

    const auto average = [h](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
      double value = unknown + 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        value *= polynomialAverage<Order>(volumeIndex(axis) * h, (volumeIndex(axis) + 1) * h);
      }
      return value;
    };

    std::vector<double> q(numberOfVolumesInPatch<VolumesPerAxis>() * NumberOfData);
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = average(volumeIndexOf<VolumesPerAxis>(volume), unknown);
      }
    }
    std::vector<std::vector<double>> faceInStorage(
      peano4::FacesPerCell,
      std::vector<double>(faceOverlapStorageSize<VolumesPerAxis, HaloWidth>(), 0.0)
    );
    forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
      [&](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceInStorage[face][overlapIndex * NumberOfData + unknown] = average(volumeIndex, unknown);
        }
      }
    );
    const double* faceInputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face] = faceInStorage[face].data();
    }

    constexpr int PredictedPerAxis = VolumesPerAxis + 2;
    double        error            = 0.0;
    for (int predicted = 0; predicted < peano4::power(PredictedPerAxis, DIMENSIONS); predicted++) {
      tarch::la::Vector<DIMENSIONS, int> volumeIndex;
      int                                remainder          = predicted;
      int                                axesOutsideOfPatch = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        volumeIndex(axis) = remainder % PredictedPerAxis - 1;
        remainder /= PredictedPerAxis;
        axesOutsideOfPatch += (volumeIndex(axis) < 0 or volumeIndex(axis) >= VolumesPerAxis) ? 1 : 0;
      }
      if (axesOutsideOfPatch > 1) {
        continue;
      }

      for (int sharedFace = -1; sharedFace < peano4::FacesPerCell; sharedFace++) {
        if (sharedFace >= 0) {
          const int axis        = sharedFace % DIMENSIONS;
          const int nextToFace  = sharedFace < DIMENSIONS ? 0 : VolumesPerAxis - 1;
          const int acrossFace  = sharedFace < DIMENSIONS ? -1 : VolumesPerAxis;
          bool      touchesFace = volumeIndex(axis) == nextToFace or volumeIndex(axis) == acrossFace;
          for (int tangential = 0; tangential < DIMENSIONS; tangential++) {
            touchesFace = touchesFace
              and (tangential == axis or (volumeIndex(tangential) >= 0 and volumeIndex(tangential) < VolumesPerAxis));
          }
          if (not touchesFace) {
            continue;
          }
        } else if (axesOutsideOfPatch > 0) {
          continue;
        }

        std::vector<double> nodalState(Basis::NodesPerCell * NumberOfData);
        std::vector<double> scratch(Reconstruction::template ScratchSize<NumberOfData>);
        Reconstruction::template reconstructPolynomial<VolumesPerAxis, NumberOfData>(
          q.data(),
          faceInputPatches,
          volumeIndex,
          sharedFace,
          nodalState.data(),
          scratch.data()
        );
        for (int cellNode = 0; cellNode < Basis::NodesPerCell; cellNode++) {
          double exact         = 1.0;
          int    nodeRemainder = cellNode;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            exact *=
              polynomialValue<Order>((volumeIndex(axis) + Basis::nodes()[nodeRemainder % Basis::NumberOfNodes]) * h);
            nodeRemainder /= Basis::NumberOfNodes;
          }
          for (int unknown = 0; unknown < NumberOfData; unknown++) {
            error = std::max(error, std::abs(nodalState[cellNode * NumberOfData + unknown] - (unknown + 1.0) * exact));
          }
        }
      }
    }
    return error;
  }

  // Returns the largest difference between the nodal polynomials that reconstructPatch() and
  // reconstructPolynomial() yield for every volume of a patch. The averages oscillate and jump
  // inside the patch and in the halo, so the stencils enter with different nonlinear weights.
  template <int Order, int VolumesPerAxis>
  double patchReconstructionDifference() {
    using Reconstruction    = WENO<Order>;
    using Basis             = typename Reconstruction::Basis;
    constexpr int HaloWidth = Reconstruction::HaloSize;
    constexpr int Volumes   = numberOfVolumesInPatch<VolumesPerAxis>();
    constexpr int NodalSize = Basis::NodesPerCell * NumberOfData;

    const auto average = [](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
      double phase = 0.3 * unknown;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        phase += (1.7 + 0.4 * axis) * volumeIndex(axis);
      }
      return std::sin(phase) + (volumeIndex(0) >= VolumesPerAxis / 2 ? 2.0 : 0.0) + unknown;
    };

    std::vector<double> q(Volumes * NumberOfData);
    for (int volume = 0; volume < Volumes; volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = average(volumeIndexOf<VolumesPerAxis>(volume), unknown);
      }
    }
    std::vector<std::vector<double>> faceInStorage(
      peano4::FacesPerCell,
      std::vector<double>(faceOverlapStorageSize<VolumesPerAxis, HaloWidth>(), 0.0)
    );
    forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
      [&](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceInStorage[face][overlapIndex * NumberOfData + unknown] = average(volumeIndex, unknown);
        }
      }
    );
    const double* faceInputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face] = faceInStorage[face].data();
    }

    std::vector<double> nodalStates(Volumes * NodalSize);
    std::vector<double> patchScratch(Reconstruction::template PatchScratchSize<VolumesPerAxis, NumberOfData>);
    Reconstruction::template reconstructPatch<VolumesPerAxis, NumberOfData>(
      q.data(),
      faceInputPatches,
      nodalStates.data(),
      patchScratch.data()
    );

    double difference = 0.0;
    for (int volume = 0; volume < Volumes; volume++) {
      std::vector<double> nodalState(NodalSize);
      std::vector<double> scratch(Reconstruction::template ScratchSize<NumberOfData>);
      Reconstruction::template reconstructPolynomial<VolumesPerAxis, NumberOfData>(
        q.data(),
        faceInputPatches,
        volumeIndexOf<VolumesPerAxis>(volume),
        -1,
        nodalState.data(),
        scratch.data()
      );
      for (int entry = 0; entry < NodalSize; entry++) {
        difference = std::max(difference, std::abs(nodalStates[volume * NodalSize + entry] - nodalState[entry]));
      }
    }
    return difference;
  }

  struct WeightsOfFirstEntry {
    GPU_CALLABLE_METHOD static int reconstructionWeightsOf(const int) { return 0; }
  };

  // Returns the largest difference between the nodal polynomial of the second entry and the sum
  // of the nodal polynomials of the first and the third entry, whose averages the second entry
  // sums. WeightSource weighs every entry.
  template <int Order, int VolumesPerAxis, typename WeightSource>
  double sumReconstructionDifference() {
    using Reconstruction    = WENO<Order>;
    using Basis             = typename Reconstruction::Basis;
    constexpr int HaloWidth = Reconstruction::HaloSize;
    constexpr int Volumes   = numberOfVolumesInPatch<VolumesPerAxis>();
    constexpr int NodalSize = Basis::NodesPerCell * NumberOfData;

    const auto average = [](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
      double first = 0.0;
      double third = 0.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        first += std::sin((1.7 + 0.4 * axis) * volumeIndex(axis));
        third += std::cos((0.9 + 0.7 * axis) * volumeIndex(axis));
      }
      first += volumeIndex(0) >= VolumesPerAxis / 2 ? 2.0 : 0.0;
      third += volumeIndex(DIMENSIONS - 1) >= VolumesPerAxis / 3 ? 5.0 : 0.0;
      return unknown == 0 ? first : unknown == 2 ? third : first + third;
    };

    std::vector<double> q(Volumes * NumberOfData);
    for (int volume = 0; volume < Volumes; volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = average(volumeIndexOf<VolumesPerAxis>(volume), unknown);
      }
    }
    std::vector<std::vector<double>> faceInStorage(
      peano4::FacesPerCell,
      std::vector<double>(faceOverlapStorageSize<VolumesPerAxis, HaloWidth>(), 0.0)
    );
    forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
      [&](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceInStorage[face][overlapIndex * NumberOfData + unknown] = average(volumeIndex, unknown);
        }
      }
    );
    const double* faceInputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face] = faceInStorage[face].data();
    }

    std::vector<double> nodalStates(Volumes * NodalSize);
    std::vector<double> patchScratch(Reconstruction::template PatchScratchSize<VolumesPerAxis, NumberOfData>);
    Reconstruction::template reconstructPatch<VolumesPerAxis, NumberOfData, WeightSource>(
      q.data(),
      faceInputPatches,
      nodalStates.data(),
      patchScratch.data()
    );

    double difference = 0.0;
    for (int node = 0; node < Volumes * Basis::NodesPerCell; node++) {
      const double* const state = &nodalStates[node * NumberOfData];
      difference = std::max(difference, std::abs(state[1] - state[0] - state[2]));
    }
    return difference;
  }
} // namespace

exahype2::fv::tests::WENOTest::WENOTest() :
  tarch::tests::TestCase("exahype2::fv::tests::WENOTest") {}

void
exahype2::fv::tests::WENOTest::run() {
  testMethod(testWENOReconstructsPolynomialsExactly);
  testMethod(testPatchReconstructionMatchesVolumeReconstruction);
  testMethod(testSharedWeightsReconstructSumsOfEntries);
}

void
exahype2::fv::tests::WENOTest::testWENOReconstructsPolynomialsExactly() {
  const double secondOrderError = wenoPolynomialReconstructionError<2, 4>();
  const double thirdOrderError  = wenoPolynomialReconstructionError<3, 6>();

  validateWithParams1(secondOrderError < 1e-10, secondOrderError);
  validateWithParams1(thirdOrderError < 1e-10, thirdOrderError);
}

/**
 * Both reconstructions run the same arithmetic per line, so they agree up to the rounding of a
 * contracted multiply-add.
 */
void
exahype2::fv::tests::WENOTest::testPatchReconstructionMatchesVolumeReconstruction() {
  const double secondOrderDifference = patchReconstructionDifference<2, 4>();
  const double thirdOrderDifference  = patchReconstructionDifference<3, 6>();

  validateWithParams1(secondOrderDifference < 1e-12, secondOrderDifference);
  validateWithParams1(thirdOrderDifference < 1e-12, thirdOrderDifference);
}

/**
 * With the weights of one entry, the reconstruction is linear, so it reconstructs the sum of two
 * entries as the sum of their reconstructions. Every entry with its own weights breaks this.
 */
void
exahype2::fv::tests::WENOTest::testSharedWeightsReconstructSumsOfEntries() {
  using exahype2::fv::reconstruction::weno::OwnWeights;
  const double secondOrderShared = sumReconstructionDifference<2, 4, WeightsOfFirstEntry>();
  const double thirdOrderShared  = sumReconstructionDifference<3, 6, WeightsOfFirstEntry>();
  const double secondOrderOwn    = sumReconstructionDifference<2, 4, OwnWeights>();
  const double thirdOrderOwn     = sumReconstructionDifference<3, 6, OwnWeights>();

  validateWithParams1(secondOrderShared < 1e-12, secondOrderShared);
  validateWithParams1(thirdOrderShared < 1e-12, thirdOrderShared);
  validateWithParams1(secondOrderOwn > 1e-3, secondOrderOwn);
  validateWithParams1(thirdOrderOwn > 1e-3, thirdOrderOwn);
}
