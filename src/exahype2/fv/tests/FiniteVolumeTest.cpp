// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FiniteVolumeTest.h"

#include "exahype2/Enumeration.h"
#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/particles/ParticleUpdate.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/reconstruction/WENO.h"
#include "exahype2/fv/timestepping/ADER.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "exahype2/fv/weno/NonlinearWeights.h"
#include "peano4/Constants.h"

namespace {
  struct TestParticle final {
    [[nodiscard]] tarch::la::Vector<DIMENSIONS, double> getX() const { return x; }
    void                 setX(const tarch::la::Vector<DIMENSIONS, double>& position) { x = position; }
    [[nodiscard]] double getData(const int index) const { return data[index]; }
    void                 setData(const int index, const double value) { data[index] = value; }

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    std::array<double, 16>                data{};
  };

  template <int NumberOfUnknowns>
  struct TestSolver {
    static constexpr double AdvectionSpeed    = 0.7;
    static constexpr double NcpAdvectionSpeed = 0.3;

    template <typename ComputePrecision>
    static void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxes[unknown] = AdvectionSpeed * state[unknown];
      }
    }

    template <typename ComputePrecision>
    static void nonconservativeProduct(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        averageState,
      const ComputePrecision* NOALIAS const                         stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               result
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        result[unknown] = static_cast<ComputePrecision>(NcpAdvectionSpeed) * stateJump[unknown];
      }
    }

    template <typename ComputePrecision>
    static void sourceTerm(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               sources
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sources[unknown] = static_cast<ComputePrecision>(0.05) * state[unknown];
      }
    }

    template <typename ComputePrecision>
    static double maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      // The wave speed bounds the combined conservative and non-conservative advection.
      return AdvectionSpeed + NcpAdvectionSpeed;
    }

    template <typename ComputePrecision>
    static void diffusiveSourceTerm(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      const ComputePrecision* NOALIAS const                         gradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               result
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        auto directionalSum = static_cast<ComputePrecision>(0);
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          directionalSum += gradient[direction * NumberOfUnknowns + unknown];
        }
        result[unknown] = static_cast<ComputePrecision>(0.01) * directionalSum;
      }
    }

    template <typename ComputePrecision>
    static double solveRiemannProblem(
      const ComputePrecision* NOALIAS const                         leftState,
      const ComputePrecision* NOALIAS const                         rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               leftGoingFlux,
      ComputePrecision* NOALIAS const                               rightGoingFlux
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const ComputePrecision centralFlux =
          static_cast<ComputePrecision>(0.5) * AdvectionSpeed * (leftState[unknown] + rightState[unknown]);
        const ComputePrecision diffusion =
          static_cast<ComputePrecision>(0.5) * AdvectionSpeed * (rightState[unknown] - leftState[unknown]);
        leftGoingFlux[unknown]  = centralFlux - diffusion;
        rightGoingFlux[unknown] = centralFlux - diffusion;
      }
      return AdvectionSpeed;
    }
  };

  constexpr int NumberOfVolumesPerAxis     = 4;
  constexpr int HaloSize                   = 1;
  constexpr int NumberOfUnknowns           = 3;
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  struct TimeDependentAdvectionSolver final : TestSolver<NumberOfUnknowns> {
    template <typename ComputePrecision>
    static void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      const auto speed = static_cast<ComputePrecision>(normal == 0 ? 1.0 + timeStamp : 0.0);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxes[unknown] = speed * state[unknown];
      }
    }

    template <typename ComputePrecision>
    static double maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal
    ) {
      return normal == 0 ? 1.0 + timeStamp : 0.0;
    }
  };

  struct NonconservativeAdvectionSolver final : TestSolver<NumberOfUnknowns> {
    template <typename ComputePrecision>
    static double solveRiemannProblem(
      const ComputePrecision* NOALIAS const                         leftState,
      const ComputePrecision* NOALIAS const                         rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               leftGoingFlux,
      ComputePrecision* NOALIAS const                               rightGoingFlux
    ) {
      constexpr double WaveSpeed =
        TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const auto stateJump   = rightState[unknown] - leftState[unknown];
        const auto centralFlux = static_cast<ComputePrecision>(0.5 * TestSolver<NumberOfUnknowns>::AdvectionSpeed)
          * (leftState[unknown] + rightState[unknown]);
        const auto diffusion = static_cast<ComputePrecision>(0.5 * WaveSpeed) * stateJump;
        const auto nonconservativeProduct =
          static_cast<ComputePrecision>(0.5 * TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed) * stateJump;
        leftGoingFlux[unknown]  = centralFlux - diffusion + nonconservativeProduct;
        rightGoingFlux[unknown] = centralFlux - diffusion - nonconservativeProduct;
      }
      return WaveSpeed;
    }
  };

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

  // The patch spans the unit cube. Halo indices can lie outside it.
  template <int VolumesPerAxis>
  tarch::la::Vector<DIMENSIONS, double> volumeCentreOf(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex) {
    tarch::la::Vector<DIMENSIONS, double> centre;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      centre(dimension) = (volumeIndex(dimension) + 0.5) / VolumesPerAxis;
    }
    return centre;
  }

  template <int VolumesPerAxis>
  int linearVolumeIndexOf(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex) {
    int linear = 0;
    for (int dimension = DIMENSIONS - 1; dimension >= 0; dimension--) {
      linear = linear * VolumesPerAxis + volumeIndex(dimension);
    }
    return linear;
  }

  // The profiles vary only along the first coordinate.
  double travellingCoordinate(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 time,
    const double                                 speed
  ) {
    return position(0) - speed * time;
  }

  double smoothProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5
      + 0.25
      * std::sin(
          2.0 * std::numbers::pi_v<double>
          * travellingCoordinate(position, time, TestSolver<NumberOfUnknowns>::AdvectionSpeed)
      );
  }

  double stepProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return travellingCoordinate(position, time, TestSolver<NumberOfUnknowns>::AdvectionSpeed) < 0.4 ? 1.0 : 0.0;
  }

  double ncpAdvectedProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    const double jointSpeed =
      TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed;
    return 1.5 + 0.25 * std::sin(2.0 * std::numbers::pi_v<double> * travellingCoordinate(position, time, jointSpeed));
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
          const int face = side == 0 ? normal : normal + DIMENSIONS;
          const int overlapIndex =
            exahype2::serialiseVoxelIndexInOverlap(overlapCell, VolumesPerAxis, HaloWidth, normal);
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

  template <int VolumesPerAxis, int HaloWidth, typename Profile>
  void fillHaloFromProfile(std::vector<std::vector<double>>& faceInStorage, const Profile profile, const double time) {
    forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
      [&faceInStorage,
       profile,
       time](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        const double value = profile(volumeCentreOf<VolumesPerAxis>(volumeIndex), time);
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceInStorage[face][overlapIndex * NumberOfData + unknown] = value;
        }
      }
    );
  }

  template <int VolumesPerAxis, int HaloWidth>
  void fillHaloPeriodically(std::vector<std::vector<double>>& faceInStorage, const std::vector<double>& q) {
    forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
      [&faceInStorage,
       &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        auto wrappedIndex = volumeIndex;
        for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
          wrappedIndex(dimension) = (wrappedIndex(dimension) + VolumesPerAxis) % VolumesPerAxis;
        }
        const int linear = linearVolumeIndexOf<VolumesPerAxis>(wrappedIndex);
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceInStorage[face][overlapIndex * NumberOfData + unknown] = q[linear * NumberOfData + unknown];
        }
      }
    );
  }

  template <
    typename Reconstruction,
    int  VolumesPerAxis,
    bool EvaluateNonconservativeProduct,
    typename Profile,
    typename SolverType        = TestSolver<NumberOfUnknowns>,
    bool EvaluateCustomRiemann = false,
    typename TimeStepping      = exahype2::fv::timestepping::SingleStep>
  std::vector<double> advectProfile(const Profile profile, const int numberOfTimeSteps, const double timeStepSize) {
    constexpr int HaloWidth        = Reconstruction::HaloSize;
    const int     patchStorageSize = numberOfVolumesInPatch<VolumesPerAxis>() * NumberOfData;
    const int     faceStorageSize  = faceOverlapStorageSize<VolumesPerAxis, HaloWidth>();

    std::vector<double> q(patchStorageSize);
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double value = profile(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), 0.0);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = value;
      }
    }

    std::vector<double>              qNext(patchStorageSize, 0.0);
    std::vector<std::vector<double>> faceInStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[peano4::FacesPerCell];
    double*                          faceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face]  = faceInStorage[face].data();
      faceOutputPatches[face] = faceOutStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      const double timeStamp = step * timeStepSize;
      fillHaloFromProfile<VolumesPerAxis, HaloWidth>(faceInStorage, profile, timeStamp);
      exahype2::fv::applyFiniteVolume<
        Reconstruction,
        TimeStepping,
        SolverType,
        VolumesPerAxis,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        true,
        EvaluateNonconservativeProduct,
        false,
        false,
        EvaluateCustomRiemann,
        false,
        true,
        double,
        double>(
        q.data(),
        qNext.data(),
        faceInputPatches,
        faceOutputPatches,
        patchCentre,
        patchExtent,
        timeStamp,
        timeStepSize,
        exahype2::fv::hostScratch<
          double,
          TimeStepping::template ScratchSize<Reconstruction, VolumesPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, EvaluateNonconservativeProduct, false>>()
      );
      q.swap(qNext);
    }
    return q;
  }

  // Returns the L1 error of the advected profile against the exact solution at
  // the final time, sampled at the volume centres.
  template <
    typename Reconstruction,
    int  VolumesPerAxis,
    bool EvaluateNonconservativeProduct,
    typename TimeStepping = exahype2::fv::timestepping::SingleStep,
    typename Profile>
  double advectionError(const Profile profile, const int numberOfTimeSteps, const double endTime) {
    const double              timeStepSize = endTime / numberOfTimeSteps;
    const std::vector<double> q            = advectProfile<
      Reconstruction,
      VolumesPerAxis,
      EvaluateNonconservativeProduct,
      Profile,
      TestSolver<NumberOfUnknowns>,
      false,
      TimeStepping>(profile, numberOfTimeSteps, timeStepSize);
    double errorSum = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double exact = profile(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), endTime);
      errorSum += std::abs(q[static_cast<std::size_t>(volume) * NumberOfData] - exact);
    }
    return errorSum / numberOfVolumesInPatch<VolumesPerAxis>();
  }

  // Halo widths use the same field at corresponding volume indices.
  double logicalState(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
    double phase = 0.7 * (unknown + 1);
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      phase += (dimension + 2.0) * volumeIndex(dimension);
    }
    return 1.0 + 0.25 * std::sin(phase);
  }

  // A vanishing result means zero slopes degenerate to the first-order
  // splitting, including the non-conservative product.
  template <bool EvaluateFlux, bool EvaluateNonconservativeProduct>
  double zeroSlopeVersusConstantMaxDifference() {
    using Solver               = TestSolver<NumberOfUnknowns>;
    using ZeroSlope            = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Zero>;
    const int patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;

    std::vector<double> qIn(patchStorageSize);
    for (int volume = 0; volume < numberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        qIn[volume * NumberOfData + unknown] = logicalState(volumeIndexOf<NumberOfVolumesPerAxis>(volume), unknown);
      }
    }

    const int constantFaceStorageSize = faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>();
    const int limitedFaceStorageSize  = faceOverlapStorageSize<NumberOfVolumesPerAxis, ZeroSlope::HaloSize>();
    std::vector<std::vector<double>> constantFaceIn(
      peano4::FacesPerCell,
      std::vector<double>(constantFaceStorageSize, 0.0)
    );
    std::vector<std::vector<double>> limitedFaceIn(
      peano4::FacesPerCell,
      std::vector<double>(limitedFaceStorageSize, 0.0)
    );
    std::vector<std::vector<double>> constantFaceOut(
      peano4::FacesPerCell,
      std::vector<double>(constantFaceStorageSize, 0.0)
    );
    std::vector<std::vector<double>> limitedFaceOut(
      peano4::FacesPerCell,
      std::vector<double>(limitedFaceStorageSize, 0.0)
    );

    forEachOverlapVoxel<NumberOfVolumesPerAxis, HaloSize>(
      [&constantFaceIn](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          constantFaceIn[face][overlapIndex * NumberOfData + unknown] = logicalState(volumeIndex, unknown);
        }
      }
    );
    forEachOverlapVoxel<NumberOfVolumesPerAxis, ZeroSlope::HaloSize>(
      [&limitedFaceIn](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          limitedFaceIn[face][overlapIndex * NumberOfData + unknown] = logicalState(volumeIndex, unknown);
        }
      }
    );

    std::vector<double> qOutConstant(patchStorageSize, 0.0);
    std::vector<double> qOutLimited(patchStorageSize, 0.0);
    const double*       constantFaceInputPatches[peano4::FacesPerCell];
    const double*       limitedFaceInputPatches[peano4::FacesPerCell];
    double*             constantFaceOutputPatches[peano4::FacesPerCell];
    double*             limitedFaceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      constantFaceInputPatches[face]  = constantFaceIn[face].data();
      limitedFaceInputPatches[face]   = limitedFaceIn[face].data();
      constantFaceOutputPatches[face] = constantFaceOut[face].data();
      limitedFaceOutputPatches[face]  = limitedFaceOut[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
    const double                                timeStepSize = 0.01;

    const double eigenvalueConstant = exahype2::fv::applyFiniteVolume<
      exahype2::fv::reconstruction::Constant,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      EvaluateFlux,
      EvaluateNonconservativeProduct,
      false,
      false,
      false,
      false,
      true,
      double,
      double>(
      qIn.data(),
      qOutConstant.data(),
      constantFaceInputPatches,
      constantFaceOutputPatches,
      patchCentre,
      patchExtent,
      0.0,
      timeStepSize,
      nullptr
    );

    const double eigenvalueLimited = exahype2::fv::applyFiniteVolume<
      ZeroSlope,
      exahype2::fv::timestepping::SingleStep,
      Solver,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      EvaluateFlux,
      EvaluateNonconservativeProduct,
      false,
      false,
      false,
      false,
      true,
      double,
      double>(
      qIn.data(),
      qOutLimited.data(),
      limitedFaceInputPatches,
      limitedFaceOutputPatches,
      patchCentre,
      patchExtent,
      0.0,
      timeStepSize,
      nullptr
    );

    double maxDifference = std::abs(eigenvalueConstant - eigenvalueLimited);
    for (int entry = 0; entry < patchStorageSize; entry++) {
      maxDifference = std::max(maxDifference, std::abs(qOutConstant[entry] - qOutLimited[entry]));
    }
    return maxDifference;
  }

  struct MimickingReconstructionSolver final : TestSolver<NumberOfUnknowns> {
    template <typename ComputePrecision>
    static void reconstruct(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        leftState,
      const ComputePrecision* NOALIAS const                         centreState,
      [[maybe_unused]] const ComputePrecision* NOALIAS const        rightState,
      const ComputePrecision* NOALIAS const                         limitedDifferences,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const int                                    normal,
      const int                                                     unknown,
      ComputePrecision&                                             reconstructedMinus,
      ComputePrecision&                                             reconstructedPlus
    ) {
      reconstructedMinus = centreState[unknown] - static_cast<ComputePrecision>(0.5) * limitedDifferences[unknown];
      reconstructedPlus  = centreState[unknown] + static_cast<ComputePrecision>(0.5) * limitedDifferences[unknown];
    }
  };

  struct MimickingMinmodSolver final : TestSolver<NumberOfUnknowns> {
    template <typename ComputePrecision>
    static ComputePrecision limitedDifference(
      const ComputePrecision* NOALIAS const                         leftState,
      const ComputePrecision* NOALIAS const                         centreState,
      const ComputePrecision* NOALIAS const                         rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const int                                    normal,
      const int                                                     unknown
    ) {
      return exahype2::fv::limiter::Minmod::limitedDifference(
        rightState[unknown] - centreState[unknown],
        centreState[unknown] - leftState[unknown]
      );
    }
  };

  template <typename SolverType, int VolumesPerAxis>
  std::vector<double> advanceLogicalField(const int numberOfTimeSteps, const double timeStepSize) {
    using SecondOrderReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;
    constexpr int HaloWidth         = SecondOrderReconstruction::HaloSize;
    const int     patchStorageSize  = numberOfVolumesInPatch<VolumesPerAxis>() * NumberOfData;
    int           faceCells         = 2 * HaloWidth;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      faceCells *= VolumesPerAxis;
    }
    const int faceStorageSize = faceCells * NumberOfData;

    std::vector<double> q(patchStorageSize);
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = logicalState(volumeIndexOf<VolumesPerAxis>(volume), unknown);
      }
    }

    std::vector<double>              qNext(patchStorageSize, 0.0);
    std::vector<std::vector<double>> faceInStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[peano4::FacesPerCell];
    double*                          faceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face]  = faceInStorage[face].data();
      faceOutputPatches[face] = faceOutStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
        [&faceInStorage,
         &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
          auto wrappedIndex = volumeIndex;
          for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
            wrappedIndex(dimension) = (wrappedIndex(dimension) + VolumesPerAxis) % VolumesPerAxis;
          }
          const int linear = linearVolumeIndexOf<VolumesPerAxis>(wrappedIndex);
          for (int unknown = 0; unknown < NumberOfData; unknown++) {
            faceInStorage[face][overlapIndex * NumberOfData + unknown] = q[linear * NumberOfData + unknown];
          }
        }
      );
      exahype2::fv::applyFiniteVolume<
        SecondOrderReconstruction,
        exahype2::fv::timestepping::SingleStep,
        SolverType,
        VolumesPerAxis,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        true,
        false,
        false,
        false,
        false,
        false,
        true,
        double,
        double>(
        q.data(),
        qNext.data(),
        faceInputPatches,
        faceOutputPatches,
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        nullptr
      );
      q.swap(qNext);
    }
    return q;
  }

  constexpr int SodUnknowns = DIMENSIONS + 2;

  // The momenta occupy the slots after the density and the energy sits last, so
  // the layout follows the dimensions of the build.
  struct SodSolver final {
    static constexpr double Gamma = 1.4;

    template <typename ComputePrecision>
    static double pressureOf(const ComputePrecision* NOALIAS const state) {
      double kineticEnergy = 0.0;
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        kineticEnergy += state[1 + dimension] * state[1 + dimension];
      }
      kineticEnergy *= 0.5 / state[0];
      return (Gamma - 1.0) * (state[DIMENSIONS + 1] - kineticEnergy);
    }

    template <typename ComputePrecision>
    static void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      const ComputePrecision normalVelocity = state[1 + normal] / state[0];
      const ComputePrecision pressure       = pressureOf(state);
      fluxes[0]                             = state[1 + normal];
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        fluxes[1 + dimension] = state[1 + dimension] * normalVelocity;
      }
      fluxes[1 + normal] += pressure;
      fluxes[DIMENSIONS + 1] = (state[DIMENSIONS + 1] + pressure) * normalVelocity;
    }

    template <typename ComputePrecision>
    static double maxEigenvalue(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal
    ) {
      const double soundSpeed = std::sqrt(Gamma * pressureOf(state) / state[0]);
      return std::abs(state[1 + normal] / state[0]) + soundSpeed;
    }
  };

  void sodInitialState(const tarch::la::Vector<DIMENSIONS, double>& position, double* NOALIAS const state) {
    const bool left = position(0) < 0.5;

    state[0] = left ? 1.0 : 0.125;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      state[1 + dimension] = 0.0;
    }
    state[DIMENSIONS + 1] = (left ? 1.0 : 0.1) / (SodSolver::Gamma - 1.0);
  }

  // The first-axis halo carries the untouched far field and the other halos copy
  // the nearest interior volume, which is exact while the waves stay inside the
  // patch and the solution does not vary along those axes.
  template <int VolumesPerAxis>
  std::vector<double> advanceSodTube(const int numberOfTimeSteps, const double timeStepSize) {
    using MinmodReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;
    constexpr int HaloWidth    = MinmodReconstruction::HaloSize;

    const int patchStorageSize = numberOfVolumesInPatch<VolumesPerAxis>() * SodUnknowns;
    int       faceCells        = 2 * HaloWidth;
    for (int dimension = 0; dimension < DIMENSIONS - 1; dimension++) {
      faceCells *= VolumesPerAxis;
    }
    const int faceStorageSize = faceCells * SodUnknowns;

    std::vector<double> q(patchStorageSize);
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      sodInitialState(
        volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)),
        &q[static_cast<std::size_t>(volume) * SodUnknowns]
      );
    }

    std::vector<double>              qNext(patchStorageSize, 0.0);
    std::vector<std::vector<double>> faceInStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[peano4::FacesPerCell];
    double*                          faceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face]  = faceInStorage[face].data();
      faceOutputPatches[face] = faceOutStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
        [&faceInStorage,
         &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
          double* const target = &faceInStorage[face][static_cast<std::size_t>(overlapIndex) * SodUnknowns];
          const int     normal = face % DIMENSIONS;
          if (normal == 0) {
            sodInitialState(volumeCentreOf<VolumesPerAxis>(volumeIndex), target);
          } else {
            auto clampedIndex          = volumeIndex;
            clampedIndex(normal)       = std::clamp(clampedIndex(normal), 0, VolumesPerAxis - 1);
            const double* const source = &q[linearVolumeIndexOf<VolumesPerAxis>(clampedIndex) * SodUnknowns];
            for (int unknown = 0; unknown < SodUnknowns; unknown++) {
              target[unknown] = source[unknown];
            }
          }
        }
      );
      exahype2::fv::applyFiniteVolume<
        MinmodReconstruction,
        exahype2::fv::timestepping::SingleStep,
        SodSolver,
        VolumesPerAxis,
        SodUnknowns,
        0,
        true,
        false,
        false,
        false,
        false,
        false,
        true,
        double,
        double>(
        q.data(),
        qNext.data(),
        faceInputPatches,
        faceOutputPatches,
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        nullptr
      );
      q.swap(qNext);
    }
    return q;
  }

  template <int Order>
  using ADERWENO = exahype2::fv::reconstruction::WENO<Order, exahype2::fv::weno::JiangShu<8>>;

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
    using Reconstruction    = ADERWENO<Order>;
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
          const int axis         = sharedFace % DIMENSIONS;
          const int nextToFace   = sharedFace < DIMENSIONS ? 0 : VolumesPerAxis - 1;
          const int acrossFace   = sharedFace < DIMENSIONS ? -1 : VolumesPerAxis;
          bool      touchesFace  = volumeIndex(axis) == nextToFace or volumeIndex(axis) == acrossFace;
          for (int tangential = 0; tangential < DIMENSIONS; tangential++) {
            touchesFace = touchesFace and (tangential == axis or (volumeIndex(tangential) >= 0 and volumeIndex(tangential) < VolumesPerAxis));
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
            exact *= polynomialValue<Order>((volumeIndex(axis) + Basis::nodes()[nodeRemainder % Basis::NumberOfNodes]) * h);
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

  // Advances the smooth profile over a torus for a few steps and returns the
  // total mass of the first unknown before and after.
  template <
    typename Reconstruction,
    int VolumesPerAxis,
    typename TimeStepping = exahype2::fv::timestepping::SingleStep>
  std::pair<double, double> periodicAdvectionMass() {
    constexpr int HaloWidth = Reconstruction::HaloSize;

    const int patchStorageSize = numberOfVolumesInPatch<VolumesPerAxis>() * NumberOfData;
    const int faceStorageSize  = faceOverlapStorageSize<VolumesPerAxis, HaloWidth>();

    std::vector<double> q(patchStorageSize);
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double value = smoothProfile(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), 0.0);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = value;
      }
    }

    double massBefore = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      massBefore += q[volume * NumberOfData];
    }

    std::vector<double>              qNext(patchStorageSize, 0.0);
    std::vector<std::vector<double>> faceInStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[peano4::FacesPerCell];
    double*                          faceOutputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face]  = faceInStorage[face].data();
      faceOutputPatches[face] = faceOutStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
    const double                                timeStepSize = 0.02 / 3.0;

    for (int step = 0; step < 3; step++) {
      fillHaloPeriodically<VolumesPerAxis, HaloWidth>(faceInStorage, q);
      exahype2::fv::applyFiniteVolume<
        Reconstruction,
        TimeStepping,
        TestSolver<NumberOfUnknowns>,
        VolumesPerAxis,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        true,
        false,
        false,
        false,
        false,
        false,
        true,
        double,
        double>(
        q.data(),
        qNext.data(),
        faceInputPatches,
        faceOutputPatches,
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        exahype2::fv::hostScratch<
          double,
          TimeStepping::template ScratchSize<Reconstruction, VolumesPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, false, false>>()
      );
      q.swap(qNext);
    }

    double massAfter = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      massAfter += q[volume * NumberOfData];
    }
    return {massBefore, massAfter};
  }
} // namespace

