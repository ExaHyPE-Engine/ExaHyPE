// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERWENOTest.h"

#include "exahype2/dg/tests/KernelTestData.h"
#include "exahype2/fv/reconstruction/WENO.h"
#include "exahype2/fv/timestepping/ADER.h"
#include "tarch/accelerator/ParallelFor.h"

#include <numbers>
#include <vector>

namespace {
  constexpr int NumberOfVolumesPerAxis     = 4;
  constexpr int NumberOfUnknowns           = 3;
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  TARGET_DECLARE
  template <bool NCP, bool Source>
  struct TestTerms {
    static constexpr double AdvectionSpeed    = 0.7;
    static constexpr double NcpAdvectionSpeed = 0.3;
    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const P* q,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&,
      double,
      double,
      int,
      P* f
    ) {
      for (int i = 0; i < NumberOfUnknowns; i++) {
        f[i] = P(AdvectionSpeed) * q[i];
      }
    }
    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const P*,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&,
      double,
      double,
      int
    ) {
      return 1.0;
    }
    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      const P*,
      const P* dq,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&,
      double,
      double,
      int,
      P* b
    )
      requires NCP
    {
      for (int i = 0; i < NumberOfUnknowns; i++) {
        b[i] = P(NcpAdvectionSpeed) * dq[i];
      }
    }
    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const P* q,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&,
      double,
      double,
      P* s
    )
      requires Source
    {
      for (int i = 0; i < NumberOfUnknowns; i++) {
        s[i] = P(0.05) * q[i];
      }
    }
  };
  template <typename R, int Volumes, bool NCP, bool Source, int Auxiliary = 0>
  struct TestSolver : TestTerms<NCP, Source> {
    using Reconstruction                             = R;
    using TimeIntegration                            = exahype2::fv::timestepping::ADER;
    static constexpr int  PatchSize                  = Volumes;
    static constexpr int  NumberOfUnknowns           = 3;
    static constexpr int  NumberOfAuxiliaryVariables = Auxiliary;
    static constexpr auto TimeSteppingRule           = exahype2::TimeStepping::Rule::Adaptive;
  };
  TARGET_DECLARE_END
  TARGET_DECLARE
  template <int Order, typename Storage, typename Compute, int Auxiliary = 0>
  struct UpdateVolume {
    using Solver = TestSolver<
      exahype2::fv::reconstruction::WENO<Order, exahype2::fv::weno::JiangShu<8>>,
      2 * Order,
      true,
      true,
      Auxiliary>;
    using Kernel = exahype2::fv::FiniteVolumeScheme<Solver, Compute>;
    const Storage* patches;
    const Storage* faces;
    Storage*       updated;
    double*        eigenvalues;

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const {
      const int      cell   = entry / Kernel::VolumesPerPatch;
      const int      volume = entry % Kernel::VolumesPerPatch;
      const Storage* facePointers[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        facePointers[face] = faces + (cell * peano4::FacesPerCell + face) * Kernel::ValuesPerFace;
      }
      eigenvalues[entry] = Kernel::computeVolumeUpdate(
        volume,
        patches + cell * Kernel::ValuesPerPatch,
        facePointers,
        tarch::la::Vector<DIMENSIONS, double>(0.5),
        tarch::la::Vector<DIMENSIONS, double>(1.0),
        0.25,
        0.001,
        0,
        updated + entry * Kernel::NumberOfData
      );
    }
  };
  TARGET_DECLARE_END

  template <int Order, typename Storage, typename Compute, int Auxiliary = 0>
  double deviceUpdateError() {
    using Work                 = UpdateVolume<Order, Storage, Compute, Auxiliary>;
    using Kernel               = typename Work::Kernel;
    constexpr int        Cells = 2;
    std::vector<Storage> patches(Cells * Kernel::ValuesPerPatch);
    std::vector<Storage> faces(Cells * peano4::FacesPerCell * Kernel::ValuesPerFace);
    for (std::size_t i = 0; i < patches.size(); i++) {
      patches[i] = static_cast<Storage>(1.5 + 0.2 * std::sin(0.1 * i));
    }
    for (std::size_t i = 0; i < faces.size(); i++) {
      faces[i] = static_cast<Storage>(1.5 + 0.2 * std::cos(0.05 * i));
    }
    std::vector<Storage>                             host(patches.size()), device(patches.size());
    std::vector<double>                              eigenvalues(Cells * Kernel::VolumesPerPatch);
    const exahype2::dg::tests::DeviceBuffer<Storage> devicePatches(patches), deviceFaces(faces), deviceUpdated(device);
    const exahype2::dg::tests::DeviceBuffer<double>  deviceEigenvalues(eigenvalues);
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Cells * Kernel::VolumesPerPatch),
      Work{devicePatches.getData(), deviceFaces.getData(), deviceUpdated.getData(), deviceEigenvalues.getData()}
    );
    deviceUpdated.copyBack(device);
    deviceEigenvalues.copyBack(eigenvalues);
    double error = 0.0;
    for (int cell = 0; cell < Cells; cell++) {
      const Storage* facePointers[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        facePointers[face] = faces.data() + (cell * peano4::FacesPerCell + face) * Kernel::ValuesPerFace;
      }
      const double eigenvalue = Kernel::update(
        patches.data() + cell * Kernel::ValuesPerPatch,
        host.data() + cell * Kernel::ValuesPerPatch,
        facePointers,
        tarch::la::Vector<DIMENSIONS, double>(0.5),
        tarch::la::Vector<DIMENSIONS, double>(1.0),
        0.25,
        0.001,
        0
      );
      for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
        if (not std::isfinite(eigenvalues[cell * Kernel::VolumesPerPatch + volume]) or not std::isfinite(eigenvalue)) {
          return std::numeric_limits<double>::infinity();
        }
        error = std::max(error, std::abs(eigenvalues[cell * Kernel::VolumesPerPatch + volume] - eigenvalue));
      }
    }
    for (std::size_t i = 0; i < host.size(); i++) {
      if (i % Kernel::NumberOfData >= Kernel::NumberOfUnknowns and (host[i] != patches[i] or device[i] != patches[i])) {
        return std::numeric_limits<double>::infinity();
      }
      if (not std::isfinite(host[i]) or not std::isfinite(device[i])) {
        return std::numeric_limits<double>::infinity();
      }
      error = std::max(error, std::abs(static_cast<double>(host[i]) - static_cast<double>(device[i])));
    }
    return error;
  }
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
          * travellingCoordinate(position, time, TestTerms<false, false>::AdvectionSpeed)
      );
  }

  double ncpAdvectedProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    const double jointSpeed = TestTerms<false, false>::AdvectionSpeed + TestTerms<false, false>::NcpAdvectionSpeed;
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

  template <typename Reconstruction, int VolumesPerAxis, bool EvaluateNonconservativeProduct, typename Profile>
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
    const double*                    faceInputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face] = faceInStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      const double timeStamp = step * timeStepSize;
      fillHaloFromProfile<VolumesPerAxis, HaloWidth>(faceInStorage, profile, timeStamp);
      using Solver = TestSolver<Reconstruction, VolumesPerAxis, EvaluateNonconservativeProduct, false>;
      static_cast<void>(exahype2::fv::FiniteVolumeScheme<Solver, double>::update(
        q.data(),
        qNext.data(),
        faceInputPatches,
        patchCentre,
        patchExtent,
        timeStamp,
        timeStepSize,
        0
      ));
      q.swap(qNext);
    }
    return q;
  }

  // Returns the L1 error of the advected profile against the exact solution at
  // the final time, sampled at the volume centres.
  template <typename Reconstruction, int VolumesPerAxis, bool EvaluateNonconservativeProduct, typename Profile>
  double advectionError(const Profile profile, const int numberOfTimeSteps, const double endTime) {
    const double              timeStepSize = endTime / numberOfTimeSteps;
    const std::vector<double> q =
      advectProfile<Reconstruction, VolumesPerAxis, EvaluateNonconservativeProduct, Profile>(
        profile,
        numberOfTimeSteps,
        timeStepSize
      );
    double errorSum = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double exact = profile(volumeCentreOf<VolumesPerAxis>(volumeIndexOf<VolumesPerAxis>(volume)), endTime);
      errorSum += std::abs(q[static_cast<std::size_t>(volume) * NumberOfData] - exact);
    }
    return errorSum / numberOfVolumesInPatch<VolumesPerAxis>();
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

  // Advances the smooth profile over a torus for a few steps and returns the
  // total mass of the first unknown before and after.
  template <typename Reconstruction, int VolumesPerAxis>
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
    const double*                    faceInputPatches[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceInputPatches[face] = faceInStorage[face].data();
    }

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
    const double                                timeStepSize = 0.02 / 3.0;

    for (int step = 0; step < 3; step++) {
      fillHaloPeriodically<VolumesPerAxis, HaloWidth>(faceInStorage, q);
      using Solver = TestSolver<Reconstruction, VolumesPerAxis, false, false>;
      static_cast<void>(exahype2::fv::FiniteVolumeScheme<Solver, double>::update(
        q.data(),
        qNext.data(),
        faceInputPatches,
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        0
      ));
      q.swap(qNext);
    }

    double massAfter = 0.0;
    for (int volume = 0; volume < numberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      massAfter += q[volume * NumberOfData];
    }
    return {massBefore, massAfter};
  }
} // namespace
exahype2::fv::tests::ADERWENOTest::ADERWENOTest() :
  tarch::tests::TestCase("exahype2::fv::tests::ADERWENOTest") {}
