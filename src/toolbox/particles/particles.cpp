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


tarch::la::Vector<Dimensions, double> toolbox::particles::mirrorParticleAlongPeriodicDomains(
    const tarch::la::Vector<Dimensions, double>& x,
    const peano4::datamanagement::VertexMarker&  marker,
    const tarch::la::Vector<Dimensions,double>   domainOffset,
    const tarch::la::Vector<Dimensions,double>   domainSize,
    const std::bitset<Dimensions>                periodicBC
) {
  tarch::la::Vector<Dimensions, double> result = x;

  for (int d=0; d<Dimensions; d++) {
    if ( periodicBC[d] ) {
      if ( tarch::la::equals( marker.x()(d), domainOffset(d) ) ) {
        result(d) -= domainSize(d);
      }
      if ( tarch::la::equals( marker.x()(d), domainOffset(d)+domainSize(d) ) ) {
        result(d) += domainSize(d);
      }
    }
  }

  return result;
}


tarch::la::Vector<Dimensions, double> toolbox::particles::applyPeriodicBoundaryConditions(
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions,double>   domainOffset,
    const tarch::la::Vector<Dimensions,double>   domainSize,
    const std::bitset<Dimensions>                periodicBC
) {
  tarch::la::Vector<Dimensions, double> result = x;

  for (int d=0; d<Dimensions; d++) {
    if ( periodicBC[d] ) {
      if ( tarch::la::smaller( result(d), domainOffset(d) ) ) {
        result(d) += domainSize(d);
      }
      if ( tarch::la::greater( result(d), domainOffset(d)+domainSize(d) ) ) {
        result(d) -= domainSize(d);
      }
    }
  }

  return result;
}