exahype2::fv::tests::FiniteVolumeTest::FiniteVolumeTest() :
  tarch::tests::TestCase("exahype2::fv::tests::FiniteVolumeTest") {}

void exahype2::fv::tests::FiniteVolumeTest::run() {
  testMethod(testZeroTimeStepPreservesState);
  testMethod(testUniformStateIsPreserved);
  testMethod(testLimitedLinearPreservesUniformState);
  testMethod(testLimitedLinearIntegratesTimeDependentAdvection);
  testMethod(testLimitedLinearAverageConvergesAtSecondOrder);
  testMethod(testConstantConvergesAtFirstOrder);
  testMethod(testLimitedLinearMinmodKeepsStepInBounds);
  testMethod(testLimitedLinearZeroSlopesMatchConstant);
  testMethod(testLimitedLinearNcpAdvectionConvergesAtSecondOrder);
  testMethod(testLimitedLinearCustomRiemannIncludesVolumeNonconservativeProduct);
  testMethod(testPeriodicAdvectionConservesMass);
  testMethod(testSodShockTubeReachesStarStates);
  testMethod(testReconstructionHookReproducingDefaultChangesNothing);
  testMethod(testLimiterHookReproducingMinmodChangesNothing);
  testMethod(testWENOReconstructsPolynomialsExactly);
  testMethod(testADERWENOPreservesUniformState);
  testMethod(testADERWENOAdvectionConvergesAtHighOrder);
  testMethod(testADERWENONcpAdvectionConvergesAtHighOrder);
  testMethod(testADERWENOPeriodicAdvectionConservesMass);
  testMethod(testParticleUpdateAdvancesWithExplicitEuler);
  testMethod(testParticleAccessorsMatchAttributeOffsets);
}

