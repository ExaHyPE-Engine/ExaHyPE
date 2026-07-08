// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeriodicTopologyTest.h"

#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/PeriodicTopology.h"

#include <bitset>
#include <set>
#include <vector>

namespace {
  tarch::la::Vector<DIMENSIONS, int> idx(int v0, int v1 = 0, int v2 = 0) {
    tarch::la::Vector<DIMENSIONS, int> result;
    result(0) = v0;
#if DIMENSIONS >= 2
    result(1) = v1;
#endif
#if DIMENSIONS >= 3
    result(2) = v2;
#endif
    return result;
  }
} // namespace

peano4::grid::tests::PeriodicTopologyTest::PeriodicTopologyTest() :
  TestCase("peano4::grid::tests::PeriodicTopologyTest") {}

void peano4::grid::tests::PeriodicTopologyTest::run() {
  testMethod(testCellIndexIdentity);
  testMethod(testCellIndexFromState);
  testMethod(testFaceStampPairing);
  testMethod(testVertexStampPairing);
  testMethod(testFaceAndVertexKeysStayDistinct);
  testMethod(testRouting);
  testMethod(testPackRoundTrip);
  testMethod(testKeyBrokerIsDeterministicAndBalanced);
}

void peano4::grid::tests::PeriodicTopologyTest::testCellIndexIdentity() {
  validateEquals(cellsPerAxis(0), 1);
  validateEquals(cellsPerAxis(1), 3);
  validateEquals(cellsPerAxis(2), 9);
  validateEquals(cellsPerAxis(3), 27);

  // Child = 3 * parent + position, exact and free of any coordinate.
  const auto parent = idx(1, 0, 2);
  const auto child  = childCellIndex(parent, idx(2, 1, 0));
  validateEquals(child(0), 5);
#if DIMENSIONS >= 2
  validateEquals(child(1), 1);
#endif
#if DIMENSIONS >= 3
  validateEquals(child(2), 6);
#endif

  // Two levels of recursion match a direct computation.
  const auto level1 = childCellIndex(idx(0, 0, 0), idx(2, 1, 0));
  const auto level2 = childCellIndex(level1, idx(0, 2, 1));
  validateEquals(level2(0), 2 * 3 + 0);
#if DIMENSIONS >= 2
  validateEquals(level2(1), 1 * 3 + 2);
#endif
#if DIMENSIONS >= 3
  validateEquals(level2(2), 0 * 3 + 1);
#endif

  // Bit d of the vertex number lifts the cell index along axis d.
  const auto cell   = idx(4, 7, 2);
  const auto corner = vertexIndexOfCellVertex(cell, TWO_POWER_D - 1);
  for (int d = 0; d < DIMENSIONS; d++) {
    validateEquals(vertexIndexOfCellVertex(cell, 0)(d), cell(d));
    validateEquals(corner(d), cell(d) + 1);
  }
}

void peano4::grid::tests::PeriodicTopologyTest::testCellIndexFromState() {
  // The floating-point fallback agrees with the exact index for a unit domain.
  const int level          = 2;
  const int cellsAlongAxis = cellsPerAxis(level);

  AutomatonState state;
  state.level = level;
  const tarch::la::Vector<DIMENSIONS, double> domainOffset(0.0);
  const auto                                  expected = idx(4, 0, cellsAlongAxis - 1);
  for (int d = 0; d < DIMENSIONS; d++) {
    state.h(d) = 1.0 / cellsAlongAxis;
    state.x(d) = domainOffset(d) + expected(d) * state.h(d);
  }

  const auto derived = cellIndexFromState(state, domainOffset);
  for (int d = 0; d < DIMENSIONS; d++) {
    validateEqualsWithParams1(derived(d), expected(d), d);
  }
}

void peano4::grid::tests::PeriodicTopologyTest::testFaceStampPairing() {
  const int level          = 2;
  const int cellsAlongAxis = cellsPerAxis(level); // 9

  // Low and high face of the same transverse column on axis 0 are partners.
  const auto low  = makeFaceStamp(level, idx(0, 3, 4), 0);
  const auto high = makeFaceStamp(level, idx(cellsAlongAxis - 1, 3, 4), DIMENSIONS);
  validate(low.key == high.key);
  validate(low.side == PeriodicSide::Low);
  validate(high.side == PeriodicSide::High);
  validate(low.matchesPartnerOf(high));
  validate(high.matchesPartnerOf(low));
  validate(not low.matchesPartnerOf(low));

  // A different transverse column has a different key.
  const auto otherColumn = makeFaceStamp(level, idx(0, 5, 4), 0);
  validate(otherColumn.key != low.key);
  validate(not otherColumn.matchesPartnerOf(high));

  // A different level has a different key.
  const auto coarser = makeFaceStamp(level - 1, idx(0, 1, 1), 0);
  validate(coarser.key != low.key);

#if DIMENSIONS >= 2
  // A face with a different normal axis has a different key (different wrap).
  const auto axis1Face = makeFaceStamp(level, idx(3, 0, 4), 1);
  validate(axis1Face.key != low.key);
#endif
}

