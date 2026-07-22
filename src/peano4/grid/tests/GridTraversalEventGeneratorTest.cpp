// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEventGeneratorTest.h"

#include "peano4/grid/FaceType.h"
#include "peano4/grid/GridTraversalEventGenerator.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/grid/VertexLookupTable.h"
#include "peano4/grid/VertexType.h"
#include "tarch/la/Vector.h"

namespace {
  peano4::grid::AutomatonState makeBaseState(bool inverted) {
    peano4::grid::AutomatonState s;
    s.level = 2;

    for (int d = 0; d < DIMENSIONS; ++d) {
      s.x[d] = 0.0;
      s.h[d] = 1.0;
    }

    s.setInverted(inverted);

    for (int i = 0; i < TWO_TIMES_D; ++i) {
      s.accessNumber[i] = 0;
    }
    return s;
  }

  peano4::grid::AutomatonState makeSimpleState() { return makeBaseState(false); }

  peano4::grid::AutomatonState makeStateWithTemporaryReadStacks() {
    auto s = makeBaseState(false);
    // Any negative face access number => temporary stacks for faces/vertices touching that face.
    // Setting all faces ensures all vertices/faces use temporary stacks.
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      s.accessNumber[i] = -1;
    }
    return s;
  }

  peano4::grid::AutomatonState makeStateWithTemporaryWriteStacks() {
    auto s = makeBaseState(false);
    // Any positive face access number => temporary write stacks for faces/vertices touching that face.
    // Setting all faces ensures all vertices/faces use temporary stacks for writing.
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      s.accessNumber[i] = 1;
    }
    return s;
  }

  tarch::la::Vector<DIMENSIONS, int> makeZeroRelPos() {
    tarch::la::Vector<DIMENSIONS, int> v;
    for (int d = 0; d < DIMENSIONS; ++d) {
      v[d] = 0;
    }
    return v;
  }

  void setAllAdjacentRanks(peano4::grid::GridVertex& vertex, int rank) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      vertex.adjacentRanks[i]         = rank;
      vertex.backupOfAdjacentRanks[i] = rank;
    }
  }

  void setAllVerticesLocal(peano4::grid::GridVertex vertices[TWO_POWER_D], int treeId) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      vertices[v].setState(peano4::grid::GridVertex::State::Unrefined);
      setAllAdjacentRanks(vertices[v], treeId);
    }
  }

  void setFaceVerticesNonLocal(peano4::grid::GridVertex vertices[TWO_POWER_D], int faceIndex, int remoteRank) {
    const auto& faceTable = peano4::grid::SpacetreeHelper::getFaceAdjTables();
    const int   normal    = faceIndex % DIMENSIONS;
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int vIdx                           = faceTable.faceVerts[faceIndex][k];
      const int i0                             = faceTable.adjIdx0[normal][vIdx];
      const int i1                             = faceTable.adjIdx1[normal][vIdx];
      vertices[vIdx].adjacentRanks[i0]         = remoteRank;
      vertices[vIdx].adjacentRanks[i1]         = remoteRank;
      vertices[vIdx].backupOfAdjacentRanks[i0] = remoteRank;
      vertices[vIdx].backupOfAdjacentRanks[i1] = remoteRank;
    }

    for (int v = 0; v < TWO_POWER_D; ++v) {
      const int owningSlot                          = TWO_POWER_D - v - 1;
      vertices[v].adjacentRanks[owningSlot]         = remoteRank;
      vertices[v].backupOfAdjacentRanks[owningSlot] = remoteRank;
    }
  }
  // Wrapper: builds precomputed vertex index/stack arrays from state and forwards to generator.
  peano4::grid::GridTraversalEvent callCreateEnterNodeTraversalEvent(
    const peano4::grid::GridTraversalEventGenerator& gen,
    peano4::grid::GridVertex                         coarseGridVertices[TWO_POWER_D],
    peano4::grid::GridVertex                         fineGridVertices[TWO_POWER_D],
    const peano4::grid::VertexType                   vertexTypes[TWO_POWER_D],
    const peano4::grid::AutomatonState&              state,
    const peano4::grid::SplitSpecification&          splitTriggered,
    const peano4::grid::TreeIdSet&                   splitting,
    const peano4::grid::TreeIdSet&                   joinTriggered,
    const peano4::grid::TreeIdSet&                   joining,
    const peano4::grid::TreeIdSet&                   hasSplit,
    const tarch::la::Vector<DIMENSIONS, int>&        relPos,
    bool                                             noParallelTopologyChanges,
    bool                                             spacetreeStateIsRunning
  ) {
    const peano4::grid::VertexLookupTable& lut = peano4::grid::createVertexLookupTable(state, relPos);
    peano4::grid::FaceType                 faceTypes[TWO_TIMES_D];
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      faceTypes[i] = peano4::grid::getFaceType(vertexTypes, lut.faceIndex[i]);
    }
    return gen.createEnterNodeTraversalEvent(
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      lut,
      noParallelTopologyChanges,
      spacetreeStateIsRunning
    );
  }

  peano4::grid::GridTraversalEvent callCreateLeaveNodeTraversalEvent(
    const peano4::grid::GridTraversalEventGenerator& gen,
    const peano4::grid::GridTraversalEvent&          enterEvent,
    peano4::grid::GridVertex                         coarseGridVertices[TWO_POWER_D],
    peano4::grid::GridVertex                         fineGridVertices[TWO_POWER_D],
    const peano4::grid::VertexType                   vertexTypes[TWO_POWER_D],
    const peano4::grid::AutomatonState&              state,
    const peano4::grid::SplitSpecification&          splitTriggered,
    const peano4::grid::TreeIdSet&                   splitting,
    const peano4::grid::TreeIdSet&                   joinTriggered,
    const peano4::grid::TreeIdSet&                   joining,
    const peano4::grid::TreeIdSet&                   hasSplit,
    const tarch::la::Vector<DIMENSIONS, int>&        relPos,
    bool                                             noParallelTopologyChanges,
    bool                                             spacetreeStateIsRunning
  ) {
    const peano4::grid::VertexLookupTable& lut = peano4::grid::createVertexLookupTable(state, relPos);
    peano4::grid::FaceType                 faceTypes[TWO_TIMES_D];
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      faceTypes[i] = peano4::grid::getFaceType(vertexTypes, lut.faceIndex[i]);
    }
    return gen.createLeaveNodeTraversalEvent(
      enterEvent,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      lut,
      noParallelTopologyChanges,
      spacetreeStateIsRunning
    );
  }
} // namespace

