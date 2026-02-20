// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEventGeneratorTest.h"

#include "peano4/grid/GridTraversalEventGenerator.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/VertexType.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <set>

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

  void setAllAdj(peano4::grid::GridVertex& v, int r) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      v.adjacentRanks[i]         = r;
      v.backupOfAdjacentRanks[i] = r;
    }
  }

  void setAllVerticesLocal(peano4::grid::GridVertex vertices[TWO_POWER_D], int treeId) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      vertices[v].setState(peano4::grid::GridVertex::State::Unrefined);
      setAllAdj(vertices[v], treeId);
    }
  }

  void setFaceVerticesNonLocal(peano4::grid::GridVertex vertices[TWO_POWER_D], int faceIndex, int remoteRank) {
    const auto& T      = peano4::grid::SpacetreeHelper::getFaceAdjTables();
    const int   normal = faceIndex % DIMENSIONS;
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int vIdx                           = T.faceVerts[faceIndex][k];
      const int i0                             = T.adjIdx0[normal][vIdx];
      const int i1                             = T.adjIdx1[normal][vIdx];
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
} // namespace

peano4::grid::tests::GridTraversalEventGeneratorTest::GridTraversalEventGeneratorTest():
  TestCase("peano4::grid::tests::GridTraversalEventGeneratorTest") {}