void peano4::grid::tests::PeriodicTopologyTest::testVertexStampPairing() {
  const int level          = 2;
  const int cellsAlongAxis = cellsPerAxis(level);

  std::bitset<DIMENSIONS> axisX;
  axisX.set(0);

  // Single-axis wrap: the two boundary vertex indices pair up, interior
  // transverse positions have to agree.
  const auto lowVertex  = makeVertexStamp(level, idx(0, 3, 7), axisX);
  const auto highVertex = makeVertexStamp(level, idx(cellsAlongAxis, 3, 7), axisX);
  validate(lowVertex.key == highVertex.key);
  validate(lowVertex.matchesPartnerOf(highVertex));
  const auto shiftedVertex = makeVertexStamp(level, idx(cellsAlongAxis, 4, 7), axisX);
  validate(shiftedVertex.key != lowVertex.key);

#if DIMENSIONS >= 2
  std::bitset<DIMENSIONS> diag;
  diag.set(0);
  diag.set(1);

  const auto d00 = makeVertexStamp(level, idx(0, 0, 0), diag);
  const auto dNN = makeVertexStamp(level, idx(cellsAlongAxis, cellsAlongAxis, 0), diag);
  const auto d0N = makeVertexStamp(level, idx(0, cellsAlongAxis, 0), diag);
  const auto dN0 = makeVertexStamp(level, idx(cellsAlongAxis, 0, 0), diag);

  // The low-low corner wraps to the high-high corner: same key, opposite side.
  validate(d00.key == dNN.key);
  validate(d00.matchesPartnerOf(dNN));
  // The low-high corner wraps to the high-low corner: same key, opposite side.
  validate(d0N.key == dN0.key);
  validate(d0N.matchesPartnerOf(dN0));
  // The two diagonals stay distinct, so a corner never merges across the wrong wrap.
  validate(d00.key != d0N.key);

  // The single-axis images of the same corner are distinct from the diagonal one.
  const auto cornerXOnly = makeVertexStamp(level, idx(0, 0, 0), axisX);
  validate(cornerXOnly.key != d00.key);
#endif
}

void peano4::grid::tests::PeriodicTopologyTest::testFaceAndVertexKeysStayDistinct() {
  // A face key and a vertex key with identical level, axis, and folded index
  // tuple must not collide: the entity tag keeps the routing tables apart.
  const int level = 1;

  std::bitset<DIMENSIONS> axisX;
  axisX.set(0);

  const auto faceStamp   = makeFaceStamp(level, idx(0, 1, 1), 0);
  const auto vertexStamp = makeVertexStamp(level, idx(0, 1, 1), axisX);
  validate(faceStamp.key.level == vertexStamp.key.level);
  validate(faceStamp.key.wrappedAxes == vertexStamp.key.wrappedAxes);
  validate(faceStamp.key != vertexStamp.key);
  validate(not faceStamp.matchesPartnerOf(vertexStamp));
}

void peano4::grid::tests::PeriodicTopologyTest::testRouting() {
  const int level          = 2;
  const int cellsAlongAxis = cellsPerAxis(level);

  // One tree owning both sides resolves onto itself: the single-tree case.
  {
    std::vector<PeriodicDescriptor> producers;
    producers.push_back({makeFaceStamp(level, idx(0, 3), 0), 0});
    producers.push_back({makeFaceStamp(level, idx(cellsAlongAxis - 1, 3), DIMENSIONS), 0});

    const auto routing = buildPeriodicRouting(producers, {});
    validateEquals(static_cast<int>(routing.size()), 1);
    const auto& sides = routing.begin()->second;
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::Low)].size()), 1);
    validateEquals(sides[toIndex(PeriodicSide::Low)].front(), 0);
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::High)].size()), 1);
    validateEquals(sides[toIndex(PeriodicSide::High)].front(), 0);
  }

  // Low side on tree 5, high side on tree 8: cross-tree pairing, and the
  // consumer records merge into the same table without duplicating trees.
  {
    std::vector<PeriodicDescriptor> producers;
    producers.push_back({makeFaceStamp(level, idx(0, 1), 0), 5});
    producers.push_back({makeFaceStamp(level, idx(cellsAlongAxis - 1, 1), DIMENSIONS), 8});
    std::vector<PeriodicDescriptor> consumers;
    consumers.push_back({makeFaceStamp(level, idx(0, 1), 0), 5});
    consumers.push_back({makeFaceStamp(level, idx(cellsAlongAxis - 1, 1), DIMENSIONS), 8});

    const auto routing = buildPeriodicRouting(producers, consumers);
    validateEquals(static_cast<int>(routing.size()), 1);
    const auto& sides = routing.begin()->second;
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::Low)].size()), 1);
    validateEquals(sides[toIndex(PeriodicSide::Low)].front(), 5);
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::High)].size()), 1);
    validateEquals(sides[toIndex(PeriodicSide::High)].front(), 8);
  }

  // A key with only one side present keeps the other side empty: its data has
  // no target, and nothing pairs up wrongly.
  {
    std::vector<PeriodicDescriptor> producers;
    producers.push_back({makeFaceStamp(level, idx(0, 4), 0), 5});

    const auto routing = buildPeriodicRouting(producers, {});
    validateEquals(static_cast<int>(routing.size()), 1);
    const auto& sides = routing.begin()->second;
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::Low)].size()), 1);
    validate(sides[toIndex(PeriodicSide::High)].empty());
  }

  // A shared boundary vertex owned by several trees per side: every owner
  // appears once, sorted, so a producer multicasts one copy per partner tree.
  {
    std::bitset<DIMENSIONS> axisX;
    axisX.set(0);
    const auto lowStamp  = makeVertexStamp(level, idx(0, 3, 1), axisX);
    const auto highStamp = makeVertexStamp(level, idx(cellsAlongAxis, 3, 1), axisX);

    std::vector<PeriodicDescriptor> producers;
    producers.push_back({lowStamp, 7});
    producers.push_back({lowStamp, 2});
    producers.push_back({highStamp, 4});
    std::vector<PeriodicDescriptor> consumers;
    consumers.push_back({lowStamp, 2});
    consumers.push_back({highStamp, 4});

    const auto routing = buildPeriodicRouting(producers, consumers);
    validateEquals(static_cast<int>(routing.size()), 1);
    const auto& sides = routing.begin()->second;
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::Low)].size()), 2);
    validateEquals(sides[toIndex(PeriodicSide::Low)][0], 2);
    validateEquals(sides[toIndex(PeriodicSide::Low)][1], 7);
    validateEquals(static_cast<int>(sides[toIndex(PeriodicSide::High)].size()), 1);
    validateEquals(sides[toIndex(PeriodicSide::High)].front(), 4);
  }
}

