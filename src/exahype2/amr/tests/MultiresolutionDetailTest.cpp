// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiresolutionDetailTest.h"

#include "exahype2/amr/MultiresolutionDetail.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

namespace {
  int numberOfVolumes(int numberOfDofsPerAxis) {
    int result = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      result *= numberOfDofsPerAxis;
    }
    return result;
  }

  // Detail of a scalar (stride-1) patch via the identity indicator.
  double detailOf(const std::vector<double>& patch, int numberOfDofsPerAxis) {
    return exahype2::amr::computeMultiresolutionDetail(
      patch.data(),
      numberOfDofsPerAxis,
      1,
      [](const double* v) { return v[0]; }
    );
  }

  // Per-axis centred node coordinate, matching the implementation.
  double xi(int i, int numberOfDofsPerAxis) {
    return (static_cast<double>(i) + 0.5) / static_cast<double>(numberOfDofsPerAxis) - 0.5;
  }
} // namespace

exahype2::amr::tests::MultiresolutionDetailTest::MultiresolutionDetailTest() :
  TestCase("exahype2::amr::tests::MultiresolutionDetailTest") {}

void exahype2::amr::tests::MultiresolutionDetailTest::run() {
  testMethod(testConstantFieldHasZeroDetail);
  testMethod(testAffineFieldHasZeroDetail);
  testMethod(testStepFieldHasLargeDetail);
  testMethod(testDetailIsBitwiseInvariantUnderReflectionsAndSwaps);
  testMethod(testNonFiniteValueRefines);
  testMethod(testCustomNodesFitTheirOwnAffineField);
  testMethod(testStridedPatchWithIndicator);
  testMethod(testSingleNodePatchHasZeroDetail);
}

void exahype2::amr::tests::MultiresolutionDetailTest::testConstantFieldHasZeroDetail() {
  const int           numberOfDofsPerAxis = 4;
  std::vector<double> patch(numberOfVolumes(numberOfDofsPerAxis), 1.0);
  validateNumericalEquals(detailOf(patch, numberOfDofsPerAxis), 0.0);
}

void exahype2::amr::tests::MultiresolutionDetailTest::testAffineFieldHasZeroDetail() {
  const int           numberOfDofsPerAxis = 4;
  std::vector<double> patch(numberOfVolumes(numberOfDofsPerAxis));
  peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    double value = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      value += static_cast<double>(d + 2) * xi(counter(d), numberOfDofsPerAxis);
    }
    patch[peano4::dLinearised(counter, numberOfDofsPerAxis)] = value;
  });
  validateTrue(std::abs(detailOf(patch, numberOfDofsPerAxis)) < 1.0e-12);
}

void exahype2::amr::tests::MultiresolutionDetailTest::testStepFieldHasLargeDetail() {
  // A jump (half the patch at 1, half at 2) cannot be fit affinely, so its detail is O(1)
  // and far larger than the detail of a smooth ramp over the same value range.
  const int           numberOfDofsPerAxis = 4;
  std::vector<double> step(numberOfVolumes(numberOfDofsPerAxis));
  std::vector<double> ramp(numberOfVolumes(numberOfDofsPerAxis));
  peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    const int linearised = peano4::dLinearised(counter, numberOfDofsPerAxis);
    step[linearised]     = (counter(0) < numberOfDofsPerAxis / 2) ? 1.0 : 2.0;
    ramp[linearised]     = 1.0 + xi(counter(0), numberOfDofsPerAxis);
  });
  const double stepDetail = detailOf(step, numberOfDofsPerAxis);
  const double rampDetail = detailOf(ramp, numberOfDofsPerAxis);
  validateTrue(stepDetail > 0.05);
  validateTrue(stepDetail > 1.0e6 * (rampDetail + 1.0e-15));
}

void exahype2::amr::tests::MultiresolutionDetailTest::testDetailIsBitwiseInvariantUnderReflectionsAndSwaps() {
  // A bitwise-identical detail for every reflected or axis-swapped patch is what turns a
  // mirror-symmetric field into identical refinement commands on both sides of the
  // symmetry plane, whatever the strict thresholds of the command mapping. The field
  // carries a cross term, so every transformation genuinely permutes distinct values,
  // and it runs once with equidistant volume centres and once with a symmetric nodal
  // layout to cover the derived node coordinates.
  const int           numberOfDofsPerAxis    = 4;
  constexpr double    NodesInUnitInterval[4] = {0.0, 0.2763932, 0.7236068, 1.0};
  const double* const nodeLayouts[2]         = {nullptr, NodesInUnitInterval};
  std::vector<double> patch(numberOfVolumes(numberOfDofsPerAxis));
  std::vector<double> transformed(numberOfVolumes(numberOfDofsPerAxis));
  peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
    double value = 1.0 + std::sin(3.0 * xi(counter(0), numberOfDofsPerAxis));
    for (int d = 1; d < DIMENSIONS; d++) {
      value += 0.3 * std::cos(2.0 * xi(counter(d), numberOfDofsPerAxis))
        + 0.7 * xi(counter(0), numberOfDofsPerAxis) * xi(counter(d), numberOfDofsPerAxis);
    }
    patch[peano4::dLinearised(counter, numberOfDofsPerAxis)] = value;
  });

  const auto detailWithNodes = [&](const std::vector<double>& values, const double* const nodes) {
    return exahype2::amr::computeMultiresolutionDetail(
      values.data(),
      numberOfDofsPerAxis,
      1,
      [](const double* v) { return v[0]; },
      nodes
    );
  };

  for (const double* const nodes : nodeLayouts) {
    const double patchDetail = detailWithNodes(patch, nodes);
    validateTrue(patchDetail > 1.0e-6);

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
        tarch::la::Vector<DIMENSIONS, int> reflected = counter;
        reflected(axis)                              = numberOfDofsPerAxis - 1 - counter(axis);
        transformed[peano4::dLinearised(counter, numberOfDofsPerAxis)] =
          patch[peano4::dLinearised(reflected, numberOfDofsPerAxis)];
      });
      validateEquals(detailWithNodes(transformed, nodes), patchDetail);
    }

    for (int firstAxis = 0; firstAxis < DIMENSIONS; firstAxis++) {
      for (int secondAxis = firstAxis + 1; secondAxis < DIMENSIONS; secondAxis++) {
        peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& counter) {
          tarch::la::Vector<DIMENSIONS, int> swapped = counter;
          swapped(firstAxis)                         = counter(secondAxis);
          swapped(secondAxis)                        = counter(firstAxis);
          transformed[peano4::dLinearised(counter, numberOfDofsPerAxis)] =
            patch[peano4::dLinearised(swapped, numberOfDofsPerAxis)];
        });
        validateEquals(detailWithNodes(transformed, nodes), patchDetail);
      }
    }
  }
}

