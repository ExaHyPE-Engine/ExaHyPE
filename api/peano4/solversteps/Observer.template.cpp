// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/DataRepository.h"

#include "peano4/grid/StackTopology.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"

#include "tarch/Assertions.h"

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
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.beginTraversal();
{%- endfor %}

  // Fill call stacks with dummies which represent level 0 such that we can call
  // standard action routines on level 1 with parents. Without these dummies, a
  // top(1) call raises an assertion.
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for cell in CELLS %}
  auto dummyParent{{ cell.logical_type_name }}Block = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(1);
  dummyParent{{ cell.logical_type_name }}Block.emplaceDefault(0);
{%- endfor %}
{%- for face in FACES %}
  auto dummyParent{{ face.logical_type_name }}Block = repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(TWO_TIMES_D);
  for (int faceIndex = 0; faceIndex < TWO_TIMES_D; faceIndex++) {
    dummyParent{{ face.logical_type_name }}Block.emplaceDefault(faceIndex);
  }
{%- endfor %}
{%- for vertex in VERTICES %}
  auto dummyParent{{ vertex.logical_type_name }}Block = repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(TWO_POWER_D);
  for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; vertexIndex++) {
    dummyParent{{ vertex.logical_type_name }}Block.emplaceDefault(vertexIndex);
  }
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.endTraversal();
{%- endfor %}

  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(1);
{%- endfor %}
{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(TWO_TIMES_D);
{%- endfor %}
{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(TWO_POWER_D);
{%- endfor %}
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

{% include "Observer_EnterNode.template.cpp" %}
{% include "Observer_LeaveNode.template.cpp" %}

{% include "Observer_LoadNodeData.template.cpp" %}
{% include "Observer_StoreNodeData.template.cpp" %}

{% include "Observer_ReceiveNodeData.template.cpp" %}