/** Checks that a time step of size zero leaves the patch unchanged. */
void exahype2::fv::tests::FiniteVolumeTest::testZeroTimeStepPreservesState() {
  const int           patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  std::vector<double> qIn(patchStorageSize);
  std::vector<double> qOut(patchStorageSize, 0.0);
  for (int entry = 0; entry < patchStorageSize; entry++) {
    qIn[entry] = 0.1 + 0.01 * static_cast<double>(entry);
  }

  std::vector<std::vector<double>> faceInStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.5)
  );
  std::vector<std::vector<double>> faceOutStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.0)
  );
  const double* faceInputPatches[peano4::FacesPerCell];
  double*       faceOutputPatches[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceInputPatches[face]  = faceInStorage[face].data();
    faceOutputPatches[face] = faceOutStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

  exahype2::fv::applyFiniteVolume<
    exahype2::fv::reconstruction::Constant,
    exahype2::fv::timestepping::SingleStep,
    TestSolver<NumberOfUnknowns>,
    NumberOfVolumesPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    false,
    false,
    false,
    true,
    double,
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.0, nullptr);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], qIn[entry]);
  }
}

/** Checks that the finite-volume update keeps a uniform state uniform. */
void exahype2::fv::tests::FiniteVolumeTest::testUniformStateIsPreserved() {
  const int           patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const double        uniformValue     = 1.23;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), uniformValue)
  );
  std::vector<std::vector<double>> faceOutStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.0)
  );
  const double* faceInputPatches[peano4::FacesPerCell];
  double*       faceOutputPatches[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceInputPatches[face]  = faceInStorage[face].data();
    faceOutputPatches[face] = faceOutStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

  const double returnedEigenvalue = exahype2::fv::applyFiniteVolume<
    exahype2::fv::reconstruction::Constant,
    exahype2::fv::timestepping::SingleStep,
    TestSolver<NumberOfUnknowns>,
    NumberOfVolumesPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    false,
    false,
    false,
    true,
    double,
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.01, nullptr);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed
  );
}

