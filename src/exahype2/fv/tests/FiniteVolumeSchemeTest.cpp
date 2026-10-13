// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FiniteVolumeSchemeTest.h"

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/tests/HaloProfileTestData.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

namespace {
  using Rule  = exahype2::TimeStepping::Rule;
  using Setup = exahype2::fv::tests::HaloProfileSetup;

  using Constant = exahype2::fv::reconstruction::Constant;
  template <typename Limiter>
  using LimitedLinear = exahype2::fv::reconstruction::LimitedLinear<Limiter>;

  TARGET_DECLARE
  constexpr int NoBoundaryFaces = 0;

  constexpr int    NumberOfVolumesPerAxis     = 4;
  constexpr int    NumberOfUnknowns           = 3;
  constexpr int    NumberOfAuxiliaryVariables = 0;
  constexpr int    NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr double AdvectionSpeed             = 0.7;
  constexpr double NcpAdvectionSpeed          = 0.3;
  constexpr double SourceRate                 = 0.05;
  constexpr double Diffusivity                = 0.01;
  constexpr double PostprocessedValue         = 5.0;
  constexpr double ExactSolutionValue         = 2.0;

  template <int PatchSizeOfLayout, int Unknowns, typename Reconstruct>
  struct Layout {
    static constexpr int PatchSize                  = PatchSizeOfLayout;
    static constexpr int NumberOfUnknowns           = Unknowns;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using Reconstruction  = Reconstruct;
    using TimeIntegration = exahype2::fv::timestepping::SingleStep;
  };

  template <int PatchSizeOfLayout, int Unknowns, typename Reconstruct, Rule RuleOfScheme>
  struct SchemeLayout : Layout<PatchSizeOfLayout, Unknowns, Reconstruct> {
    static constexpr Rule TimeSteppingRule = RuleOfScheme;
  };