void
exahype2::fv::tests::ADERWENOTest::run() {
  testMethod(testWENOReconstructsPolynomialsExactly);
  testMethod(testADERWENOPreservesUniformState);
  testMethod(testADERWENOAdvectionConvergesAtHighOrder);
  testMethod(testADERWENONcpAdvectionConvergesAtHighOrder);
  testMethod(testADERWENOPeriodicAdvectionConservesMass);
  testMethod(testDeviceUpdatesMatchHost);
}
void
exahype2::fv::tests::ADERWENOTest::testWENOReconstructsPolynomialsExactly() {
  const double secondOrderError = wenoPolynomialReconstructionError<2, 4>();
  const double thirdOrderError  = wenoPolynomialReconstructionError<3, 6>();

  validateWithParams1(secondOrderError < 1e-10, secondOrderError);
  validateWithParams1(thirdOrderError < 1e-10, thirdOrderError);
}

void
exahype2::fv::tests::ADERWENOTest::testDeviceUpdatesMatchHost() {
  const double doubleError    = deviceUpdateError<2, double, double>();
  const double mixedError     = deviceUpdateError<2, float, double>();
  const double floatError     = deviceUpdateError<2, float, float>();
  const double cubicError     = deviceUpdateError<3, double, double>();
  const double auxiliaryError = deviceUpdateError<2, double, double, 1>();
  validateWithParams1(doubleError < 1e-10, doubleError);
  validateWithParams1(mixedError < 1e-6, mixedError);
  validateWithParams1(floatError < 1e-5, floatError);
  validateWithParams1(cubicError < 1e-10, cubicError);
  validateWithParams1(auxiliaryError < 1e-10, auxiliaryError);
}

