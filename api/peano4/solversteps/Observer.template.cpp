// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/DataRepository.h"

#include "peano4/grid/SendReceiveContext.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "peano4/dataview/VertexMarker.h"

#include "tarch/Assertions.h"

namespace {
  /**
   * Blanks one pool slot. A pooled entity keeps its chunk and only loses its
   * values; every other record is replaced.
   */
  template <typename Container>
  inline void clearSlot(Container& container, const int slot) {
    using ElementType = typename Container::ElementType;
    if constexpr (requires(ElementType& element) { element.clearValues(); }) {
      container.getPoolSlot(slot).clearValues();
    } else {
      container.getPoolSlot(slot) = ElementType{};
    }
  }
} // namespace

/**
 * @param treeId TreePrototype creates the prototype. The rank id names the spine
 *   observer, and subsequent ids name subtree-task observers.
 */
{{FULL_QUALIFIED_CLASSNAME}}::{{CLASSNAME}}(int treeId):
  _treeId(treeId)
{%- for action_set in ACTIVE_ACTION_SETS %}
  , {{ action_set.name }}(treeId)
{%- endfor %}
  {}

::peano4::grid::GridTraversalObserver* {{FULL_QUALIFIED_CLASSNAME}}::clone(int treeId) {
  return new {{CLASSNAME}}(treeId);
}

std::vector<::peano4::grid::GridControlEvent> {{FULL_QUALIFIED_CLASSNAME}}::getGridControlEvents() const {
{%- set numberOfActionSets = ACTIVE_ACTION_SETS | length %}

{%- if numberOfActionSets == 0 %}
  return {};
{%- elif numberOfActionSets == 1 %}
{%- for action_set in ACTIVE_ACTION_SETS %}
  return {{ action_set.name }}.getGridControlEvents();
{%- endfor %}
{%- else %}
{%- for action_set in ACTIVE_ACTION_SETS %}
  auto eventsOfActionSet{{ loop.index0 }} = {{ action_set.name }}.getGridControlEvents();
{%- endfor %}

  std::size_t totalNumberOfEvents = 0;
{%- for _ in ACTIVE_ACTION_SETS %}
  totalNumberOfEvents += eventsOfActionSet{{ loop.index0 }}.size();
{%- endfor %}

  std::vector<::peano4::grid::GridControlEvent> result;
  result.reserve(totalNumberOfEvents);

{%- for _ in ACTIVE_ACTION_SETS %}
  result.insert(
    result.end(),
    std::make_move_iterator(eventsOfActionSet{{ loop.index0 }}.begin()),
    std::make_move_iterator(eventsOfActionSet{{ loop.index0 }}.end())
  );
{%- endfor %}

  return result;
{%- endif %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::beginTraversal(
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.beginTraversal();
{%- endfor %}

  // The root has no parent, so the path starts with an entry whose positions
  // carry no entity. They resolve to the containers' sentinels, so an action
  // set on level one sees the sentinels as its coarse grid.
  _activePath.clear();
  _activePath.reserve(32);
  PathEntry rootParent;
  rootParent.vertexSlots.fill(-1);
  rootParent.faceSlots.fill(-1);
  rootParent.cellSlot.fill(-1);
  _activePath.push_back(rootParent);
}

void {{FULL_QUALIFIED_CLASSNAME}}::endTraversal(
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.endTraversal();
{%- endfor %}

  _activePath.clear();
}

void {{FULL_QUALIFIED_CLASSNAME}}::prepareTraversal() {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.full_qualified_name }}::prepareTraversal();
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::unprepareTraversal() {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.full_qualified_name }}::unprepareTraversal();
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::enterNode(const ::peano4::grid::GridTraversalEvent& event) {
  // The cell joins the active path, so the entry below it is its parent.
  PathEntry& fineGridSlots = _activePath.emplace_back();
  fineGridSlots.vertexSlots = event.getVertexPoolIndices();
  fineGridSlots.faceSlots   = event.getFacePoolIndices();
  fineGridSlots.cellSlot[0] = event.getCellPoolIndex();
  const PathEntry& coarseGridSlots = _activePath[_activePath.size() - 2];

{%- for vertex in VERTICES %}
  ::peano4::dataview::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getPoolData(),
    fineGridSlots.vertexSlots.data(),
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getSentinelSlot()
  );
  ::peano4::dataview::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getPoolData(),
    coarseGridSlots.vertexSlots.data(),
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

{%- for face in FACES %}
  ::peano4::dataview::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getPoolData(),
    fineGridSlots.faceSlots.data(),
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getSentinelSlot()
  );
  ::peano4::dataview::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getPoolData(),
    coarseGridSlots.faceSlots.data(),
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