// std::max() returns its first argument when the second is a NaN, so a non-finite value would
// vanish in the running maximum and leave the detail at zero, which the caller maps to Erase.
// The routine reports infinity instead, so a patch holding a NaN refines.
void exahype2::amr::tests::MultiresolutionDetailTest::testNonFiniteValueRefines() {
  constexpr int NumberOfDofsPerAxis = 4;

  int numberOfVolumes = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    numberOfVolumes *= NumberOfDofsPerAxis;
  }
  const auto indicator = [](const double* value) { return value[0]; };

  std::vector<double> patch(numberOfVolumes, 1.0);
  validateNumericalEqualsWithEps(
    exahype2::amr::computeMultiresolutionDetail(patch.data(), NumberOfDofsPerAxis, 1, indicator),
    0.0,
    1e-12
  );

  patch[numberOfVolumes / 2] = std::numeric_limits<double>::quiet_NaN();
  validateTrue(
    std::isinf(exahype2::amr::computeMultiresolutionDetail(patch.data(), NumberOfDofsPerAxis, 1, indicator))
  );

  patch[numberOfVolumes / 2] = std::numeric_limits<double>::infinity();
  validateTrue(
    std::isinf(exahype2::amr::computeMultiresolutionDetail(patch.data(), NumberOfDofsPerAxis, 1, indicator))
  );
}

void exahype2::amr::tests::MultiresolutionDetailTest::testCustomNodesFitTheirOwnAffineField() {
  // Gauss-Lobatto style nodes: Symmetric about one half, so the normal equations of the fit
  // stay diagonal. A field that is affine in the node positions is fit exactly with the node
  // layout passed in, and misfit when the fit assumes equidistant volume centres.
  constexpr int    NumberOfNodesPerAxis                      = 4;
  constexpr double NodesInUnitInterval[NumberOfNodesPerAxis] = {0.0, 0.2763932, 0.7236068, 1.0};

  int numberOfNodes = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    numberOfNodes *= NumberOfNodesPerAxis;
  }
  std::vector<double> patch(numberOfNodes, 0.0);
  peano4::dForEach(NumberOfNodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    double value = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      value += (2.0 + d) * NodesInUnitInterval[node(d)];
    }
    patch[peano4::dLinearised(node, NumberOfNodesPerAxis)] = value;
  });

  const double detailWithNodes = exahype2::amr::computeMultiresolutionDetail(
    patch.data(),
    NumberOfNodesPerAxis,
    1,
    [](const double* value) { return value[0]; },
    NodesInUnitInterval
  );
  const double detailWithVolumeCentres = exahype2::amr::computeMultiresolutionDetail(
    patch.data(),
    NumberOfNodesPerAxis,
    1,
    [](const double* value) { return value[0]; }
  );

  validateNumericalEqualsWithEps(detailWithNodes, 0.0, 1e-12);
  validate(detailWithVolumeCentres > 1e-3);
}

void exahype2::amr::tests::MultiresolutionDetailTest::testStridedPatchWithIndicator() {
  // An AoS patch with several unknowns per volume: The indicator selects one of them, and
  // the other unknowns never influence the detail.
  constexpr int NumberOfDofsPerAxis = 4;
  constexpr int Unknowns            = 3;

  int numberOfVolumes = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    numberOfVolumes *= NumberOfDofsPerAxis;
  }
  std::vector<double> patch(static_cast<std::size_t>(numberOfVolumes) * Unknowns, 0.0);
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * Unknowns;
    patch[index]     = volume(0) < NumberOfDofsPerAxis / 2 ? -5.0 : 5.0;
    patch[index + 1] = 2.0 + 0.5 * volume(0) + 0.25 * volume(1);
    patch[index + 2] = 1000.0 * volume(0) * volume(0);
  });

  const double smoothDetail =
    exahype2::amr::computeMultiresolutionDetail(patch.data(), NumberOfDofsPerAxis, Unknowns, [](const double* volume) {
      return volume[1];
    });
  const double stepDetail =
    exahype2::amr::computeMultiresolutionDetail(patch.data(), NumberOfDofsPerAxis, Unknowns, [](const double* volume) {
      return volume[0];
    });

  validateNumericalEqualsWithEps(smoothDetail, 0.0, 1e-12);
  validate(stepDetail > 1e-1);
}

void exahype2::amr::tests::MultiresolutionDetailTest::testSingleNodePatchHasZeroDetail() {
  // A single value is trivially affine.
  const double value = 42.0;
  validateEquals(
    exahype2::amr::computeMultiresolutionDetail(&value, 1, 1, [](const double* volume) { return volume[0]; }),
    0.0
  );
}