/**
 * Only the source term acts on a uniform state, and the space-time predictor integrates it to the
 * exponential growth rather than the forward Euler step.
 */
void
exahype2::fv::tests::ADERWENOTest::testADERWENOPreservesUniformState() {
  using Reconstruction       = ADERWENO<2>;
  const int patchStorageSize = numberOfVolumesInPatch<NumberOfVolumesPerAxis>() * NumberOfData;
  const int faceStorageSize  = faceOverlapStorageSize<NumberOfVolumesPerAxis, Reconstruction::HaloSize>();

  const double        uniformValue = 1.23;
  const double        timeStepSize = 0.01;
  std::vector<double> qIn(patchStorageSize, uniformValue);
  std::vector<double> qOut(patchStorageSize, 0.0);

  std::vector<std::vector<double>> faceInStorage(
    peano4::FacesPerCell,
    std::vector<double>(faceStorageSize, uniformValue)
  );
  const double* faceInputPatches[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceInputPatches[face] = faceInStorage[face].data();
  }

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

  using Solver                    = TestSolver<Reconstruction, NumberOfVolumesPerAxis, true, true>;
  const double returnedEigenvalue = exahype2::fv::FiniteVolumeScheme<Solver, double>::update(
    qIn.data(),
    qOut.data(),
    faceInputPatches,
    patchCentre,
    patchExtent,
    0.0,
    timeStepSize,
    0
  );

  const double grownValue = uniformValue * std::exp(0.05 * timeStepSize);
  for (int entry = 0; entry < patchStorageSize; entry++) {
    validateNumericalEqualsWithParams1(qOut[entry], grownValue, entry);
  }
  validateNumericalEquals(
    returnedEigenvalue,
    (TestTerms<true, true>::AdvectionSpeed + TestTerms<true, true>::NcpAdvectionSpeed)
  );
}

/** The predictor lifts the order in time to the order of the reconstruction. */
void
exahype2::fv::tests::ADERWENOTest::testADERWENOAdvectionConvergesAtHighOrder() {
  constexpr double EndTime = 0.12;

  const double secondOrderCoarseError = advectionError<ADERWENO<2>, 8, false>(smoothProfile, 6, EndTime);
  const double secondOrderFineError   = advectionError<ADERWENO<2>, 16, false>(smoothProfile, 12, EndTime);
  const double secondOrder            = std::log(secondOrderCoarseError / secondOrderFineError) / std::log(2.0);
  validateWithParams3(secondOrder > 2.7, secondOrderCoarseError, secondOrderFineError, secondOrder);

  const double thirdOrderCoarseError = advectionError<ADERWENO<3>, 8, false>(smoothProfile, 6, EndTime);
  const double thirdOrderFineError   = advectionError<ADERWENO<3>, 16, false>(smoothProfile, 12, EndTime);
  const double thirdOrder            = std::log(thirdOrderCoarseError / thirdOrderFineError) / std::log(2.0);
  validateWithParams3(thirdOrder > 3.7, thirdOrderCoarseError, thirdOrderFineError, thirdOrder);
}

/** The fluctuations across the faces and the predictor's volume part advect at high order. */
void
exahype2::fv::tests::ADERWENOTest::testADERWENONcpAdvectionConvergesAtHighOrder() {
  constexpr double EndTime = 0.12;

  const double coarseError = advectionError<ADERWENO<2>, 8, true>(ncpAdvectedProfile, 8, EndTime);
  const double fineError   = advectionError<ADERWENO<2>, 16, true>(ncpAdvectedProfile, 16, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(2.0);

  validateWithParams3(order > 2.7, coarseError, fineError, order);
}

/** Confirms periodic face fluxes conserve the patch's total mass under ADER-WENO. */
void
exahype2::fv::tests::ADERWENOTest::testADERWENOPeriodicAdvectionConservesMass() {
  const auto [massBefore, massAfter] = periodicAdvectionMass<ADERWENO<2>, 16>();

  validateWithParams2(std::abs(massAfter - massBefore) <= 1e-11 * std::abs(massBefore), massBefore, massAfter);
}
