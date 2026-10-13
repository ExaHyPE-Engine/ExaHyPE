// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::fv::ader {
  /**
   * What repeats its step with the fallback scheme: the whole patch with a troubled volume, only
   * the troubled volumes, or the troubled volumes and the volumes that share a face with them in
   * the patch.
   */
  enum class FallbackGranularity { Patch, Volumes, VolumesAndNeighbours };

  /**
   * Returns the fallback granularity of Derived, FallbackGranularity::Patch unless it names one.
   */
  template <typename Derived>
  constexpr FallbackGranularity getFallbackGranularity() {
    if constexpr (requires { Derived::FallbackGranularity; }) {
      return Derived::FallbackGranularity;
    } else {
      return FallbackGranularity::Patch;
    }
  }
} // namespace exahype2::fv::ader
