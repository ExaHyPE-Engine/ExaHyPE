#include "CellMetaData.h"


std::vector<int> swift2::CellMetaData::_dummyNumberOfCoalescedActiveParticlesPerVertex;


swift2::CellMetaData::CellMetaData(
  const peano4::datamanagement::CellMarker&  marker_,
  const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex_,
  const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex_,
  const std::vector<bool>&                   localParticleIsContainedInCell_
):
  marker(marker_),
  numberOfCoalescedLocalParticlesPerVertex(numberOfCoalescedLocalParticlesPerVertex_),
  numberOfCoalescedActiveParticlesPerVertex(numberOfCoalescedActiveParticlesPerVertex_),
  localParticleIsContainedInCell(localParticleIsContainedInCell_) {
}

swift2::CellMetaData::CellMetaData(
  const peano4::datamanagement::CellMarker&  marker_,
  const std::vector<bool>&                   localParticleIsContainedInCell_
):
  marker(marker_),
  numberOfCoalescedLocalParticlesPerVertex(_dummyNumberOfCoalescedActiveParticlesPerVertex),
  numberOfCoalescedActiveParticlesPerVertex(_dummyNumberOfCoalescedActiveParticlesPerVertex),
  localParticleIsContainedInCell(localParticleIsContainedInCell_) {
}
