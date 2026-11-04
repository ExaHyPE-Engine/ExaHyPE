// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/FiniteVolumeScheme.h"

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/reconstruction/LimitedLinear.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/fv/HaloProfileTestData.h"

namespace {
  using Rule  = exahype2::TimeStepping::Rule;
  using Setup = exahype2::fv::unittests::HaloProfileSetup;

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
   * A Riemann solver whose fluxes include the non-conservative advection.
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
   * The discrete homogeneous Neumann condition.
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
   * Writes the inner value plus one into the outer half. The increment marks every written volume.
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

  template <int VolumesPerAxis>
  struct CopyBoundaryMusclSolver final
    : Layout<VolumesPerAxis, NumberOfUnknowns, LimitedLinear<exahype2::fv::limiter::Zero>>,
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
   * Acoustics with a flux that a swap of two axes maps onto itself.
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

  /**
   * A scalar that every axis advects with the position along that axis as its velocity.
   */
  template <typename Reconstruct>
  struct RadialAdvectionSolver final : SchemeLayout<NumberOfVolumesPerAxis, 1, Reconstruct, Rule::Adaptive> {
    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const ComputePrecision* NOALIAS const                         state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal,
      ComputePrecision* NOALIAS const                               fluxes
    ) {
      fluxes[0] = static_cast<ComputePrecision>(position(normal)) * state[0];
    }

    template <typename ComputePrecision>
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const ComputePrecision* NOALIAS const        state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     normal
    ) {
      return std::abs(position(normal));
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

  /**
   * A solver whose initial condition stores the centre and the size of the volume along every axis and counts its calls
   * in the last unknown.
   */
  struct VolumeRecordingSolver final : Layout<NumberOfVolumesPerAxis, 2 * DIMENSIONS + 1, Constant> {
    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        Q[axis]              = x(axis);
        Q[DIMENSIONS + axis] = h(axis);
      }
      Q[2 * DIMENSIONS] += 1.0;
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

  struct FaceRecordingSolver final {
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
      constexpr double    TimeStamp     = exahype2::dg::unittests::KernelTestSetup::TimeStamp;
      constexpr double    TimeStepSize  = exahype2::dg::unittests::KernelTestSetup::TimeStepSize;
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

  /**
   * Advances the patch of one cell in place with FiniteVolumeScheme::updateInTeam() of Kernel and
   * reduces the wave speeds of every thread into the accumulator of the team.
   */
  template <typename Kernel>
  struct UpdatePatchOfCellInTeam final {
    double*       patches{nullptr};
    const double* faces{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    double        timeStepSize{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            cell,
      const tarch::accelerator::TeamHandle team,
      double&                              accumulator
    ) const {
      const double* facesOfCell[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        facesOfCell[faceNumber] =
          faces + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * Kernel::ValuesPerFace;
      }
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      const double                                partial = Kernel::updateInTeam(
        team,
        patches + static_cast<std::ptrdiff_t>(cell) * Kernel::ValuesPerPatch,
        facesOfCell,
        cellCentre,
        cellSize,
        exahype2::dg::unittests::KernelTestSetup::TimeStamp,
        timeStepSize,
        cell % 2 == 0 ? NoBoundaryFaces : 1
      );
      const tarch::accelerator::Custom<exahype2::MaxEigenvalueReduction> reducer{accumulator};
      tarch::accelerator::reduceOverTeam(team, partial, reducer);
    }
  };

  /**
   * Fills the outer volumes of one face of a cell at one tangential position with
   * FiniteVolumeScheme::applyBoundaryConditions() of Kernel. entry runs over the tangential volumes
   * of the faces of the cells with the tangential volume fastest and the face number next.
   */
  template <typename Kernel>
  struct ApplyBoundaryConditionsAtTangentialVolume final {
    double*       faces{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      const int            face             = entry / Kernel::VolumesPerLayer;
      const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(face / peano4::FacesPerCell) * DIMENSIONS;
      Kernel::applyBoundaryConditions(
        faces + static_cast<std::ptrdiff_t>(face) * Kernel::ValuesPerFace,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        exahype2::dg::unittests::KernelTestSetup::TimeStamp,
        face % peano4::FacesPerCell,
        entry % Kernel::VolumesPerLayer
      );
    }
  };

  /**
   * Copies the boundary layers of the patch of one cell into the faces of the cell with
   * FiniteVolumeScheme::projectOntoFacesInTeam() of Kernel.
   */
  template <typename Kernel>
  struct ProjectPatchOfCellInTeam final {
    const double* patches{nullptr};
    double*       faces{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      double* facesOfCell[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        facesOfCell[faceNumber] =
          faces + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * Kernel::ValuesPerFace;
      }
      Kernel::projectOntoFacesInTeam(
        team,
        patches + static_cast<std::ptrdiff_t>(cell) * Kernel::ValuesPerPatch,
        facesOfCell
      );
    }
  };
  TARGET_DECLARE_END

  template <int VolumesPerAxis>
  constexpr int getNumberOfVolumesInPatch() {
    return peano4::power(VolumesPerAxis, DIMENSIONS);
  }

  /**
   * Fills the outer half of face faceNumber with FiniteVolumeScheme::applyBoundaryConditions() of
   * Kernel at every tangential volume.
   */
  template <typename Kernel>
  void applyBoundaryConditionsToFace(
    double* const                                face,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const double                                 timeStamp,
    const int                                    faceNumber
  ) {
    for (int tangentialVolume = 0; tangentialVolume < Kernel::VolumesPerLayer; tangentialVolume++) {
      Kernel::applyBoundaryConditions(face, cellCentre, cellSize, timeStamp, faceNumber, tangentialVolume);
    }
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
        faceStorage[static_cast<std::size_t>(faceNumber)][static_cast<std::size_t>(entry)] =
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
   * Returns a state that depends on the volume index only. Patches with different halo sizes read the same field.
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

  /**
   * One face of the halo profile per normal and side.
   */
  using FacePerNormalAndSide = std::array<std::array<std::vector<double>, 2>, DIMENSIONS>;

  /**
   * One face of the halo profile per normal, side and subface.
   */
  using FacePerNormalSideAndSubface =
    std::array<std::array<std::array<std::vector<double>, peano4::SubfacesPerFace>, 2>, DIMENSIONS>;

  std::vector<double> getCoarseFaceWithProfile(const int normal) {
    std::vector<double> coarseFace(Setup::ValuesPerFace, 0.0);
    for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
      for (int j = 0; j < Setup::SecondExtent; j++) {
        for (int i = 0; i < Setup::DofsPerAxis; i++) {
          const int volume = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
          for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
            coarseFace[volume * Setup::NumberOfData + unknown] =
              exahype2::fv::unittests::getLinearHaloProfile(layer, i, j, unknown);
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

  using TransferKernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;

  /**
   * Fills every volume of child from parent with interpolateVolume().
   */
  void interpolateEveryVolume(const double* const parent, double* const child, const int childPosition) {
    for (int fineVolume = 0; fineVolume < TransferKernel::VolumesPerPatch; fineVolume++) {
      TransferKernel::interpolateVolume(parent, child, childPosition, fineVolume);
    }
  }

  /**
   * Fills every volume of parent from children with restrictVolume().
   */
  void restrictEveryVolume(const double* const children[peano4::ChildrenPerCell], double* const parent) {
    for (int coarseVolume = 0; coarseVolume < TransferKernel::VolumesPerPatch; coarseVolume++) {
      TransferKernel::restrictVolume(children, parent, coarseVolume);
    }
  }

  std::vector<double> updateOnDevice(
    const std::vector<double>&                patches,
    const std::vector<double>&                faces,
    const exahype2::dg::unittests::CellChain& geometry,
    std::vector<double>&                      updated
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double> devicePatches(patches);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceUpdated(updated);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaces(faces);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    std::vector<double> eigenvalues(exahype2::dg::unittests::KernelTestSetup::NumberOfCells, 0.0);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceEigenvalues(eigenvalues);

    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(exahype2::dg::unittests::KernelTestSetup::NumberOfCells),
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

  /**
   * Advances every patch of patches in place by timeStepSize with the teams of an update of Kernel
   * on the device and returns the largest wave speed.
   */
  template <typename Kernel>
  double updateInTeamsOnDevice(
    std::vector<double>&                      patches,
    const std::vector<double>&                faces,
    const exahype2::dg::unittests::CellChain& geometry,
    const double                              timeStepSize
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double> devicePatches(patches);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaces(faces);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    double                                              eigenvalue = 0.0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::TeamRange(
        exahype2::dg::unittests::KernelTestSetup::NumberOfCells,
        Kernel::getTeamThreads(),
        Kernel::template getTeamScratchBytes<double>()
      ),
      UpdatePatchOfCellInTeam<Kernel>{
        devicePatches.getData(),
        deviceFaces.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        timeStepSize
      },
      tarch::accelerator::Custom<exahype2::MaxEigenvalueReduction>{eigenvalue}
    );
    devicePatches.copyBack(patches);
    return eigenvalue;
  }

  /**
   * Advances every patch of patches in place by timeStepSize on the host and returns the largest
   * wave speed.
   */
  template <typename Kernel>
  double updateInPlaceOnHost(
    std::vector<double>&                      patches,
    const std::vector<double>&                faces,
    const exahype2::dg::unittests::CellChain& geometry,
    const double                              timeStepSize
  ) {
    double eigenvalue = 0.0;
    for (int cell = 0; cell < exahype2::dg::unittests::KernelTestSetup::NumberOfCells; cell++) {
      const double* facesOfCell[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        facesOfCell[faceNumber] = faces.data()
          + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * Kernel::ValuesPerFace;
      }
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
      double* const patch = patches.data() + static_cast<std::ptrdiff_t>(cell) * Kernel::ValuesPerPatch;
      eigenvalue          = exahype2::MaxEigenvalueReduction::combine(
        eigenvalue,
        Kernel::update(
          patch,
          patch,
          facesOfCell,
          cellCentre,
          cellSize,
          exahype2::dg::unittests::KernelTestSetup::TimeStamp,
          timeStepSize,
          cell % 2 == 0 ? NoBoundaryFaces : 1
        )
      );
    }
    return eigenvalue;
  }

  /**
   * Writes the smooth Euler states of fillSmoothEulerPatchAndFaces() into the patch and the faces of
   * every cell of the test chain.
   */
  template <typename Solver>
  void fillSmoothEulerPatchesAndFacesOfChain(std::vector<double>& patches, std::vector<double>& faces) {
    std::vector<double>              q;
    std::vector<std::vector<double>> faceStorage;
    fillSmoothEulerPatchAndFaces<Solver>(q, faceStorage);
    patches.clear();
    faces.clear();
    for (int cell = 0; cell < exahype2::dg::unittests::KernelTestSetup::NumberOfCells; cell++) {
      patches.insert(patches.end(), q.begin(), q.end());
      for (const std::vector<double>& face : faceStorage) {
        faces.insert(faces.end(), face.begin(), face.end());
      }
    }
  }

  /**
   * The patches and the largest wave speed after an in-place update of every cell of the test chain
   * by the teams of a device and by the host.
   */
  struct InPlaceUpdates final {
    std::vector<double> devicePatches{};
    std::vector<double> hostPatches{};
    double              deviceEigenvalue{0.0};
    double              hostEigenvalue{0.0};
  };

  template <typename Kernel>
  InPlaceUpdates updateInPlaceOnDeviceAndHost(
    const std::vector<double>& patches,
    const std::vector<double>& faces,
    const double               timeStepSize
  ) {
    const exahype2::dg::unittests::CellChain geometry;
    InPlaceUpdates                           updates{patches, patches};
    updates.deviceEigenvalue = updateInTeamsOnDevice<Kernel>(updates.devicePatches, faces, geometry, timeStepSize);
    updates.hostEigenvalue   = updateInPlaceOnHost<Kernel>(updates.hostPatches, faces, geometry, timeStepSize);
    return updates;
  }

  template <typename Solver>
  InPlaceUpdates updateSmoothEulerPatchesInPlaceOnDeviceAndHost(const double timeStepSize) {
    std::vector<double> patches;
    std::vector<double> faces;
    fillSmoothEulerPatchesAndFacesOfChain<Solver>(patches, faces);
    return updateInPlaceOnDeviceAndHost<exahype2::fv::FiniteVolumeScheme<Solver, double>>(patches, faces, timeStepSize);
  }

  /**
   * Returns the faces of the test chain after a device and after a host fill of their outer volumes
   * by the boundary conditions of Kernel, both from the same ramp.
   */
  template <typename Kernel>
  std::pair<std::vector<double>, std::vector<double>> applyBoundaryConditionsOnDeviceAndHost() {
    constexpr int Faces = exahype2::dg::unittests::KernelTestSetup::NumberOfCells * peano4::FacesPerCell;
    const exahype2::dg::unittests::CellChain geometry;
    std::vector<double>                      hostFaces =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Faces) * Kernel::ValuesPerFace, 1.0, 0.001);
    std::vector<double> deviceFaces(hostFaces);

    const exahype2::dg::unittests::DeviceBuffer<double> facesOnDevice(deviceFaces);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Faces * Kernel::VolumesPerLayer),
      ApplyBoundaryConditionsAtTangentialVolume<Kernel>{
        facesOnDevice.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
      }
    );
    facesOnDevice.copyBack(deviceFaces);
    for (int face = 0; face < Faces; face++) {
      const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(face / peano4::FacesPerCell) * DIMENSIONS;
      applyBoundaryConditionsToFace<Kernel>(
        hostFaces.data() + static_cast<std::ptrdiff_t>(face) * Kernel::ValuesPerFace,
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        exahype2::dg::unittests::KernelTestSetup::TimeStamp,
        face % peano4::FacesPerCell
      );
    }
    return {deviceFaces, hostFaces};
  }

  /**
   * Returns the faces of the test chain after the teams of a device and after the host copied the
   * boundary layers of a ramp of patches into them with Kernel.
   */
  template <typename Kernel>
  std::pair<std::vector<double>, std::vector<double>> projectOntoFacesOnDeviceAndHost() {
    constexpr int             Cells = exahype2::dg::unittests::KernelTestSetup::NumberOfCells;
    const std::vector<double> patches =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Cells) * Kernel::ValuesPerPatch, 1.0, 0.001);
    std::vector<double> hostFaces(static_cast<std::size_t>(Cells) * peano4::FacesPerCell * Kernel::ValuesPerFace, -1.0);
    std::vector<double> deviceFaces(hostFaces);
    const exahype2::dg::unittests::DeviceBuffer<double> devicePatches(patches);
    const exahype2::dg::unittests::DeviceBuffer<double> facesOnDevice(deviceFaces);
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(Cells, tarch::accelerator::getTeamThreads(Kernel::ValuesPerHalfFace), 0),
      ProjectPatchOfCellInTeam<Kernel>{devicePatches.getData(), facesOnDevice.getData()}
    );
    facesOnDevice.copyBack(deviceFaces);
    for (int cell = 0; cell < Cells; cell++) {
      double* facesOfCell[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        facesOfCell[faceNumber] = hostFaces.data()
          + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * Kernel::ValuesPerFace;
      }
      Kernel::projectOntoFaces(
        patches.data() + static_cast<std::ptrdiff_t>(cell) * Kernel::ValuesPerPatch,
        facesOfCell
      );
    }
    return {deviceFaces, hostFaces};
  }

  /**
   * Returns the volume x, y, z of a patch with NumberOfVolumesPerAxis volumes along every axis.
   */
  int getRadialAdvectionVolume(const int x, const int y, const int z) {
    return (z * NumberOfVolumesPerAxis + y) * NumberOfVolumesPerAxis + x;
  }

  /**
   * Returns a patch symmetric along the first axis after one update of RadialAdvectionSolver with
   * Reconstruct on a patch centred at the origin.
   */
  template <typename Reconstruct>
  std::vector<double> getRadiallyAdvectedPatch() {
    using Kernel = exahype2::fv::FiniteVolumeScheme<RadialAdvectionSolver<Reconstruct>, double>;
    constexpr int       VolumesPerAxisIn3D = (DIMENSIONS == 3) ? NumberOfVolumesPerAxis : 1;
    std::vector<double> qIn(Kernel::ValuesPerPatch, 0.0);
    std::vector<double> qOut(Kernel::ValuesPerPatch, 0.0);
    for (int z = 0; z < VolumesPerAxisIn3D; z++) {
      for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
        for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
          const double offset                    = x - 0.5 * (NumberOfVolumesPerAxis - 1);
          qIn[getRadialAdvectionVolume(x, y, z)] = 1.0 + 0.03 * offset * offset + 0.05 * y + 0.07 * z;
        }
      }
    }
    const std::vector<std::vector<double>> faceStorage(
      peano4::FacesPerCell,
      std::vector<double>(Kernel::ValuesPerFace, 1.0)
    );

    // A centre at the origin and a width without a binary representation make the positions round.
    // A time step that spans several volume widths turns a rounding of a face position into a
    // rounding of the state.
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.0);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(0.7);
    static_cast<void>(Kernel::update(
      qIn.data(),
      qOut.data(),
      getFacePointersOf(faceStorage).data(),
      patchCentre,
      patchExtent,
      0.0,
      0.5,
      NoBoundaryFaces
    ));
    return qOut;
  }
} // namespace