/** Checks that the limited linear reconstruction keeps a uniform state uniform. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearPreservesUniformState() {
  using UniformReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::VanAlbada>;
  const int patchStorageSize  = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const int faceStorageSize   = faceOverlapStorageSize<NumberOfVolumesPerAxis, UniformReconstruction::HaloSize>();

  const double        uniformValue = 1.23;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceStorageSize, uniformValue)
  );
  std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
  const double*                    faceInputPatches[peano4::FacesPerCell];
  double*                          faceOutputPatches[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceInputPatches[face]  = faceInStorage[face].data();
    faceOutputPatches[face] = faceOutStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

  const double returnedEigenvalue = exahype2::fv::applyFiniteVolume<
    UniformReconstruction,
    exahype2::fv::timestepping::SingleStep,
    TestSolver<NumberOfUnknowns>,
    NumberOfVolumesPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    false,
    false,
    false,
    true,
    double,
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.01, nullptr);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed
  );
}

/** Checks the exact displacement of a linear profile under time-dependent advection. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearIntegratesTimeDependentAdvection() {
  using Reconstruction          = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double TimeStepSize = 0.01;
  const auto       profile      = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5 + position(0) - time - 0.5 * time * time;
  };
  const auto solution =
    advectProfile<Reconstruction, NumberOfVolumesPerAxis, false, decltype(profile), TimeDependentAdvectionSolver>(
      profile,
      1,
      TimeStepSize
    );

  for (int volume = 0; volume < numberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
    const auto   centre   = volumeCentreOf<NumberOfVolumesPerAxis>(volumeIndexOf<NumberOfVolumesPerAxis>(volume));
    const double expected = profile(centre, TimeStepSize);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(solution[volume * NumberOfData + unknown], expected, volume, unknown);
    }
  }
}

/** Measures the limited-linear order against an analytic smooth-advection solution. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearAverageConvergesAtSecondOrder() {
  using SecondOrderReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double EndTime        = 0.12;

  const double coarseError = advectionError<SecondOrderReconstruction, 9, false>(smoothProfile, 6, EndTime);
  const double fineError   = advectionError<SecondOrderReconstruction, 27, false>(smoothProfile, 18, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 1.7, coarseError, fineError, order);
}

/** Measures the constant reconstruction order against an analytic smooth-advection solution. */
void exahype2::fv::tests::FiniteVolumeTest::testConstantConvergesAtFirstOrder() {
  constexpr double EndTime = 0.12;

  const double coarseError =
    advectionError<exahype2::fv::reconstruction::Constant, 9, false>(smoothProfile, 6, EndTime);
  const double fineError =
    advectionError<exahype2::fv::reconstruction::Constant, 27, false>(smoothProfile, 18, EndTime);
  const double order = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 0.6 and order < 1.4, coarseError, fineError, order);
}

