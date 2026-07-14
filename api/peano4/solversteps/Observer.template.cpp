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
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"

{{FULL_QUALIFIED_CLASSNAME}}::{{CLASSNAME}}(int spacetreeId):
  _spacetreeId(spacetreeId)
{%- for action_set in ACTIVE_ACTION_SETS %}
  , {{ action_set.name }}(spacetreeId)
{%- endfor %}
  {}

::peano4::grid::GridTraversalObserver* {{FULL_QUALIFIED_CLASSNAME}}::clone(int spacetreeId) {
  return new {{CLASSNAME}}(spacetreeId);
}

std::vector<::peano4::grid::GridControlEvent> {{FULL_QUALIFIED_CLASSNAME}}::getGridControlEvents() const {
{%- set N = ACTIVE_ACTION_SETS | length %}

{%- if N == 0 %}
  return {};
{%- elif N == 1 %}
{%- for action_set in ACTIVE_ACTION_SETS %}
  return {{ action_set.name }}.getGridControlEvents();
{%- endfor %}
{%- else %}
{%- for action_set in ACTIVE_ACTION_SETS %}
  const auto& v{{ loop.index0 }} = {{ action_set.name }}.getGridControlEvents();
{%- endfor %}

  std::size_t total = 0;
{%- for _ in ACTIVE_ACTION_SETS %}
  total += v{{ loop.index0 }}.size();
{%- endfor %}

  std::vector<::peano4::grid::GridControlEvent> result;
  result.reserve(total);

{%- for _ in ACTIVE_ACTION_SETS %}
  result.insert(result.end(),
                std::make_move_iterator(v{{ loop.index0 }}.begin()),
                std::make_move_iterator(v{{ loop.index0 }}.end()));
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

  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(1);
{%- endfor %}
{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(TWO_TIMES_D);
{%- endfor %}
{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(TWO_POWER_D);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.endTraversal();
{%- endfor %}

  const auto numberOfNeighbourTrees = ::peano4::parallel::SpacetreeSet::getInstance().getSpacetree(_spacetreeId).getNumberOfNeighbourTrees();

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.resizeParallelExchangeStacks(_spacetreeId, numberOfNeighbourTrees);
{%- endfor %}
{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}StackContainer.resizeParallelExchangeStacks(_spacetreeId, numberOfNeighbourTrees);
  // repositories::DataRepository::{{ face.logical_type_name }}HangingStackContainer.resizeParallelExchangeStacks(_spacetreeId, numberOfNeighbourTrees);
{%- endfor %}
{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.resizeParallelExchangeStacks(_spacetreeId, numberOfNeighbourTrees);
  // repositories::DataRepository::{{ vertex.logical_type_name }}HangingStackContainer.resizeParallelExchangeStacks(_spacetreeId, numberOfNeighbourTrees);
{%- endfor %}

  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).popBlock(1);
{%- endfor %}
{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).popBlock(TWO_TIMES_D);
{%- endfor %}
{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).popBlock(TWO_POWER_D);
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

void {{FULL_QUALIFIED_CLASSNAME}}::setLocalTrees() {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::setLocalTrees(repositories::DataRepository::{{cell.logical_type_name}}StackContainer);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::setLocalTrees(repositories::DataRepository::{{face.logical_type_name}}StackContainer);
  ::peano4::parallel::SpacetreeSet::setLocalTrees(repositories::DataRepository::{{face.logical_type_name}}HangingStackContainer);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::setLocalTrees(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer);
  ::peano4::parallel::SpacetreeSet::setLocalTrees(repositories::DataRepository::{{vertex.logical_type_name}}HangingStackContainer);
{%- endfor %}
}

{% include "Observer_EnterNode.template.cpp" %}
{% include "Observer_LeaveNode.template.cpp" %}

{% include "Observer_LoadNodeData.template.cpp" %}
{% include "Observer_StoreNodeData.template.cpp" %}

{% include "Observer_SendNodeData.template.cpp" %}
{% include "Observer_ReceiveNodeData.template.cpp" %}

{% include "Observer_ExchangeNodeData.template.cpp" %}
{% include "Observer_StreamNodeData.template.cpp" %}