void peano4::grid::tests::GridTraversalEventGeneratorTest::run() {
  testMethod(testCreateEnterNodeTraversalEvent_AllLocalPersistent);
  testMethod(testCreateEnterNodeTraversalEvent_InOutNoDataAndHasSplitOverride);
  testMethod(testCreateEnterNodeTraversalEvent_TemporaryStacksDontYieldNoData);
  testMethod(testCreateEnterNodeTraversalEvent_VertexStateSpecialCases);
  testMethod(testCreateEnterNodeTraversalEvent_CellNoDataAndHasSplitOverride);
  testMethod(testCreateLeaveNodeTraversalEvent_DeleteOnInOut);
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

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;
  const std::set<int>      hasSplit;

  const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
      if (PeanoCurve::isInOutStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        chosenVertexStack = stack;
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);

  // Make the chosen vertex non-local.
  setAllAdj(fineGridVertices[chosenVertexPos], remoteRank);

  // Choose a face along the curve whose read stack is an in/out stack.
  int chosenFaceI     = -1;
  int chosenFaceIndex = -1;
  int chosenFaceStack = -1;
  {
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
      const int stack     = PeanoCurve::getFaceReadStackNumber(state, faceIndex);
      if (PeanoCurve::isInOutStack(stack)) {
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

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;

  // Without hasSplit, in/out + non-local yields NoData.
  {
    const std::set<int>      hasSplit;
    const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
      true
    );

    validate(not event.getIsVertexLocal(chosenVertexPos));
    validateEquals(event.getVertexDataFrom(chosenVertexI), GridTraversalObserver::NoData);

    validate(not event.getIsFaceLocal(chosenFaceIndex));
    validateEquals(event.getFaceDataFrom(chosenFaceI), GridTraversalObserver::NoData);
  }

  // With hasSplit containing the remote owner, the generator must keep the stack id.
  {
    const std::set<int>      hasSplit = {remoteRank};
    const GridTraversalEvent event    = generator.createEnterNodeTraversalEvent(
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
      if (PeanoCurve::isTemporaryStack(stack)) {
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
      if (PeanoCurve::isTemporaryStack(stack)) {
        chosenFaceI     = i;
        chosenFaceIndex = faceIndex;
        chosenFaceStack = stack;
        break;
      }
    }
  }
  validate(chosenFaceI >= 0);

  // Make chosen vertex and face non-local.
  setAllAdj(fineGridVertices[chosenVertexPos], remoteRank);
  setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;
  const std::set<int>      hasSplit;

  const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;
  const std::set<int>      hasSplit;

  // Select a deterministic vertex along the curve (i=0).
  auto pickVertexAtI0 =
    [](const AutomatonState& state, int& outVertexI, int& outVertexPos, std::bitset<DIMENSIONS>& outVertexIndex) {
      const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
      outVertexI                                = 0;
      outVertexIndex                            = (coordinates ^ std::bitset<DIMENSIONS>(0));
      outVertexPos                              = static_cast<int>(outVertexIndex.to_ulong());
    };

  // (1) HangingVertex => CreateOrDestroyHangingGridEntity
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

    const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
      true
    );

    validateEquals(event.getVertexDataFrom(vertexI), GridTraversalObserver::CreateOrDestroyHangingGridEntity);
  }

  // (2) New + in/out stack => CreateOrDestroyPersistentGridEntity
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
    validate(PeanoCurve::isInOutStack(stackNumber)); // this subtest targets the in/out path

    fineGridVertices[vertexPos].setState(GridVertex::State::New);

    const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
      true
    );

    validateEquals(event.getVertexDataFrom(vertexI), GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
  }

  // (3) New + temporary stack => stack number (not CreateOrDestroyPersistentGridEntity)
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
    validate(PeanoCurve::isTemporaryStack(stackNumber)); // this subtest targets the temporary-stack path

    fineGridVertices[vertexPos].setState(GridVertex::State::New);

    const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
    setAllAdj(fineGridVertices[v], remoteRank);
  }

  VertexType vertexTypes[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexTypes[i] = VertexType::Persistent;
  }

  const AutomatonState state  = makeSimpleState();
  const auto           relPos = makeZeroRelPos();

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;

  // Without hasSplit: in/out cell stack + non-local => NoData.
  {
    const std::set<int>      hasSplit;
    const GridTraversalEvent event = generator.createEnterNodeTraversalEvent(
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
      true
    );

    validate(not event.getIsCellLocal());
    validateEquals(event.getCellData(), GridTraversalObserver::NoData);
  }

  // With hasSplit containing the owning tree => keep the cell stack id.
  {
    const std::set<int>      hasSplit = {remoteRank};
    const GridTraversalEvent event    = generator.createEnterNodeTraversalEvent(
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

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;
  const std::set<int>      hasSplit;

  // Pick a vertex along the curve where the write stack is an in/out stack.
  int                     chosenVertexI   = -1;
  int                     chosenVertexPos = -1;
  std::bitset<DIMENSIONS> chosenVertexIndex;
  {
    const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
      const int                     stack = PeanoCurve::getVertexWriteStackNumber(state, vertexIndex);
      if (PeanoCurve::isInOutStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);

  // (A) Local vertex + Delete + in/out => CreateOrDestroyPersistentGridEntity
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Delete);

    const GridTraversalEvent enter = generator.createEnterNodeTraversalEvent(
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
      true
    );
    const GridTraversalEvent leave = generator.createLeaveNodeTraversalEvent(
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
      true
    );

    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
  }

  // (B) Non-local vertex + Unrefined + in/out => NoData (filtered by in/out + not local check)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    setAllAdj(fineGridVertices[chosenVertexPos], remoteRank);
    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Unrefined);

    const GridTraversalEvent enter = generator.createEnterNodeTraversalEvent(
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
      true
    );
    const GridTraversalEvent leave = generator.createLeaveNodeTraversalEvent(
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
      true
    );

    validate(not leave.getIsVertexLocal(chosenVertexPos));
    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::NoData);
  }

  // (C) Non-local vertex + Delete + in/out => CreateOrDestroyPersistentGridEntity (not filtered, as the filter checks
  // in/out stacks)
  {
    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];
    setAllVerticesLocal(coarseGridVertices, treeId);
    setAllVerticesLocal(fineGridVertices, treeId);

    setAllAdj(fineGridVertices[chosenVertexPos], remoteRank);
    fineGridVertices[chosenVertexPos].setState(GridVertex::State::Delete);

    const GridTraversalEvent enter = generator.createEnterNodeTraversalEvent(
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
      true
    );
    const GridTraversalEvent leave = generator.createLeaveNodeTraversalEvent(
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
      true
    );

    validate(not leave.getIsVertexLocal(chosenVertexPos));
    validateEquals(leave.getVertexDataTo(chosenVertexI), GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
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
        if (PeanoCurve::isInOutStack(stack)) {
          chosenFaceI     = i;
          chosenFaceIndex = faceIndex;
          break;
        }
      }
    }
    validate(chosenFaceI >= 0);

    setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

    const GridTraversalEvent enter = generator.createEnterNodeTraversalEvent(
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
      true
    );
    const GridTraversalEvent leave = generator.createLeaveNodeTraversalEvent(
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
      true
    );

    validate(not leave.getIsFaceLocal(chosenFaceIndex));
    validateEquals(leave.getFaceDataTo(chosenFaceI), GridTraversalObserver::NoData);
  }
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
      if (PeanoCurve::isTemporaryStack(stack)) {
        chosenVertexI     = i;
        chosenVertexIndex = vertexIndex;
        chosenVertexPos   = static_cast<int>(vertexIndex.to_ulong());
        chosenVertexStack = stack;
        break;
      }
    }
  }
  validate(chosenVertexI >= 0);

  // Choose a face with a temporary write stack.
  int chosenFaceI     = -1;
  int chosenFaceIndex = -1;
  int chosenFaceStack = -1;
  {
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex = PeanoCurve::getFaceNumberAlongCurve(state, i);
      const int stack     = PeanoCurve::getFaceWriteStackNumber(state, faceIndex);
      if (PeanoCurve::isTemporaryStack(stack)) {
        chosenFaceI     = i;
        chosenFaceIndex = faceIndex;
        chosenFaceStack = stack;
        break;
      }
    }
  }
  validate(chosenFaceI >= 0);

  // Make vertex and face non-local.
  setAllAdj(fineGridVertices[chosenVertexPos], remoteRank);
  setFaceVerticesNonLocal(fineGridVertices, chosenFaceIndex, remoteRank);

  // Vertex state must be one of the ones that writes to stackNumber (not Delete/Hanging).
  fineGridVertices[chosenVertexPos].setState(GridVertex::State::Unrefined);

  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;
  const std::set<int>      hasSplit;

  const GridTraversalEvent enter = generator.createEnterNodeTraversalEvent(
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
    true
  );
  const GridTraversalEvent leave = generator.createLeaveNodeTraversalEvent(
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
    true
  );

  // Temporary stacks must not be turned into NoData for non-local vertices/faces.
  validate(not leave.getIsVertexLocal(chosenVertexPos));
  validateEquals(leave.getVertexDataTo(chosenVertexI), chosenVertexStack);

  validate(not leave.getIsFaceLocal(chosenFaceIndex));
  validateEquals(leave.getFaceDataTo(chosenFaceI), chosenFaceStack);
}