peano4::grid::tests::GridTraversalEventGeneratorTest::GridTraversalEventGeneratorTest() :
  TestCase("peano4::grid::tests::GridTraversalEventGeneratorTest") {}

void peano4::grid::tests::GridTraversalEventGeneratorTest::run() {
  testMethod(testCreateEnterNodeTraversalEvent_AllLocalPersistent);
  testMethod(testCreateEnterNodeTraversalEvent_VertexBoundaryImpliesLocalInFastPath);
  testMethod(testCreateEnterNodeTraversalEvent_InOutNoDataAndHasSplitOverride);
  testMethod(testCreateEnterNodeTraversalEvent_TemporaryStacksDontYieldNoData);
  testMethod(testCreateEnterNodeTraversalEvent_VertexStateSpecialCases);
  testMethod(testCreateEnterNodeTraversalEvent_CellNoDataAndHasSplitOverride);
  testMethod(testCreateLeaveNodeTraversalEvent_DeleteOnInOut);
  testMethod(testCreateLeaveNodeTraversalEvent_RecomputesHangingFacesAfterDFS);
  testMethod(testCreateLeaveNodeTraversalEvent_TemporaryStacksDontYieldNoData);
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateEnterNodeTraversalEvent_AllLocalPersistent() {
  const int treeId = 1;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
    generator,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );

  validate(event.getIsCellLocal());

  const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
    const int                     vertexPosition = static_cast<int>(vertexIndex.to_ulong());

    validateEqualsWithParams2(event.getVertexDataTo(i), vertexPosition, i, vertexPosition);
    validate(event.getIsVertexLocal(vertexPosition));

    const int stackNumber = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
    validateEqualsWithParams2(event.getVertexDataFrom(i), stackNumber, i, stackNumber);
  }

  for (int i = 0; i < TWO_TIMES_D; ++i) {
    const int faceIndex   = PeanoCurve::getFaceNumberAlongCurve(state, i);
    const int stackNumber = PeanoCurve::getFaceReadStackNumber(state, faceIndex);

    validateEqualsWithParams2(event.getFaceDataTo(i), faceIndex, i, faceIndex);
    validate(event.getIsFaceLocal(faceIndex));
    validateEqualsWithParams2(event.getFaceDataFrom(i), stackNumber, i, stackNumber);
  }

  const int cellStack = PeanoCurve::getCellReadStackNumber(state);
  validateEquals(event.getCellData(), cellStack);
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateEnterNodeTraversalEvent_VertexBoundaryImpliesLocalInFastPath() {
  const int treeId = 1;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  // Craft a vertex whose current adjacency is non-local while its backup adjacency
  // mixes local and remote ranks. This exercises the fast-path invariant that every
  // reported boundary vertex is local.
  fineGridVertices[0].setState(GridVertex::State::Unrefined);
  for (int j = 0; j < TWO_POWER_D; ++j) {
    fineGridVertices[0].adjacentRanks[j]         = 2;
    fineGridVertices[0].backupOfAdjacentRanks[j] = (j % 2 == 0) ? treeId : 2;
  }

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
    generator,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true, // noParallelTopologyChanges
    true
  );

  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateWithParams2(
      not event.getIsVertexAdjacentToParallelDomainBoundary(i) or event.getIsVertexLocal(i),
      i,
      event.toString()
    );
  }
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateEnterNodeTraversalEvent_InOutNoDataAndHasSplitOverride() {
  const int treeId     = 1;
  const int remoteRank = 2;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  // Choose a vertex along the curve whose read stack is an in/out stack.
  int                     chosenVertexI     = -1;
  int                     chosenVertexPos   = -1;
  int                     chosenVertexStack = -1;
  std::bitset<DIMENSIONS> chosenVertexIndex;
  {
    const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
      const int                     stack = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
      if (StackTopology::isInOutStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        chosenVertexStack = stack;
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);
  validateEquals(chosenVertexPos, static_cast<int>(chosenVertexIndex.to_ulong()));
  validateEquals(chosenVertexStack, PeanoCurve::getVertexReadStackNumber(state, chosenVertexIndex));
  validate(StackTopology::isInOutStack(chosenVertexStack));

  // Make the chosen vertex non-local.
  setAllAdjacentRanks(fineGridVertices[chosenVertexPos], remoteRank);

  // Choose a face along the curve whose read stack is an in/out stack.
  int chosenFaceI     = -1;
  int chosenFaceIndex = -1;
  int chosenFaceStack = -1;
  {
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
      const int stack     = PeanoCurve::getFaceReadStackNumber(state, faceIndex);
      if (StackTopology::isInOutStack(stack)) {
        chosenFaceI     = i;
        chosenFaceIndex = faceIndex;
        chosenFaceStack = stack;
        break;
      }
    }
  }
  validate(chosenFaceI >= 0);

  // Make the chosen face non-local by changing the adjacency entries that define that face.
  setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;

  // Without hasSplit, in/out + non-local yields NoData.
  {
    const peano4::grid::TreeIdSet hasSplit;
    const GridTraversalEvent      event = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not event.getIsVertexLocal(chosenVertexPos));
    validateEquals(event.getVertexDataFrom(chosenVertexI), GridTraversalObserver::NoData);

    validate(not event.getIsFaceLocal(chosenFaceIndex));
    validateEquals(event.getFaceDataFrom(chosenFaceI), GridTraversalObserver::NoData);
  }

  // With hasSplit containing the remote owner, the generator must keep the stack id.
  {
    const peano4::grid::TreeIdSet hasSplit = {remoteRank};
    const GridTraversalEvent      event    = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validateEquals(event.getVertexDataFrom(chosenVertexI), chosenVertexStack);
    validateEquals(event.getFaceDataFrom(chosenFaceI), chosenFaceStack);
  }
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateEnterNodeTraversalEvent_TemporaryStacksDontYieldNoData() {
  const int treeId     = 1;
  const int remoteRank = 2;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeStateWithTemporaryReadStacks();
  const auto           relPos = makeZeroRelPos();

  // Pick a vertex with a temporary read stack.
  int chosenVertexI     = -1;
  int chosenVertexPos   = -1;
  int chosenVertexStack = -1;
  {
    const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
      const int                     stack = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
      if (StackTopology::isLocalExchangeStack(stack)) {
        chosenVertexI     = i;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        chosenVertexStack = stack;
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);

  // Pick a face with a temporary read stack.
  int chosenFaceI     = -1;
  int chosenFaceIndex = -1;
  int chosenFaceStack = -1;
  {
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
      const int stack     = PeanoCurve::getFaceReadStackNumber(state, faceIndex);
      if (StackTopology::isLocalExchangeStack(stack)) {
        chosenFaceI     = i;
        chosenFaceIndex = faceIndex;
        chosenFaceStack = stack;
        break;
      }
    }
  }
  validate(chosenFaceI >= 0);

  // Make chosen vertex and face non-local.
  setAllAdjacentRanks(fineGridVertices[chosenVertexPos], remoteRank);
  setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
    generator,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );

  // Temporary stacks must not be reset to NoData even if non-local.
  validate(not event.getIsVertexLocal(chosenVertexPos));
  validateEquals(event.getVertexDataFrom(chosenVertexI), chosenVertexStack);

  validate(not event.getIsFaceLocal(chosenFaceIndex));
  validateEquals(event.getFaceDataFrom(chosenFaceI), chosenFaceStack);
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateEnterNodeTraversalEvent_VertexStateSpecialCases() {
  const int treeId = 1;

  GridTraversalEventGenerator generator(treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  // Select a deterministic vertex along the curve (i=0).
  auto pickVertexAtI0 =
    [](const AutomatonState& state, int& outVertexI, int& outVertexPos, std::bitset<DIMENSIONS>& outVertexIndex) {
      const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
      outVertexI                                = 0;
      outVertexIndex                            = (coordinates ^ std::bitset<DIMENSIONS>(0));
      outVertexPos                              = static_cast<int>(outVertexIndex.to_ulong());
    };

  // (1) HangingVertex => CreateOrDestroyGridEntity + hanging marker set
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    const AutomatonState state  = makeSimpleState();
    const auto           relPos = makeZeroRelPos();

    int                     vertexI, vertexPos;
    std::bitset<DIMENSIONS> vertexIndex;
    pickVertexAtI0(state, vertexI, vertexPos, vertexIndex);

    fineGridVertices[vertexPos].setState(GridVertex::State::HangingVertex);

    const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(event.getIsVertexHanging(vertexPos));
    validateEquals(event.getVertexDataFrom(vertexI), GridTraversalObserver::CreateOrDestroyGridEntity);
  }

  // (2) New + in/out stack => CreateOrDestroyGridEntity
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    const AutomatonState state  = makeSimpleState();
    const auto           relPos = makeZeroRelPos();

    int                     vertexI, vertexPos;
    std::bitset<DIMENSIONS> vertexIndex;
    pickVertexAtI0(state, vertexI, vertexPos, vertexIndex);

    const int stackNumber = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
    validate(StackTopology::isInOutStack(stackNumber)); // This subtest targets the in/out path

    fineGridVertices[vertexPos].setState(GridVertex::State::New);

    const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validateEquals(event.getVertexDataFrom(vertexI), GridTraversalObserver::CreateOrDestroyGridEntity);
  }

  // (3) New + temporary stack => stack number (not CreateOrDestroyGridEntity)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    const AutomatonState state  = makeStateWithTemporaryReadStacks();
    const auto           relPos = makeZeroRelPos();

    int                     vertexI, vertexPos;
    std::bitset<DIMENSIONS> vertexIndex;
    pickVertexAtI0(state, vertexI, vertexPos, vertexIndex);

    const int stackNumber = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
    validate(StackTopology::isLocalExchangeStack(stackNumber)); // This subtest targets the local stack path

    fineGridVertices[vertexPos].setState(GridVertex::State::New);

    const GridTraversalEvent event = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validateEquals(event.getVertexDataFrom(vertexI), stackNumber);
  }
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateEnterNodeTraversalEvent_CellNoDataAndHasSplitOverride() {
  const int treeId     = 1;
  const int remoteRank = 2;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  // Make the whole cell non-local (for isSpacetreeNodeLocal with splitting/joining treated as local).
  for (int v = 0; v < TWO_POWER_D; ++v) {
    setAllAdjacentRanks(fineGridVertices[v], remoteRank);
  }

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;

  // Without hasSplit: in/out cell stack + non-local => NoData.
  {
    const peano4::grid::TreeIdSet hasSplit;
    const GridTraversalEvent      event = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not event.getIsCellLocal());
    validateEquals(event.getCellData(), GridTraversalObserver::NoData);
  }

  // With hasSplit containing the owning tree => keep the cell stack id.
  {
    const peano4::grid::TreeIdSet hasSplit = {remoteRank};
    const GridTraversalEvent      event    = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not event.getIsCellLocal());
    const int cellStack = PeanoCurve::getCellReadStackNumber(state);
    validateEquals(event.getCellData(), cellStack);
  }
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateLeaveNodeTraversalEvent_DeleteOnInOut() {
  const int treeId     = 1;
  const int remoteRank = 2;

  GridTraversalEventGenerator generator(treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  // Pick a vertex along the curve where the write stack is an in/out stack.
  int                     chosenVertexI   = -1;
  int                     chosenVertexPos = -1;
  std::bitset<DIMENSIONS> chosenVertexIndex;
  {
    const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
      const int                     stack = PeanoCurve::getVertexWriteStackNumber(state, vertexIndex);
      if (StackTopology::isInOutStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);
  validateEquals(chosenVertexPos, static_cast<int>(chosenVertexIndex.to_ulong()));
  validate(StackTopology::isInOutStack(PeanoCurve::getVertexWriteStackNumber(state, chosenVertexIndex)));

  // (A) Local vertex + Delete + in/out => CreateOrDestroyGridEntity
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Delete);

    const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );
    const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
      generator,
      enter,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::CreateOrDestroyGridEntity);
  }

  // (B) Non-local vertex + Unrefined + in/out => NoData (filtered by in/out + not local check)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    setAllAdjacentRanks(fineGridVertices[chosenVertexPos], remoteRank);
    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Unrefined);

    const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );
    const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
      generator,
      enter,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not leave.getIsVertexLocal(chosenVertexPos));
    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::NoData);
  }

  // (C) Non-local vertex + Delete + in/out => NoData (not filtered, as the filter checks in/out stacks)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    setAllAdjacentRanks(fineGridVertices[chosenVertexPos], remoteRank);
    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Delete);

    const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );
    const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
      generator,
      enter,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not leave.getIsVertexLocal(chosenVertexPos));
    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::NoData);
  }

  // (D) Face: in/out write stack + non-local => NoData (early-out in leave event)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    int chosenFaceI     = -1;
    int chosenFaceIndex = -1;
    {
      for (int i = 0; i < TWO_TIMES_D; ++i) {
        const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
        const int stack     = PeanoCurve::getFaceWriteStackNumber(state, faceIndex);
        if (StackTopology::isInOutStack(stack)) {
          chosenFaceI     = i;
          chosenFaceIndex = faceIndex;
          break;
        }
      }
    }
    validate(chosenFaceI >= 0);

    setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

    const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
      generator,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );
    const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
      generator,
      enter,
      coarseGridVertices,
      fineGridVertices,
      vertexTypes,
      state,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      hasSplit,
      relPos,
      true,
      true
    );

    validate(not leave.getIsFaceLocal(chosenFaceIndex));
    validateEquals(leave.getFaceDataTo(chosenFaceI), GridTraversalObserver::NoData);
  }
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateLeaveNodeTraversalEvent_RecomputesHangingFacesAfterDFS() {
  const int treeId = 1;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();
  const auto&          lut    = createVertexLookupTable(state, relPos);

  const int chosenFaceIndex = 0;
  int       chosenFaceI     = -1;
  for (int i = 0; i < TWO_TIMES_D; ++i) {
    if (lut.faceIndex[i] == chosenFaceIndex) {
      chosenFaceI = i;
      break;
    }
  }
  validate(chosenFaceI >= 0);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const auto& faceTable = SpacetreeHelper::getFaceAdjTables();
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int v    = faceTable.faceVerts[chosenFaceIndex][k];
    vertexTypes[v] = VertexType::Hanging;
    fineGridVertices[v].setState(GridVertex::State::HangingVertex);
  }

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
    generator,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );
  validate(enter.getIsFaceHanging(chosenFaceIndex));

  // The vertex types are precomputed before the descent. Flipping the vertex states behind
  // them mimics AMR effects within the depth-first sweep: on leave, the face is no longer
  // hanging even though the precomputed types still say it is.
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int v = faceTable.faceVerts[chosenFaceIndex][k];
    fineGridVertices[v].setState(GridVertex::State::Unrefined);
  }

  const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
    generator,
    enter,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );

  validate(leave.getIsFaceHanging(chosenFaceIndex));
  validateEquals(leave.getFaceDataTo(chosenFaceI), GridTraversalObserver::CreateOrDestroyGridEntity);
}

