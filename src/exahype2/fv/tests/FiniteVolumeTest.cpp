// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FiniteVolumeTest.h"

#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "exahype2/Enumeration.h"

namespace {
  // Exposes every term the kernel can evaluate through a small PDE. The exact
  // form does not matter: the kernel only calls the stateless functors, and the
  // invariant tests keep the extra terms off.
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
      // The eigenvalue covers the joint advection of the flux and the
      // non-conservative product, so the Rusanov diffusion stays sufficient
      // whichever terms a test enables.
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

  template <int VolumesPerAxis>
  constexpr int numberOfVolumesInPatch() {
    int product = 1;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      product *= VolumesPerAxis;
    }
    return product;
  }

  // A face's halo overlap holds 2 * HaloWidth layers along the normal and
  // VolumesPerAxis layers along every other axis.
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

  // The patch spans the unit cube, so the centre of a volume follows from its
  // index alone. Indices outside the patch give the centres of halo volumes.
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

  // Profiles that only depend on the first coordinate solve the PDE the
  // TestSolver induces exactly: the advection along the other directions
  // leaves them invariant, so this travelling coordinate is an analytic
  // oracle. The speed is AdvectionSpeed for the flux alone and grows by
  // NcpAdvectionSpeed when the non-conservative product is active.
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

  // Enumerates every overlap voxel of every face and hands the functor the
  // index of the volume the voxel mirrors, the face number, and the serialised
  // position inside that face's overlap storage.
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

  // Fills every overlap voxel of every face from the exact profile at the given
  // time, which acts as an exact boundary condition around the patch.
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

  // Fills every overlap voxel by wrapping its volume index around the patch, so
  // the patch behaves as a torus and every face flux has a matching partner.
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

  // Advances the profile with the requested reconstruction over the given
  // number of time steps, feeding exact halo data before every step, and
  // returns the patch state at the final time.
  template <typename Reconstruction, int VolumesPerAxis, bool EvaluateNonconservativeProduct, typename Profile>
  std::vector<double> advectProfile(const Profile profile, const int numberOfTimeSteps, const double timeStepSize) {
    using Solver                   = TestSolver<NumberOfUnknowns>;
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
    std::vector<std::vector<double>> faceInStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[TWO_TIMES_D];
    double*                          faceOutputPatches[TWO_TIMES_D];
    for (int face = 0; face < TWO_TIMES_D; face++) {
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
        exahype2::fv::timestepping::SingleStep,
        Solver,
        VolumesPerAxis,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        true,
        EvaluateNonconservativeProduct,
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
        timeStamp,
        timeStepSize
      );
      q.swap(qNext);
    }
    return q;
  }

  // Returns the L1 error of the advected profile against the exact solution at
  // the final time, sampled at the volume centres.
  template <typename Reconstruction, int VolumesPerAxis, bool EvaluateNonconservativeProduct, typename Profile>
  double advectionError(const Profile profile, const int numberOfTimeSteps, const double endTime) {
    const double              timeStepSize = endTime / numberOfTimeSteps;
    const std::vector<double> q = advectProfile<Reconstruction, VolumesPerAxis, EvaluateNonconservativeProduct>(
      profile,
      numberOfTimeSteps,
      timeStepSize
    );
    double errorSum = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double exact = profile(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), endTime);
      errorSum += std::abs(q[volume * NumberOfData] - exact);
    }
    return errorSum / numberOfVolumesInPatch<VolumesPerAxis>();
  }

  // Returns the deterministic state of one logical volume, defined for halo
  // indices outside the patch as well, so halo layouts of different widths can
  // be filled with the same neighbour values.
  double logicalState(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
    double phase = 0.7 * (unknown + 1);
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      phase += (dimension + 2.0) * volumeIndex(dimension);
    }
    return 1.0 + 0.25 * std::sin(phase);
  }

  // Advances the same logical field with the first-order scheme and with the
  // limited reconstruction under zero slopes, then returns the largest
  // difference over the patch and the reported eigenvalues. A vanishing result
  // means zero slopes degenerate to the first-order splitting, including the
  // non-conservative product.
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
    std::vector<std::vector<double>> constantFaceIn(TWO_TIMES_D, std::vector<double>(constantFaceStorageSize, 0.0));
    std::vector<std::vector<double>> limitedFaceIn(TWO_TIMES_D, std::vector<double>(limitedFaceStorageSize, 0.0));
    std::vector<std::vector<double>> constantFaceOut(TWO_TIMES_D, std::vector<double>(constantFaceStorageSize, 0.0));
    std::vector<std::vector<double>> limitedFaceOut(TWO_TIMES_D, std::vector<double>(limitedFaceStorageSize, 0.0));

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
    const double*       constantFaceInputPatches[TWO_TIMES_D];
    const double*       limitedFaceInputPatches[TWO_TIMES_D];
    double*             constantFaceOutputPatches[TWO_TIMES_D];
    double*             limitedFaceOutputPatches[TWO_TIMES_D];
    for (int face = 0; face < TWO_TIMES_D; face++) {
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
      timeStepSize
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
      timeStepSize
    );

    double maxDifference = std::abs(eigenvalueConstant - eigenvalueLimited);
    for (int entry = 0; entry < patchStorageSize; entry++) {
      maxDifference = std::max(maxDifference, std::abs(qOutConstant[entry] - qOutLimited[entry]));
    }
    return maxDifference;
  }

  // Reproduces the default boundary extrapolation in a solver's reconstruct
  // method, so the reconstruction hook must leave every result unchanged. Any
  // deviation means the kernel wired the hook wrongly.
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

  // Reproduces the Minmod policy in a solver's limitedDifference method, so the
  // limiter hook must leave every result identical to the plain Minmod
  // reconstruction. Any deviation means the kernel wired the hook wrongly.
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

  // Advances the logical field with the given solver over the requested number
  // of time steps, wrapping the halo around the patch, and returns the final
  // patch state.
  template <typename Solver, int VolumesPerAxis>
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
    std::vector<std::vector<double>> faceInStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[TWO_TIMES_D];
    double*                          faceOutputPatches[TWO_TIMES_D];
    for (int face = 0; face < TWO_TIMES_D; face++) {
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
        Solver,
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
        timeStepSize
      );
      q.swap(qNext);
    }
    return q;
  }

  constexpr int SodUnknowns = DIMENSIONS + 2;

  // Evaluates the Euler terms of the Sod shock tube along the first axis. The
  // momenta occupy the slots after the density and the energy sits last, so the
  // layout follows the dimensions of the build.
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

  // Advances the Sod tube with the Minmod limiter. The first-axis halo carries
  // the untouched far field and the other halos copy the nearest interior
  // volume, which is exact while the waves stay inside the patch and the
  // solution does not vary along those axes.
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
      sodInitialState(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), &q[volume * SodUnknowns]);
    }

    std::vector<double>              qNext(patchStorageSize, 0.0);
    std::vector<std::vector<double>> faceInStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    std::vector<std::vector<double>> faceOutStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
    const double*                    faceInputPatches[TWO_TIMES_D];
    double*                          faceOutputPatches[TWO_TIMES_D];
    for (int face = 0; face < TWO_TIMES_D; face++) {
      faceInputPatches[face]  = faceInStorage[face].data();
      faceOutputPatches[face] = faceOutStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      forEachOverlapVoxel<VolumesPerAxis, HaloWidth>(
        [&faceInStorage,
         &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int overlapIndex) {
          double* const target = &faceInStorage[face][overlapIndex * SodUnknowns];
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
        timeStepSize
      );
      q.swap(qNext);
    }
    return q;
  }
} // namespace

