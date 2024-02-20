#include "Utils.h"

bool swift2::boundaryconditions::isVertexOnGlobalBoundary(
  const peano4::datamanagement::VertexMarker&   marker,
  const tarch::la::Vector<Dimensions, double>&  domainOffset,
  const tarch::la::Vector<Dimensions, double>&  domainSize
) {
  return tarch::la::oneSmallerEquals( marker.x() - 0.5 * marker.h(), domainOffset )
      or tarch::la::oneGreaterEquals( marker.x() + 0.5 * marker.h(), domainOffset + domainSize );
}
