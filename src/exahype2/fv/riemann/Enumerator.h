// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::fv::riemann {
  /**
   * Templated Riemann enumerator to efficiently access volumes within patch that is also optimised for GPU offloading
   * and is a POD type and trivially copyable.
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  template <int NumberOfFiniteVolumesPerAxisPerPatch, int HaloSize, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  struct Enumerator {
    inline GPUCallableMethod
    int operator () (const tarch::la::Vector<Dimensions, int>& volume, const int unknown) const InlineMethod {
        constexpr auto NumberOfFiniteVolumesPerAxisPerPatchWithHalo = (NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize);

      #if Dimensions == 2
      return (volume(1) + HaloSize) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) * NumberOfFiniteVolumesPerAxisPerPatchWithHalo
           + (volume(0) + HaloSize) * (NumberOfUnknowns + NumberOfAuxiliaryVariables)
           + unknown;
      #else
      return (volume(2) + HaloSize) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) * NumberOfFiniteVolumesPerAxisPerPatchWithHalo * NumberOfFiniteVolumesPerAxisPerPatchWithHalo
           + (volume(1) + HaloSize) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) * NumberOfFiniteVolumesPerAxisPerPatchWithHalo
           + (volume(0) + HaloSize) * (NumberOfUnknowns + NumberOfAuxiliaryVariables)
           + unknown;
      #endif
    }
  };
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  static_assert(std::is_pod<Enumerator<1,1,1,1>>::value);
  static_assert(std::is_trivially_copyable<Enumerator<1,1,1,1>>::value);
  #if defined(SharedSYCL) or defined(GPUOffloadingSYCL)
  static_assert(sycl::is_device_copyable<Enumerator<1,1,1,1>>::value);
  #endif
} // namespace exahype2::fv::riemann
