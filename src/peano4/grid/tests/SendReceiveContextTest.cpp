// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SendReceiveContextTest.h"

#include "peano4/grid/SendReceiveContext.h"
#include "tarch/tests/TestMacros.h"

namespace {
  /** Element type declaring that its own merge() rebuilds a consolidated slot. */
  struct RebuildsByMerge {
    static constexpr bool RebuildsConsolidationSlotByMerge = true;
  };

  /** Element type declaring that an assignment rebuilds a consolidated slot. */
  struct AssignsOnConsolidation {
    static constexpr bool RebuildsConsolidationSlotByMerge = false;
  };

  struct DeclaresNothing {};

  /** Counts the merge() calls applyIncomingCopy() makes on one entity copy. */
  template <typename Declaration>
  struct MergeCountingValue : Declaration {
    int value{0};
    int numberOfMerges{0};

    bool receiveAndMerge(
      [[maybe_unused]] const int                              marker,
      [[maybe_unused]] const peano4::grid::SendReceiveContext context
    ) const {
      return true;
    }

    void merge(
      [[maybe_unused]] const peano4::grid::SendReceiveContext context,
      const MergeCountingValue&                               neighbour,
      [[maybe_unused]] const int                              marker,
      [[maybe_unused]] const int                              treeId
    ) {
      value = neighbour.value;
      numberOfMerges++;
    }
  };

  template <typename Element>
  void applyOneCopy(Element& data, const peano4::grid::SendReceiveContext context, const int incomingValue) {
    Element incoming;
    incoming.value = incomingValue;
    peano4::grid::applyIncomingCopy(data, context, 0, 0, incoming);
  }
} // namespace

peano4::grid::tests::SendReceiveContextTest::SendReceiveContextTest() :
  TestCase("peano4::grid::tests::SendReceiveContextTest") {}

void peano4::grid::tests::SendReceiveContextTest::run() {
  testMethod(testTheConsolidationContextPicksThePathTheElementDeclares);
}

/**
 * A consolidation overwrites the rank's slot. An element type that declares
 * RebuildsConsolidationSlotByMerge true overwrites it inside merge(), so it can
 * delete what the slot held. Every other element type is overwritten by an
 * assignment.
 */
void peano4::grid::tests::SendReceiveContextTest::testTheConsolidationContextPicksThePathTheElementDeclares() {
  MergeCountingValue<RebuildsByMerge>        rebuildingValue;
  MergeCountingValue<AssignsOnConsolidation> assigningValue;
  MergeCountingValue<DeclaresNothing>        silentValue;

  applyOneCopy(rebuildingValue, peano4::grid::SendReceiveContext::Consolidation, 11);
  validateEquals(rebuildingValue.value, 11);
  validateEquals(rebuildingValue.numberOfMerges, 1);

  applyOneCopy(assigningValue, peano4::grid::SendReceiveContext::Consolidation, 22);
  validateEquals(assigningValue.value, 22);
  validateEquals(assigningValue.numberOfMerges, 0);

  applyOneCopy(silentValue, peano4::grid::SendReceiveContext::Consolidation, 33);
  validateEquals(silentValue.value, 33);
  validateEquals(silentValue.numberOfMerges, 0);

  applyOneCopy(rebuildingValue, peano4::grid::SendReceiveContext::IntraRank, 44);
  validateEquals(rebuildingValue.value, 44);
  validateEquals(rebuildingValue.numberOfMerges, 2);
  applyOneCopy(assigningValue, peano4::grid::SendReceiveContext::InterRank, 55);
  validateEquals(assigningValue.value, 55);
  validateEquals(assigningValue.numberOfMerges, 1);
  applyOneCopy(silentValue, peano4::grid::SendReceiveContext::IntraRank, 66);
  validateEquals(silentValue.value, 66);
  validateEquals(silentValue.numberOfMerges, 1);
}