UNIT_TEST("The face volume index runs the first tangential axis fastest and the layer slowest") {
  GIVEN("The last layer and the last volume along both tangential axes of a face of the halo profile") {
    constexpr int LastLayer            = 2 * Setup::HaloSize - 1;
    constexpr int LastFirstTangential  = Setup::DofsPerAxis - 1;
    constexpr int LastSecondTangential = Setup::SecondExtent - 1;

    WHEN("The test takes the index along every normal for sample volumes of the first two layers and the last volume") {
      std::array<int, DIMENSIONS> firstVolume{};
      std::array<int, DIMENSIONS> thirdVolume{};
      std::array<int, DIMENSIONS> firstVolumeOfSecondLayer{};
      std::array<int, DIMENSIONS> lastVolume{};
      std::array<int, DIMENSIONS> secondVolumeOfThirdRow{};
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        firstVolume[normal] = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 0, 0);
        thirdVolume[normal] = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 2, 0);
        firstVolumeOfSecondLayer[normal] =
          exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 1, 0, 0);
        lastVolume[normal] = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(
          normal,
          LastLayer,
          LastFirstTangential,
          LastSecondTangential
        );
        if constexpr (DIMENSIONS == 3) {
          secondVolumeOfThirdRow[normal] =
            exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, 0, 1, 2);
        }
      }

      THEN("The indices count along the first tangential axis, the second one and the layers up to the last") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          VALIDATE_EQUALS(firstVolume[normal], 0, normal);
          VALIDATE_EQUALS(thirdVolume[normal], 2, normal);
          VALIDATE_EQUALS(firstVolumeOfSecondLayer[normal], Setup::VolumesPerLayer, normal);
          VALIDATE_EQUALS(lastVolume[normal], Setup::VolumesPerFace - 1, normal);
          if constexpr (DIMENSIONS == 3) {
            VALIDATE_EQUALS(secondVolumeOfThirdRow[normal], 1 + 2 * Setup::DofsPerAxis, normal);
          }
        }
      }
    }
  }
}

