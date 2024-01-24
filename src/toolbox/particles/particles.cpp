#include "particles.h"
#include "peano4/peano.h"
#include "MultiscaleTransitions.h"


bool toolbox::particles::particleAssignedToVertexWillBeLocal(
  const tarch::la::Vector<Dimensions, double>&  x,
  const peano4::datamanagement::VertexMarker&   marker
) {
  std::bitset<TwoPowerD> adjacentCellWhichHostsParticle = getAdjacentCellsOwningParticle(
    x,
    marker
  );

  return adjacentCellWhichHostsParticle.to_ulong() & marker.areAdjacentCellsLocal().to_ulong();
}

