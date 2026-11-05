// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * This file declares the pieces that the reconstruction schemes of the finite-volume solvers build
 * on.
 */
namespace exahype2::fv {
  template <typename Entry, int LookaheadOfRing>
  class TermRing;

  struct NoTermRing;

  template <typename ComputePrecision>
  struct WaveSpeedsOfTeam;

  /**
   * A reconstruction scheme forms the states that meet at a volume face and adds their flux
   * divergence to the update. The constant HaloSize is the number of layers its stencil reads.
   */
  template <typename ReconstructionPolicy>
  concept ReconstructionScheme = std::is_empty_v<ReconstructionPolicy> and requires {
    { ReconstructionPolicy::HaloSize } -> std::convertible_to<int>;
  };

  TARGET_DECLARE
  template <typename SolverType, int VolumesPerAxis, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemOnVolumeFace(
    const ComputePrecision*                      QL,
    const ComputePrecision*                      QR,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    const tarch::la::Vector<DIMENSIONS, int>&    volume,
    int                                          side,
    int                                          boundaryFaces,
    ComputePrecision*                            FL,
    ComputePrecision*                            FR
  );

  TARGET_DECLARE_END
} // namespace exahype2::fv

/**
 * A sweep over a patch updates the volumes in linearised order and reads the terms of the states
 * at most Lookahead states before and behind the current volume. The ring holds one Entry for
 * each of these 2 * Lookahead + 1 states. The function at(state) masks the state with the power of
 * two RingSize above the window and returns its entry. The ring lives on the host only.
 */
template <typename Entry, int LookaheadOfRing>
class exahype2::fv::TermRing final {
public:
  static constexpr int Lookahead = LookaheadOfRing;
  static constexpr int RingSize  = std::bit_ceil(static_cast<unsigned>(2 * Lookahead + 1));
  static_assert(Lookahead > 0, "The finite-volume update must read at least one volume ahead of its sweep.");
  static_assert(RingSize > 2 * Lookahead, "The ring buffer of the finite-volume update must cover its window.");

  [[nodiscard]] INLINE_METHOD Entry& at(int state);
  [[nodiscard]] INLINE_METHOD const Entry& at(int state) const;

private:
  Entry _entries[RingSize]{};
};

/**
 * Takes the place of a ring for a reconstruction that evaluates every term at the volume.
 */
struct exahype2::fv::NoTermRing final {
  static constexpr int Lookahead = 0;
};

/**
 * Points into the team scratch of a device update. Entry normal * volumes + volume is the largest
 * wave speed along normal of volume of the patch. The team writes every entry before the first
 * volume of the patch updates.
 */
template <typename ComputePrecision>
struct exahype2::fv::WaveSpeedsOfTeam final {
  static constexpr int Lookahead = 0;

  ComputePrecision* waveSpeeds{nullptr};
  int               volumes{0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD ComputePrecision& at(int volume, int normal) const;
};

#include "exahype2/fv/reconstruction/ReconstructionScheme.cpph"