UNIT_TEST("The vector form of the face volume index equals the tangential form and numbers every face volume once") {
  GIVEN("The halo volumes of a face of the halo profile along every normal") {
    std::array<std::vector<tarch::la::Vector<DIMENSIONS, int>>, DIMENSIONS> haloVolumes;
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      tarch::la::Vector<DIMENSIONS, int> range(Setup::DofsPerAxis);
      range(normal) = 2 * Setup::HaloSize;
      peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
        haloVolumes[normal].push_back(haloVolume);
      });
    }

    WHEN("Both forms of the index number every halo volume") {
      std::array<std::vector<int>, DIMENSIONS> vectorFormIndices;
      std::array<std::vector<int>, DIMENSIONS> tangentialFormIndices;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        for (const auto& haloVolume : haloVolumes[normal]) {
          const int first  = haloVolume(normal == 0 ? 1 : 0);
          const int second = DIMENSIONS == 3 ? haloVolume(normal == 2 ? 1 : 2) : 0;
          vectorFormIndices[normal].push_back(
            exahype2::fv::getFaceVolumeIndex(haloVolume, Setup::DofsPerAxis, Setup::HaloSize, normal)
          );
          tangentialFormIndices[normal].push_back(
            exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(
              normal,
              haloVolume(normal),
              first,
              second
            )
          );
        }
      }

      THEN("Both forms agree at every halo volume and every face numbers its volumes once") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          std::set<int> visited;
          for (std::size_t volume = 0; volume < haloVolumes[normal].size(); volume++) {
            const int index = vectorFormIndices[normal][volume];
            VALIDATE_EQUALS(index, tangentialFormIndices[normal][volume], haloVolumes[normal][volume], normal);
            VALIDATE(visited.find(index) == visited.end(), haloVolumes[normal][volume]);
            visited.insert(index);
          }
          VALIDATE_EQUALS(static_cast<int>(visited.size()), Setup::VolumesPerFace, normal);
        }
      }
    }
  }
}

UNIT_TEST("The kernels hand every volume its own entries, its centre, its size and its measure") {
  GIVEN("A cell of width three from the origin and a patch of zeros of a solver that records its initial condition") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<VolumeRecordingSolver, double>;
    constexpr double                            CellWidth = 3.0;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5 * CellWidth);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
    std::vector<double>                         values(Kernel::ValuesPerPatch, 0.0);

    WHEN("The kernel initialises the patch") {
      Kernel::initialise(values.data(), cellCentre, cellSize);

      THEN("The entries of every volume store its centre and its size along every axis and one call") {
        for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
          const auto          volumeIndex = peano4::dDelinearised(volume, Kernel::PatchSize);
          const double* const value       = values.data() + static_cast<std::ptrdiff_t>(volume) * Kernel::NumberOfData;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_NUMERICAL_EQUALS(
              value[axis],
              (volumeIndex(axis) + 0.5) * CellWidth / Kernel::PatchSize,
              volume,
              axis
            );
            VALIDATE_NUMERICAL_EQUALS(value[DIMENSIONS + axis], CellWidth / Kernel::PatchSize, volume, axis);
          }
          VALIDATE_EQUALS(value[2 * DIMENSIONS], 1.0, volume);
        }
      }
    }
  }

  GIVEN("A cell of width three from the origin and the error one half") {
    using Kernel                                          = exahype2::fv::FiniteVolumeScheme<ErrorSolver, double>;
    constexpr double                            CellWidth = 3.0;
    constexpr double                            Error     = 0.5;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5 * CellWidth);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);

    WHEN("The kernel measures the errors of patches at the exact solution but for the error in one volume each") {
      std::vector<double> weightedErrorOfVolume;
      for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
        std::vector<double> patch(Kernel::ValuesPerPatch, ExactSolutionValue);
        patch[static_cast<std::size_t>(volume) * Kernel::NumberOfData] += Error;
        double errors[exahype2::ErrorMeasurement::NumberOfNorms]{};
        Kernel::measureErrors(patch.data(), cellCentre, cellSize, 0.0, 0.0, errors);
        weightedErrorOfVolume.push_back(errors[0]);
      }

      THEN("Every L1 error is the error times the measure of the volume") {
        for (std::size_t volume = 0; volume < weightedErrorOfVolume.size(); volume++) {
          VALIDATE_NUMERICAL_EQUALS(
            weightedErrorOfVolume[volume],
            Error * std::pow(CellWidth / Kernel::PatchSize, DIMENSIONS),
            volume
          );
        }
      }
    }
  }
}

UNIT_TEST("The accumulation of wave speeds keeps the largest one and converts the precision") {
  GIVEN("Two maxima of zero, the wave speeds three, two and zero and the single-precision wave speed 2.5") {
    double           maximum                  = 0.0;
    double           mixedPrecisionMaximum    = 0.0;
    constexpr double FirstWaveSpeed           = 3.0;
    constexpr double SecondWaveSpeed          = 2.0;
    constexpr double ThirdWaveSpeed           = 0.0;
    constexpr float  SinglePrecisionWaveSpeed = 2.5F;

    WHEN("The double-precision wave speeds accumulate into the first maximum and the other one into the second") {
      exahype2::fv::accumulateMaxEigenvalue(maximum, FirstWaveSpeed);
      exahype2::fv::accumulateMaxEigenvalue(maximum, SecondWaveSpeed);
      exahype2::fv::accumulateMaxEigenvalue(maximum, ThirdWaveSpeed);
      exahype2::fv::accumulateMaxEigenvalue(mixedPrecisionMaximum, SinglePrecisionWaveSpeed);

      THEN("The maxima are three and 2.5") {
        VALIDATE_NUMERICAL_EQUALS(maximum, 3.0);
        VALIDATE_NUMERICAL_EQUALS(mixedPrecisionMaximum, 2.5);
      }
    }
  }
}

#if PEANO_ASSERTIONS == 0
UNIT_TEST("Without assertions the accumulation of wave speeds keeps a negative or an infinite maximum") {
  GIVEN("The maxima three and zero, the negative wave speed minus two, infinity and the wave speed five") {
    double           negativeMaximum   = 3.0;
    double           infiniteMaximum   = 0.0;
    constexpr double NegativeWaveSpeed = -2.0;
    constexpr auto   PositiveInfinity  = std::numeric_limits<double>::infinity();
    constexpr double LargerWaveSpeed   = 5.0;

    WHEN("The negative wave speed and five accumulate into the first maximum and infinity and five into the second") {
      exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, NegativeWaveSpeed);
      exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, LargerWaveSpeed);
      exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, PositiveInfinity);
      exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, LargerWaveSpeed);

      THEN("The first maximum keeps minus two and the second one stays infinite") {
        VALIDATE_EQUALS(negativeMaximum, -2.0);
        VALIDATE_EQUALS(infiniteMaximum, PositiveInfinity);
      }
    }
  }
}
#endif

UNIT_TEST("The initialisation writes the initial condition at every volume centre") {
  GIVEN("A unit cell around the point two and a patch of zeros") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<InitialConditionSolver, double>;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(2.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    std::vector<double>                         patch(Kernel::ValuesPerPatch, 0.0);

    WHEN("The kernel initialises the patch") {
      Kernel::initialise(patch.data(), cellCentre, cellSize);

      THEN("Every volume stores the first coordinate of its centre, its width and the last coordinate of its centre") {
        peano4::dForEach(Kernel::PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto          centre = exahype2::fv::getVolumeCentre(cellCentre, cellSize, Kernel::PatchSize, volume);
          const double* const value  = patch.data()
            + static_cast<std::ptrdiff_t>(peano4::dLinearised(volume, Kernel::PatchSize)) * Kernel::NumberOfData;
          VALIDATE_NUMERICAL_EQUALS(value[0], centre(0), volume);
          VALIDATE_NUMERICAL_EQUALS(value[1], 1.0 / Kernel::PatchSize, volume);
          VALIDATE_NUMERICAL_EQUALS(value[2], centre(DIMENSIONS - 1), volume);
        });
      }
    }
  }
}

UNIT_TEST("The boundary conditions copy a uniform state onto the outer half of every face") {
  GIVEN("The unit patch and one face of the uniform value 1.23 per face number") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<CopyBoundarySolver, double>;
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
    constexpr double                            UniformValue = 1.23;
    std::vector<std::vector<double>>            faces(
      peano4::FacesPerCell,
      std::vector<double>(Kernel::ValuesPerFace, UniformValue)
    );

    WHEN("The kernel applies the boundary conditions to every face") {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        applyBoundaryConditionsToFace<Kernel>(faces[faceNumber].data(), patchCentre, patchSize, 0.0, faceNumber);
      }

      THEN("Every entry of every face keeps the uniform value") {
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          for (const double entry : faces[faceNumber]) {
            VALIDATE_NUMERICAL_EQUALS(entry, UniformValue, faceNumber);
          }
        }
      }
    }
  }
}

UNIT_TEST("With two halo layers the outer layer k mirrors the inner layer k of the same tangential volume") {
  GIVEN("The unit patch and per face number a face with distinct values in its inner layers and minus one elsewhere") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<CopyBoundaryMusclSolver<NumberOfVolumesPerAxis>, double>;
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
    std::vector<std::vector<double>> faces(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, -1.0));
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int normal = faceNumber % DIMENSIONS;
      for (int tangentialVolume = 0; tangentialVolume < Kernel::VolumesPerLayer; tangentialVolume++) {
        auto haloVolume = peano4::dDelinearisedExclusive(tangentialVolume, Kernel::PatchSize, normal);
        for (int layer = 0; layer < Kernel::HaloSize; layer++) {
          haloVolume(normal) = faceNumber < DIMENSIONS ? Kernel::HaloSize + layer : Kernel::HaloSize - 1 - layer;
          const int inside = exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
          for (int entry = 0; entry < Kernel::NumberOfData; entry++) {
            faces[faceNumber][inside * Kernel::NumberOfData + entry] =
              static_cast<double>(100 * tangentialVolume + 10 * layer + entry);
          }
        }
      }
    }

    WHEN("The kernel applies the boundary conditions to every face") {
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        applyBoundaryConditionsToFace<Kernel>(faces[faceNumber].data(), patchCentre, patchSize, 0.0, faceNumber);
      }

      THEN("Every outer layer contains the values of the inner layer with the same distance to the face") {
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const int normal = faceNumber % DIMENSIONS;
          for (int tangentialVolume = 0; tangentialVolume < Kernel::VolumesPerLayer; tangentialVolume++) {
            auto haloVolume = peano4::dDelinearisedExclusive(tangentialVolume, Kernel::PatchSize, normal);
            for (int layer = 0; layer < Kernel::HaloSize; layer++) {
              haloVolume(normal) = faceNumber < DIMENSIONS ? Kernel::HaloSize - 1 - layer : Kernel::HaloSize + layer;
              const int outside =
                exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
              for (int entry = 0; entry < Kernel::NumberOfData; entry++) {
                VALIDATE_NUMERICAL_EQUALS(
                  faces[faceNumber][outside * Kernel::NumberOfData + entry],
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
    }
  }
}

UNIT_TEST("The boundary conditions write every outer volume of a face once") {
  GIVEN("The unit patch and one face of zeros per face number") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<IncrementBoundarySolver, double>;
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchSize(1.0);
    std::vector<std::vector<double>> faces(peano4::FacesPerCell, std::vector<double>(Kernel::ValuesPerFace, 0.0));

    WHEN("The kernel applies boundary conditions that write the inner value plus one to every face") {
      std::array<int, peano4::FacesPerCell>    writtenEntries{};
      std::array<double, peano4::FacesPerCell> maximumEntry{};
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        applyBoundaryConditionsToFace<Kernel>(faces[faceNumber].data(), patchCentre, patchSize, 0.0, faceNumber);
        for (const double entry : faces[faceNumber]) {
          if (entry > 0.5) {
            writtenEntries[faceNumber]++;
          }
          maximumEntry[faceNumber] = std::max(entry, maximumEntry[faceNumber]);
        }
      }

      THEN("The kernel writes every entry of the outer half of every face and the largest entry is one") {
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          VALIDATE_EQUALS(
            writtenEntries[faceNumber],
            Kernel::HaloSize * Kernel::VolumesPerLayer * Kernel::NumberOfData,
            faceNumber
          );
          VALIDATE_NUMERICAL_EQUALS(maximumEntry[faceNumber], 1.0, faceNumber);
        }
      }
    }
  }
}

