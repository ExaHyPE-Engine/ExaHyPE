// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class KernelsOnDeviceTest;
} // namespace exahype2::fv::tests

/**
 * Checks that every finite-volume kernel run over many patches or faces
 * through tarch::accelerator::parallelFor over device memory reproduces the
 * serial host loop.
 */
class exahype2::fv::tests::KernelsOnDeviceTest final : public tarch::tests::TestCase {
public:
  KernelsOnDeviceTest();
  KernelsOnDeviceTest(const KernelsOnDeviceTest&)            = delete;
  KernelsOnDeviceTest(KernelsOnDeviceTest&&)                 = delete;
  KernelsOnDeviceTest& operator=(const KernelsOnDeviceTest&) = delete;
  KernelsOnDeviceTest& operator=(KernelsOnDeviceTest&&)      = delete;
  ~KernelsOnDeviceTest() override                            = default;

  void run() override;

  // These test bodies launch extended __device__ lambdas, which nvcc forbids
  // inside private or protected member functions.
  void testApplyFiniteVolumeWithConstantReconstruction();
  void testApplyFiniteVolumeWithLimitedLinearReconstruction();
  void testApplyFiniteVolumeWithWENOReconstruction();
  void testApplyFiniteVolumeWithCustomRiemannSolver();
  void testApplyBoundaryConditionsOnFaceWithOneHaloLayer();
  void testApplyBoundaryConditionsOnFaceWithTwoHaloLayers();
  void testApplyBoundaryConditionsOnFaceWithThreeHaloLayers();

  template <
    typename Reconstruction,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    bool EvaluateCustomRiemann>
  void runApplyFiniteVolume();

  template <int HaloSize>
  void runApplyBoundaryConditionsOnFace();

private:
  void validateBuffersMatch(
    const std::vector<double>& deviceResult,
    const std::vector<double>& hostResult,
    const std::string&         bufferName
  );
};