void peano4::grid::tests::PeriodicTopologyTest::testPackRoundTrip() {
  // The cross-rank discovery and payload exchange ship stamps respectively
  // descriptors as flat int blocks; pack followed by unpack must be the
  // identity so a remote rank rebuilds the exact key, side, and tree id.
  const int level          = 3;
  const int cellsAlongAxis = cellsPerAxis(level);

  std::bitset<DIMENSIONS> axisX;
  axisX.set(0);

  std::vector<PeriodicDescriptor> originals;
  originals.push_back({makeFaceStamp(level, idx(0, 5, 2), 0), 7});
  originals.push_back({makeFaceStamp(level, idx(cellsAlongAxis - 1, 5, 2), DIMENSIONS), 13});
  originals.push_back({makeVertexStamp(level, idx(0, 5, 2), axisX), 99});
#if DIMENSIONS >= 2
  originals.push_back({makeFaceStamp(level, idx(4, 0, 1), 1), 21});
#endif

  std::vector<int> buffer(originals.size() * PackedPeriodicDescriptorSize);
  for (std::size_t i = 0; i < originals.size(); i++) {
    packPeriodicDescriptor(originals[i], buffer.data() + i * PackedPeriodicDescriptorSize);
  }
  for (std::size_t i = 0; i < originals.size(); i++) {
    const PeriodicDescriptor restored = unpackPeriodicDescriptor(buffer.data() + i * PackedPeriodicDescriptorSize);
    validate(restored.stamp.key == originals[i].stamp.key);
    validate(restored.stamp.side == originals[i].stamp.side);
    validateEquals(restored.treeId, originals[i].treeId);
  }

  std::vector<int> stampBuffer(PackedPeriodicStampSize);
  packPeriodicStamp(originals.front().stamp, stampBuffer.data());
  const PeriodicStamp restoredStamp = unpackPeriodicStamp(stampBuffer.data());
  validate(restoredStamp.key == originals.front().stamp.key);
  validate(restoredStamp.side == originals.front().stamp.side);
}

void peano4::grid::tests::PeriodicTopologyTest::testKeyBrokerIsDeterministicAndBalanced() {
  constexpr int Level           = 3;
  constexpr int NumberOfBrokers = 8;

  std::set<int> brokersHit;
  for (int transverse = 0; transverse < cellsPerAxis(Level); transverse++) {
    tarch::la::Vector<DIMENSIONS, int> cellIndex(transverse);
    cellIndex(0) = 0;

    const auto stamp  = makeFaceStamp(Level, cellIndex, 0);
    const int  broker = periodicKeyBroker(stamp.key, NumberOfBrokers);
    validate(broker >= 0 and broker < NumberOfBrokers);
    validateEquals(broker, periodicKeyBroker(stamp.key, NumberOfBrokers));
    brokersHit.insert(broker);

    // Both sides of a wrap meet on one broker: same key, so same hash.
    tarch::la::Vector<DIMENSIONS, int> partnerIndex = cellIndex;
    partnerIndex(0)                                 = cellsPerAxis(Level) - 1;
    const auto partnerStamp                         = makeFaceStamp(Level, partnerIndex, DIMENSIONS);
    validateEquals(periodicKeyBroker(partnerStamp.key, NumberOfBrokers), broker);
  }
  validate(static_cast<int>(brokersHit.size()) > NumberOfBrokers / 2);
}