exahype2::fv::tests::FiniteVolumeTest::FiniteVolumeTest() :
  tarch::tests::TestCase("exahype2::fv::tests::FiniteVolumeTest") {}

void exahype2::fv::tests::FiniteVolumeTest::run() {
  testMethod(testZeroTimeStepPreservesState);
  testMethod(testUniformStateIsPreserved);
  testMethod(testLimitedLinearPreservesUniformState);
  testMethod(testLimitedLinearAverageConvergesAtSecondOrder);
  testMethod(testConstantConvergesAtFirstOrder);
  testMethod(testLimitedLinearMinmodKeepsStepInBounds);
  testMethod(testLimitedLinearZeroSlopesMatchConstant);
  testMethod(testLimitedLinearNcpAdvectionConvergesAtSecondOrder);
  testMethod(testPeriodicAdvectionConservesMass);
  testMethod(testSodShockTubeReachesStarStates);
  testMethod(testReconstructionHookReproducingDefaultChangesNothing);
  testMethod(testLimiterHookReproducingMinmodChangesNothing);
}

void exahype2::fv::tests::FiniteVolumeTest::testZeroTimeStepPreservesState() {
  const int           patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  std::vector<double> qIn(patchStorageSize);
  std::vector<double> qOut(patchStorageSize, 0.0);
  for (int entry = 0; entry < patchStorageSize; entry++) {
    qIn[entry] = 0.1 + 0.01 * static_cast<double>(entry);
  }

  std::vector<std::vector<double>> faceInStorage(
    TWO_TIMES_D,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.5)
  );
  std::vector<std::vector<double>> faceOutStorage(
    TWO_TIMES_D,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.0)
  );
  const double* faceInputPatches[TWO_TIMES_D];
  double*       faceOutputPatches[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
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
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.0);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], qIn[entry]);
  }
}

void exahype2::fv::tests::FiniteVolumeTest::testUniformStateIsPreserved() {
  const int           patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const double        uniformValue     = 1.23;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(
    TWO_TIMES_D,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), uniformValue)
  );
  std::vector<std::vector<double>> faceOutStorage(
    TWO_TIMES_D,
    std::vector<double>(faceOverlapStorageSize<NumberOfVolumesPerAxis, HaloSize>(), 0.0)
  );
  const double* faceInputPatches[TWO_TIMES_D];
  double*       faceOutputPatches[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
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
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.01);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed
  );
}