/** Confirms the Minmod limiter keeps a step profile in bounds and never grows its total variation. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearMinmodKeepsStepInBounds() {
  using MinmodReconstruction   = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;
  constexpr int VolumesPerAxis = 27;

  const std::vector<double> q = advectProfile<MinmodReconstruction, VolumesPerAxis, false>(stepProfile, 8, 0.02 / 3.0);

  for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    const double profileValue = q[static_cast<std::size_t>(volume) * NumberOfData];
    validateWithParams2(profileValue >= -1e-12, volume, profileValue);
    validateWithParams2(profileValue <= 1.0 + 1e-12, volume, profileValue);
  }

  // The total variation along the advection direction never exceeds the single
  // initial jump.
  const int numberOfRows = numberOfVolumesInPatch<VolumesPerAxis>() / VolumesPerAxis;
  for (int row = 0; row < numberOfRows; row++) {
    double totalVariation = 0.0;
    for (int volume = 0; volume + 1 < VolumesPerAxis; volume++) {
      const double left  = q[static_cast<std::size_t>(row * VolumesPerAxis + volume) * NumberOfData];
      const double right = q[static_cast<std::size_t>(row * VolumesPerAxis + volume + 1) * NumberOfData];
      totalVariation += std::abs(right - left);
    }
    validateWithParams2(totalVariation <= 1.0 + 1e-12, row, totalVariation);
  }
}

/** Confirms zero slopes reduce the limited reconstruction to the first-order splitting. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearZeroSlopesMatchConstant() {
  validateNumericalEquals((zeroSlopeVersusConstantMaxDifference<true, false>()), 0.0);
  validateNumericalEquals((zeroSlopeVersusConstantMaxDifference<false, true>()), 0.0);
  validateNumericalEquals((zeroSlopeVersusConstantMaxDifference<true, true>()), 0.0);
}

/** Measures the order of a profile the flux and the non-conservative product advect together. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearNcpAdvectionConvergesAtSecondOrder() {
  using SecondOrderReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double EndTime        = 0.12;

  const double coarseError = advectionError<SecondOrderReconstruction, 9, true>(ncpAdvectedProfile, 8, EndTime);
  const double fineError   = advectionError<SecondOrderReconstruction, 27, true>(ncpAdvectedProfile, 24, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 1.7, coarseError, fineError, order);
}

/** Checks that a custom Riemann solver retains non-conservative advection inside each volume. */
void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearCustomRiemannIncludesVolumeNonconservativeProduct() {
  using Reconstruction          = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double TimeStepSize = 0.01;
  const auto       profile      = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5 + position(0)
      - (TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed) * time;
  };
  const auto solution =
    advectProfile<Reconstruction, NumberOfVolumesPerAxis, true, decltype(profile), NonconservativeAdvectionSolver, true>(
      profile,
      1,
      TimeStepSize
    );

  for (int volume = 0; volume < numberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
    const auto   centre   = volumeCentreOf<NumberOfVolumesPerAxis>(volumeIndexOf<NumberOfVolumesPerAxis>(volume));
    const double expected = profile(centre, TimeStepSize);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(solution[volume * NumberOfData + unknown], expected, volume, unknown);
    }
  }
}

