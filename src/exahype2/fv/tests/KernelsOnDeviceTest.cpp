// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsOnDeviceTest.h"

#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/fv/ApplyBoundaryConditionsOnFace.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/reconstruction/WENO.h"
#include "exahype2/fv/tests/DeviceKernelFixtures.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "exahype2/fv/weno/NonlinearWeights.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  using Setup = exahype2::fv::tests::DeviceKernelSetup;
} // namespace

exahype2::fv::tests::KernelsOnDeviceTest::KernelsOnDeviceTest() :
  tarch::tests::TestCase("exahype2::fv::tests::KernelsOnDeviceTest") {}

void exahype2::fv::tests::KernelsOnDeviceTest::run() {
  testMethod(testApplyFiniteVolumeWithConstantReconstruction);
  testMethod(testApplyFiniteVolumeWithLimitedLinearReconstruction);
  testMethod(testApplyFiniteVolumeWithWENOReconstruction);
  testMethod(testApplyFiniteVolumeWithCustomRiemannSolver);
  testMethod(testApplyBoundaryConditionsOnFaceWithOneHaloLayer);
  testMethod(testApplyBoundaryConditionsOnFaceWithTwoHaloLayers);
  testMethod(testApplyBoundaryConditionsOnFaceWithThreeHaloLayers);
}

void exahype2::fv::tests::KernelsOnDeviceTest::validateBuffersMatch(
  const std::vector<double>& deviceResult,
  const std::vector<double>& hostResult,
  const std::string&         bufferName
) {
  validateEquals(deviceResult.size(), hostResult.size());
  for (std::size_t entry = 0; entry < deviceResult.size(); entry++) {
    validateNumericalEqualsWithParams2(deviceResult[entry], hostResult[entry], bufferName, entry);
  }
}

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyFiniteVolumeWithConstantReconstruction() {
  runApplyFiniteVolume<exahype2::fv::reconstruction::Constant, true, true, true, true, false>();
}

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyFiniteVolumeWithLimitedLinearReconstruction() {
  runApplyFiniteVolume<
    exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>,
    true,
    true,
    true,
    false,
    false>();
}

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyFiniteVolumeWithWENOReconstruction() {
  runApplyFiniteVolume<
    exahype2::fv::reconstruction::WENO<exahype2::fv::weno::JiangShu>,
    true,
    true,
    true,
    false,
    false>();
}

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyFiniteVolumeWithCustomRiemannSolver() {
  runApplyFiniteVolume<exahype2::fv::reconstruction::Constant, true, false, false, false, true>();
}

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyBoundaryConditionsOnFaceWithOneHaloLayer() { runApplyBoundaryConditionsOnFace<1>(); }

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyBoundaryConditionsOnFaceWithTwoHaloLayers() { runApplyBoundaryConditionsOnFace<2>(); }

void exahype2::fv::tests::KernelsOnDeviceTest::testApplyBoundaryConditionsOnFaceWithThreeHaloLayers() { runApplyBoundaryConditionsOnFace<3>(); }

template <
  typename Reconstruction,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateDiffusiveSource,
  bool EvaluateCustomRiemann>