void peano4::grid::tests::GridTraversalEventGeneratorTest::
  testCreateLeaveNodeTraversalEvent_TemporaryStacksDontYieldNoData() {
  const int treeId     = 1;
  const int remoteRank = 2;

  GridTraversalEventGenerator generator(treeId);

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  setAllVerticesLocal(coarseGridVertices, treeId);
  setAllVerticesLocal(fineGridVertices, treeId);

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeStateWithTemporaryWriteStacks();
  const auto           relPos = makeZeroRelPos();

  // Choose a vertex with a temporary write stack.
  int                     chosenVertexI     = -1;
  int                     chosenVertexPos   = -1;
  int                     chosenVertexStack = -1;
  std::bitset<DIMENSIONS> chosenVertexIndex;
  {
    const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
      const int                     stack = PeanoCurve::getVertexWriteStackNumber(state, vertexIndex);
      if (StackTopology::isLocalExchangeStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        chosenVertexStack = stack;
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);
  validateEquals(chosenVertexPos, static_cast<int>(chosenVertexIndex.to_ulong()));
  validateEquals(chosenVertexStack, PeanoCurve::getVertexWriteStackNumber(state, chosenVertexIndex));
  validate(StackTopology::isLocalExchangeStack(chosenVertexStack));

  // Choose a face with a temporary write stack.
  int chosenFaceI     = -1;
  int chosenFaceIndex = -1;
  int chosenFaceStack = -1;
  {
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
      const int stack     = PeanoCurve::getFaceWriteStackNumber(state, faceIndex);
      if (StackTopology::isLocalExchangeStack(stack)) {
        chosenFaceI     = i;
        chosenFaceIndex = faceIndex;
        chosenFaceStack = stack;
        break;
      }
    }
  }
  validate(chosenFaceI >= 0);

  // Make vertex and face non-local.
  setAllAdjacentRanks(fineGridVertices[chosenVertexPos], remoteRank);
  setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

  // Vertex state must be one of the ones that writes to stackNumber (not Delete/Hanging).
  fineGridVertices[chosenVertexPos].setState(GridVertex::State::Unrefined);

  const SplitSpecification      splitTriggered;
  const peano4::grid::TreeIdSet splitting;
  const peano4::grid::TreeIdSet joinTriggered;
  const peano4::grid::TreeIdSet joining;
  const peano4::grid::TreeIdSet hasSplit;

  const GridTraversalEvent enter = callCreateEnterNodeTraversalEvent(
    generator,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );
  const GridTraversalEvent leave = callCreateLeaveNodeTraversalEvent(
    generator,
    enter,
    coarseGridVertices,
    fineGridVertices,
    vertexTypes,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    hasSplit,
    relPos,
    true,
    true
  );

  // Temporary stacks must not be turned into NoData for non-local vertices/faces.
  validate(not leave.getIsVertexLocal(chosenVertexPos));
  validateEquals(leave.getVertexDataTo(chosenVertexI), chosenVertexStack);

  validate(not leave.getIsFaceLocal(chosenFaceIndex));
  validateEquals(leave.getFaceDataTo(chosenFaceI), chosenFaceStack);
}
