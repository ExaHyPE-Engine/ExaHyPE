#include "legacy/hydro_dimensions.h"
#include "legacy/kernel_hydro.h"

namespace swift2 {
  namespace kernels {
    template <typename ParticleA, typename ParticleB = ParticleA>
    bool forceKernelDistanceCheck(
      ParticleA& __restrict__ localParticle, const ParticleB& __restrict__ activeParticle
    );
  }
}

#include "ParticleParticleInteraction.cpph"