void exahype2::fv::tests::KernelsOnDeviceTest::runApplyFiniteVolume() {
  constexpr int FaceCardinality = Setup::faceOverlapCardinality(Reconstruction::HaloSize);

  const exahype2::fv::tests::PatchChain geometry;
  const std::vector<double> inputPatches = exahype2::fv::tests::makeRamp(Setup::AllPatchesCardinality, 0.4, 0.013);
  const std::vector<double> outputPatches(Setup::AllPatchesCardinality, 0.0);
  const std::vector<double> faceInputs =
    exahype2::fv::tests::makeRamp(geometry.faceOverlapEntries(Reconstruction::HaloSize), 0.7, 0.009);
  const std::vector<double> faceOutputs(geometry.faceOverlapEntries(Reconstruction::HaloSize), 0.0);

  const exahype2::fv::tests::DeviceBuffer<double>        deviceInputPatches(inputPatches);
  const exahype2::fv::tests::DeviceBuffer<double>        deviceOutputPatches(outputPatches);
  const exahype2::fv::tests::DeviceBuffer<double>        deviceFaceInputs(faceInputs);
  const exahype2::fv::tests::DeviceBuffer<double>        deviceFaceOutputs(faceOutputs);
  const exahype2::fv::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
  const exahype2::fv::tests::DeviceBuffer<double>        devicePatchCentres(geometry.patchCentres);
  const exahype2::fv::tests::DeviceBuffer<double>        devicePatchSizes(geometry.patchSizes);

  const double* const        onDeviceInputPatches  = deviceInputPatches.data();
  double* const              onDeviceOutputPatches = deviceOutputPatches.data();
  const double* const        onDeviceFaceInputs    = deviceFaceInputs.data();
  double* const              onDeviceFaceOutputs   = deviceFaceOutputs.data();
  const std::uint64_t* const onDeviceFaceIndices   = deviceFaceIndices.data();
  const double* const        onDevicePatchCentres  = devicePatchCentres.data();
  const double* const        onDevicePatchSizes    = devicePatchSizes.data();

  // A patch writes the half of each of its faces that lies on its own side, so
  // the two patches at a face never touch the same entries.
  double deviceMaxEigenvalue = 0.0;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(Setup::NumberOfPatches),
    [=] GPU_CALLABLE_LAMBDA(const int patch, double& maximum) {
      const double* faceInputPatches[peano4::FacesPerCell];
      double*       faceOutputPatches[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        const std::uint64_t faceOffset = onDeviceFaceIndices[patch * peano4::FacesPerCell + face] * FaceCardinality;
        faceInputPatches[face]         = onDeviceFaceInputs + faceOffset;
        faceOutputPatches[face]        = onDeviceFaceOutputs + faceOffset;
      }
      const int    patchOffset      = patch * Setup::PatchCardinality;
      const int    coordinateOffset = patch * DIMENSIONS;
      const double maxEigenvalue    = exahype2::fv::applyFiniteVolume<
        Reconstruction,
        exahype2::fv::timestepping::SingleStep,
        exahype2::fv::tests::StubSolver,
        Setup::VolumesPerAxis,
        Setup::Unknowns,
        Setup::AuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateDiffusiveSource,
        EvaluateCustomRiemann,
        true,
        true,
        double,
        double>(
        onDeviceInputPatches + patchOffset,
        onDeviceOutputPatches + patchOffset,
        faceInputPatches,
        faceOutputPatches,
        tarch::la::Vector<DIMENSIONS, double>(onDevicePatchCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(onDevicePatchSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize
      );
      maximum = (maxEigenvalue > maximum) ? maxEigenvalue : maximum;
    },
    tarch::accelerator::Max<double>{deviceMaxEigenvalue}
  );

  std::vector<double> deviceOutputPatchesResult(outputPatches.size());
  deviceOutputPatches.copyBack(deviceOutputPatchesResult);
  std::vector<double> deviceFaceOutputsResult(faceOutputs.size());
  deviceFaceOutputs.copyBack(deviceFaceOutputsResult);

  std::vector<double> hostOutputPatches = outputPatches;
  std::vector<double> hostFaceOutputs   = faceOutputs;
  double              hostMaxEigenvalue = 0.0;
  for (int patch = 0; patch < Setup::NumberOfPatches; patch++) {
    const double* faceInputPatches[peano4::FacesPerCell];
    double*       faceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      const std::uint64_t faceOffset = geometry.faceIndices[patch * peano4::FacesPerCell + face] * FaceCardinality;
      faceInputPatches[face]         = faceInputs.data() + faceOffset;
      faceOutputPatches[face]        = hostFaceOutputs.data() + faceOffset;
    }
    const int    patchOffset      = patch * Setup::PatchCardinality;
    const int    coordinateOffset = patch * DIMENSIONS;
    const double maxEigenvalue    = exahype2::fv::applyFiniteVolume<
      Reconstruction,
      exahype2::fv::timestepping::SingleStep,
      exahype2::fv::tests::StubSolver,
      Setup::VolumesPerAxis,
      Setup::Unknowns,
      Setup::AuxiliaryVariables,
      EvaluateFlux,
      EvaluateNonconservativeProduct,
      EvaluateSource,
      EvaluateDiffusiveSource,
      EvaluateCustomRiemann,
      true,
      true,
      double,
      double>(
      inputPatches.data() + patchOffset,
      hostOutputPatches.data() + patchOffset,
      faceInputPatches,
      faceOutputPatches,
      tarch::la::Vector<DIMENSIONS, double>(geometry.patchCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.patchSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
    hostMaxEigenvalue = std::max(hostMaxEigenvalue, maxEigenvalue);
  }

  validateBuffersMatch(deviceOutputPatchesResult, hostOutputPatches, "outputPatches");
  validateBuffersMatch(deviceFaceOutputsResult, hostFaceOutputs, "faceOutputs");
  // A maximum ignores the order in which the tree combines the patches, so the
  // parallel reduction has to land on exactly the serial value.
  validateNumericalEquals(deviceMaxEigenvalue, hostMaxEigenvalue);
  validate(hostMaxEigenvalue > 0.0);
}

template <int HaloSize>
void exahype2::fv::tests::KernelsOnDeviceTest::runApplyBoundaryConditionsOnFace() {
  constexpr int FaceCardinality = Setup::faceOverlapCardinality(HaloSize);

  const exahype2::fv::tests::PatchChain geometry;
  const std::vector<double> faceData =
    exahype2::fv::tests::makeRamp(geometry.faceOverlapEntries(HaloSize), 0.2, 0.011);

  const exahype2::fv::tests::DeviceBuffer<double>        deviceFaceData(faceData);
  const exahype2::fv::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
  const exahype2::fv::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
  const exahype2::fv::tests::DeviceBuffer<std::uint64_t> devicePatchIndices(geometry.workPatchIndices);
  const exahype2::fv::tests::DeviceBuffer<double>        devicePatchCentres(geometry.patchCentres);
  const exahype2::fv::tests::DeviceBuffer<double>        devicePatchSizes(geometry.patchSizes);

  double* const              onDeviceFaceData     = deviceFaceData.data();
  const std::uint64_t* const onDeviceFaceIndices  = deviceFaceIndices.data();
  const std::uint8_t* const  onDeviceFaceNumbers  = deviceFaceNumbers.data();
  const std::uint64_t* const onDevicePatchIndices = devicePatchIndices.data();
  const double* const        onDevicePatchCentres = devicePatchCentres.data();
  const double* const        onDevicePatchSizes   = devicePatchSizes.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(geometry.numberOfWorkFaces()),
    [=] GPU_CALLABLE_LAMBDA(const int face) {
      exahype2::fv::tests::StubSolver solver;
      const int                       faceNumber       = onDeviceFaceNumbers[face];
      const int                       patch            = onDevicePatchIndices[face];
      const int                       coordinateOffset = patch * DIMENSIONS;
      exahype2::fv::applyBoundaryConditionsOnFace<
        exahype2::fv::tests::StubSolver,
        Setup::VolumesPerAxis,
        HaloSize,
        Setup::Unknowns,
        Setup::AuxiliaryVariables,
        double>(
        solver,
        tarch::la::Vector<DIMENSIONS, double>(onDevicePatchCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(onDevicePatchSizes + coordinateOffset),
        Setup::TimeStamp,
        faceNumber,
        onDeviceFaceData + onDeviceFaceIndices[face] * FaceCardinality
      );
    }
  );

  std::vector<double> deviceResult(faceData.size());
  deviceFaceData.copyBack(deviceResult);

  std::vector<double> hostResult = faceData;
  for (int face = 0; face < geometry.numberOfWorkFaces(); face++) {
    exahype2::fv::tests::StubSolver solver;
    const int                       faceNumber       = geometry.workFaceNumbers[face];
    const int                       patch            = geometry.workPatchIndices[face];
    const int                       coordinateOffset = patch * DIMENSIONS;
    exahype2::fv::applyBoundaryConditionsOnFace<
      exahype2::fv::tests::StubSolver,
      Setup::VolumesPerAxis,
      HaloSize,
      Setup::Unknowns,
      Setup::AuxiliaryVariables,
      double>(
      solver,
      tarch::la::Vector<DIMENSIONS, double>(geometry.patchCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.patchSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      faceNumber,
      hostResult.data() + geometry.workFaceIndices[face] * FaceCardinality
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceData");
}
