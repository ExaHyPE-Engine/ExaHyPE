// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::tests {
  struct DeviceKernelSetup;
  struct StubSolver;
  struct DeviceDeleter;
  template <typename T>
  class DeviceBuffer;
  struct PatchChain;

  std::vector<double> makeRamp(std::size_t count, double offset, double increment);
} // namespace exahype2::fv::tests

/**
 * Sizes of the patches and faces the device kernel tests run with.
 */
TARGET_DECLARE
struct exahype2::fv::tests::DeviceKernelSetup final {
  static constexpr int    VolumesPerAxis        = 4;
  static constexpr int    Unknowns              = 2;
  static constexpr int    AuxiliaryVariables    = 1;
  static constexpr int    Data                  = Unknowns + AuxiliaryVariables;
  static constexpr int    VolumesPerFaceLayer   = VolumesPerAxis * ((DIMENSIONS == 3) ? VolumesPerAxis : 1);
  static constexpr int    VolumesPerPatch       = VolumesPerFaceLayer * VolumesPerAxis;
  static constexpr int    PatchCardinality      = VolumesPerPatch * Data;
  static constexpr int    NumberOfPatches       = 8;
  static constexpr int    AllPatchesCardinality = NumberOfPatches * PatchCardinality;
  static constexpr int    CoordinateCardinality = NumberOfPatches * DIMENSIONS;
  static constexpr int    FaceSlotCardinality   = NumberOfPatches * TWO_TIMES_D;
  static constexpr double TimeStamp             = 0.75;
  static constexpr double TimeStepSize          = 0.0125;

  /**
   * A face overlap holds 2 * haloSize layers along the normal and
   * VolumesPerAxis layers along every other axis.
   */
  static constexpr int faceOverlapCardinality(const int haloSize) { return 2 * haloSize * VolumesPerFaceLayer * Data; }
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * Every callback depends on its state and on the direction it is asked about,
 * so a volume or face read at the wrong offset produces a different number.
 */
struct exahype2::fv::tests::StubSolver final {
  static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
    const double* NOALIAS                        insideState,
    double* NOALIAS                              outsideState,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    int                                          normal
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    double* NOALIAS                              fluxes
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              sources
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const double* NOALIAS                        state,
    const double* NOALIAS                        stateGradient,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              sources
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const double* NOALIAS                        averageState,
    const double* NOALIAS                        stateJump,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    const double* NOALIAS                        leftState,
    const double* NOALIAS                        rightState,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    double* NOALIAS                              leftGoingFlux,
    double* NOALIAS                              rightGoingFlux
  );
};
TARGET_DECLARE_END

/**
 * Frees device memory, so a unique pointer can own a device buffer.
 */
struct exahype2::fv::tests::DeviceDeleter final {
  void operator()(void* data) const;
};

/**
 * Owns one buffer in device memory for the lifetime of a test. A build without
 * a GPU backend allocates and copies in plain host memory, so the tests run the
 * same code path everywhere.
 */
template <typename T>
class exahype2::fv::tests::DeviceBuffer final {
public:
  INLINE_METHOD explicit DeviceBuffer(const std::vector<T>& hostData);
  DeviceBuffer(const DeviceBuffer& other)            = delete;
  DeviceBuffer(DeviceBuffer&& other)                 = delete;
  DeviceBuffer& operator=(const DeviceBuffer& other) = delete;
  DeviceBuffer& operator=(DeviceBuffer&& other)      = delete;
  ~DeviceBuffer()                                    = default;

  INLINE_METHOD T* data() const;

  INLINE_METHOD void copyBack(std::vector<T>& target) const;

private:
  const std::size_t                       _count;
  const std::unique_ptr<T, DeviceDeleter> _data;
};

/**
 * A chain of patches along the first axis. Two neighbours along that axis hold
 * the same face, which is the arrangement in which a parallel dispatch could
 * produce a write conflict, and the faces along the other axes belong to one
 * patch each. The work list holds every unique face once, with one owning
 * patch and the face number under which that patch sees it.
 */
struct exahype2::fv::tests::PatchChain final {
  std::vector<double>        patchCentres = std::vector<double>(DeviceKernelSetup::CoordinateCardinality);
  std::vector<double>        patchSizes   = std::vector<double>(DeviceKernelSetup::CoordinateCardinality);
  std::vector<std::uint64_t> faceIndices  = std::vector<std::uint64_t>(DeviceKernelSetup::FaceSlotCardinality);
  std::vector<std::uint64_t> workFaceIndices{};
  std::vector<std::uint8_t>  workFaceNumbers{};
  std::vector<std::uint64_t> workPatchIndices{};
  int                        numberOfUniqueFaces{0};

  PatchChain();

  [[nodiscard]] int numberOfWorkFaces() const;
  [[nodiscard]] int faceOverlapEntries(int haloSize) const;
};

#include "exahype2/fv/tests/DeviceKernelFixtures.cpph"