UNIT_TEST(
  "Every thread of a device range fills the outer volumes of its face at its tangential position like the host"
) {
  GIVEN("A MUSCL solver with 20 volumes per axis and a ramp over the faces of the test chain") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<CopyBoundaryMusclSolver<20>, double>;

    WHEN("One device thread per face and tangential volume and the host apply the boundary conditions") {
      const auto [deviceFaces, hostFaces] = applyBoundaryConditionsOnDeviceAndHost<Kernel>();

      THEN("The device writes the faces of the host within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaces, hostFaces);
      }
    }
  }
}

UNIT_TEST("The projection copies the boundary layers next to every face into the half of the face on the patch side") {
  GIVEN("A patch with a distinct value per volume and unknown and faces of zeros") {
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

    WHEN("The kernel projects the patch onto its faces") {
      Kernel::projectOntoFaces(patch.data(), faces);

      THEN("The upper half of every lower face and the lower half of every upper face store the layers next to them") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          tarch::la::Vector<DIMENSIONS, int> range(Kernel::PatchSize);
          range(normal) = 2 * Kernel::HaloSize;
          peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& haloVolume) {
            const int faceVolume =
              exahype2::fv::getFaceVolumeIndex(haloVolume, Kernel::PatchSize, Kernel::HaloSize, normal);
            const bool upperHalf      = haloVolume(normal) >= Kernel::HaloSize;
            auto       boundaryVolume = haloVolume;
            boundaryVolume(normal)    = upperHalf ? haloVolume(normal) - Kernel::HaloSize
                                                  : haloVolume(normal) + Kernel::PatchSize - Kernel::HaloSize;
            const int boundaryIndex   = peano4::dLinearised(boundaryVolume, Kernel::PatchSize);
            const int face            = upperHalf ? normal : normal + DIMENSIONS;
            for (int unknown = 0; unknown < Kernel::NumberOfData; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                faceStorage[static_cast<std::size_t>(face)][faceVolume * Kernel::NumberOfData + unknown],
                getPatchValue(boundaryIndex, unknown),
                haloVolume,
                normal
              );
            }
          });
        }
      }
    }
  }
}

UNIT_TEST("Every team of a device range copies the boundary layers of its patch into its faces like one host thread") {
  GIVEN("A ramp over the patches of the test chain for the projection solver and for a Sod solver of 20 volumes") {
    using ProjectionKernel = exahype2::fv::FiniteVolumeScheme<ProjectionSolver, double>;
    using SodKernel        = exahype2::fv::FiniteVolumeScheme<SodSolver<20, Constant>, double>;

    WHEN("The teams of a device and the host project every patch onto its faces") {
      const auto [projectionDeviceFaces, projectionHostFaces] = projectOntoFacesOnDeviceAndHost<ProjectionKernel>();
      const auto [sodDeviceFaces, sodHostFaces]               = projectOntoFacesOnDeviceAndHost<SodKernel>();

      THEN("The device writes the faces of the host within rounding for both solvers") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(projectionDeviceFaces, projectionHostFaces);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(sodDeviceFaces, sodHostFaces);
      }
    }
  }
}