  /**
   * Advection with AdvectionSpeed along every axis. maxEigenvalue() also bounds the non-conservative advection of
   * NcpTerm.
   */
  template <int Unknowns>
  struct AdvectionTerms {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        fluxes[unknown] = AdvectionSpeed * state[unknown];
      }
    }

    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      return AdvectionSpeed + NcpAdvectionSpeed;
    }
  };

  template <int Unknowns>
  struct NcpTerm {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        averageState,
      const ComputePrecision* NOALIAS const                         stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      ComputePrecision* NOALIAS const                               result
    ) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        result[unknown] = static_cast<ComputePrecision>(NcpAdvectionSpeed) * stateJump[unknown];
      }
    }
  };

  template <int Unknowns>
  struct SourceTerm {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               sources
    ) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        sources[unknown] = static_cast<ComputePrecision>(SourceRate) * state[unknown];
      }
    }
  };

  template <int Unknowns>
  struct DiffusiveTerm {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      const ComputePrecision* NOALIAS const                         gradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      ComputePrecision* NOALIAS const                               result
    ) {
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        auto directionalSum = static_cast<ComputePrecision>(0);
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          directionalSum += gradient[direction * Unknowns + unknown];
        }
        result[unknown] = static_cast<ComputePrecision>(Diffusivity) * directionalSum;
      }
    }
  };

  /**
   * Riemann solver whose fluxes include the non-conservative advection.
   */
  template <int Unknowns>
  struct RiemannNcpTerm {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
      const ComputePrecision* NOALIAS const                         leftState,
      const ComputePrecision* NOALIAS const                         rightState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const bool                                   isBoundaryFace,
      [[maybe_unused]] const int                                    faceIndex,
      ComputePrecision* NOALIAS const                               leftGoingFlux,
      ComputePrecision* NOALIAS const                               rightGoingFlux
    ) {
      constexpr double WaveSpeed = AdvectionSpeed + NcpAdvectionSpeed;
      for (int unknown = 0; unknown < Unknowns; unknown++) {
        const auto stateJump = rightState[unknown] - leftState[unknown];
        const auto centralFlux =
          static_cast<ComputePrecision>(0.5 * AdvectionSpeed) * (leftState[unknown] + rightState[unknown]);
        const auto diffusion              = static_cast<ComputePrecision>(0.5 * WaveSpeed) * stateJump;
        const auto nonconservativeProduct = static_cast<ComputePrecision>(0.5 * NcpAdvectionSpeed) * stateJump;
        leftGoingFlux[unknown]            = centralFlux - diffusion + nonconservativeProduct;
        rightGoingFlux[unknown]           = centralFlux - diffusion - nonconservativeProduct;
      }
      return WaveSpeed;
    }
  };

  struct ReconstructHook {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void reconstruct(
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

  struct MinmodHook {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD ComputePrecision limitedDifference(
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

  /**
   * Discrete homogeneous Neumann condition.
   */
  template <int Unknowns>
  struct CopyBoundaryTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int entry = 0; entry < Unknowns; entry++) {
        outsideState[entry] = insideState[entry];
      }
    }
  };

  /**
   * Writes the inner value plus one into the outer half. The offset marks every written volume.
   */
  template <int Unknowns>
  struct IncrementBoundaryTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    normal
    ) {
      for (int entry = 0; entry < Unknowns; entry++) {
        outsideState[entry] = insideState[entry] + 1.0;
      }
    }
  };

  template <int VolumesPerAxis, typename Reconstruct>
  struct AdvectionSolver final : SchemeLayout<VolumesPerAxis, NumberOfUnknowns, Reconstruct, Rule::Adaptive>,
                                 AdvectionTerms<NumberOfUnknowns> {};

  template <int VolumesPerAxis, typename Reconstruct>
  struct NcpAdvectionSolver final : SchemeLayout<VolumesPerAxis, NumberOfUnknowns, Reconstruct, Rule::Adaptive>,
                                    AdvectionTerms<NumberOfUnknowns>,
                                    NcpTerm<NumberOfUnknowns> {};

  template <typename Reconstruct>
  struct RiemannNcpAdvectionSolver final
    : SchemeLayout<NumberOfVolumesPerAxis, NumberOfUnknowns, Reconstruct, Rule::Adaptive>,
      AdvectionTerms<NumberOfUnknowns>,
      NcpTerm<NumberOfUnknowns>,
      RiemannNcpTerm<NumberOfUnknowns> {};

  struct SourceAdvectionSolver final : SchemeLayout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant, Rule::Adaptive>,
                                       AdvectionTerms<NumberOfUnknowns>,
                                       SourceTerm<NumberOfUnknowns> {};

  struct DiffusiveAdvectionSolver final
    : SchemeLayout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant, Rule::Adaptive>,
      AdvectionTerms<NumberOfUnknowns>,
      DiffusiveTerm<NumberOfUnknowns> {};

  struct FixedRuleAdvectionSolver final : SchemeLayout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant, Rule::Fixed>,
                                          AdvectionTerms<NumberOfUnknowns> {};

  struct ReconstructHookSolver final
    : SchemeLayout<9, NumberOfUnknowns, LimitedLinear<exahype2::fv::limiter::Minmod>, Rule::Adaptive>,
      AdvectionTerms<NumberOfUnknowns>,
      ReconstructHook {};

  struct MinmodHookSolver final
    : SchemeLayout<9, NumberOfUnknowns, LimitedLinear<exahype2::fv::limiter::Minmod>, Rule::Adaptive>,
      AdvectionTerms<NumberOfUnknowns>,
      MinmodHook {};

  struct CopyBoundarySolver final : Layout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant>,
                                    CopyBoundaryTerm<NumberOfUnknowns> {};

  struct CopyBoundaryMusclSolver final
    : Layout<NumberOfVolumesPerAxis, NumberOfUnknowns, LimitedLinear<exahype2::fv::limiter::Zero>>,
      CopyBoundaryTerm<NumberOfUnknowns> {};

  struct IncrementBoundarySolver final : Layout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant>,
                                         IncrementBoundaryTerm<NumberOfUnknowns> {};

  using HaloProfileSolver = Layout<Setup::DofsPerAxis, Setup::NumberOfData, LimitedLinear<exahype2::fv::limiter::Zero>>;
  static_assert(HaloProfileSolver::Reconstruction::HaloSize == Setup::HaloSize);

  using ProjectionSolver = Layout<4, 2, LimitedLinear<exahype2::fv::limiter::Zero>>;

  struct TimeDependentAdvectionSolver final : SchemeLayout<
                                                NumberOfVolumesPerAxis,
                                                NumberOfUnknowns,
                                                LimitedLinear<exahype2::fv::limiter::Average>,
                                                Rule::Adaptive
                                              > {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
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
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
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

  constexpr int AcousticUnknowns = 1 + DIMENSIONS;

  /**
   * Acoustics. A swap of two axes maps the flux onto itself.
   */
  struct AcousticSolver final : SchemeLayout<NumberOfVolumesPerAxis, AcousticUnknowns, Constant, Rule::Adaptive> {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      for (int unknown = 0; unknown < AcousticUnknowns; unknown++) {
        fluxes[unknown] = static_cast<ComputePrecision>(0);
      }
      fluxes[0]          = state[1 + normal];
      fluxes[1 + normal] = state[0];
    }

    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      return 1.0;
    }
  };

  constexpr int    SodUnknowns = DIMENSIONS + 2;
  constexpr double SodGamma    = 1.4;

  /**
   * The Euler equations with the density first, the momenta next and the energy last.
   */
  template <int VolumesPerAxis, typename Reconstruct = LimitedLinear<exahype2::fv::limiter::Minmod>>
  struct SodSolver final : SchemeLayout<VolumesPerAxis, SodUnknowns, Reconstruct, Rule::Adaptive> {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double getPressureOf(const ComputePrecision* NOALIAS const state) {
      double kineticEnergy = 0.0;
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        kineticEnergy += state[1 + dimension] * state[1 + dimension];
      }
      kineticEnergy *= 0.5 / state[0];
      return (SodGamma - 1.0) * (state[DIMENSIONS + 1] - kineticEnergy);
    }

    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      const ComputePrecision normalVelocity = state[1 + normal] / state[0];
      const ComputePrecision pressure       = getPressureOf(state);
      fluxes[0]                             = state[1 + normal];
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        fluxes[1 + dimension] = state[1 + dimension] * normalVelocity;
      }
      fluxes[1 + normal] += pressure;
      fluxes[DIMENSIONS + 1] = (state[DIMENSIONS + 1] + pressure) * normalVelocity;
    }

    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal
    ) {
      const double soundSpeed = std::sqrt(SodGamma * getPressureOf(state) / state[0]);
      return std::abs(state[1 + normal] / state[0]) + soundSpeed;
    }
  };

  struct PostprocessingSolver final : SchemeLayout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant, Rule::Adaptive> {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const ComputePrecision* NOALIAS const                         state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal
    ) {
      return static_cast<double>(state[0]) + normal;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
      double* NOALIAS const                                         Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt
    ) {
      for (int volume = 0; volume < peano4::power(PatchSize, DIMENSIONS); volume++) {
        Q[static_cast<std::ptrdiff_t>(volume) * NumberOfUnknowns] = PostprocessedValue;
      }
    }
  };

  struct InitialConditionSolver final : Layout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant> {
    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = x(0);
      Q[1] = h(0);
      Q[2] = x(DIMENSIONS - 1);
    }
  };

  struct ErrorSolver final : Layout<NumberOfVolumesPerAxis, NumberOfUnknowns, Constant> {
    static constexpr std::array<int, 1> MeasuredUnknowns{0};

    static INLINE_METHOD GPU_CALLABLE_METHOD void analyticalSolution(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const double* const                          Q,
      double* const                                                 solution
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        solution[unknown] = ExactSolutionValue;
      }
    }
  };

  INLINE_METHOD GPU_CALLABLE_METHOD double encodeRiemannFace(
    const int  normal,
    const bool isBoundaryFace,
    const int  faceIndex
  ) {
    return normal + (isBoundaryFace ? 10.0 : 0.0) + 100.0 * faceIndex;
  }

  struct FaceRecordingSolver {
    static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
      [[maybe_unused]] const double* const                          QL,
      [[maybe_unused]] const double* const                          QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      const int                                                     normal,
      const bool                                                    isBoundaryFace,
      const int                                                     faceIndex,
      [[maybe_unused]] double* const                                FL,
      [[maybe_unused]] double* const                                FR
    ) {
      return encodeRiemannFace(normal, isBoundaryFace, faceIndex);
    }
  };

  using DeviceSolver = NcpAdvectionSolver<NumberOfVolumesPerAxis, LimitedLinear<exahype2::fv::limiter::Minmod>>;
  using DeviceKernel = exahype2::fv::FiniteVolumeScheme<DeviceSolver, double>;

  /**
   * Advances the patch of one cell of a device range with one thread per volume and reduces the
   * wave speeds of the update and of the updated volumes into the wave speed of the cell.
   */
  struct UpdatePatchOfCell final {
    const double* patches{nullptr};
    double*       updated{nullptr};
    const double* faces{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    double*       eigenvalues{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      const double* facesOfCell[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        facesOfCell[faceNumber] =
          faces + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * DeviceKernel::ValuesPerFace;
      }
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      const double* const patch         = patches + static_cast<std::ptrdiff_t>(cell) * DeviceKernel::ValuesPerPatch;
      double* const       updatedPatch  = updated + static_cast<std::ptrdiff_t>(cell) * DeviceKernel::ValuesPerPatch;
      const int           boundaryFaces = cell % 2 == 0 ? NoBoundaryFaces : 1;
      constexpr double    TimeStamp     = exahype2::dg::tests::KernelTestSetup::TimeStamp;
      constexpr double    TimeStepSize  = exahype2::dg::tests::KernelTestSetup::TimeStepSize;
      tarch::accelerator::parallelReduce(
        team,
        tarch::accelerator::Range(DeviceKernel::VolumesPerPatch),
        [=](const int volume, double& partial) {
          partial = exahype2::MaxEigenvalueReduction::combine(
            partial,
            DeviceKernel::computeVolumeUpdate(
              volume,
              patch,
              facesOfCell,
              cellCentre,
              cellSize,
              TimeStamp,
              TimeStepSize,
              boundaryFaces,
              updatedPatch + static_cast<std::ptrdiff_t>(volume) * DeviceKernel::NumberOfData
            )
          );
          partial = exahype2::MaxEigenvalueReduction::combine(
            partial,
            DeviceKernel::getMaxEigenvalueOfVolume(
              volume,
              updatedPatch,
              cellCentre,
              cellSize,
              TimeStamp + TimeStepSize,
              TimeStepSize
            )
          );
        },
        tarch::accelerator::Custom<exahype2::MaxEigenvalueReduction>{eigenvalues[cell]}
      );
    }
  };
  TARGET_DECLARE_END

  template <int VolumesPerAxis>
  constexpr int getNumberOfVolumesInPatch() {
    return peano4::power(VolumesPerAxis, DIMENSIONS);
  }

  template <typename Precision>
  std::array<const Precision*, peano4::FacesPerCell> getFacePointersOf(
    const std::vector<std::vector<Precision>>& faceStorage
  );

  /**
   * Fills q with a smooth Euler state on the unit patch of Solver and every face with a state
   * that differs from face to face and from entry to entry.
   */
  template <typename Solver>
  void fillSmoothEulerPatchAndFaces(std::vector<double>& q, std::vector<std::vector<double>>& faceStorage) {
    using Kernel = exahype2::fv::FiniteVolumeScheme<Solver, double>;
    q.assign(Kernel::ValuesPerPatch, 0.0);
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      double* const state = q.data() + static_cast<std::ptrdiff_t>(volume) * SodUnknowns;
      state[0]            = 1.0 + 0.25 * std::sin(1.0 + volume);
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        state[1 + dimension] = 0.125 * std::cos(1.0 + (2.0 + dimension) * volume);
      }
      state[DIMENSIONS + 1] = 2.5 + 0.5 * std::cos(0.5 * volume);
    }
    faceStorage.assign(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      for (int entry = 0; entry < Kernel::ValuesPerFace; entry++) {
        const int unknown = entry % SodUnknowns;
        faceStorage[faceNumber][entry] =
          (unknown == 0 or unknown == DIMENSIONS + 1) ? 1.5 + 0.01 * faceNumber + 0.001 * entry : 0.02 * faceNumber;
      }
    }
  }

  /**
   * Advances a smooth Euler state on the unit patch of Solver once with the sweep and once volume
   * by volume without the ring, writes both wave speeds and returns the number of values that
   * differ between the two results.
   */
  template <typename Solver>
  int getNumberOfSweepMismatches(double& sweepEigenvalue, double& perVolumeEigenvalue) {
    using Kernel                  = exahype2::fv::FiniteVolumeScheme<Solver, double>;
    constexpr double TimeStamp    = 0.25;
    constexpr double TimeStepSize = 0.01;

    std::vector<double>              q;
    std::vector<std::vector<double>> faceStorage;
    fillSmoothEulerPatchAndFaces<Solver>(q, faceStorage);
    const auto                                  faces = getFacePointersOf(faceStorage);
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    std::vector<double> sweep(Kernel::ValuesPerPatch, 0.0);
    sweepEigenvalue = Kernel::update(
      q.data(),
      sweep.data(),
      faces.data(),
      patchCentre,
      patchExtent,
      TimeStamp,
      TimeStepSize,
      NoBoundaryFaces
    );

    std::vector<double> perVolume(Kernel::ValuesPerPatch, 0.0);
    perVolumeEigenvalue = 0.0;
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      perVolumeEigenvalue = exahype2::MaxEigenvalueReduction::combine(
        perVolumeEigenvalue,
        Kernel::computeVolumeUpdate(
          volume,
          q.data(),
          faces.data(),
          patchCentre,
          patchExtent,
          TimeStamp,
          TimeStepSize,
          NoBoundaryFaces,
          perVolume.data() + static_cast<std::ptrdiff_t>(volume) * Kernel::NumberOfData
        )
      );
    }
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      perVolumeEigenvalue = exahype2::MaxEigenvalueReduction::combine(
        perVolumeEigenvalue,
        Kernel::getMaxEigenvalueOfVolume(
          volume,
          perVolume.data(),
          patchCentre,
          patchExtent,
          TimeStamp + TimeStepSize,
          TimeStepSize
        )
      );
    }

    int mismatches = 0;
    for (std::size_t entry = 0; entry < sweep.size(); entry++) {
      if (sweep[entry] != perVolume[entry]) {
        mismatches++;
      }
    }
    return mismatches;
  }

  /**
   * Advances a smooth Euler state on the unit patch of Solver once into a second patch and once
   * in place, writes both wave speeds and returns the number of values that differ between the
   * two results.
   */
  template <typename Solver>
  int getNumberOfInPlaceMismatches(double& separateEigenvalue, double& inPlaceEigenvalue) {
    using Kernel                  = exahype2::fv::FiniteVolumeScheme<Solver, double>;
    constexpr double TimeStamp    = 0.25;
    constexpr double TimeStepSize = 0.01;

    std::vector<double>              q;
    std::vector<std::vector<double>> faceStorage;
    fillSmoothEulerPatchAndFaces<Solver>(q, faceStorage);
    const auto                                  faces = getFacePointersOf(faceStorage);
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    std::vector<double> separate(Kernel::ValuesPerPatch, 0.0);
    separateEigenvalue = Kernel::update(
      q.data(),
      separate.data(),
      faces.data(),
      patchCentre,
      patchExtent,
      TimeStamp,
      TimeStepSize,
      NoBoundaryFaces
    );
    inPlaceEigenvalue = Kernel::update(
      q.data(),
      q.data(),
      faces.data(),
      patchCentre,
      patchExtent,
      TimeStamp,
      TimeStepSize,
      NoBoundaryFaces
    );

    int mismatches = 0;
    for (std::size_t entry = 0; entry < separate.size(); entry++) {
      if (separate[entry] != q[entry]) {
        mismatches++;
      }
    }
    return mismatches;
  }

  template <int VolumesPerAxis>
  tarch::la::Vector<DIMENSIONS, int> getVolumeIndexOf(int linearVolume) {
    tarch::la::Vector<DIMENSIONS, int> volumeIndex;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      volumeIndex(dimension) = linearVolume % VolumesPerAxis;
      linearVolume /= VolumesPerAxis;
    }
    return volumeIndex;
  }

  // The patch spans the unit cube. A halo volume can lie outside it.
  template <int VolumesPerAxis>
  tarch::la::Vector<DIMENSIONS, double> getVolumeCentreOf(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex) {
    tarch::la::Vector<DIMENSIONS, double> centre;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      centre(dimension) = (volumeIndex(dimension) + 0.5) / VolumesPerAxis;
    }
    return centre;
  }

  template <int VolumesPerAxis>
  int getLinearVolumeIndexOf(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex) {
    int linear = 0;
    for (int dimension = DIMENSIONS - 1; dimension >= 0; dimension--) {
      linear = linear * VolumesPerAxis + volumeIndex(dimension);
    }
    return linear;
  }

  // The profiles vary along the first coordinate alone.
  double getTravellingCoordinate(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 time,
    const double                                 speed
  ) {
    return position(0) - speed * time;
  }

  double getSmoothProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5
      + 0.25 * std::sin(2.0 * std::numbers::pi_v<double> * getTravellingCoordinate(position, time, AdvectionSpeed));
  }

  double getStepProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return getTravellingCoordinate(position, time, AdvectionSpeed) < 0.4 ? 1.0 : 0.0;
  }

  double getNcpAdvectedProfile(const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    const double jointSpeed = AdvectionSpeed + NcpAdvectionSpeed;
    return 1.5
      + 0.25 * std::sin(2.0 * std::numbers::pi_v<double> * getTravellingCoordinate(position, time, jointSpeed));
  }

  /**
   * Calls functor(volumeIndex, face, faceVolume) for every halo volume of the patch, with volumeIndex in patch
   * coordinates.
   */
  template <int VolumesPerAxis, int HaloWidth, typename Functor>
  void forEachHaloVolume(Functor functor) {
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      tarch::la::Vector<DIMENSIONS, int> range(VolumesPerAxis);
      range(normal) = 2 * HaloWidth;
      peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
        for (int side = 0; side < 2; side++) {
          auto volumeIndex = haloVolume;
          volumeIndex(normal) =
            side == 0 ? haloVolume(normal) - HaloWidth : VolumesPerAxis - HaloWidth + haloVolume(normal);
          const int face       = side == 0 ? normal : normal + DIMENSIONS;
          const int faceVolume = exahype2::fv::getFaceVolumeIndex(haloVolume, VolumesPerAxis, HaloWidth, normal);
          functor(volumeIndex, face, faceVolume);
        }
      });
    }
  }

  template <int VolumesPerAxis, int HaloWidth, typename Profile>
  void fillHaloFromProfile(std::vector<std::vector<double>>& faceStorage, const Profile profile, const double time) {
    forEachHaloVolume<VolumesPerAxis, HaloWidth>(
      [&faceStorage,
       profile,
       time](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int faceVolume) {
        const double value = profile(getVolumeCentreOf<VolumesPerAxis>(volumeIndex), time);
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceStorage[face][faceVolume * NumberOfData + unknown] = value;
        }
      }
    );
  }

  template <int VolumesPerAxis, int HaloWidth>
  void fillHaloPeriodically(std::vector<std::vector<double>>& faceStorage, const std::vector<double>& q) {
    forEachHaloVolume<VolumesPerAxis, HaloWidth>(
      [&faceStorage, &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int faceVolume) {
        auto wrappedIndex = volumeIndex;
        for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
          wrappedIndex(dimension) = (wrappedIndex(dimension) + VolumesPerAxis) % VolumesPerAxis;
        }
        const int linear = getLinearVolumeIndexOf<VolumesPerAxis>(wrappedIndex);
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceStorage[face][faceVolume * NumberOfData + unknown] = q[linear * NumberOfData + unknown];
        }
      }
    );
  }

  template <typename Precision>
  std::array<const Precision*, peano4::FacesPerCell> getFacePointersOf(
    const std::vector<std::vector<Precision>>& faceStorage
  ) {
    std::array<const Precision*, peano4::FacesPerCell> faces{};
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faces[static_cast<std::size_t>(face)] = faceStorage[static_cast<std::size_t>(face)].data();
    }
    return faces;
  }

  /**
   * Advances a profile on the unit patch of Solver and refills the halo from the exact profile before every step.
   */
  template <typename Solver, typename Profile>
  std::vector<double> advectProfile(const Profile profile, const int numberOfTimeSteps, const double timeStepSize) {
    using Kernel                 = exahype2::fv::FiniteVolumeScheme<Solver, double>;
    constexpr int VolumesPerAxis = Solver::PatchSize;
    constexpr int HaloWidth      = Kernel::HaloSize;

    std::vector<double> q(Kernel::ValuesPerPatch);
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      const double value = profile(getVolumeCentreOf<VolumesPerAxis>(getVolumeIndexOf<VolumesPerAxis>(volume)), 0.0);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = value;
      }
    }

    std::vector<double>              qNext(Kernel::ValuesPerPatch, 0.0);
    std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
    const auto                       faces = getFacePointersOf(faceStorage);

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      const double timeStamp = step * timeStepSize;
      fillHaloFromProfile<VolumesPerAxis, HaloWidth>(faceStorage, profile, timeStamp);
      static_cast<void>(Kernel::update(
        q.data(),
        qNext.data(),
        faces.data(),
        patchCentre,
        patchExtent,
        timeStamp,
        timeStepSize,
        NoBoundaryFaces
      ));
      q.swap(qNext);
    }
    return q;
  }

  /**
   * Returns the L1 error of the advected profile at endTime, sampled at the volume centres.
   */
  template <typename Solver, typename Profile>
  double getAdvectionError(const Profile profile, const int numberOfTimeSteps, const double endTime) {
    constexpr int             VolumesPerAxis = Solver::PatchSize;
    const double              timeStepSize   = endTime / numberOfTimeSteps;
    const std::vector<double> q              = advectProfile<Solver>(profile, numberOfTimeSteps, timeStepSize);
    double                    errorSum       = 0.0;
    for (int volume = 0; volume < getNumberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
      const double exact =
        profile(getVolumeCentreOf<VolumesPerAxis>(getVolumeIndexOf<VolumesPerAxis>(volume)), endTime);
      errorSum += std::abs(q[static_cast<std::size_t>(volume) * NumberOfData] - exact);
    }
    return errorSum / getNumberOfVolumesInPatch<VolumesPerAxis>();
  }

  /**
   * Returns a state that depends on the volume index only. Patches with different halo sizes read
   * the same field.
   */
  double getLogicalState(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int unknown) {
    double phase = 0.7 * (unknown + 1);
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      phase += (dimension + 2.0) * volumeIndex(dimension);
    }
    return 1.0 + 0.25 * std::sin(phase);
  }

  template <typename Kernel>
  std::vector<std::vector<double>> getLogicalHalo() {
    std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
    forEachHaloVolume<Kernel::PatchSize, Kernel::HaloSize>(
      [&faceStorage](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int faceVolume) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          faceStorage[face][faceVolume * NumberOfData + unknown] = getLogicalState(volumeIndex, unknown);
        }
      }
    );
    return faceStorage;
  }

  template <typename ConstantSolver, typename ZeroSlopeSolver>
  double getZeroSlopeVersusConstantMaxDifference() {
    using ConstantKernel  = exahype2::fv::FiniteVolumeScheme<ConstantSolver, double>;
    using ZeroSlopeKernel = exahype2::fv::FiniteVolumeScheme<ZeroSlopeSolver, double>;
    static_assert(ConstantKernel::ValuesPerPatch == ZeroSlopeKernel::ValuesPerPatch);

    std::vector<double> qIn(ConstantKernel::ValuesPerPatch);
    for (int volume = 0; volume < ConstantKernel::VolumesPerPatch; volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        qIn[volume * NumberOfData + unknown] =
          getLogicalState(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume), unknown);
      }
    }
    const auto constantHalo  = getLogicalHalo<ConstantKernel>();
    const auto zeroSlopeHalo = getLogicalHalo<ZeroSlopeKernel>();

    std::vector<double> qOutConstant(ConstantKernel::ValuesPerPatch, 0.0);
    std::vector<double> qOutZeroSlope(ConstantKernel::ValuesPerPatch, 0.0);

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
    const double                                timeStepSize = 0.01;

    const double eigenvalueConstant = ConstantKernel::update(
      qIn.data(),
      qOutConstant.data(),
      getFacePointersOf(constantHalo).data(),
      patchCentre,
      patchExtent,
      0.0,
      timeStepSize,
      NoBoundaryFaces
    );
    const double eigenvalueZeroSlope = ZeroSlopeKernel::update(
      qIn.data(),
      qOutZeroSlope.data(),
      getFacePointersOf(zeroSlopeHalo).data(),
      patchCentre,
      patchExtent,
      0.0,
      timeStepSize,
      NoBoundaryFaces
    );

    double maxDifference = std::abs(eigenvalueConstant - eigenvalueZeroSlope);
    for (int entry = 0; entry < ConstantKernel::ValuesPerPatch; entry++) {
      maxDifference = std::max(maxDifference, std::abs(qOutConstant[entry] - qOutZeroSlope[entry]));
    }
    return maxDifference;
  }

  template <typename Solver>
  std::vector<double> advanceLogicalField(const int numberOfTimeSteps, const double timeStepSize) {
    using Kernel                 = exahype2::fv::FiniteVolumeScheme<Solver, double>;
    constexpr int VolumesPerAxis = Solver::PatchSize;

    std::vector<double> q(Kernel::ValuesPerPatch);
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        q[volume * NumberOfData + unknown] = getLogicalState(getVolumeIndexOf<VolumesPerAxis>(volume), unknown);
      }
    }

    std::vector<double>              qNext(Kernel::ValuesPerPatch, 0.0);
    std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
    const auto                       faces = getFacePointersOf(faceStorage);

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      fillHaloPeriodically<VolumesPerAxis, Kernel::HaloSize>(faceStorage, q);
      static_cast<void>(Kernel::update(
        q.data(),
        qNext.data(),
        faces.data(),
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        NoBoundaryFaces
      ));
      q.swap(qNext);
    }
    return q;
  }

  void sodInitialState(const tarch::la::Vector<DIMENSIONS, double>& position, double* NOALIAS const state) {
    const bool left = position(0) < 0.5;

    state[0] = left ? 1.0 : 0.125;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      state[1 + dimension] = 0.0;
    }
    state[DIMENSIONS + 1] = (left ? 1.0 : 0.1) / (SodGamma - 1.0);
  }

  /**
   * Advances the Sod tube with the initial far field in the halo along the first axis and copies of
   * the nearest volume in the other halos. Both are exact while the waves stay inside the patch.
   */
  template <int VolumesPerAxis>
  std::vector<double> advanceSodTube(const int numberOfTimeSteps, const double timeStepSize) {
    using Solver = SodSolver<VolumesPerAxis>;
    using Kernel = exahype2::fv::FiniteVolumeScheme<Solver, double>;

    std::vector<double> q(Kernel::ValuesPerPatch);
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      sodInitialState(
        getVolumeCentreOf<VolumesPerAxis>(getVolumeIndexOf<VolumesPerAxis>(volume)),
        &q[static_cast<std::size_t>(volume) * SodUnknowns]
      );
    }

    std::vector<double>              qNext(Kernel::ValuesPerPatch, 0.0);
    std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
    const auto                       faces = getFacePointersOf(faceStorage);

    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    for (int step = 0; step < numberOfTimeSteps; step++) {
      forEachHaloVolume<VolumesPerAxis, Kernel::HaloSize>(
        [&faceStorage,
         &q](const tarch::la::Vector<DIMENSIONS, int>& volumeIndex, const int face, const int faceVolume) {
          double* const target = &faceStorage[face][static_cast<std::size_t>(faceVolume) * SodUnknowns];
          const int     normal = face % DIMENSIONS;
          if (normal == 0) {
            sodInitialState(getVolumeCentreOf<VolumesPerAxis>(volumeIndex), target);
          } else {
            auto clampedIndex          = volumeIndex;
            clampedIndex(normal)       = std::clamp(clampedIndex(normal), 0, VolumesPerAxis - 1);
            const double* const source = &q[getLinearVolumeIndexOf<VolumesPerAxis>(clampedIndex) * SodUnknowns];
            for (int unknown = 0; unknown < SodUnknowns; unknown++) {
              target[unknown] = source[unknown];
            }
          }
        }
      );
      static_cast<void>(Kernel::update(
        q.data(),
        qNext.data(),
        faces.data(),
        patchCentre,
        patchExtent,
        step * timeStepSize,
        timeStepSize,
        NoBoundaryFaces
      ));
      q.swap(qNext);
    }
    return q;
  }

  template <typename Solver>
  double getEncodedRiemannFace(
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    const int                                 normal,
    const int                                 side,
    const int                                 boundaryFaces
  ) {
    const double state[NumberOfData]{};
    double       leftFlux[NumberOfUnknowns]{};
    double       rightFlux[NumberOfUnknowns]{};
    return exahype2::fv::solveRiemannProblemOnVolumeFace<Solver, NumberOfVolumesPerAxis>(
      state,
      state,
      tarch::la::Vector<DIMENSIONS, double>(0.5),
      tarch::la::Vector<DIMENSIONS, double>(1.0 / NumberOfVolumesPerAxis),
      0.0,
      0.0,
      normal,
      volumeIndex,
      side,
      boundaryFaces,
      leftFlux,
      rightFlux
    );
  }

  template <typename Kernel>
  double updateUniformPatch(const double uniformValue, const double timeStepSize, std::vector<double>& qOut) {
    const std::vector<double>              qIn(Kernel::ValuesPerPatch, uniformValue);
    const std::vector<std::vector<double>> faceStorage(
      peano4::FacesPerCell,
      std::vector<double>(Kernel::ValuesPerFace, uniformValue)
    );
    qOut.assign(Kernel::ValuesPerPatch, 0.0);
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
    return Kernel::update(
      qIn.data(),
      qOut.data(),
      getFacePointersOf(faceStorage).data(),
      patchCentre,
      patchExtent,
      0.0,
      timeStepSize,
      NoBoundaryFaces
    );
  }

  std::vector<double> getCoarseFaceWithProfile(const int normal) {
    std::vector<double> coarseFace(Setup::ValuesPerFace, 0.0);
    for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
      for (int j = 0; j < Setup::SecondExtent; j++) {
        for (int i = 0; i < Setup::DofsPerAxis; i++) {
          const int volume = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
          for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
            coarseFace[volume * Setup::NumberOfData + unknown] =
              exahype2::fv::tests::getLinearHaloProfile(layer, i, j, unknown);
          }
        }
      }
    }
    return coarseFace;
  }

  std::vector<double> getConstantFace() {
    std::vector<double> face(Setup::ValuesPerFace, 0.0);
    for (int volume = 0; volume < Setup::VolumesPerFace; volume++) {
      for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
        face[volume * Setup::NumberOfData + unknown] = 1.0 / 3.0 + 0.1 * unknown;
      }
    }
    return face;
  }

  /**
   * Returns a patch with a distinct value in every entry.
   */
  std::vector<double> getRampPatch() {
    std::vector<double> patch(Setup::ValuesPerCell);
    for (int value = 0; value < Setup::ValuesPerCell; value++) {
      patch[static_cast<std::size_t>(value)] = 0.25 * value - 7.0;
    }
    return patch;
  }

  std::vector<double> updateOnDevice(
    const std::vector<double>&            patches,
    const std::vector<double>&            faces,
    const exahype2::dg::tests::CellChain& geometry,
    std::vector<double>&                  updated
  ) {
    const exahype2::dg::tests::DeviceBuffer<double> devicePatches(patches);
    const exahype2::dg::tests::DeviceBuffer<double> deviceUpdated(updated);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaces(faces);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    std::vector<double> eigenvalues(exahype2::dg::tests::KernelTestSetup::NumberOfCells, 0.0);
    const exahype2::dg::tests::DeviceBuffer<double> deviceEigenvalues(eigenvalues);

    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(exahype2::dg::tests::KernelTestSetup::NumberOfCells),
      UpdatePatchOfCell{
        devicePatches.getData(),
        deviceUpdated.getData(),
        deviceFaces.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceEigenvalues.getData()
      }
    );

    deviceUpdated.copyBack(updated);
    deviceEigenvalues.copyBack(eigenvalues);
    return eigenvalues;
  }
} // namespace