/** Confirms periodic face fluxes conserve the patch's total mass. */
void exahype2::fv::tests::FiniteVolumeTest::testPeriodicAdvectionConservesMass() {
  using MinmodReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;

  const auto [massBefore, massAfter] = periodicAdvectionMass<MinmodReconstruction, 27>();

  validateWithParams2(std::abs(massAfter - massBefore) <= 1e-11 * std::abs(massBefore), massBefore, massAfter);
}

/** Confirms the shock-capturing path reaches the Sod tube's known star-region states. */
void exahype2::fv::tests::FiniteVolumeTest::testSodShockTubeReachesStarStates() {
  constexpr int    VolumesPerAxis = DIMENSIONS == 2 ? 54 : 36;
  constexpr int    Steps          = 116;
  constexpr double TimeStepSize   = 0.00175;

  const std::vector<double> q = advanceSodTube<VolumesPerAxis>(Steps, TimeStepSize);

  for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    const double density  = q[static_cast<std::size_t>(volume) * SodUnknowns];
    const double pressure = SodSolver::pressureOf(&q[static_cast<std::size_t>(volume) * SodUnknowns]);
    validateWithParams2(density > 0.0, volume, density);
    validateWithParams2(pressure > 0.0, volume, pressure);
    validateWithParams2(density <= 1.0 + 1e-6, volume, density);
    validateWithParams2(density >= 0.125 - 1e-6, volume, density);
  }

  // Exact star-region values of the standard Sod problem at the sampled
  // plateau positions.
  constexpr double StarPressure     = 0.30313;
  constexpr double StarVelocity     = 0.92745;
  constexpr double StarDensityLeft  = 0.42632;
  constexpr double StarDensityRight = 0.26557;

  // The first axis runs fastest, so its volumes lead the patch.
  const auto volumeAt = [](const double position) { return static_cast<int>(position * VolumesPerAxis); };
  const auto probe    = [&q](const int volumeAlongFirstAxis, const int unknown) {
    return q[volumeAlongFirstAxis * SodUnknowns + unknown];
  };
  // The left plateau lies between the rarefaction tail and the contact.
  for (const double position : {0.54, 0.58}) {
    validateWithParams2(
      std::abs(probe(volumeAt(position), 0) - StarDensityLeft) < 0.03,
      position,
      probe(volumeAt(position), 0)
    );
  }
  // The right plateau lies between the contact and the shock.
  for (const double position : {0.76, 0.79}) {
    validateWithParams2(
      std::abs(probe(volumeAt(position), 0) - StarDensityRight) < 0.03,
      position,
      probe(volumeAt(position), 0)
    );
  }
  // Velocity and pressure stay constant across the contact, so a sample on
  // either side of it must give the same star value.
  for (const double position : {0.58, 0.76}) {
    const double velocity = probe(volumeAt(position), 1) / probe(volumeAt(position), 0);
    const double pressure = SodSolver::pressureOf(&q[static_cast<std::size_t>(volumeAt(position)) * SodUnknowns]);
    validateWithParams2(std::abs(velocity - StarVelocity) < 0.05, position, velocity);
    validateWithParams2(std::abs(pressure - StarPressure) < 0.03, position, pressure);
  }
}

