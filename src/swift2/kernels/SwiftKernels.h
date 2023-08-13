#pragma once

namespace swift2 {
  namespace kernels {

    template <typename Particle>
    void hydro_end_density(Particle& localParticle);


    template <typename Particle>
    void hydro_prepare_density(Particle& localParticle);

  } // namespace kernels
} // namespace swift2

#include "SwiftKernels.cpph"