{%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getPoolSlot(
    fineGridSlots.cellSlot[0] >= 0
      ? fineGridSlots.cellSlot[0]
      : repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getSentinelSlot()
  );
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getPoolSlot(
    coarseGridSlots.cellSlot[0] >= 0
      ? coarseGridSlots.cellSlot[0]
      : repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

  // An entity whose data does not persist at this point of the grid hierarchy
  // has to reach the action sets blank, exactly as a freshly created one does.
  // A repeat touch keeps what the earlier toucher of this sweep left.
{%- if VERTICES %}
  {
    auto guardMarker = ::peano4::dataview::VertexMarker(event);
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
      const auto slot       = fineGridSlots.vertexSlots[vertexNumber];
      const auto inputStack = event.getVertexDataFrom(vertexNumber);
      const auto isCreated  = inputStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
      if (slot < 0 or (not isCreated and not ::peano4::grid::StackTopology::isInOutStack(inputStack))) {
        continue;
      }
      guardMarker.select(event.getVertexDataTo(vertexNumber));
{%- for vertex in VERTICES %}
      if (isCreated or not {{ vertex.full_qualified_type }}::load(guardMarker)) {
        clearSlot(repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer, slot);
      }
{%- endfor %}
    }
  }
{%- endif %}
{%- if FACES %}
  {
    auto guardMarker = event.getFaceMarker();
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      const auto slot       = fineGridSlots.faceSlots[faceNumber];
      const auto inputStack = event.getFaceDataFrom(faceNumber);
      const auto isCreated  = inputStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
      if (slot < 0 or (not isCreated and not ::peano4::grid::StackTopology::isInOutStack(inputStack))) {
        continue;
      }
      guardMarker.select(event.getFaceDataTo(faceNumber));
{%- for face in FACES %}
      if (isCreated or not {{ face.full_qualified_type }}::load(guardMarker)) {
        clearSlot(repositories::DataRepository::{{ face.logical_type_name }}StackContainer, slot);
      }
{%- endfor %}
    }
  }
{%- endif %}
{%- if CELLS %}
  {
    const auto inputStack  = event.getCellData();
    const auto isCreated   = inputStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto guardMarker = event.getCellMarker();
    if (fineGridSlots.cellSlot[0] >= 0
        and (isCreated or ::peano4::grid::StackTopology::isInOutStack(inputStack))) {
{%- for cell in CELLS %}
      if (isCreated or not {{ cell.full_qualified_type }}::load(guardMarker)) {
        clearSlot(repositories::DataRepository::{{ cell.logical_type_name }}StackContainer, fineGridSlots.cellSlot[0]);
      }
{%- endfor %}
    }
  }
{%- endif %}

{%- if VERTICES %}
  auto vertexMarker = ::peano4::dataview::VertexMarker(event);
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    const auto inVertexStack = event.getVertexDataFrom(vertexNumber);
    const auto pick          = event.getVertexDataTo(vertexNumber);
    vertexMarker.select(pick);

    const auto isLocal = vertexMarker.isLocal();
    if (not isLocal) {
      continue;
    }

    const auto isCreateOrDestroy = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = vertexMarker.isHanging();
    const auto isInOut           = ::peano4::grid::StackTopology::isInOutStack(inVertexStack);

    if (isCreateOrDestroy and not isHanging) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
    if (isHanging and (isCreateOrDestroy or isInOut)) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchHangingVertexFirstTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateOrDestroy or isInOut) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchVertexFirstTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

{%- if FACES %}
  auto faceMarker = event.getFaceMarker();
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const auto inFaceStack = event.getFaceDataFrom(faceNumber);
    const auto pick        = event.getFaceDataTo(faceNumber);
    faceMarker.select(pick);

    const auto isLocal = faceMarker.isLocal();
    if (not isLocal) {
      continue;
    }

    const auto isCreateOrDestroy = inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = faceMarker.isHanging();
    const auto isInOut           = ::peano4::grid::StackTopology::isInOutStack(inFaceStack);

    if (isCreateOrDestroy and not isHanging) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
    if (isHanging and (isCreateOrDestroy or isInOut)) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchHangingFaceFirstTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateOrDestroy or isInOut) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchFaceFirstTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