UNIT_TEST("The slope correction reproduces a profile linear along the tangential axes at every fine centre") {
  GIVEN("A coarse face per normal with a profile linear along the tangential axes") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
    std::array<std::vector<double>, DIMENSIONS> coarseFaces;
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      coarseFaces[normal] = getCoarseFaceWithProfile(normal);
    }

    WHEN("The kernel interpolates both sides of every subface along every normal into fine faces of minus ones") {
      FacePerNormalSideAndSubface fineFaces;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        for (int side = 0; side < 2; side++) {
          for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
            auto& fineFace = fineFaces[normal][side][subface];
            fineFace.assign(Setup::ValuesPerFace, -1.0);
            Kernel::interpolateHalo(coarseFaces[normal].data(), fineFace.data(), side, normal, subface);
          }
        }
      }

      THEN("The half on the interpolated side contains the profile at the fine centres and the other half minus one") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          for (int side = 0; side < 2; side++) {
            for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
              const auto& fineFace     = fineFaces[normal][side][subface];
              const auto  expectedFace = exahype2::fv::unittests::getFineFaceWithProfile(
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
                    VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("A face that is constant per unknown returns its values bitwise on every subface") {
  GIVEN("A coarse face that is constant per unknown") {
    using Kernel                         = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
    const std::vector<double> coarseFace = getConstantFace();

    WHEN("The kernel interpolates both sides of every subface along every normal into fine faces of minus ones") {
      FacePerNormalSideAndSubface fineFaces;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        for (int side = 0; side < 2; side++) {
          for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
            auto& fineFace = fineFaces[normal][side][subface];
            fineFace.assign(Setup::ValuesPerFace, -1.0);
            Kernel::interpolateHalo(coarseFace.data(), fineFace.data(), side, normal, subface);
          }
        }
      }

      THEN("The half on the interpolated side contains the coarse values bitwise") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          for (int side = 0; side < 2; side++) {
            for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
              const auto& fineFace = fineFaces[normal][side][subface];
              for (int layer = side * Setup::HaloSize; layer < (side + 1) * Setup::HaloSize; layer++) {
                for (int volumeInLayer = 0; volumeInLayer < Setup::VolumesPerLayer; volumeInLayer++) {
                  const int volume = layer * Setup::VolumesPerLayer + volumeInLayer;
                  for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
                    VALIDATE_EQUALS(
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
    }
  }
}

UNIT_TEST("Every fine volume of the injected half equals the coarse volume that covers it bitwise") {
  GIVEN("A coarse face per normal with a profile linear along the tangential axes") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
    std::array<std::vector<double>, DIMENSIONS> coarseFaces;
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      coarseFaces[normal] = getCoarseFaceWithProfile(normal);
    }

    WHEN("The kernel injects both sides of every subface along every normal into fine faces of minus ones") {
      FacePerNormalSideAndSubface fineFaces;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        for (int side = 0; side < 2; side++) {
          for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
            auto& fineFace = fineFaces[normal][side][subface];
            fineFace.assign(Setup::ValuesPerFace, -1.0);
            Kernel::injectHalo(coarseFaces[normal].data(), fineFace.data(), side, normal, subface);
          }
        }
      }

      THEN("Every fine volume of the injected half equals its covering coarse volume and the other half minus one") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          for (int side = 0; side < 2; side++) {
            for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
              const auto& fineFace      = fineFaces[normal][side][subface];
              const int   firstSubface  = subface % Setup::SubdivisionFactor;
              const int   secondSubface = subface / Setup::SubdivisionFactor;
              for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
                const bool inHalf = layer / Setup::HaloSize == side;
                for (int j = 0; j < Setup::SecondExtent; j++) {
                  const int coarseJ =
                    DIMENSIONS == 3 ? (j + Setup::DofsPerAxis * secondSubface) / Setup::SubdivisionFactor : 0;
                  for (int i = 0; i < Setup::DofsPerAxis; i++) {
                    const int coarseI = (i + Setup::DofsPerAxis * firstSubface) / Setup::SubdivisionFactor;
                    const int fineVolume =
                      exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
                    const int coarseVolume = exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(
                      normal,
                      layer,
                      coarseI,
                      coarseJ
                    );
                    for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
                      const double expected =
                        inHalf ? coarseFaces[normal][coarseVolume * Setup::NumberOfData + unknown] : -1.0;
                      VALIDATE_EQUALS(
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
    }
  }
}

UNIT_TEST("The average over the covering fine volumes of a linear profile is the profile at the coarse centre") {
  GIVEN("The fine faces of every subface along every normal with the profile at their volume centres") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
    std::array<std::vector<std::vector<double>>, DIMENSIONS> fineFaces;
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      fineFaces[normal].reserve(peano4::SubfacesPerFace);
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        fineFaces[normal].push_back(
          exahype2::fv::unittests::getFineFaceWithProfile(
            normal,
            subface % Setup::SubdivisionFactor,
            subface / Setup::SubdivisionFactor
          )
        );
      }
    }

    WHEN("The kernel restricts the fine faces of every normal into coarse faces of minus ones on both sides") {
      FacePerNormalAndSide coarseFaces;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        const double* finePointers[peano4::SubfacesPerFace];
        for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
          finePointers[subface] = fineFaces[normal][static_cast<std::size_t>(subface)].data();
        }
        for (int side = 0; side < 2; side++) {
          auto& coarseFace = coarseFaces[normal][side];
          coarseFace.assign(Setup::ValuesPerFace, -1.0);
          Kernel::restrictHalo(finePointers, coarseFace.data(), side, normal);
        }
      }

      THEN("The half on the restricted side contains the profile at the coarse centres and the other half minus one") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          for (int side = 0; side < 2; side++) {
            const auto& coarseFace = coarseFaces[normal][side];
            for (int layer = 0; layer < 2 * Setup::HaloSize; layer++) {
              const bool inHalf = layer / Setup::HaloSize == side;
              for (int j = 0; j < Setup::SecondExtent; j++) {
                for (int i = 0; i < Setup::DofsPerAxis; i++) {
                  const int volume =
                    exahype2::fv::getFaceVolumeIndex<Setup::DofsPerAxis, Setup::HaloSize>(normal, layer, i, j);
                  for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
                    const double expected =
                      inHalf ? exahype2::fv::unittests::getLinearHaloProfile(layer, i, j, unknown) : -1.0;
                    VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("Fine faces that are constant per unknown restrict to their values bitwise") {
  GIVEN("A fine face that is constant per unknown for every subface") {
    using Kernel                       = exahype2::fv::FiniteVolumeScheme<HaloProfileSolver, double>;
    const std::vector<double> fineFace = getConstantFace();
    const double*             finePointers[peano4::SubfacesPerFace];
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      finePointers[subface] = fineFace.data();
    }

    WHEN("The kernel restricts the fine faces along every normal into coarse faces of minus ones on both sides") {
      FacePerNormalAndSide coarseFaces;
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        for (int side = 0; side < 2; side++) {
          auto& coarseFace = coarseFaces[normal][side];
          coarseFace.assign(Setup::ValuesPerFace, -1.0);
          Kernel::restrictHalo(finePointers, coarseFace.data(), side, normal);
        }
      }

      THEN("The half on the restricted side contains the fine values bitwise") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          for (int side = 0; side < 2; side++) {
            const auto& coarseFace = coarseFaces[normal][side];
            for (int layer = side * Setup::HaloSize; layer < (side + 1) * Setup::HaloSize; layer++) {
              for (int volumeInLayer = 0; volumeInLayer < Setup::VolumesPerLayer; volumeInLayer++) {
                const int volume = layer * Setup::VolumesPerLayer + volumeInLayer;
                for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
                  VALIDATE_EQUALS(
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
    }
  }
}

UNIT_TEST("Every fine volume of an interpolated child copies its covering coarse volume") {
  GIVEN("A parent patch with a distinct value per entry") {
    const std::vector<double> parent = getRampPatch();

    WHEN("The kernel interpolates every volume of every child of the parent") {
      std::vector<std::vector<double>> children(Setup::ChildrenPerCell, std::vector<double>(Setup::ValuesPerCell, 0.0));
      for (int child = 0; child < Setup::ChildrenPerCell; child++) {
        interpolateEveryVolume(parent.data(), children[static_cast<std::size_t>(child)].data(), child);
      }

      THEN("Every fine volume of every child equals the parent volume that covers it bitwise") {
        for (int child = 0; child < Setup::ChildrenPerCell; child++) {
          const auto& fine          = children[static_cast<std::size_t>(child)];
          const auto  childPosition = peano4::dDelinearised(child, Setup::SubdivisionFactor);
          peano4::dForEach(Setup::DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& fineVolume) {
            tarch::la::Vector<DIMENSIONS, int> coarseVolume;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              coarseVolume(axis) =
                (fineVolume(axis) + Setup::DofsPerAxis * childPosition(axis)) / Setup::SubdivisionFactor;
            }
            for (int unknown = 0; unknown < Setup::NumberOfData; unknown++) {
              VALIDATE_EQUALS(
                fine[peano4::dLinearised(fineVolume, Setup::DofsPerAxis) * Setup::NumberOfData + unknown],
                parent[peano4::dLinearised(coarseVolume, Setup::DofsPerAxis) * Setup::NumberOfData + unknown],
                child,
                unknown
              );
            }
          });
        }
      }
    }
  }
}

UNIT_TEST("The children of a parent restrict to the parent bitwise") {
  GIVEN("A parent patch with a distinct value per entry and its interpolated children") {
    const std::vector<double>        parent = getRampPatch();
    std::vector<std::vector<double>> children(Setup::ChildrenPerCell, std::vector<double>(Setup::ValuesPerCell, 0.0));
    const double*                    childPointers[Setup::ChildrenPerCell];
    for (int child = 0; child < Setup::ChildrenPerCell; child++) {
      interpolateEveryVolume(parent.data(), children[static_cast<std::size_t>(child)].data(), child);
      childPointers[child] = children[static_cast<std::size_t>(child)].data();
    }

    WHEN("The kernel restricts the children onto every volume of the parent") {
      std::vector<double> restricted(Setup::ValuesPerCell, 0.0);
      restrictEveryVolume(childPointers, restricted.data());

      THEN("The restricted patch equals the parent bitwise") {
        for (int value = 0; value < Setup::ValuesPerCell; value++) {
          VALIDATE_EQUALS(restricted[static_cast<std::size_t>(value)], parent[static_cast<std::size_t>(value)], value);
        }
      }
    }
  }
}

UNIT_TEST("Two cancelling fine values leave the small ones in the correctly rounded mean of the restriction") {
  GIVEN("Children of ones whose first child stores 1e16 and -1e16 in its first two volumes") {
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

    WHEN("The kernel restricts the children onto every volume of the parent") {
      std::vector<double> restricted(Setup::ValuesPerCell, 0.0);
      restrictEveryVolume(childPointers, restricted.data());

      THEN("The first coarse volume covers both large values and equals the mean of its ones, every other volume one") {
        for (int value = 0; value < Setup::ValuesPerCell; value++) {
          const double expected = value < Setup::NumberOfData
            ? static_cast<double>(Setup::ChildrenPerCell - 2) / Setup::ChildrenPerCell
            : 1.0;
          VALIDATE_EQUALS(restricted[static_cast<std::size_t>(value)], expected, value);
        }
      }
    }
  }
}

UNIT_TEST("A volume face on a patch face with a set boundary bit reaches the Riemann solver as a boundary face") {
  GIVEN("The corner volumes and an inner volume of a patch and the full and the empty set of boundary bits") {
    constexpr int                            EveryFace = (1 << peano4::FacesPerCell) - 1;
    const tarch::la::Vector<DIMENSIONS, int> lowerCorner(0);
    const tarch::la::Vector<DIMENSIONS, int> upperCorner(NumberOfVolumesPerAxis - 1);
    const tarch::la::Vector<DIMENSIONS, int> inside(1);

    WHEN("The Riemann solver records the lower faces of the lower corner and of the inner volume and the upper faces") {
      const double lowerFaceWithTheBit = getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 0, EveryFace);
      const double lowerFaceWithoutTheBit =
        getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 0, NoBoundaryFaces);
      const double upperFaceOfTheLowerCorner = getEncodedRiemannFace<FaceRecordingSolver>(lowerCorner, 0, 1, EveryFace);
      const double lowerFaceOfTheInnerVolume = getEncodedRiemannFace<FaceRecordingSolver>(inside, 1, 0, EveryFace);
      const double upperFaceOfTheUpperCorner = getEncodedRiemannFace<FaceRecordingSolver>(upperCorner, 1, 1, EveryFace);

      THEN("Every face on a patch face with the bit receives its face number and every other face its normal") {
        VALIDATE_NUMERICAL_EQUALS(lowerFaceWithTheBit, encodeRiemannFace(0, true, 0));
        VALIDATE_NUMERICAL_EQUALS(lowerFaceWithoutTheBit, encodeRiemannFace(0, false, 0));
        VALIDATE_NUMERICAL_EQUALS(upperFaceOfTheLowerCorner, encodeRiemannFace(0, false, 0));
        VALIDATE_NUMERICAL_EQUALS(lowerFaceOfTheInnerVolume, encodeRiemannFace(1, false, 1));
        VALIDATE_NUMERICAL_EQUALS(upperFaceOfTheUpperCorner, encodeRiemannFace(1, true, 1 + DIMENSIONS));
      }
    }
  }
}

UNIT_TEST("A time step of size zero leaves the patch unchanged") {
  GIVEN("A patch of an advection solver with a distinct value per entry and faces of one half") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<AdvectionSolver<NumberOfVolumesPerAxis, Constant>, double>;
    std::vector<double> qIn(Kernel::ValuesPerPatch);
    for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
      qIn[entry] = 0.1 + 0.01 * static_cast<double>(entry);
    }
    const std::vector<std::vector<double>> faceStorage(
      peano4::FacesPerCell,
      std::vector<double>(Kernel::ValuesPerFace, 0.5)
    );
    const tarch::la::Vector<DIMENSIONS, double> patchCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> patchExtent(1.0);

    WHEN("The kernel advances the patch by a time step of size zero") {
      std::vector<double> qOut(Kernel::ValuesPerPatch, 0.0);
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

      THEN("Every entry keeps its value") {
        for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
          VALIDATE_NUMERICAL_EQUALS(qOut[entry], qIn[entry]);
        }
      }
    }
  }
}

UNIT_TEST("The update keeps a uniform state uniform and returns the wave speed of the advection") {
  GIVEN("An advection solver with the constant reconstruction, the uniform value 1.23 and the time step size 0.01") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<AdvectionSolver<NumberOfVolumesPerAxis, Constant>, double>;
    constexpr double UniformValue = 1.23;
    constexpr double TimeStepSize = 0.01;

    WHEN("The kernel advances the uniform patch with uniform halos by one time step") {
      std::vector<double> qOut;
      const double        returnedEigenvalue = updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut);

      THEN("Every entry keeps the uniform value and the wave speed is the one of the advection") {
        for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
          VALIDATE_NUMERICAL_EQUALS(qOut[entry], UniformValue);
        }
        VALIDATE_NUMERICAL_EQUALS(returnedEigenvalue, AdvectionSpeed + NcpAdvectionSpeed);
      }
    }
  }
}

UNIT_TEST("A fixed time step rule reports the wave speed zero and keeps a uniform state") {
  GIVEN("An advection solver with the fixed rule, the uniform value 1.23 and the time step size 0.01") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<FixedRuleAdvectionSolver, double>;
    static_assert(not Kernel::ReportEigenvalues);
    constexpr double UniformValue = 1.23;
    constexpr double TimeStepSize = 0.01;

    WHEN("The kernel advances the uniform patch with uniform halos by one time step") {
      std::vector<double> qOut;
      const double        returnedEigenvalue = updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut);

      THEN("The wave speed is zero and every entry keeps the uniform value") {
        VALIDATE_NUMERICAL_EQUALS(returnedEigenvalue, 0.0);
        for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
          VALIDATE_NUMERICAL_EQUALS(qOut[entry], UniformValue);
        }
      }
    }
  }
}