exahype2::fv::tests::FiniteVolumeSchemeTest::FiniteVolumeSchemeTest() :
  tarch::tests::TestCase("exahype2::fv::tests::FiniteVolumeSchemeTest") {}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::run() {
  testMethod(testTheFaceVolumeIndexRunsLayerSlowest);
  testMethod(testBothFaceVolumeIndexFormsAgreeAndVisitEveryVolumeOnce);
  testMethod(testForEachVolumeVisitsEveryVolumeInLinearisedOrder);
  testMethod(testAccumulateMaxEigenvalueKeepsTheLargestAndAFault);
  testMethod(testInitialiseWritesTheInitialConditionAtEveryVolumeCentre);
  testMethod(testBoundaryConditionsCopyAUniformStateOntoTheOuterHalf);
  testMethod(testEachOuterLayerReadsTheMatchingInnerLayer);
  testMethod(testEachOuterVolumeIsWrittenOnce);
  testMethod(testProjectionFillsTheOwnHalfOfEveryFace);
  testMethod(testHaloInterpolationReproducesALinearProfile);
  testMethod(testAConstantFaceInterpolatesBitwise);
  testMethod(testHaloInjectionCopiesTheCoveringVolume);
  testMethod(testHaloRestrictionAveragesTheCoveringVolumes);
  testMethod(testConstantFineFacesRestrictBitwise);
  testMethod(testPatchInterpolationCopiesTheCoveringVolume);
  testMethod(testPatchRestrictionInvertsTheInterpolation);
  testMethod(testPatchRestrictionAveragesExactly);
  testMethod(testAVolumeFaceOnTheDomainBoundaryReachesTheRiemannSolver);
  testMethod(testZeroTimeStepPreservesState);
  testMethod(testUniformStateIsPreserved);
  testMethod(testAFixedRuleReportsNoWaveSpeed);
  testMethod(testASourceTermGrowsAUniformStateByItsRate);
  testMethod(testADiffusiveSourceTermReadsTheForwardDifference);
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
  testMethod(testMirroredVolumesReceiveMirroredStates);
  testMethod(testReconstructionHookReproducingDefaultChangesNothing);
  testMethod(testLimiterHookReproducingMinmodChangesNothing);
  testMethod(testThePostprocessingRunsBeforeTheWaveSpeedsOfTheUpdatedPatch);
  testMethod(testReduceMaxEigenvalueTakesTheLargestWaveSpeedOverTheVolumes);
  testMethod(testMeasureErrorsSumsTheWeightedNorms);
  testMethod(testEveryCellOfADeviceRangeUpdatesLikeTheHost);
  testMethod(testTheTermRingKeepsAWindowOfEntries);
  testMethod(testTheSweepUpdatesLikeTheVolumeUpdates);
  testMethod(testTheSweepUpdatesInPlaceLikeIntoASecondPatch);
}