void exahype2::fv::tests::FiniteVolumeTest::testLimitedLinearPreservesUniformState() {
  using UniformReconstruction = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::VanAlbada>;
  const int patchStorageSize  = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const int faceStorageSize   = faceOverlapStorageSize<NumberOfVolumesPerAxis, UniformReconstruction::HaloSize>();

  const double        uniformValue = 1.23;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, uniformValue));
  std::vector<std::vector<double>> faceOutStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
  const double*                    faceInputPatches[TWO_TIMES_D];
  double*                          faceOutputPatches[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
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
    double>(qIn.data(), qOut.data(), faceInputPatches, faceOutputPatches, patchCentre, patchExtent, 0.0, 0.01);

  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    TestSolver<NumberOfUnknowns>::AdvectionSpeed + TestSolver<NumberOfUnknowns>::NcpAdvectionSpeed
  );
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
    validateWithParams2(q[volume * NumberOfData] >= -1e-12, volume, q[volume * NumberOfData]);
    validateWithParams2(q[volume * NumberOfData] <= 1.0 + 1e-12, volume, q[volume * NumberOfData]);
  }

  // The total variation along the advection direction never exceeds the single
  // initial jump.
  const int numberOfRows = numberOfVolumesInPatch<VolumesPerAxis>() / VolumesPerAxis;
  for (int row = 0; row < numberOfRows; row++) {
    double totalVariation = 0.0;
    for (int volume = 0; volume + 1 < VolumesPerAxis; volume++) {
      const double left  = q[(row * VolumesPerAxis + volume) * NumberOfData];
      const double right = q[(row * VolumesPerAxis + volume + 1) * NumberOfData];
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

/** Confirms periodic face fluxes conserve the patch's total mass. */
void exahype2::fv::tests::FiniteVolumeTest::testPeriodicAdvectionConservesMass() {
  using MinmodReconstruction   = exahype2::fv::reconstruction::LimitedLinear<exahype2::fv::limiter::Minmod>;
  constexpr int VolumesPerAxis = 27;
  constexpr int HaloWidth      = MinmodReconstruction::HaloSize;

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
  std::vector<std::vector<double>> faceInStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
  std::vector<std::vector<double>> faceOutStorage(TWO_TIMES_D, std::vector<double>(faceStorageSize, 0.0));
  const double*                    faceInputPatches[TWO_TIMES_D];
  double*                          faceOutputPatches[TWO_TIMES_D];
  for (int face = 0; face < TWO_TIMES_D; face++) {
    faceInputPatches[face]  = faceInStorage[face].data();
    faceOutputPatches[face] = faceOutStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
  const double                                timeStepSize = 0.02 / 3.0;

  for (int step = 0; step < 3; step++) {
    fillHaloPeriodically<VolumesPerAxis, HaloWidth>(faceInStorage, q);
    exahype2::fv::applyFiniteVolume<
      MinmodReconstruction,
      exahype2::fv::timestepping::SingleStep,
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
      timeStepSize
    );
    q.swap(qNext);
  }

  double massAfter = 0.0;
  for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    massAfter += q[volume * NumberOfData];
  }

  validateWithParams2(std::abs(massAfter - massBefore) <= 1e-11 * std::abs(massBefore), massBefore, massAfter);
}

/** Confirms the shock-capturing path reaches the Sod tube's known star-region states. */
void exahype2::fv::tests::FiniteVolumeTest::testSodShockTubeReachesStarStates() {
  // The tube varies along the first axis alone, so a cubic patch repeats the
  // same profile once per transverse volume. Three dimensions therefore use
  // fewer volumes per axis: enough to resolve the star plateaus the samples
  // read, and wide enough that the shared step stays stable.
  constexpr int    VolumesPerAxis = DIMENSIONS == 2 ? 54 : 36;
  constexpr int    Steps          = 116;
  constexpr double TimeStepSize   = 0.00175;

  const std::vector<double> q = advanceSodTube<VolumesPerAxis>(Steps, TimeStepSize);

  for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    const double density  = q[volume * SodUnknowns];
    const double pressure = SodSolver::pressureOf(&q[volume * SodUnknowns]);
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

  // A sample is a coordinate, so it names the plateau it reads and follows
  // whichever resolution is in force. The first axis runs fastest, so its
  // volumes lead the patch.
  const auto volumeAt = [](const double position) { return static_cast<int>(position * VolumesPerAxis); };
  const auto probe    = [&q](const int volumeAlongFirstAxis, const int unknown) {
    return q[volumeAlongFirstAxis * SodUnknowns + unknown];
  };
  // The left plateau lies between the rarefaction tail and the contact. Both
  // of its ends smear, so the samples sit in the middle.
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
    const double pressure = SodSolver::pressureOf(&q[volumeAt(position) * SodUnknowns]);
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