UNIT_TEST("A source term grows a uniform state by the source rate times the step") {
  GIVEN("An advection solver with a source term, the uniform value 1.23 and the time step size 0.01") {
    using Kernel                  = exahype2::fv::FiniteVolumeScheme<SourceAdvectionSolver, double>;
    constexpr double UniformValue = 1.23;
    constexpr double TimeStepSize = 0.01;

    WHEN("The kernel advances the uniform patch with uniform halos by one time step") {
      std::vector<double> qOut;
      static_cast<void>(updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut));

      THEN("Every entry grows by the source rate times the time step size") {
        for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
          VALIDATE_NUMERICAL_EQUALS(qOut[entry], UniformValue * (1.0 + TimeStepSize * SourceRate), entry);
        }
      }
    }
  }
}

UNIT_TEST("A linear profile gains the diffusive source of its forward difference minus the advection of its slope") {
  GIVEN("A profile with the slope two along the first axis and the time step size 0.01") {
    using Kernel                  = exahype2::fv::FiniteVolumeScheme<DiffusiveAdvectionSolver, double>;
    constexpr double Slope        = 2.0;
    constexpr double TimeStepSize = 0.01;
    const auto       getProfile =
      [](const tarch::la::Vector<DIMENSIONS, double>& position, [[maybe_unused]] const double time) {
        return 1.0 + Slope * position(0);
      };

    WHEN("The kernel of a solver with a diffusive source term advances the profile by one time step") {
      const std::vector<double> solution = advectProfile<DiffusiveAdvectionSolver>(getProfile, 1, TimeStepSize);

      THEN("Every volume changes by the step size times the difference of diffusivity and speed times the slope") {
        for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
          const auto centre =
            getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
          const double expected = getProfile(centre, 0.0) + TimeStepSize * (Diffusivity - AdvectionSpeed) * Slope;
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(solution[volume * NumberOfData + unknown], expected, volume, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("The limited linear reconstruction keeps a uniform state uniform") {
  GIVEN("An advection solver with the van Albada limiter, the uniform value 1.23 and the time step size 0.01") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<
      AdvectionSolver<NumberOfVolumesPerAxis, LimitedLinear<exahype2::fv::limiter::VanAlbada>>,
      double
    >;
    constexpr double UniformValue = 1.23;
    constexpr double TimeStepSize = 0.01;

    WHEN("The kernel advances the uniform patch with uniform halos by one time step") {
      std::vector<double> qOut;
      const double        returnedEigenvalue = updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut);

      THEN("Every entry keeps the uniform value and the wave speed is the one of the advection") {
        for (int entry = 0; entry < Kernel::ValuesPerPatch; entry++) {
          VALIDATE_NUMERICAL_EQUALS(qOut[entry], UniformValue);
        }
        VALIDATE_NUMERICAL_EQUALS(returnedEigenvalue, AdvectionSpeed + NcpAdvectionSpeed);
      }
    }
  }
}

UNIT_TEST("A linear profile under time-dependent advection moves by the exact displacement") {
  GIVEN("A linear profile that an advection speed growing with time moves and the time step size 0.01") {
    constexpr double TimeStepSize = 0.01;
    const auto       getProfile   = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
      return 1.5 + position(0) - time - 0.5 * time * time;
    };

    WHEN("The limited linear reconstruction advances the profile by one time step") {
      const auto solution = advectProfile<TimeDependentAdvectionSolver>(getProfile, 1, TimeStepSize);

      THEN("Every volume equals the profile at the end of the step") {
        for (int volume = 0; volume < getNumberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
          const auto centre =
            getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
          const double expected = getProfile(centre, TimeStepSize);
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(solution[volume * NumberOfData + unknown], expected, volume, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("The limited linear reconstruction converges at second order on a smooth advected profile") {
  GIVEN("The unlimited linear reconstruction and the end time 0.12") {
    using Average            = LimitedLinear<exahype2::fv::limiter::Average>;
    constexpr double EndTime = 0.12;

    WHEN("The reconstruction advects the smooth profile on 9 and on 27 volumes per axis") {
      const double coarseError = getAdvectionError<AdvectionSolver<9, Average>>(getSmoothProfile, 6, EndTime);
      const double fineError   = getAdvectionError<AdvectionSolver<27, Average>>(getSmoothProfile, 18, EndTime);
      const double order       = std::log(coarseError / fineError) / std::log(3.0);

      THEN("The order of convergence exceeds 1.7") { VALIDATE(order > 1.7, coarseError, fineError, order); }
    }
  }
}

UNIT_TEST("The constant reconstruction converges at first order on a smooth advected profile") {
  GIVEN("The end time 0.12") {
    constexpr double EndTime = 0.12;

    WHEN("The constant reconstruction advects the smooth profile on 9 and on 27 volumes per axis") {
      const double coarseError = getAdvectionError<AdvectionSolver<9, Constant>>(getSmoothProfile, 6, EndTime);
      const double fineError   = getAdvectionError<AdvectionSolver<27, Constant>>(getSmoothProfile, 18, EndTime);
      const double order       = std::log(coarseError / fineError) / std::log(3.0);

      THEN("The order of convergence lies between 0.6 and 1.4") {
        VALIDATE(order > 0.6 and order < 1.4, coarseError, fineError, order);
      }
    }
  }
}

UNIT_TEST("The Minmod limiter keeps a step profile in bounds and never grows its total variation") {
  GIVEN("An advection solver with the Minmod limiter on 27 volumes per axis") {
    constexpr int VolumesPerAxis = 27;
    using Solver                 = AdvectionSolver<VolumesPerAxis, LimitedLinear<exahype2::fv::limiter::Minmod>>;

    WHEN("The solver advects the step profile by eight time steps") {
      const std::vector<double> q            = advectProfile<Solver>(getStepProfile, 8, 0.02 / 3.0);
      const int                 numberOfRows = getNumberOfVolumesInPatch<VolumesPerAxis>() / VolumesPerAxis;
      std::vector<double>       totalVariationOfRow;
      for (int row = 0; row < numberOfRows; row++) {
        double totalVariation = 0.0;
        for (int volume = 0; volume + 1 < VolumesPerAxis; volume++) {
          const double left  = q[static_cast<std::size_t>(row * VolumesPerAxis + volume) * NumberOfData];
          const double right = q[static_cast<std::size_t>(row * VolumesPerAxis + volume + 1) * NumberOfData];
          totalVariation += std::abs(right - left);
        }
        totalVariationOfRow.push_back(totalVariation);
      }

      THEN("Every value stays between zero and one and every row keeps a total variation of at most one") {
        for (int volume = 0; volume < getNumberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
          const double profileValue = q[static_cast<std::size_t>(volume) * NumberOfData];
          VALIDATE(profileValue >= -1e-12, volume, profileValue);
          VALIDATE(profileValue <= 1.0 + 1e-12, volume, profileValue);
        }
        for (std::size_t row = 0; row < totalVariationOfRow.size(); row++) {
          VALIDATE(totalVariationOfRow[row] <= 1.0 + 1e-12, row, totalVariationOfRow[row]);
        }
      }
    }
  }
}

UNIT_TEST("Zero slopes reduce the limited reconstruction to the constant one, also for a non-conservative product") {
  GIVEN("The zero-slope reconstruction") {
    using ZeroSlope = LimitedLinear<exahype2::fv::limiter::Zero>;

    WHEN("Solvers with and without the non-conservative product advance the logical state with both reconstructions") {
      const double differenceWithoutTheProduct = getZeroSlopeVersusConstantMaxDifference<
        AdvectionSolver<NumberOfVolumesPerAxis, Constant>,
        AdvectionSolver<NumberOfVolumesPerAxis, ZeroSlope>
      >();
      const double differenceWithTheProduct = getZeroSlopeVersusConstantMaxDifference<
        NcpAdvectionSolver<NumberOfVolumesPerAxis, Constant>,
        NcpAdvectionSolver<NumberOfVolumesPerAxis, ZeroSlope>
      >();

      THEN("Both reconstructions give the same patch and wave speed") {
        VALIDATE_NUMERICAL_EQUALS(differenceWithoutTheProduct, 0.0);
        VALIDATE_NUMERICAL_EQUALS(differenceWithTheProduct, 0.0);
      }
    }
  }
}

UNIT_TEST("A profile the flux and the non-conservative product advect together converges at second order") {
  GIVEN("The unlimited linear reconstruction and the end time 0.12") {
    using Average            = LimitedLinear<exahype2::fv::limiter::Average>;
    constexpr double EndTime = 0.12;

    WHEN("A solver with both terms advects the profile on 9 and on 27 volumes per axis") {
      const double coarseError = getAdvectionError<NcpAdvectionSolver<9, Average>>(getNcpAdvectedProfile, 8, EndTime);
      const double fineError   = getAdvectionError<NcpAdvectionSolver<27, Average>>(getNcpAdvectedProfile, 24, EndTime);
      const double order       = std::log(coarseError / fineError) / std::log(3.0);

      THEN("The order of convergence exceeds 1.7") { VALIDATE(order > 1.7, coarseError, fineError, order); }
    }
  }
}

UNIT_TEST("A Riemann solver of the generated class keeps the non-conservative advection inside each volume") {
  GIVEN("A linear profile that the flux and the non-conservative product advect and the time step size 0.01") {
    constexpr double TimeStepSize = 0.01;
    const auto       getProfile   = [](const tarch::la::Vector<DIMENSIONS, double>& position, const double time) {
      return 1.5 + position(0) - (AdvectionSpeed + NcpAdvectionSpeed) * time;
    };

    WHEN("A solver with its own Riemann solver advances the profile by one time step") {
      const auto solution = advectProfile<RiemannNcpAdvectionSolver<LimitedLinear<exahype2::fv::limiter::Average>>>(
        getProfile,
        1,
        TimeStepSize
      );

      THEN("Every volume equals the profile at the end of the step") {
        for (int volume = 0; volume < getNumberOfVolumesInPatch<NumberOfVolumesPerAxis>(); volume++) {
          const auto centre =
            getVolumeCentreOf<NumberOfVolumesPerAxis>(getVolumeIndexOf<NumberOfVolumesPerAxis>(volume));
          const double expected = getProfile(centre, TimeStepSize);
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(solution[volume * NumberOfData + unknown], expected, volume, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("Periodic face fluxes conserve the total mass of the patch") {
  GIVEN("The smooth profile on a patch of 27 volumes per axis and its mass") {
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

    WHEN("The Minmod reconstruction advances the patch by three time steps with periodic halos") {
      std::vector<double>              qNext(Kernel::ValuesPerPatch, 0.0);
      std::vector<std::vector<double>> faceStorage(
        peano4::FacesPerCell,
        std::vector<double>(Kernel::ValuesPerFace, 0.0)
      );
      const auto                                  faces = getFacePointersOf(faceStorage);
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

      THEN("The mass after the steps divided by the mass before equals one") {
        VALIDATE_NUMERICAL_EQUALS(massAfter / massBefore, 1.0, massBefore, massAfter);
      }
    }
  }
}

UNIT_TEST("The shock-capturing path reaches the star states of the Sod tube") {
  GIVEN("The Sod tube on 54 volumes per axis in 2D and on 36 in 3D, 116 time steps and the time step size 0.00175") {
    constexpr int    VolumesPerAxis = DIMENSIONS == 2 ? 54 : 36;
    constexpr int    Steps          = 116;
    constexpr double TimeStepSize   = 0.00175;
    using Solver                    = SodSolver<VolumesPerAxis>;

    WHEN("The Minmod reconstruction advances the tube") {
      const std::vector<double> q = advanceSodTube<VolumesPerAxis>(Steps, TimeStepSize);

      THEN("Densities stay between the initial values, pressures positive and the plateaus reach the star states") {
        for (int volume = 0; volume < getNumberOfVolumesInPatch<VolumesPerAxis>(); volume++) {
          const double density  = q[static_cast<std::size_t>(volume) * SodUnknowns];
          const double pressure = Solver::getPressureOf(&q[static_cast<std::size_t>(volume) * SodUnknowns]);
          VALIDATE(density > 0.0, volume, density);
          VALIDATE(pressure > 0.0, volume, pressure);
          VALIDATE(density <= 1.0 + 1e-6, volume, density);
          VALIDATE(density >= 0.125 - 1e-6, volume, density);
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
          VALIDATE(
            std::abs(getValue(getVolumeAt(position), 0) - StarDensityLeft) < 0.03,
            position,
            getValue(getVolumeAt(position), 0)
          );
        }
        // The right plateau lies between the contact and the shock.
        for (const double position : {0.76, 0.79}) {
          VALIDATE(
            std::abs(getValue(getVolumeAt(position), 0) - StarDensityRight) < 0.03,
            position,
            getValue(getVolumeAt(position), 0)
          );
        }
        // Velocity and pressure stay constant across the contact. A sample on either side of it gives
        // the same star value.
        for (const double position : {0.58, 0.76}) {
          const double velocity = getValue(getVolumeAt(position), 1) / getValue(getVolumeAt(position), 0);
          const double pressure =
            Solver::getPressureOf(&q[static_cast<std::size_t>(getVolumeAt(position)) * SodUnknowns]);
          VALIDATE(std::abs(velocity - StarVelocity) < 0.05, position, velocity);
          VALIDATE(std::abs(pressure - StarPressure) < 0.03, position, pressure);
        }
      }
    }
  }
}

UNIT_TEST("A patch whose state a swap of the first two axes maps onto itself keeps that symmetry to the last bit") {
  GIVEN("An acoustic state that the swap of the first two axes maps onto itself and uniform halos") {
    using Kernel                           = exahype2::fv::FiniteVolumeScheme<AcousticSolver, double>;
    constexpr int       VolumesPerAxisIn3D = (DIMENSIONS == 3) ? NumberOfVolumesPerAxis : 1;
    std::vector<double> qIn(Kernel::ValuesPerPatch, 0.0);

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

    WHEN("The kernel advances the patch by one time step") {
      std::vector<double> qOut(Kernel::ValuesPerPatch, 0.0);
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

      THEN("Every volume equals the swapped state of its mirror bitwise, as every axis adds in a mirrored order") {
        for (int z = 0; z < VolumesPerAxisIn3D; z++) {
          for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
            for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
              VALIDATE_EQUALS(qOut[getVolumeOffset(x, y, z) + 0], qOut[getVolumeOffset(y, x, z) + 0], x, y);
              VALIDATE_EQUALS(qOut[getVolumeOffset(x, y, z) + 1], qOut[getVolumeOffset(y, x, z) + 2], x, y);
              VALIDATE_EQUALS(qOut[getVolumeOffset(x, y, z) + 2], qOut[getVolumeOffset(y, x, z) + 1], x, y);
              if constexpr (DIMENSIONS == 3) {
                VALIDATE_EQUALS(qOut[getVolumeOffset(x, y, z) + 3], qOut[getVolumeOffset(y, x, z) + 3], x, y);
              }
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A position-dependent flux keeps two volumes that mirror at the patch centre bitwise equal") {
  GIVEN("A patch symmetric along the first axis that the position along every axis advects, centred at the origin") {
    constexpr int VolumesPerAxisIn3D = (DIMENSIONS == 3) ? NumberOfVolumesPerAxis : 1;

    WHEN("The constant and the Minmod reconstruction update the patch with a step over several volume widths") {
      const std::vector<double> constant = getRadiallyAdvectedPatch<Constant>();
      const std::vector<double> minmod   = getRadiallyAdvectedPatch<LimitedLinear<exahype2::fv::limiter::Minmod>>();

      THEN("Every volume equals its mirror along the first axis bitwise with either reconstruction") {
        for (int z = 0; z < VolumesPerAxisIn3D; z++) {
          for (int y = 0; y < NumberOfVolumesPerAxis; y++) {
            for (int x = 0; x < NumberOfVolumesPerAxis; x++) {
              const int volume       = getRadialAdvectionVolume(x, y, z);
              const int mirrorVolume = getRadialAdvectionVolume(NumberOfVolumesPerAxis - 1 - x, y, z);
              VALIDATE_EQUALS(constant[volume], constant[mirrorVolume], x, y, z);
              VALIDATE_EQUALS(minmod[volume], minmod[mirrorVolume], x, y, z);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A reconstruct hook that reproduces the linear reconstruction keeps the result unchanged") {
  GIVEN("An advection solver with the Minmod limiter on nine volumes per axis, three steps and the step size 0.01") {
    constexpr int    Steps        = 3;
    constexpr double TimeStepSize = 0.01;
    using Default                 = AdvectionSolver<9, LimitedLinear<exahype2::fv::limiter::Minmod>>;

    WHEN("The solver advances the logical state without the hook and with a hook that reproduces it") {
      const std::vector<double> withoutHook   = advanceLogicalField<Default>(Steps, TimeStepSize);
      const std::vector<double> withHook      = advanceLogicalField<ReconstructHookSolver>(Steps, TimeStepSize);
      double                    maxDifference = 0.0;
      for (std::size_t entry = 0; entry < withoutHook.size(); entry++) {
        maxDifference = std::max(maxDifference, std::abs(withoutHook[entry] - withHook[entry]));
      }

      THEN("Both results agree") { VALIDATE_NUMERICAL_EQUALS(maxDifference, 0.0); }
    }
  }
}

UNIT_TEST("A limitedDifference hook that reproduces the Minmod limiter keeps the result unchanged") {
  GIVEN("An advection solver with the Minmod limiter on nine volumes per axis, three steps and the step size 0.01") {
    constexpr int    Steps        = 3;
    constexpr double TimeStepSize = 0.01;
    using Default                 = AdvectionSolver<9, LimitedLinear<exahype2::fv::limiter::Minmod>>;

    WHEN("The solver advances the logical state with the limiter and with a hook that reproduces it") {
      const std::vector<double> fromPolicy    = advanceLogicalField<Default>(Steps, TimeStepSize);
      const std::vector<double> fromHook      = advanceLogicalField<MinmodHookSolver>(Steps, TimeStepSize);
      double                    maxDifference = 0.0;
      for (std::size_t entry = 0; entry < fromPolicy.size(); entry++) {
        maxDifference = std::max(maxDifference, std::abs(fromPolicy[entry] - fromHook[entry]));
      }

      THEN("Both results agree") { VALIDATE_NUMERICAL_EQUALS(maxDifference, 0.0); }
    }
  }
}

UNIT_TEST("The postprocessing overwrites the first value and the returned wave speed reads the overwritten patch") {
  GIVEN("A solver whose postprocessing overwrites the first value, the uniform value one and the step size 0.01") {
    using Kernel                  = exahype2::fv::FiniteVolumeScheme<PostprocessingSolver, double>;
    constexpr double UniformValue = 1.0;
    constexpr double TimeStepSize = 0.01;

    WHEN("The kernel advances the uniform patch with uniform halos by one time step") {
      std::vector<double> qOut;
      const double        eigenvalue = updateUniformPatch<Kernel>(UniformValue, TimeStepSize, qOut);

      THEN("The wave speed reads the overwritten value and every volume stores it next to its unchanged second value") {
        VALIDATE_NUMERICAL_EQUALS(eigenvalue, PostprocessedValue + (DIMENSIONS - 1));
        for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
          VALIDATE_NUMERICAL_EQUALS(
            qOut[static_cast<std::size_t>(volume) * Kernel::NumberOfData],
            PostprocessedValue,
            volume
          );
          VALIDATE_NUMERICAL_EQUALS(qOut[static_cast<std::size_t>(volume) * Kernel::NumberOfData + 1], 1.0, volume);
        }
      }
    }
  }
}

UNIT_TEST("The reduction over the volumes takes the largest first value plus the largest axis") {
  GIVEN("A patch on the unit cell whose first value grows by one half per volume") {
    using Kernel = exahype2::fv::FiniteVolumeScheme<PostprocessingSolver, double>;
    std::vector<double> patch(Kernel::ValuesPerPatch, 0.0);
    for (int volume = 0; volume < Kernel::VolumesPerPatch; volume++) {
      patch[static_cast<std::size_t>(volume) * Kernel::NumberOfData] = 0.5 * volume;
    }
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

    WHEN("The kernels of an adaptive and of a fixed time step rule reduce the wave speeds of the patch") {
      using FixedKernel                  = exahype2::fv::FiniteVolumeScheme<FixedRuleAdvectionSolver, double>;
      const double adaptiveMaxEigenvalue = Kernel::reduceMaxEigenvalue(patch.data(), cellCentre, cellSize, 0.0);
      const double fixedMaxEigenvalue    = FixedKernel::reduceMaxEigenvalue(patch.data(), cellCentre, cellSize, 0.0);

      THEN("The adaptive rule gives the largest first value plus the largest axis and the fixed rule zero") {
        VALIDATE_NUMERICAL_EQUALS(adaptiveMaxEigenvalue, 0.5 * (Kernel::VolumesPerPatch - 1) + (DIMENSIONS - 1));
        VALIDATE_NUMERICAL_EQUALS(fixedMaxEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST("The error measurement sums the weighted norms of a constant error over the unit cell") {
  GIVEN("A patch on the unit cell with the constant error one half and error norms of minus one") {
    using Kernel                                      = exahype2::fv::FiniteVolumeScheme<ErrorSolver, double>;
    constexpr double                            Error = 0.5;
    const std::vector<double>                   patch(Kernel::ValuesPerPatch, ExactSolutionValue + Error);
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    double                                      errors[exahype2::ErrorMeasurement::NumberOfNorms]{-1.0, -1.0, -1.0};

    WHEN("The kernel measures the errors of the patch") {
      Kernel::measureErrors(patch.data(), cellCentre, cellSize, 0.0, 0.0, errors);

      THEN("The L1 error is one half, the squared L2 error one quarter and the largest error one half") {
        VALIDATE_NUMERICAL_EQUALS(errors[0], Error);
        VALIDATE_NUMERICAL_EQUALS(errors[1], Error * Error);
        VALIDATE_NUMERICAL_EQUALS(errors[2], Error);
      }
    }
  }
}

UNIT_TEST("Every cell of a device range receives the update and the wave speed of the serial host loop") {
  GIVEN("A chain of cells with rising values in their patches and faces") {
    constexpr int                            Cells = exahype2::dg::unittests::KernelTestSetup::NumberOfCells;
    const exahype2::dg::unittests::CellChain geometry;
    const std::vector<double>                patches =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Cells) * DeviceKernel::ValuesPerPatch, 1.0, 0.001);
    const std::vector<double> faces = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Cells) * peano4::FacesPerCell * DeviceKernel::ValuesPerFace,
      1.5,
      0.002
    );

    WHEN("The device and a serial host loop advance every cell by one time step") {
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
          exahype2::dg::unittests::KernelTestSetup::TimeStamp,
          exahype2::dg::unittests::KernelTestSetup::TimeStepSize,
          cell % 2 == 0 ? NoBoundaryFaces : 1
        );
      }

      THEN("The device updates and wave speeds equal the host ones") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceUpdated, hostUpdated);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceEigenvalues, hostEigenvalues);
      }
    }
  }
}

UNIT_TEST("Every team of a device range advances its patch in place like one host thread") {
  GIVEN("Ramps for the device kernel and smooth Euler states for Sod patches of three sizes on the test chain") {
    constexpr int    Cells             = exahype2::dg::unittests::KernelTestSetup::NumberOfCells;
    constexpr double EulerTimeStepSize = 0.001;
    const auto       patches =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Cells) * DeviceKernel::ValuesPerPatch, 1.0, 0.001);
    const auto faces = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Cells) * peano4::FacesPerCell * DeviceKernel::ValuesPerFace,
      1.5,
      0.002
    );

    WHEN("The teams of a device and the host update every patch in place") {
      const InPlaceUpdates muscl = updateInPlaceOnDeviceAndHost<DeviceKernel>(
        patches,
        faces,
        exahype2::dg::unittests::KernelTestSetup::TimeStepSize
      );
      // The three sizes cover a team that stages the patch with its wave speeds, a team that stages
      // the patch alone and a team that advances the patch in rounds.
      const std::array<InPlaceUpdates, 3> sod{
        updateSmoothEulerPatchesInPlaceOnDeviceAndHost<SodSolver<4, Constant>>(EulerTimeStepSize),
        updateSmoothEulerPatchesInPlaceOnDeviceAndHost<SodSolver<DIMENSIONS == 2 ? 20 : 7, Constant>>(
          EulerTimeStepSize
        ),
        updateSmoothEulerPatchesInPlaceOnDeviceAndHost<SodSolver<DIMENSIONS == 2 ? 48 : 20, Constant>>(
          EulerTimeStepSize
        )
      };

      THEN("The device writes the patches and the wave speed of the host within rounding for every patch size") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(muscl.devicePatches, muscl.hostPatches);
        VALIDATE_NUMERICAL_EQUALS(muscl.deviceEigenvalue, muscl.hostEigenvalue);
        for (std::size_t size = 0; size < sod.size(); size++) {
          VALIDATE_NUMERICAL_BUFFERS_EQUALS(sod[size].devicePatches, sod[size].hostPatches, size);
          VALIDATE_NUMERICAL_EQUALS(sod[size].deviceEigenvalue, sod[size].hostEigenvalue, size);
        }
      }
    }
  }
}

UNIT_TEST("The term ring maps a state and the state one ring size later onto the same entry") {
  GIVEN("A ring of integers with the lookahead two whose entries store ten plus their state") {
    exahype2::fv::TermRing<int, 2> ring;
    for (int state = 0; state < ring.RingSize; state++) {
      ring.at(state) = 10 + state;
    }
    std::vector<int> entriesOfTheFirstRound;
    for (int state = 0; state < ring.RingSize; state++) {
      entriesOfTheFirstRound.push_back(ring.at(state));
    }

    WHEN("The state one ring size after state zero writes twenty") {
      ring.at(ring.RingSize) = 20;

      THEN("The ring contains eight entries, state zero reads twenty and the other states keep their entries") {
        VALIDATE_EQUALS(ring.RingSize, 8);
        for (int state = 0; state < ring.RingSize; state++) {
          VALIDATE_EQUALS(entriesOfTheFirstRound[static_cast<std::size_t>(state)], 10 + state, state);
        }
        VALIDATE_EQUALS(std::as_const(ring).at(0), 20);
        VALIDATE_EQUALS(std::as_const(ring).at(1), 11);
        VALIDATE_EQUALS(std::as_const(ring).at(2 * ring.RingSize + 3), 13);
      }
    }
  }
}

UNIT_TEST("The sweep with the term ring writes the bits and the wave speed of the update of every single volume") {
  GIVEN("A smooth Euler state on the unit patch with the constant and with the limited linear reconstruction") {
    using ConstantSod      = SodSolver<NumberOfVolumesPerAxis, Constant>;
    using LimitedLinearSod = SodSolver<NumberOfVolumesPerAxis>;

    WHEN("The sweep and the update of every single volume advance the state of both reconstructions") {
      double    constantSweepEigenvalue          = 0.0;
      double    constantPerVolumeEigenvalue      = 0.0;
      double    limitedLinearSweepEigenvalue     = 0.0;
      double    limitedLinearPerVolumeEigenvalue = 0.0;
      const int constantMismatches =
        getNumberOfSweepMismatches<ConstantSod>(constantSweepEigenvalue, constantPerVolumeEigenvalue);
      const int limitedLinearMismatches =
        getNumberOfSweepMismatches<LimitedLinearSod>(limitedLinearSweepEigenvalue, limitedLinearPerVolumeEigenvalue);

      THEN("Both results agree bitwise for both reconstructions and the wave speeds agree and are positive") {
        VALIDATE_EQUALS(constantMismatches, 0);
        VALIDATE(constantSweepEigenvalue > 0.0, constantSweepEigenvalue);
        VALIDATE_EQUALS(constantSweepEigenvalue, constantPerVolumeEigenvalue);
        VALIDATE_EQUALS(limitedLinearMismatches, 0);
        VALIDATE(limitedLinearSweepEigenvalue > 0.0, limitedLinearSweepEigenvalue);
        VALIDATE_EQUALS(limitedLinearSweepEigenvalue, limitedLinearPerVolumeEigenvalue);
      }
    }
  }
}

UNIT_TEST("The sweep that overwrites its patch writes the bits and the wave speed of the sweep into a second patch") {
  GIVEN("A smooth Euler state on the unit patch with the constant and with the limited linear reconstruction") {
    using ConstantSod      = SodSolver<NumberOfVolumesPerAxis, Constant>;
    using LimitedLinearSod = SodSolver<NumberOfVolumesPerAxis>;

    WHEN("The sweep advances the state of both reconstructions into a second patch and in place") {
      double    constantSeparateEigenvalue      = 0.0;
      double    constantInPlaceEigenvalue       = 0.0;
      double    limitedLinearSeparateEigenvalue = 0.0;
      double    limitedLinearInPlaceEigenvalue  = 0.0;
      const int constantMismatches =
        getNumberOfInPlaceMismatches<ConstantSod>(constantSeparateEigenvalue, constantInPlaceEigenvalue);
      const int limitedLinearMismatches =
        getNumberOfInPlaceMismatches<LimitedLinearSod>(limitedLinearSeparateEigenvalue, limitedLinearInPlaceEigenvalue);

      THEN("Both results agree bitwise for both reconstructions and the wave speeds agree and are positive") {
        VALIDATE_EQUALS(constantMismatches, 0);
        VALIDATE(constantSeparateEigenvalue > 0.0, constantSeparateEigenvalue);
        VALIDATE_EQUALS(constantSeparateEigenvalue, constantInPlaceEigenvalue);
        VALIDATE_EQUALS(limitedLinearMismatches, 0);
        VALIDATE(limitedLinearSeparateEigenvalue > 0.0, limitedLinearSeparateEigenvalue);
        VALIDATE_EQUALS(limitedLinearSeparateEigenvalue, limitedLinearInPlaceEigenvalue);
      }
    }
  }
}