/**
 * The ring maps a state and the state one ring size later onto the same entry and the states in
 * between onto other entries.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testTheTermRingKeepsAWindowOfEntries() {
  exahype2::fv::TermRing<int, 2> ring;
  validateEquals(ring.RingSize, 8);
  for (int state = 0; state < ring.RingSize; state++) {
    ring.at(state) = 10 + state;
  }
  for (int state = 0; state < ring.RingSize; state++) {
    validateEqualsWithParams1(ring.at(state), 10 + state, state);
  }
  ring.at(ring.RingSize) = 20;
  validateEquals(std::as_const(ring).at(0), 20);
  validateEquals(std::as_const(ring).at(1), 11);
  validateEquals(std::as_const(ring).at(2 * ring.RingSize + 3), 13);
}

/**
 * The sweep over a patch with the term ring writes the same bits and reports the same wave speed
 * as the update of every volume without the ring, for the constant and the limited linear
 * reconstruction.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testTheSweepUpdatesLikeTheVolumeUpdates() {
  double sweepEigenvalue     = 0.0;
  double perVolumeEigenvalue = 0.0;

  validateEquals(
    (getNumberOfSweepMismatches<SodSolver<NumberOfVolumesPerAxis, Constant>>(sweepEigenvalue, perVolumeEigenvalue)),
    0
  );
  validateWithParams1(sweepEigenvalue > 0.0, sweepEigenvalue);
  validateEquals(sweepEigenvalue, perVolumeEigenvalue);

  validateEquals(
    (getNumberOfSweepMismatches<SodSolver<NumberOfVolumesPerAxis>>(sweepEigenvalue, perVolumeEigenvalue)),
    0
  );
  validateWithParams1(sweepEigenvalue > 0.0, sweepEigenvalue);
  validateEquals(sweepEigenvalue, perVolumeEigenvalue);
}

/**
 * The sweep that overwrites its patch writes the same bits and reports the same wave speed as
 * the sweep into a second patch, for the constant and the limited linear reconstruction.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testTheSweepUpdatesInPlaceLikeIntoASecondPatch() {
  double separateEigenvalue = 0.0;
  double inPlaceEigenvalue  = 0.0;

  validateEquals(
    (getNumberOfInPlaceMismatches<SodSolver<NumberOfVolumesPerAxis, Constant>>(separateEigenvalue, inPlaceEigenvalue)),
    0
  );
  validateWithParams1(separateEigenvalue > 0.0, separateEigenvalue);
  validateEquals(separateEigenvalue, inPlaceEigenvalue);

  validateEquals(
    (getNumberOfInPlaceMismatches<SodSolver<NumberOfVolumesPerAxis>>(separateEigenvalue, inPlaceEigenvalue)),
    0
  );
  validateWithParams1(separateEigenvalue > 0.0, separateEigenvalue);
  validateEquals(separateEigenvalue, inPlaceEigenvalue);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testTheFaceVolumeIndexRunsLayerSlowest() {
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    validateEquals((exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 0, 0)), 0);
    validateEquals((exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 2, 0)), 2);
    validateEquals(
      (exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 1, 0, 0)),
      Setup::VolumesPerLayer
    );
    validateEquals(
      (exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(
        normal,
        2 * Setup::HaloSize - 1,
        Setup::DofsPerAxis - 1,
        Setup::SecondExtent - 1
      )),
      Setup::VolumesPerFace - 1
    );
    if constexpr (DIMENSIONS == 3) {
      validateEquals(
        (exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 1, 2)),
        1 + 2 * Setup::DofsPerAxis
      );
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testBothFaceVolumeIndexFormsAgreeAndVisitEveryVolumeOnce() {
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    tarch::la::Vector<DIMENSIONS, int> range(Setup::DofsPerAxis);
    range(normal) = 2 * Setup::HaloSize;
    std::set<int> visited;
    peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
      const int index  = exahype2::fv::getFaceVolumeIndex(haloVolume, Setup::DofsPerAxis, Setup::HaloSize, normal);
      const int first  = haloVolume(normal == 0 ? 1 : 0);
      const int second = DIMENSIONS == 3 ? haloVolume(normal == 2 ? 1 : 2) : 0;
      validateEqualsWithParams2(
        index,
        (exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(
          normal,
          haloVolume(normal),
          first,
          second
        )),
        haloVolume,
        normal
      );
      validateWithParams1(visited.find(index) == visited.end(), haloVolume);
      visited.insert(index);
    });
    validateEqualsWithParams1(static_cast<int>(visited.size()), Setup::VolumesPerFace, normal);
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testForEachVolumeVisitsEveryVolumeInLinearisedOrder() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<InitialConditionSolver, double>;
  constexpr double                            CellWidth = 3.0;
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5 * CellWidth);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
  std::vector<double>                         values(Kernel::ValuesPerPatch, 0.0);
  int                                         visits = 0;

  Kernel::forEachVolume(
    cellCentre,
    cellSize,
    values.data(),
    [&](
      double* const                                value,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double                                 weight
    ) {
      validateEquals(value - values.data(), static_cast<std::ptrdiff_t>(visits) * Kernel::NumberOfData);
      const auto volume = peano4::dDelinearised(visits, Kernel::PatchSize);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams1(x(axis), (volume(axis) + 0.5) * CellWidth / Kernel::PatchSize, axis);
        validateNumericalEqualsWithParams1(h(axis), CellWidth / Kernel::PatchSize, axis);
      }
      validateNumericalEquals(weight, std::pow(CellWidth / Kernel::PatchSize, DIMENSIONS));
      value[0] = visits;
      visits++;
    }
  );
  validateEquals(visits, Kernel::VolumesPerPatch);
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    validateNumericalEqualsWithParams1(values[static_cast<std::size_t>(volume) * Kernel::NumberOfData], volume, volume);
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testAccumulateMaxEigenvalueKeepsTheLargestAndAFault() {
  double maximum = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(maximum, 3.0);
  exahype2::fv::accumulateMaxEigenvalue(maximum, 2.0);
  exahype2::fv::accumulateMaxEigenvalue(maximum, 0.0);
  validateNumericalEquals(maximum, 3.0);

  double mixedPrecisionMaximum = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(mixedPrecisionMaximum, 2.5F);
  validateNumericalEquals(mixedPrecisionMaximum, 2.5);

#if PEANO_ASSERTIONS == 0
  double negativeMaximum = 3.0;
  exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, -2.0);
  exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, 5.0);
  validateEquals(negativeMaximum, -2.0);

  constexpr auto PositiveInfinity = std::numeric_limits<double>::infinity();
  double         infiniteMaximum  = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, PositiveInfinity);
  exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, 5.0);
  validateEquals(infiniteMaximum, PositiveInfinity);
#endif
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testInitialiseWritesTheInitialConditionAtEveryVolumeCentre() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<InitialConditionSolver, double>;
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(2.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
  std::vector<double>                         patch(Kernel::ValuesPerPatch, 0.0);

  Kernel::initialise(patch.data(), cellCentre, cellSize);

  peano4::dForEach(Kernel::PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto    centre = exahype2::fv::getVolumeCentre(cellCentre, cellSize, Kernel::PatchSize, volume);
    const double* value =
      patch.data() + static_cast<std::ptrdiff_t>(peano4::dLinearised(volume, Kernel::PatchSize)) * Kernel::NumberOfData;
    validateNumericalEqualsWithParams1(value[0], centre(0), volume);
    validateNumericalEqualsWithParams1(value[1], 1.0 / Kernel::PatchSize, volume);
    validateNumericalEqualsWithParams1(value[2], centre(DIMENSIONS - 1), volume);
  });
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testBoundaryConditionsCopyAUniformStateOntoTheOuterHalf() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<CopyBoundarySolver, double>;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
  const double                                uniformValue = 1.23;

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    std::vector<double> face(Kernel::ValuesPerFace, uniformValue);
    Kernel::applyBoundaryConditions(face.data(), patchCentre, patchSize, 0.0, faceNumber);
    for (const double entry : face) {
      validateNumericalEqualsWithParams1(entry, uniformValue, faceNumber);
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testEachOuterLayerReadsTheMatchingInnerLayer() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<CopyBoundaryMusclSolver, double>;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    std::vector<double> face(Kernel::ValuesPerFace, -1.0);
    const int           normal = faceNumber % DIMENSIONS;

    for (int tangentialVolume = 0; tangentialVolume < Kernel::VolumesPerLayer; tangentialVolume++) {
      auto haloVolume = peano4::dDelinearisedExclusive(tangentialVolume, Kernel::PatchSize, normal);
      for (int layer = 0; layer < Kernel::HaloSize; layer++) {
        haloVolume(normal) = faceNumber < DIMENSIONS ? Kernel::HaloSize + layer : Kernel::HaloSize - 1 - layer;
        const int inside   = exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
        for (int entry = 0; entry < Kernel::NumberOfData; entry++) {
          face[inside * Kernel::NumberOfData + entry] =
            static_cast<double>(100 * tangentialVolume + 10 * layer + entry);
        }
      }
    }

    Kernel::applyBoundaryConditions(face.data(), patchCentre, patchSize, 0.0, faceNumber);

    for (int tangentialVolume = 0; tangentialVolume < Kernel::VolumesPerLayer; tangentialVolume++) {
      auto haloVolume = peano4::dDelinearisedExclusive(tangentialVolume, Kernel::PatchSize, normal);
      for (int layer = 0; layer < Kernel::HaloSize; layer++) {
        haloVolume(normal) = faceNumber < DIMENSIONS ? Kernel::HaloSize - 1 - layer : Kernel::HaloSize + layer;
        const int outside  = exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
        for (int entry = 0; entry < Kernel::NumberOfData; entry++) {
          validateNumericalEqualsWithParams3(
            face[outside * Kernel::NumberOfData + entry],
            static_cast<double>(100 * tangentialVolume + 10 * layer + entry),
            faceNumber,
            tangentialVolume,
            layer
          );
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testEachOuterVolumeIsWrittenOnce() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<IncrementBoundarySolver, double>;
  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
  const int expectedWrittenEntries = Kernel::HaloSize * Kernel::VolumesPerLayer * Kernel::NumberOfData;

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    std::vector<double> face(Kernel::ValuesPerFace, 0.0);
    Kernel::applyBoundaryConditions(face.data(), patchCentre, patchSize, 0.0, faceNumber);

    int    numberOfWrittenEntries = 0;
    double maximumEntry           = 0.0;
    for (const double entry : face) {
      if (entry > 0.5) {
        numberOfWrittenEntries++;
      }
      maximumEntry = std::max(entry, maximumEntry);
    }
    validateEqualsWithParams1(numberOfWrittenEntries, expectedWrittenEntries, faceNumber);
    validateNumericalEqualsWithParams1(maximumEntry, 1.0, faceNumber);
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testProjectionFillsTheOwnHalfOfEveryFace() {
  using Kernel             = exahype2::fv::FiniteVolumeScheme<ProjectionSolver, double>;
  const auto getPatchValue = [](const int volume, const int unknown) { return 1000.0 * volume + unknown + 1; };

  std::vector<double> patch(Kernel::ValuesPerPatch);
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    for (int unknown = 0; unknown < Kernel::NumberOfData; unknown++) {
      patch[volume * Kernel::NumberOfData + unknown] = getPatchValue(volume, unknown);
    }
  }
  std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
  double*                          faces[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faces[face] = faceStorage[static_cast<std::size_t>(face)].data();
  }

  Kernel::projectOntoFaces(patch.data(), faces);

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    tarch::la::Vector<DIMENSIONS, int> range(Kernel::PatchSize);
    range(normal) = 2 * Kernel::HaloSize;
    peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
      const int  faceVolume = exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
      const bool upperHalf  = haloVolume(normal) >= Kernel::HaloSize;
      auto       boundaryVolume = haloVolume;
      boundaryVolume(normal) =
        upperHalf ? haloVolume(normal) - Kernel::HaloSize : haloVolume(normal) + Kernel::PatchSize - Kernel::HaloSize;
      const int boundaryIndex = peano4::dLinearised(boundaryVolume, Kernel::PatchSize);
      const int face          = upperHalf ? normal : normal + DIMENSIONS;
      for (int unknown = 0; unknown < Kernel::NumberOfData; unknown++) {
        validateNumericalEqualsWithParams2(
          faceStorage[static_cast<std::size_t>(face)][faceVolume * Kernel::NumberOfData + unknown],
          getPatchValue(boundaryIndex, unknown),
          haloVolume,
          normal
        );
      }
    });
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testHaloInterpolationReproducesALinearProfile() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    const std::vector<double> coarseFace = getCoarseFaceWithProfile(normal);
    for (int side = 0; side < 2; side++) {
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        std::vector<double> fineFace(Setup::ValuesPerFace, -1.0);
        Kernel::interpolateHalo(coarseFace.data(), fineFace.data(), side, normal, subface);
        const auto expectedFace = exahype2::fv::tests::getFineFaceWithProfile(
          normal,
          subface % Setup::SubdivisionFactor,
          subface / Setup::SubdivisionFactor
        );
        for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
          const bool inHalf = layer / Setup::HaloSize == side;
          for (int volumeInLayer = 0; volumeInLayer < Setup::VolumesPerLayer; volumeInLayer++) {
            const int volume = layer * Setup::VolumesPerLayer + volumeInLayer;
            for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
              const double expected = inHalf ? expectedFace[volume * Setup::NumberOfData + unknown] : -1.0;
              validateNumericalEqualsWithParams4(
                fineFace[volume * Setup::NumberOfData + unknown],
                expected,
                normal,
                side,
                subface,
                volume
              );
            }
          }
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testAConstantFaceInterpolatesBitwise() {
  using Kernel                         = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  const std::vector<double> coarseFace = getConstantFace();
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    for (int side = 0; side < 2; side++) {
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        std::vector<double> fineFace(Setup::ValuesPerFace, -1.0);
        Kernel::interpolateHalo(coarseFace.data(), fineFace.data(), side, normal, subface);
        for (int layer = side * Setup::HaloSize; layer < (side + 1) * Setup::HaloSize; layer++) {
          for (int volumeInLayer = 0; volumeInLayer < Setup::VolumesPerLayer; volumeInLayer++) {
            const int volume = layer * Setup::VolumesPerLayer + volumeInLayer;
            for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
              validateEqualsWithParams4(
                fineFace[volume * Setup::NumberOfData + unknown],
                coarseFace[volume * Setup::NumberOfData + unknown],
                normal,
                side,
                subface,
                volume
              );
            }
          }
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testHaloInjectionCopiesTheCoveringVolume() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    const std::vector<double> coarseFace = getCoarseFaceWithProfile(normal);
    for (int side = 0; side < 2; side++) {
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        std::vector<double> fineFace(Setup::ValuesPerFace, -1.0);
        Kernel::injectHalo(coarseFace.data(), fineFace.data(), side, normal, subface);
        const int firstSubface  = subface % Setup::SubdivisionFactor;
        const int secondSubface = subface / Setup::SubdivisionFactor;
        for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
          const bool inHalf = layer / Setup::HaloSize == side;
          for (int j = 0; j < Setup::SecondExtent; j++) {
            const int coarseJ =
              DIMENSIONS == 3 ? (j + Setup::DofsPerAxis * secondSubface) / Setup::SubdivisionFactor : 0;
            for (int i = 0; i < Setup::DofsPerAxis; i++) {
              const int coarseI = (i + Setup::DofsPerAxis * firstSubface) / Setup::SubdivisionFactor;
              const int fineVolume =
                exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
              const int coarseVolume =
                exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, coarseI, coarseJ);
              for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
                const double expected = inHalf ? coarseFace[coarseVolume * Setup::NumberOfData + unknown] : -1.0;
                validateEqualsWithParams4(
                  fineFace[fineVolume * Setup::NumberOfData + unknown],
                  expected,
                  normal,
                  side,
                  subface,
                  fineVolume
                );
              }
            }
          }
        }
      }
    }
  }
}

/**
 * The average over the covering fine volumes of a linear profile is the profile at the coarse centre.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testHaloRestrictionAveragesTheCoveringVolumes() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    std::vector<std::vector<double>> fineFaces;
    fineFaces.reserve(peano4::SubfacesPerFace);
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      fineFaces.push_back(
        exahype2::fv::tests::getFineFaceWithProfile(
          normal,
          subface % Setup::SubdivisionFactor,
          subface / Setup::SubdivisionFactor
        )
      );
    }
    const double* finePointers[peano4::SubfacesPerFace];
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      finePointers[subface] = fineFaces[static_cast<std::size_t>(subface)].data();
    }

    for (int side = 0; side < 2; side++) {
      std::vector<double> coarseFace(Setup::ValuesPerFace, -1.0);
      Kernel::restrictHalo(finePointers, coarseFace.data(), side, normal);
      for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
        const bool inHalf = layer / Setup::HaloSize == side;
        for (int j = 0; j < Setup::SecondExtent; j++) {
          for (int i = 0; i < Setup::DofsPerAxis; i++) {
            const int volume =
              exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
            for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
              const double expected = inHalf ? exahype2::fv::tests::getLinearHaloProfile(layer, i, j, unknown) : -1.0;
              validateNumericalEqualsWithParams3(
                coarseFace[volume * Setup::NumberOfData + unknown],
                expected,
                normal,
                side,
                layer
              );
            }
          }
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testConstantFineFacesRestrictBitwise() {
  using Kernel                       = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  const std::vector<double> fineFace = getConstantFace();
  const double*             finePointers[peano4::SubfacesPerFace];
  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    finePointers[subface] = fineFace.data();
  }
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    for (int side = 0; side < 2; side++) {
      std::vector<double> coarseFace(Setup::ValuesPerFace, -1.0);
      Kernel::restrictHalo(finePointers, coarseFace.data(), side, normal);
      for (int layer = side * Setup::HaloSize; layer < (side + 1) * Setup::HaloSize; layer++) {
        for (int volumeInLayer = 0; volumeInLayer < Setup::VolumesPerLayer; volumeInLayer++) {
          const int volume = layer * Setup::VolumesPerLayer + volumeInLayer;
          for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
            validateEqualsWithParams3(
              coarseFace[volume * Setup::NumberOfData + unknown],
              fineFace[volume * Setup::NumberOfData + unknown],
              normal,
              side,
              volume
            );
          }
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testPatchInterpolationCopiesTheCoveringVolume() {
  using Kernel                     = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  const std::vector<double> parent = getRampPatch();
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    std::vector<double> fine(Setup::ValuesPerCell, 0.0);
    Kernel::interpolatePatch(parent.data(), fine.data(), child);
    const auto childPosition = peano4::dDelinearised(child, Setup::SubdivisionFactor);
    peano4::dForEach(Setup::DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& fineVolume) {
      tarch::la::Vector<DIMENSIONS, int> coarseVolume;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        coarseVolume(axis) = (fineVolume(axis) + Setup::DofsPerAxis * childPosition(axis)) / Setup::SubdivisionFactor;
      }
      for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
        validateEqualsWithParams2(
          fine[peano4::dLinearised(fineVolume, Setup::DofsPerAxis) * Setup::NumberOfData + unknown],
          parent[peano4::dLinearised(coarseVolume, Setup::DofsPerAxis) * Setup::NumberOfData + unknown],
          child,
          unknown
        );
      }
    });
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testPatchRestrictionInvertsTheInterpolation() {
  using Kernel                     = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  const std::vector<double> parent = getRampPatch();

  std::vector<std::vector<double>> children(Setup::ChildrenPerCell, std::vector<double>(Setup::ValuesPerCell, 0.0));
  const double*                    childPointers[Setup::ChildrenPerCell];
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    Kernel::interpolatePatch(parent.data(), children[static_cast<std::size_t>(child)].data(), child);
    childPointers[child] = children[static_cast<std::size_t>(child)].data();
  }

  std::vector<double> restricted(Setup::ValuesPerCell, 0.0);
  Kernel::restrictPatch(childPointers, restricted.data());
  for (int value = 0; value < Setup::ValuesPerCell; value++) {
    validateEqualsWithParams1(
      restricted[static_cast<std::size_t>(value)],
      parent[static_cast<std::size_t>(value)],
      value
    );
  }
}

/**
 * Two cancelling large values in the first coarse volume leave the exact mean of the remaining ones.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testPatchRestrictionAveragesExactly() {
  using Kernel                           = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
  constexpr double                 Large = 1.0e16;
  std::vector<std::vector<double>> children(Setup::ChildrenPerCell, std::vector<double>(Setup::ValuesPerCell, 1.0));
  const double*                    childPointers[Setup::ChildrenPerCell];
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    childPointers[child] = children[static_cast<std::size_t>(child)].data();
  }
  for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
    children[0][static_cast<std::size_t>(unknown)]                                                 = Large;
    children[0][static_cast<std::size_t>(Setup::NumberOfData) + static_cast<std::size_t>(unknown)] = -Large;
  }

  std::vector<double> restricted(Setup::ValuesPerCell, 0.0);
  Kernel::restrictPatch(childPointers, restricted.data());
  for (int value = 0; value < Setup::ValuesPerCell; value++) {
    const double expected =
      value < Setup::NumberOfData ? static_cast<double>(Setup::ChildrenPerCell - 2) / Setup::ChildrenPerCell : 1.0;
    validateEqualsWithParams1(restricted[static_cast<std::size_t>(value)], expected, value);
  }
}

/**
 * A volume face on a boundary patch face reaches the Riemann solver with the patch face number and every other
 * face with the normal.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testAVolumeFaceOnTheDomainBoundaryReachesTheRiemannSolver() {
  constexpr int                            EveryFace = (1 << peano4::FacesPerCell) - 1;
  const tarch::la::Vector<DIMENSIONS, int> lowerCorner(0);
  const tarch::la::Vector<DIMENSIONS, int> upperCorner(NumberOfVolumesPerAxis - 1);
  const tarch::la::Vector<DIMENSIONS, int> inside(1);

  validateNumericalEquals(
    getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 0, EveryFace),
    encodeRiemannFace(0, true, 0)
  );
  validateNumericalEquals(
    getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 0, NoBoundaryFaces),
    encodeRiemannFace(0, false, 0)
  );
  validateNumericalEquals(
    getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 1, EveryFace),
    encodeRiemannFace(0, false, 0)
  );
  validateNumericalEquals(
    getEncodedRiemannFace<FaceRecordingSolver>(inside, 1, 0, EveryFace),
    encodeRiemannFace(1, false, 1)
  );
  validateNumericalEquals(
    getEncodedRiemannFace<FaceRecordingSolver>(upperCorner, 1, 1, EveryFace),
    encodeRiemannFace(1, true, 1 + DIMENSIONS)
  );
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testZeroTimeStepPreservesState() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<AdvectionSolver<NumberOfVolumesPerAxis, Constant>, double>;
  std::vector<double> qIn(Kernel::ValuesPerPatch);
  std::vector<double> qOut(Kernel::ValuesPerPatch, 0.0);
  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    qIn[entry] = 0.1 + 0.01 * static_cast<double>(entry);
  }
  const std::vector<std::vector<double>> faceStorage(
    peano4::FacesPerCell,
    std::vector<double>(Kernel::ValuesPerFace, 0.5)
  );

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
  static_cast<void>(Kernel::update(
    qIn.data(),
    qOut.data(),
    getFacePointersOf(faceStorage).data(),
    patchCentre,
    patchExtent,
    0.0,
    0.0,
    NoBoundaryFaces
  ));

  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    validateNumericalEquals(qOut[entry], qIn[entry]);
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testUniformStateIsPreserved() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<AdvectionSolver<NumberOfVolumesPerAxis, Constant>, double>;
  const double        uniformValue = 1.23;
  std::vector<double> qOut;
  const double        returnedEigenvalue = updateUniformPatch<Kernel>(uniformValue, 0.01, qOut);

  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(returnedEigenvalue, AdvectionSpeed + NcpAdvectionSpeed);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testAFixedRuleReportsNoWaveSpeed() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<FixedRuleAdvectionSolver, double>;
  static_assert(not Kernel::ReportEigenvalues);
  std::vector<double> qOut;
  validateNumericalEquals(updateUniformPatch<Kernel>(1.23, 0.01, qOut), 0.0);
  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    validateNumericalEquals(qOut[entry], 1.23);
  }
}

/**
 * The flux divergence of a uniform state is zero. The update is the source rate times the step.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testASourceTermGrowsAUniformStateByItsRate() {
  using Kernel                     = exahype2::fv::FiniteVolumeScheme<SourceAdvectionSolver, double>;
  constexpr double    UniformValue = 1.23;
  constexpr double    TimeStepSize = 0.01;
  std::vector<double> qOut;
  static_cast<void>(updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut));
  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    validateNumericalEqualsWithParams1(qOut[entry], UniformValue * (1.0 + TimeStepSize * SourceRate), entry);
  }
}

/**
 * A linear profile loses the advection of its slope and gains the diffusive source of its exact forward difference.
 * That difference reads the halo at the last volume.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testADiffusiveSourceTermReadsTheForwardDifference() {
  using Kernel                  = exahype2::fv::FiniteVolumeScheme<DiffusiveAdvectionSolver, double>;
  constexpr double Slope        = 2.0;
  constexpr double TimeStepSize = 0.01;
  const auto       getProfile =
    [](const tarch::la::Vector<DIMENSIONS, double>& position, [[maybe_unused]] const double time) {
      return 1.0 + Slope * position(0);
    };
  const std::vector<double> solution = advectProfile<DiffusiveAdvectionSolver>(getProfile, 1, TimeStepSize);

  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    const auto   centre   = getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
    const double expected = getProfile(centre, 0.0) + TimeStepSize * (Diffusivity - AdvectionSpeed) * Slope;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(solution[volume * NumberOfData + unknown], expected, volume, unknown);
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearPreservesUniformState() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<
    AdvectionSolver<NumberOfVolumesPerAxis, LimitedLinear<exahype2::fv::limiter::VanAlbada>>,
    double
  >;
  const double        uniformValue = 1.23;
  std::vector<double> qOut;
  const double        returnedEigenvalue = updateUniformPatch<Kernel>(uniformValue, 0.01, qOut);

  for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
    validateNumericalEquals(qOut[entry], uniformValue);
  }
  validateNumericalEquals(returnedEigenvalue, AdvectionSpeed + NcpAdvectionSpeed);
}

/**
 * A linear profile under time-dependent advection moves by the exact displacement.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearIntegratesTimeDependentAdvection() {
  constexpr double TimeStepSize = 0.01;
  const auto       getProfile   = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5 + position(0) - time - 0.5 * time * time;
  };
  const auto solution = advectProfile<TimeDependentAdvectionSolver>(getProfile, 1, TimeStepSize);

  for (int volume = 0; volume < getNumberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
    const auto   centre   = getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
    const double expected = getProfile(centre, TimeStepSize);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(solution[volume * NumberOfData + unknown], expected, volume, unknown);
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearAverageConvergesAtSecondOrder() {
  using Average            = LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double EndTime = 0.12;

  const double coarseError = getAdvectionError<AdvectionSolver<9, Average>>(getSmoothProfile, 6, EndTime);
  const double fineError   = getAdvectionError<AdvectionSolver<27, Average>>(getSmoothProfile, 18, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 1.7, coarseError, fineError, order);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testConstantConvergesAtFirstOrder() {
  constexpr double EndTime = 0.12;

  const double coarseError = getAdvectionError<AdvectionSolver<9, Constant>>(getSmoothProfile, 6, EndTime);
  const double fineError   = getAdvectionError<AdvectionSolver<27, Constant>>(getSmoothProfile, 18, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 0.6 and order < 1.4, coarseError, fineError, order);
}

/**
 * The Minmod limiter keeps a step profile in bounds and never grows its total variation.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearMinmodKeepsStepInBounds() {
  constexpr int VolumesPerAxis = 27;
  using Solver                 = AdvectionSolver<VolumesPerAxis, LimitedLinear<exahype2::fv::limiter::Minmod>>;

  const std::vector<double> q = advectProfile<Solver>(getStepProfile, 8, 0.02 / 3.0);

  for (int volume = 0; volume < getNumberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    const double profileValue = q[static_cast<std::size_t>(volume) * NumberOfData];
    validateWithParams2(profileValue >= -1e-12, volume, profileValue);
    validateWithParams2(profileValue <= 1.0 + 1e-12, volume, profileValue);
  }

  const int numberOfRows = getNumberOfVolumesInPatch<VolumesPerAxis>() / VolumesPerAxis;
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

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearZeroSlopesMatchConstant() {
  using ZeroSlope = LimitedLinear<exahype2::fv::limiter::Zero>;
  validateNumericalEquals(
    (getZeroSlopeVersusConstantMaxDifference<
      AdvectionSolver<NumberOfVolumesPerAxis, Constant>,
      AdvectionSolver<NumberOfVolumesPerAxis, ZeroSlope>
    >()),
    0.0
  );
  validateNumericalEquals(
    (getZeroSlopeVersusConstantMaxDifference<
      NcpAdvectionSolver<NumberOfVolumesPerAxis, Constant>,
      NcpAdvectionSolver<NumberOfVolumesPerAxis, ZeroSlope>
    >()),
    0.0
  );
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearNcpAdvectionConvergesAtSecondOrder() {
  using Average            = LimitedLinear<exahype2::fv::limiter::Average>;
  constexpr double EndTime = 0.12;

  const double coarseError = getAdvectionError<NcpAdvectionSolver<9, Average>>(getNcpAdvectedProfile, 8, EndTime);
  const double fineError   = getAdvectionError<NcpAdvectionSolver<27, Average>>(getNcpAdvectedProfile, 24, EndTime);
  const double order       = std::log(coarseError / fineError) / std::log(3.0);

  validateWithParams3(order > 1.7, coarseError, fineError, order);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimitedLinearCustomRiemannIncludesVolumeNonconservativeProduct() {
  constexpr double TimeStepSize = 0.01;
  const auto       getProfile   = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
    return 1.5 + position(0) - (AdvectionSpeed + NcpAdvectionSpeed) * time;
  };
  const auto solution = advectProfile<RiemannNcpAdvectionSolver<LimitedLinear<exahype2::fv::limiter::Average>>>(
    getProfile,
    1,
    TimeStepSize
  );

  for (int volume = 0; volume < getNumberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
    const auto   centre   = getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
    const double expected = getProfile(centre, TimeStepSize);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(solution[volume * NumberOfData + unknown], expected, volume, unknown);
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testPeriodicAdvectionConservesMass() {
  constexpr int VolumesPerAxis = 27;
  using Kernel                 = exahype2::fv::FiniteVolumeScheme<
    AdvectionSolver<VolumesPerAxis, LimitedLinear<exahype2::fv::limiter::Minmod>>,
    double
  >;

  std::vector<double> q(Kernel::ValuesPerPatch);
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    const double value =
      getSmoothProfile(getVolumeCentreOf<VolumesPerAxis>(getVolumeIndexOf<VolumesPerAxis>(volume)), 0.0);
    for (int unknown = 0; unknown < NumberOfData; unknown++) {
      q[volume * NumberOfData + unknown] = value;
    }
  }

  double massBefore = 0.0;
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    massBefore += q[static_cast<std::size_t>(volume) * NumberOfData];
  }

  std::vector<double>              qNext(Kernel::ValuesPerPatch, 0.0);
  std::vector<std::vector<double>> faceStorage(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));
  const auto                       faces = getFacePointersOf(faceStorage);

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
  const double                                timeStepSize = 0.02 / 3.0;

  for (int step = 0; step < 3; step++) {
    fillHaloPeriodically<VolumesPerAxis, Kernel::HaloSize>(faceStorage, q);
    static_cast<void>(Kernel::update(
      q.data(),
      qNext.data(),
      faces.data(),
      patchCentre,
      patchExtent,
      step * timeStepSize,
      timeStepSize,
      NoBoundaryFaces
    ));
    q.swap(qNext);
  }

  double massAfter = 0.0;
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    massAfter += q[static_cast<std::size_t>(volume) * NumberOfData];
  }

  validateWithParams2(std::abs(massAfter - massBefore) <= 1e-11 * std::abs(massBefore), massBefore, massAfter);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testSodShockTubeReachesStarStates() {
  constexpr int    VolumesPerAxis = DIMENSIONS == 2 ? 54 : 36;
  constexpr int    Steps          = 116;
  constexpr double TimeStepSize   = 0.00175;
  using Solver                    = SodSolver<VolumesPerAxis>;

  const std::vector<double> q = advanceSodTube<VolumesPerAxis>(Steps, TimeStepSize);

  for (int volume = 0; volume < getNumberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
    const double density  = q[static_cast<std::size_t>(volume) * SodUnknowns];
    const double pressure = Solver::getPressureOf(&q[static_cast<std::size_t>(volume) * SodUnknowns]);
    validateWithParams2(density > 0.0, volume, density);
    validateWithParams2(pressure > 0.0, volume, pressure);
    validateWithParams2(density <= 1.0 + 1e-6, volume, density);
    validateWithParams2(density >= 0.125 - 1e-6, volume, density);
  }

  // Exact star-region values of the Sod problem.
  constexpr double StarPressure     = 0.30313;
  constexpr double StarVelocity     = 0.92745;
  constexpr double StarDensityLeft  = 0.42632;
  constexpr double StarDensityRight = 0.26557;

  // The first axis runs fastest. Its volumes lead the patch.
  const auto getVolumeAt = [](const double position) { return static_cast<int>(position * VolumesPerAxis); };
  const auto getValue    = [&q](const int volumeAlongFirstAxis, const int unknown) {
    return q[volumeAlongFirstAxis * SodUnknowns + unknown];
  };
  // The left plateau lies between the rarefaction tail and the contact.
  for (const double position : {0.54, 0.58}) {
    validateWithParams2(
      std::abs(getValue(getVolumeAt(position), 0) - StarDensityLeft) < 0.03,
      position,
      getValue(getVolumeAt(position), 0)
    );
  }
  // The right plateau lies between the contact and the shock.
  for (const double position : {0.76, 0.79}) {
    validateWithParams2(
      std::abs(getValue(getVolumeAt(position), 0) - StarDensityRight) < 0.03,
      position,
      getValue(getVolumeAt(position), 0)
    );
  }
  // Velocity and pressure stay constant across the contact. A sample on either side of it gives
  // the same star value.
  for (const double position : {0.58, 0.76}) {
    const double velocity = getValue(getVolumeAt(position), 1) / getValue(getVolumeAt(position), 0);
    const double pressure = Solver::getPressureOf(&q[static_cast<std::size_t>(getVolumeAt(position)) * SodUnknowns]);
    validateWithParams2(std::abs(velocity - StarVelocity) < 0.05, position, velocity);
    validateWithParams2(std::abs(pressure - StarPressure) < 0.03, position, pressure);
  }
}

/**
 * A state symmetric under a swap of the first two axes stays symmetric to the last bit.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testMirroredVolumesReceiveMirroredStates() {
  using Kernel                           = exahype2::fv::FiniteVolumeScheme<AcousticSolver, double>;
  constexpr int       VolumesPerAxisIn3D = (DIMENSIONS == 3) ? NumberOfVolumesPerAxis : 1;
  std::vector<double> qIn(Kernel::ValuesPerPatch, 0.0);
  std::vector<double> qOut(Kernel::ValuesPerPatch, 0.0);

  const auto getVolumeOffset = [](const int x, const int y, const int z) {
    return ((z * NumberOfVolumesPerAxis + y) * NumberOfVolumesPerAxis + x) * AcousticUnknowns;
  };
  for (int z = 0; z < VolumesPerAxisIn3D; z++) {
    for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
      for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
        qIn[getVolumeOffset(x, y, z) + 0] = 1.0 + 0.25 * std::sin(1.0 + x + y + 3.0 * x * y + z);
        qIn[getVolumeOffset(x, y, z) + 1] = 0.125 * std::cos(1.0 + 2.0 * x + 5.0 * y + z);
        qIn[getVolumeOffset(x, y, z) + 2] = 0.125 * std::cos(1.0 + 2.0 * y + 5.0 * x + z);
        if constexpr (DIMENSIONS == 3) {
          qIn[getVolumeOffset(x, y, z) + 3] = 0.125 * std::sin(1.0 + x + y + z);
        }
      }
    }
  }
  const std::vector<std::vector<double>> faceStorage(
    peano4::FacesPerCell,
    std::vector<double>(Kernel::ValuesPerFace, 0.75)
  );

  const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);
  static_cast<void>(Kernel::update(
    qIn.data(),
    qOut.data(),
    getFacePointersOf(faceStorage).data(),
    patchCentre,
    patchExtent,
    0.0,
    0.01,
    NoBoundaryFaces
  ));

  for (int z = 0; z < VolumesPerAxisIn3D; z++) {
    for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
      for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
        validateEqualsWithParams2(qOut[getVolumeOffset(x, y, z) + 0], qOut[getVolumeOffset(y, x, z) + 0], x, y);
        validateEqualsWithParams2(qOut[getVolumeOffset(x, y, z) + 1], qOut[getVolumeOffset(y, x, z) + 2], x, y);
        validateEqualsWithParams2(qOut[getVolumeOffset(x, y, z) + 2], qOut[getVolumeOffset(y, x, z) + 1], x, y);
        if constexpr (DIMENSIONS == 3) {
          validateEqualsWithParams2(qOut[getVolumeOffset(x, y, z) + 3], qOut[getVolumeOffset(y, x, z) + 3], x, y);
        }
      }
    }
  }
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testReconstructionHookReproducingDefaultChangesNothing() {
  constexpr int    Steps        = 3;
  constexpr double TimeStepSize = 0.01;
  using Default                 = AdvectionSolver<9, LimitedLinear<exahype2::fv::limiter::Minmod>>;

  const std::vector<double> withoutHook = advanceLogicalField<Default>(Steps, TimeStepSize);
  const std::vector<double> withHook    = advanceLogicalField<ReconstructHookSolver>(Steps, TimeStepSize);

  double maxDifference = 0.0;
  for (std::size_t entry = 0; entry < withoutHook.size(); entry++) {
    maxDifference = std::max(maxDifference, std::abs(withoutHook[entry] - withHook[entry]));
  }
  validateNumericalEquals(maxDifference, 0.0);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testLimiterHookReproducingMinmodChangesNothing() {
  constexpr int    Steps        = 3;
  constexpr double TimeStepSize = 0.01;
  using Default                 = AdvectionSolver<9, LimitedLinear<exahype2::fv::limiter::Minmod>>;

  const std::vector<double> fromPolicy = advanceLogicalField<Default>(Steps, TimeStepSize);
  const std::vector<double> fromHook   = advanceLogicalField<MinmodHookSolver>(Steps, TimeStepSize);

  double maxDifference = 0.0;
  for (std::size_t entry = 0; entry < fromPolicy.size(); entry++) {
    maxDifference = std::max(maxDifference, std::abs(fromPolicy[entry] - fromHook[entry]));
  }
  validateNumericalEquals(maxDifference, 0.0);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testThePostprocessingRunsBeforeTheWaveSpeedsOfTheUpdatedPatch() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<PostprocessingSolver, double>;
  std::vector<double> qOut;
  const double        eigenvalue = updateUniformPatch<Kernel>(1.0, 0.01, qOut);
  validateNumericalEquals(eigenvalue, PostprocessedValue + (DIMENSIONS - 1));
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    validateNumericalEqualsWithParams1(
      qOut[static_cast<std::size_t>(volume) * Kernel::NumberOfData],
      PostprocessedValue,
      volume
    );
    validateNumericalEqualsWithParams1(qOut[static_cast<std::size_t>(volume) * Kernel::NumberOfData + 1], 1.0, volume);
  }
}

/**
 * The reduction over the volumes takes the largest first value plus the largest axis.
 */