/**
 * Confirms that the solver's reconstruct hook supplies the face states evolved
 * by the half-step.
 */
void exahype2::fv::tests::FiniteVolumeTest::testReconstructionHookReproducingDefaultChangesNothing() {
  constexpr int    VolumesPerAxis = 9;
  constexpr int    Steps          = 3;
  constexpr double TimeStepSize   = 0.01;

  const std::vector<double> withoutHook =
    advanceLogicalField<TestSolver<NumberOfUnknowns>, VolumesPerAxis>(Steps, TimeStepSize);
  const std::vector<double> withHook =
    advanceLogicalField<MimickingReconstructionSolver, VolumesPerAxis>(Steps, TimeStepSize);

  double maxDifference = 0.0;
  for (std::size_t entry = 0; entry < withoutHook.size(); entry++) {
    maxDifference = std::max(maxDifference, std::abs(withoutHook[entry] - withHook[entry]));
  }
  validateNumericalEquals(maxDifference, 0.0);
}

/**
 * Confirms that the solver's limitedDifference hook supplies the slopes
 * consumed by reconstruction.
 */
void exahype2::fv::tests::FiniteVolumeTest::testLimiterHookReproducingMinmodChangesNothing() {
  constexpr int    VolumesPerAxis = 9;
  constexpr int    Steps          = 3;
  constexpr double TimeStepSize   = 0.01;

  const std::vector<double> fromPolicy =
    advanceLogicalField<TestSolver<NumberOfUnknowns>, VolumesPerAxis>(Steps, TimeStepSize);
  const std::vector<double> fromHook = advanceLogicalField<MimickingMinmodSolver, VolumesPerAxis>(Steps, TimeStepSize);

  double maxDifference = 0.0;
  for (std::size_t entry = 0; entry < fromPolicy.size(); entry++) {
    maxDifference = std::max(maxDifference, std::abs(fromPolicy[entry] - fromHook[entry]));
  }
  validateNumericalEquals(maxDifference, 0.0);
}

/**
 * Every candidate stencil reproduces a polynomial of the reconstruction's order, so the
 * combination does too, also where the stencils beyond the patch's edges and corners drop out.
 */
void exahype2::fv::tests::FiniteVolumeTest::testWENOReconstructsPolynomialsExactly() {
  const double secondOrderError = wenoPolynomialReconstructionError<2, 4>();
  const double thirdOrderError  = wenoPolynomialReconstructionError<3, 6>();

  validateWithParams1(secondOrderError < 1e-10, secondOrderError);
  validateWithParams1(thirdOrderError < 1e-10, thirdOrderError);
}

/**
 * Only the source term acts on a uniform state, and the space-time predictor integrates it to the
 * exponential growth rather than the forward Euler step.
 */
void exahype2::fv::tests::FiniteVolumeTest::testADERWENOPreservesUniformState() {
  using Reconstruction       = ADERWENO<2>;
  const int patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const int faceStorageSize  = faceOverlapStorageSize<NumberOfVolumesPerAxis, Reconstruction::HaloSize>();

  const double        uniformValue = 1.23;
  const double        timeStepSize = 0.01;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, uniformValue));
  std::vector<std::vector<double>> faceOutStorage(peano4::FacesPerCell, std::vector<double>(faceStorageSize, 0.0));
  const double*                    faceInputPatches[peano4::FacesPerCell];
  double*                          faceOutputPatches[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceInputPatches[face]  = faceInStorage[face].data();
    faceOutputPatches[face] = faceOutStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

  const double returnedEigenvalue = exahype2::fv::applyFiniteVolume<
    Reconstruction,
    exahype2::fv::timestepping::ADER,
    TestSolver<NumberOfUnknowns>,
    NumberOfVolumesPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    false,
    false,
    false,
    true,
    double,
    double>(
    qIn.data(),
    qOut.data(),
    faceInputPatches,
    faceOutputPatches,
    patchCentre,
    patchExtent,
    0.0,
    timeStepSize,
    exahype2::fv::hostScratch<
      double,
      exahype2::fv::timestepping::ADER::
        ScratchSize<Reconstruction, NumberOfVolumesPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, true, true>>()
  );

  const double grownValue = uniformValue * std::exp(0.05 * timeStepSize);
  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEqualsWithParams1(qOut[entry], grownValue, entry);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed
  );
}

/** The predictor lifts the order in time to the order of the reconstruction. */
void exahype2::fv::tests::FiniteVolumeTest::testADERWENOAdvectionConvergesAtHighOrder() {
  using ADER                = exahype2::fv::timestepping::ADER;
  constexpr double EndTime  = 0.12;

  const double secondOrderCoarseError = advectionError<ADERWENO<2>, 8, false, ADER>(smoothProfile, 6, EndTime);
  const double secondOrderFineError   = advectionError<ADERWENO<2>, 16, false, ADER>(smoothProfile, 12, EndTime);
  const double secondOrder            = std::log(secondOrderCoarseError / secondOrderFineError) / std::log(2.0);
  validateWithParams3(secondOrder > 2.7, secondOrderCoarseError, secondOrderFineError, secondOrder);

  const double thirdOrderCoarseError = advectionError<ADERWENO<3>, 8, false, ADER>(smoothProfile, 6, EndTime);
  const double thirdOrderFineError   = advectionError<ADERWENO<3>, 16, false, ADER>(smoothProfile, 12, EndTime);
  const double thirdOrder            = std::log(thirdOrderCoarseError / thirdOrderFineError) / std::log(2.0);
  validateWithParams3(thirdOrder > 3.7, thirdOrderCoarseError, thirdOrderFineError, thirdOrder);
}