{% if CELLS %}
  const auto cellMarker = event.getCellMarker();
  const auto cellIsLocal = cellMarker.isLocal();
  if (cellIsLocal) {
    const auto cellData          = event.getCellData();
    const auto isCreateOrDestroy = cellData == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    if (isCreateOrDestroy) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createCell(cellMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} fineGrid{{ cell.logical_type_name }}, {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
{%- for action_set in ACTIVE_ACTION_SETS %}
    {{action_set.name}}.touchCellFirstTime(cellMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} fineGrid{{ cell.logical_type_name }}, {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
    );
{%- endfor %}
  }
{%- endif %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {
  const PathEntry& fineGridSlots   = _activePath.back();
  const PathEntry& coarseGridSlots = _activePath[_activePath.size() - 2];

{%- for vertex in VERTICES %}
  ::peano4::dataview::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getPoolData(),
    fineGridSlots.vertexSlots.data(),
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getSentinelSlot()
  );
  ::peano4::dataview::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getPoolData(),
    coarseGridSlots.vertexSlots.data(),
    repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

{%- for face in FACES %}
  ::peano4::dataview::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getPoolData(),
    fineGridSlots.faceSlots.data(),
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getSentinelSlot()
  );
  ::peano4::dataview::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getPoolData(),
    coarseGridSlots.faceSlots.data(),
    repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

{%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getPoolSlot(
    fineGridSlots.cellSlot[0] >= 0
      ? fineGridSlots.cellSlot[0]
      : repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getSentinelSlot()
  );
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getPoolSlot(
    coarseGridSlots.cellSlot[0] >= 0
      ? coarseGridSlots.cellSlot[0]
      : repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getSentinelSlot()
  );
{%- endfor %}

{%- if CELLS %}
  const auto cellMarker = event.getCellMarker();
  const auto cellIsLocal = cellMarker.isLocal();
  if (cellIsLocal) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
    {{action_set.name}}.touchCellLastTime(cellMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} fineGrid{{ cell.logical_type_name }}, {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
    );
{%- endfor %}

    const auto cellData = event.getCellData();
    if (cellData == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyCell(cellMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} fineGrid{{ cell.logical_type_name }}, {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

{%- if FACES %}
  auto faceMarker = event.getFaceMarker();
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const auto outFaceStack = event.getFaceDataTo(faceNumber);
    const auto pick         = event.getFaceDataFrom(faceNumber);
    faceMarker.select(pick);

    const auto isLocal = faceMarker.isLocal();
    if (not isLocal) {
      continue;
    }

    const auto isCreateOrDestroy = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = faceMarker.isHanging();
    const auto isInOut           = ::peano4::grid::StackTopology::isInOutStack(outFaceStack);

    if (isHanging and (isInOut or isCreateOrDestroy)) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchHangingFaceLastTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isInOut or isCreateOrDestroy) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchFaceLastTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
    if (isCreateOrDestroy and not isHanging) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

{%- if VERTICES %}
  auto vertexMarker = ::peano4::dataview::VertexMarker(event);
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    const auto outVertexStack = event.getVertexDataTo(vertexNumber);
    const auto pick           = event.getVertexDataFrom(vertexNumber);
    vertexMarker.select(pick);

    const auto isLocal = vertexMarker.isLocal();
    if (not isLocal) {
      continue;
    }

    const auto isCreateOrDestroy = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = vertexMarker.isHanging();
    const auto isInOut           = ::peano4::grid::StackTopology::isInOutStack(outVertexStack);

    if (isHanging and (isCreateOrDestroy or isInOut)) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchHangingVertexLastTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateOrDestroy or isInOut) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchVertexLastTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
    if (isCreateOrDestroy and not isHanging) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

  _activePath.pop_back();
}

void {{FULL_QUALIFIED_CLASSNAME}}::receiveNodeData(
  ::peano4::grid::GridTraversalObserver::NodeData nodeData,
  int                                             position,
  int                                             fromStack,
  ::peano4::grid::SendReceiveContext              context,
  const ::peano4::grid::GridTraversalEvent&       event
) {
  using NodeData = ::peano4::grid::GridTraversalObserver::NodeData;

  // The live pool slot of the subtree task merges the frozen copy that fromStack
  // addresses, and the context says whether that copy comes from a partner task,
  // from a neighbour rank, or rebuilds the entity slot in a consolidation. The
  // frozen copy stays read-only, so the other partners read it afterwards.
  switch (nodeData) {
  case NodeData::Vertex:
    {
{%- if VERTICES %}
      assertion(event.getVertexPoolIndex(position) >= 0);
      auto marker = ::peano4::dataview::VertexMarker(event, position);
{%- for vertex in VERTICES %}
      {
        auto&       data     = repositories::DataRepository::{{vertex.logical_type_name}}StackContainer.getPoolSlot(event.getVertexPoolIndex(position));
        const auto& incoming = repositories::DataRepository::{{vertex.logical_type_name}}StackContainer.getPoolSlot(fromStack);
        ::peano4::grid::applyIncomingCopy(data, context, marker, _treeId, incoming);
      }
{%- endfor %}
{%- endif %}
    }
    break;
  case NodeData::Face:
    {
{%- if FACES %}
      assertion(event.getFacePoolIndex(position) >= 0);
      auto marker = event.getFaceMarker();
      marker.select(position);
{%- for face in FACES %}
      {
        auto&       data     = repositories::DataRepository::{{face.logical_type_name}}StackContainer.getPoolSlot(event.getFacePoolIndex(position));
        const auto& incoming = repositories::DataRepository::{{face.logical_type_name}}StackContainer.getPoolSlot(fromStack);
        ::peano4::grid::applyIncomingCopy(data, context, marker, _treeId, incoming);
      }
{%- endfor %}
{%- endif %}
    }
    break;
  case NodeData::Cell:
    assertion(false, "Cell data never travels between subtree tasks or ranks.");
    break;
  }
}