void
exahype2::fv::tests::FiniteVolumeSchemeTest::testReduceMaxEigenvalueTakesTheLargestWaveSpeedOverTheVolumes() {
  using Kernel = exahype2::fv::FiniteVolumeScheme<PostprocessingSolver, double>;
  std::vector<double> patch(Kernel::ValuesPerPatch, 0.0);
  for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
    patch[static_cast<std::size_t>(volume) * Kernel::NumberOfData] = 0.5 * volume;
  }
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
  validateNumericalEquals(
    Kernel::reduceMaxEigenvalue(patch.data(), cellCentre, cellSize, 0.0),
    0.5 * (Kernel::VolumesPerPatch - 1) + (DIMENSIONS - 1)
  );

  using FixedKernel = exahype2::fv::FiniteVolumeScheme<FixedRuleAdvectionSolver, double>;
  validateNumericalEquals(FixedKernel::reduceMaxEigenvalue(patch.data(), cellCentre, cellSize, 0.0), 0.0);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testMeasureErrorsSumsTheWeightedNorms() {
  using Kernel                                      = exahype2::fv::FiniteVolumeScheme<ErrorSolver, double>;
  constexpr double                            Error = 0.5;
  std::vector<double>                         patch(Kernel::ValuesPerPatch, ExactSolutionValue + Error);
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
  double                                      errors[exahype2::ErrorMeasurement::NumberOfNorms]{-1.0, -1.0, -1.0};

  Kernel::measureErrors(patch.data(), cellCentre, cellSize, 0.0, 0.0, errors);

  validateNumericalEquals(errors[0], Error);
  validateNumericalEquals(errors[1], Error * Error);
  validateNumericalEquals(errors[2], Error);
}