/** The fluctuations across the faces and the predictor's volume part advect at high order. */
void exahype2::fv::tests::FiniteVolumeTest::testADERWENONcpAdvectionConvergesAtHighOrder() {
  using ADER               = exahype2::fv::timestepping::ADER;
  constexpr double EndTime = 0.12;

  const double coarseError = advectionError<ADERWENO<2>, 8, true, ADER>(ncpAdvectedProfile, 8, EndTime);
  const double fineError   = advectionError<ADERWENO<2>, 16, true, ADER>(ncpAdvectedProfile, 16, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(2.0);

  validateWithParams3(order > 2.7, coarseError, fineError, order);
}

/** Confirms periodic face fluxes conserve the patch's total mass under ADER-WENO. */
void exahype2::fv::tests::FiniteVolumeTest::testADERWENOPeriodicAdvectionConservesMass() {
  const auto [massBefore, massAfter] = periodicAdvectionMass<ADERWENO<2>, 16, exahype2::fv::timestepping::ADER>();

  validateWithParams2(std::abs(massAfter - massBefore) <= 1e-11 * std::abs(massBefore), massBefore, massAfter);
}

/**
 * Checks that a particle without force keeps its velocity and moves by the velocity times the
 * time step size.
 */
void exahype2::fv::tests::FiniteVolumeTest::testParticleUpdateAdvancesWithExplicitEuler() {
  const int  originalForceMethod         = exahype2::fv::particles::forces::globalForceMethod;
  const int  originalVoxelsPerAxis       = exahype2::fv::particles::interpolation::globalVoxelsPerAxis;
  const int  originalUnknownsPerVoxel    = exahype2::fv::particles::interpolation::globalUnknownsPerVoxel;
  const int  originalInterpolationMethod = exahype2::fv::particles::interpolation::globalInterpolationMethod;
  const auto originalDomainOffset        = exahype2::fv::particles::utils::domainOffset;
  const auto originalDomainSize          = exahype2::fv::particles::utils::domainSize;
  const auto originalPeriodicity         = exahype2::fv::particles::utils::isPeriodic;

  TestParticle particle;
  particle.x = tarch::la::Vector<DIMENSIONS, double>(0.25);

  particle.data[exahype2::fv::particles::utils::ParticleVelocityAttributeOffset] = 2.0;

  exahype2::fv::particles::forces::disableAllForces();
  exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(1, 1, 0);
  exahype2::fv::particles::utils::setGlobalParameters(
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    tarch::la::Vector<DIMENSIONS, int>(1)
  );

  peano4::dataview::CellMarker marker{};
  marker.centre              = tarch::la::Vector<DIMENSIONS, double>(0.5);
  marker.h                   = tarch::la::Vector<DIMENSIONS, double>(1.0);
  const double cellValues[1] = {0.0};

  exahype2::fv::particles::updateParticleState<0>(particle, marker, cellValues, 0.1, 1);
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    const double expectedPosition = dimension == 0 ? 0.45 : 0.25;
    const double expectedVelocity = dimension == 0 ? 2.0 : 0.0;
    validateNumericalEquals(particle.x(dimension), expectedPosition);
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleVelocityAttributeOffset + dimension],
      expectedVelocity
    );
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleAccelerationAttributeOffset + dimension],
      0.0
    );
  }

  exahype2::fv::particles::forces::setForceMethod(originalForceMethod);
  exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
    originalVoxelsPerAxis,
    originalUnknownsPerVoxel,
    originalInterpolationMethod
  );
  exahype2::fv::particles::utils::setGlobalParameters(originalDomainOffset, originalDomainSize, originalPeriodicity);
}

/**
 * Checks that the particle accessors read and write the data entries their offsets name and
 * leave the interpolated fluid velocity untouched.
 */
void exahype2::fv::tests::FiniteVolumeTest::testParticleAccessorsMatchAttributeOffsets() {
  TestParticle particle;

  tarch::la::Vector<DIMENSIONS, double> velocity;
  tarch::la::Vector<DIMENSIONS, double> acceleration;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    velocity(axis)     = 1.0 + axis;
    acceleration(axis) = -1.0 - axis;

    particle.data[exahype2::fv::particles::utils::FluidVelocityAttributeOffset + axis] = 10.0 + axis;
  }
  particle.data[exahype2::fv::particles::utils::FluidDensityAttributeOffset] = 7.0;

  exahype2::fv::particles::utils::setVelocity(particle, velocity);
  exahype2::fv::particles::utils::setAcceleration(particle, acceleration);
  exahype2::fv::particles::utils::setMass(particle, 3.0);
  exahype2::fv::particles::utils::setRadius(particle, 0.5);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::FluidVelocityAttributeOffset + axis],
      10.0 + axis
    );
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleVelocityAttributeOffset + axis],
      1.0 + axis
    );
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleAccelerationAttributeOffset + axis],
      -1.0 - axis
    );
  }

  validateNumericalEquals(exahype2::fv::particles::utils::getInterpolatedRho(particle), 7.0);
  validateNumericalEquals(exahype2::fv::particles::utils::getMass(particle), 3.0);
  validateNumericalEquals(exahype2::fv::particles::utils::getRadius(particle), 0.5);
  validateNumericalEquals(
    exahype2::fv::particles::utils::computeKineticEnergy(particle),
    0.5 * 3.0 * tarch::la::dot(velocity, velocity)
  );

  const auto state    = exahype2::fv::particles::utils::getParticleState(particle);
  const auto momentum = exahype2::fv::particles::utils::computeMomentum(particle);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(state.velocity(axis), velocity(axis));
    validateNumericalEquals(state.acceleration(axis), acceleration(axis));
    validateNumericalEquals(momentum(axis), 3.0 * velocity(axis));
  }

  exahype2::fv::particles::utils::resetAcceleration(particle);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleAccelerationAttributeOffset + axis],
      0.0
    );
  }

  exahype2::fv::particles::utils::setParticleState(particle, state);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(
      particle.data[exahype2::fv::particles::utils::ParticleAccelerationAttributeOffset + axis],
      acceleration(axis)
    );
  }
}