void
exahype2::fv::tests::FiniteVolumeSchemeTest::testEveryCellOfADeviceRangeUpdatesLikeTheHost() {
  constexpr int                        Cells = exahype2::dg::tests::KernelTestSetup::NumberOfCells;
  const exahype2::dg::tests::CellChain geometry;
  const std::vector<double>            patches =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Cells) * DeviceKernel::ValuesPerPatch, 1.0, 0.001);
  const std::vector<double> faces = exahype2::dg::tests::getRamp(
    static_cast<std::size_t>(Cells) * peano4::FacesPerCell * DeviceKernel::ValuesPerFace,
    1.5,
    0.002
  );

  std::vector<double>       deviceUpdated(patches.size(), 0.0);
  const std::vector<double> deviceEigenvalues = updateOnDevice(patches, faces, geometry, deviceUpdated);

  std::vector<double> hostUpdated(patches.size(), 0.0);
  std::vector<double> hostEigenvalues(Cells, 0.0);
  for (int cell = 0; cell < Cells; cell++) {
    const double* facesOfCell[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      facesOfCell[faceNumber] = faces.data()
        + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * DeviceKernel::ValuesPerFace;
    }
    const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    hostEigenvalues[static_cast<std::size_t>(cell)] = DeviceKernel::update(
      patches.data() + static_cast<std::ptrdiff_t>(cell) * DeviceKernel::ValuesPerPatch,
      hostUpdated.data() + static_cast<std::ptrdiff_t>(cell) * DeviceKernel::ValuesPerPatch,
      facesOfCell,
      cellCentre,
      cellSize,
      exahype2::dg::tests::KernelTestSetup::TimeStamp,
      exahype2::dg::tests::KernelTestSetup::TimeStepSize,
      cell % 2 == 0 ? NoBoundaryFaces : 1
    );
  }

  validateNumericalBuffersEquals(deviceUpdated, hostUpdated);
  validateNumericalBuffersEquals(deviceEigenvalues, hostEigenvalues);
}
