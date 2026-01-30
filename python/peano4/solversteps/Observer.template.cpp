// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{{DATAREPOSITORY_INCLUDES}}

#include "peano4/grid/PeanoCurve.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/Assertions.h"

#include <utility>
#include <iterator>

tarch::logging::Log {{FULL_QUALIFIED_CLASSNAME}}::_log("{{FULL_QUALIFIED_CLASSNAME}}");

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


void {{FULL_QUALIFIED_CLASSNAME}}::prepareTraversal() {
  logTraceIn("prepareTraversal()");
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.full_qualified_name }}::prepareTraversal();
{%- endfor %}
  logTraceOut("prepareTraversal()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::beginTraversal(const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& h) {
  logTraceInWith2Arguments("beginTraversal()", x, h);
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.beginTraversal();
{%- endfor %}

  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPush(key)->pushBlock(1);
{%- endfor %}

{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPush(key)->pushBlock(TWO_TIMES_D);
{%- endfor %}

{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPush(key)->pushBlock(TWO_POWER_D);
{%- endfor %}

  logTraceOutWith2Arguments("beginTraversal()", x, h);
}


void {{FULL_QUALIFIED_CLASSNAME}}::enterNode(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("enterNode()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  ::peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TWO_POWER_D - 1));
  ::peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TWO_POWER_D * 2 - 1));
{%- endfor %}

{%- for face in FACES %}
  ::peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(&repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(key)->top(TWO_TIMES_D - 1));
  ::peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(&repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(key)->top(TWO_TIMES_D * 2 - 1));
{%- endfor %}

{%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(key)->top(0);
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(key)->top(1);
{%- endfor %}

{%- if VERTICES %}
  ::peano4::datamanagement::VertexMarker vertexMarker(event);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const int inVertexStack = event.getVertexDataFrom(i);
    const int pick          = event.getVertexDataTo(i);
    vertexMarker.select(pick);

    const bool isLocal = vertexMarker.isLocal();
    if (not isLocal) { continue; }

    const bool isCreatePersistent = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
    const bool isCreateHanging    = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isInOut            = ::peano4::grid::PeanoCurve::isInOutStack(inVertexStack);

    if (isCreatePersistent) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createPersistentVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );

      {{action_set.name}}.touchVertexFirstTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateHanging) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createHangingVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isInOut) {
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
  ::peano4::datamanagement::FaceMarker faceMarker = event.getFaceMarker();
  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int inFaceStack   = event.getFaceDataFrom(i);
    const int pick          = event.getFaceDataTo(i);
    faceMarker.select(pick);

    const bool isLocal = faceMarker.isLocal();
    if (not isLocal) { continue; }

    const bool isCreatePersistent = inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
    const bool isCreateHanging    = inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isInOut            = ::peano4::grid::PeanoCurve::isInOutStack(inFaceStack);

    if (isCreatePersistent) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createPersistentFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );

      {{action_set.name}}.touchFaceFirstTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateHanging) {
{%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createHangingFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isInOut) {
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
  ::peano4::datamanagement::CellMarker cellMarker = event.getCellMarker();
  const bool cellIsLocal = cellMarker.isLocal();
  if (cellIsLocal) {
    const int cellData = event.getCellData();
    const bool cellCreatePersistent = cellData == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
    if (cellCreatePersistent) {
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

  logTraceOut("enterNode()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("leaveNode()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  ::peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TWO_POWER_D - 1));
  ::peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TWO_POWER_D * 2 - 1));
{%- endfor %}

{%- for face in FACES %}
  ::peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(&repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(key)->top(TWO_TIMES_D - 1));
  ::peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(&repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(key)->top(TWO_TIMES_D * 2 - 1));
{%- endfor %}

{%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(key)->top(0);
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(key)->top(1);
{%- endfor %}

{%- if CELLS %}
  ::peano4::datamanagement::CellMarker cellMarker = event.getCellMarker();
  const bool cellIsLocal = cellMarker.isLocal();
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

    const int cellData = event.getCellData();
    if (cellData == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity) {
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
  ::peano4::datamanagement::FaceMarker faceMarker = event.getFaceMarker();
  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int outFaceStack  = event.getFaceDataTo(i);
    const int pick          = event.getFaceDataFrom(i);
    faceMarker.select(pick);

    const bool isLocal = faceMarker.isLocal();
    if (not isLocal) { continue; }

    const bool isCreatePersistent = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
    const bool isCreateHanging    = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isInOut            = ::peano4::grid::PeanoCurve::isInOutStack(outFaceStack);

    if (isCreatePersistent) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchFaceLastTime(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
      {{action_set.name}}.destroyPersistentFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateHanging) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyHangingFace(faceMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} fineGrid{{ face.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isInOut) {
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
  }
{%- endif %}

{%- if VERTICES %}
  ::peano4::datamanagement::VertexMarker vertexMarker(event);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const int outVertexStack  = event.getVertexDataTo(i);
    const int pick            = event.getVertexDataFrom(i);
    vertexMarker.select(pick);

    const bool isLocal = vertexMarker.isLocal();
    if (not isLocal) { continue; }

    const bool isCreatePersistent = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
    const bool isCreateHanging    = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isInOut            = ::peano4::grid::PeanoCurve::isInOutStack(outVertexStack);

    if (isCreatePersistent) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchVertexLastTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );

      {{action_set.name}}.destroyPersistentVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isCreateHanging) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyHangingVertex(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    } else if (isInOut) {
{%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchVertexLastTime(vertexMarker,
{%- for vertex in VERTICES %} fineGrid{{ vertex.logical_type_name }}(pick), {%- endfor %}
{%- for vertex in VERTICES %} coarseGrid{{ vertex.logical_type_name }}, {%- endfor %}
{%- for face in FACES %} coarseGrid{{ face.logical_type_name }}, {%- endfor %}
{%- for cell in CELLS %} coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }} {%- endfor %}
      );
{%- endfor %}
    }
  }
{%- endif %}

  logTraceOutWith1Argument("leaveNode()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::endTraversal(const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& h) {
  logTraceInWith2Arguments("endTraversal()", x, h);
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.endTraversal();
{%- endfor %}

  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for cell in CELLS %}
  repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(key)->popBlock(1);
{%- endfor %}

{%- for face in FACES %}
  repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(key)->popBlock(TWO_TIMES_D);
{%- endfor %}

{%- for vertex in VERTICES %}
  repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(key)->popBlock(TWO_POWER_D);
{%- endfor %}

  logTraceOutWith2Arguments("endTraversal()", x, h);
}


void {{FULL_QUALIFIED_CLASSNAME}}::unprepareTraversal() {
  logTraceIn("unprepareTraversal()");
{%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.full_qualified_name }}::unprepareTraversal();
{%- endfor %}
  logTraceOut("unprepareTraversal()");
}


{% if VERTICES %}
void {{FULL_QUALIFIED_CLASSNAME}}::loadVertexData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("loadVertexData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyOut(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  auto pushBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TWO_POWER_D);
{%- endfor %}

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int inVertexStack          = event.getVertexDataFrom(i);
    const int outVertexStackPosition = event.getVertexDataTo(i);
    const bool isInOutStack          = ::peano4::grid::PeanoCurve::isInOutStack(inVertexStack);

    const bool isCreateOrDestroy = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity or
                                   inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isNoData          = inVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    const bool dataResidesOnIntermediateStack       = not isInOutStack and not isCreateOrDestroy and not isNoData;
    const bool initialiseNewStackContentWithData    = isCreateOrDestroy;
    const bool initialiseNewStackContentWithoutData = isNoData;

    ::peano4::datamanagement::VertexMarker vertexMarker(event, outVertexStackPosition);
    repositories::DataRepository::DataKey vertexKeyIn(_spacetreeId, inVertexStack);

{%- for vertex in VERTICES %}
    // Load vertex {{vertex.logical_type_name}}
    {
      const auto flag           = {{vertex.full_qualified_type}}::loadStoreComputeFlag(vertexMarker);
      const bool computeOnData  = ::peano4::grid::computeOnData(flag);
      const bool loadData       = ::peano4::grid::loadPersistently(flag);

      if (dataResidesOnIntermediateStack or (isInOutStack and loadData)) {
        auto* _pop = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPop(vertexKeyIn);
        assertion(not _pop->empty());
        pushBlock{{vertex.logical_type_name}}.set(outVertexStackPosition, _pop->pop());
      } else if (initialiseNewStackContentWithoutData or not computeOnData) {
        // Do nothing as we have already pushed the block and emplaced entities without any data
      } else if (initialiseNewStackContentWithData or computeOnData) {
        pushBlock{{vertex.logical_type_name}}.emplace_default(outVertexStackPosition);
      } else { // TODO: When does this happen? Document this!
        // Unused grid entity: Do nothing
      }
    }
{%- endfor %}
  }

  logTraceOutWith1Argument("loadVertexData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::storeVertexData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("storeVertexData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyIn(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES | reverse %}
  auto popBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TWO_POWER_D);
{%- endfor %}

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int inVertexStackPosition   = event.getVertexDataFrom(i);
    const int outVertexStack          = event.getVertexDataTo(i);
    const bool isInOutStack           = ::peano4::grid::PeanoCurve::isInOutStack(outVertexStack);

    const bool isCreateOrDestroy = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity or
                                   outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isNoData          = outVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    const bool dataShallBePushedOntoIntermediateStack = not isInOutStack and not isCreateOrDestroy and not isNoData;

    ::peano4::datamanagement::VertexMarker vertexMarker(event, inVertexStackPosition);
    repositories::DataRepository::DataKey vertexKeyOut(_spacetreeId, outVertexStack);

{%- for vertex in VERTICES | reverse %}
    // Load vertex {{vertex.logical_type_name}}
    {
      const auto flag       = {{vertex.full_qualified_type}}::loadStoreComputeFlag(vertexMarker);
      const bool storeData  = ::peano4::grid::storePersistently(flag);

      if (dataShallBePushedOntoIntermediateStack or (isInOutStack and storeData)) {
        auto& data = popBlock{{vertex.logical_type_name}}.get(inVertexStackPosition);
        auto* _push = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPush(vertexKeyOut);
        _push->push(std::move(data));
      } else {
        // Unused grid entity: Do nothing
      }
    }
{%- endfor %}
  }

  logTraceOutWith1Argument("storeVertexData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::sendVertexData(int position, int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("sendVertexData()", position, toStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::VertexMarker marker(event, position);
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  {
    /*const*/ auto& data = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPop(key)->top(TWO_POWER_D - 1 - position);
    if (
      ::peano4::grid::storePersistently({{vertex.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == ::peano4::stacks::SendReceiveContext::ForkDomain or context == ::peano4::stacks::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data); // std::move(data)
    }
  }
{%- endfor %}

  logTraceOut("sendVertexData()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeVertexData(int position, int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("receiveAndMergeVertexData()", position, fromStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::VertexMarker marker(event, position);
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  {
    auto& data = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPush(key)->top(TWO_POWER_D - 1 - position);
    if (::peano4::grid::loadPersistently({{vertex.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      const auto& incomingData = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
{%- endfor %}

  logTraceOut("receiveAndMergeVertexData()");
}
{% endif %}


{% if FACES %}
void {{FULL_QUALIFIED_CLASSNAME}}::loadFaceData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("loadFaceData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyOut(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for face in FACES %}
  auto pushBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TWO_TIMES_D);
{%- endfor %}

  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int inFaceStack           = event.getFaceDataFrom(i);
    const int outFaceStackPosition  = event.getFaceDataTo(i);
    const bool isInOutStack         = ::peano4::grid::PeanoCurve::isInOutStack(inFaceStack);

    const bool isCreateOrDestroy = inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity or
                                   inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isNoData          = inFaceStack == ::peano4::grid::GridTraversalObserver::NoData;

    const bool dataResidesOnIntermediateStack       = not isInOutStack and not isCreateOrDestroy and not isNoData;
    const bool initialiseNewStackContentWithData    = isCreateOrDestroy;
    const bool initialiseNewStackContentWithoutData = isNoData;

    ::peano4::datamanagement::FaceMarker faceMarker = event.getFaceMarker();
    faceMarker.select(outFaceStackPosition);
    repositories::DataRepository::DataKey faceKeyIn(_spacetreeId, inFaceStack);

{%- for face in FACES %}
    // Load face {{face.logical_type_name}}
    {
      const auto flag           = {{face.full_qualified_type}}::loadStoreComputeFlag(faceMarker);
      const bool computeOnData  = ::peano4::grid::computeOnData(flag);
      const bool loadData       = ::peano4::grid::loadPersistently(flag);

      if (dataResidesOnIntermediateStack or (isInOutStack and loadData)) {
        auto* _pop = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPop(faceKeyIn);
        assertion(not _pop->empty());
        pushBlock{{face.logical_type_name}}.set(outFaceStackPosition, _pop->pop());
      } else if (initialiseNewStackContentWithoutData or not computeOnData) {
        // Do nothing as we have already pushed the block and emplaced entities without any data
      } else if (initialiseNewStackContentWithData or computeOnData) {
        pushBlock{{face.logical_type_name}}.emplace_default(outFaceStackPosition);
      } else { // TODO: When does this happen? Document this!
        // Unused grid entity: Do nothing
      }
    }
{%- endfor %}
  }

  logTraceOutWith1Argument("loadFaceData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::storeFaceData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("storeFaceData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyIn(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for face in FACES | reverse %}
  auto popBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TWO_TIMES_D);
{%- endfor %}

  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int inFaceStackPosition  = event.getFaceDataFrom(i);
    const int outFaceStack         = event.getFaceDataTo(i);
    const bool isInOutStack        = ::peano4::grid::PeanoCurve::isInOutStack(outFaceStack);

    const bool isCreateOrDestroy = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity or
                                   outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity;
    const bool isNoData          = outFaceStack == ::peano4::grid::GridTraversalObserver::NoData;

    const bool dataShallBePushedOntoIntermediateStack = not isInOutStack and not isCreateOrDestroy and not isNoData;

    ::peano4::datamanagement::FaceMarker faceMarker = event.getFaceMarker();
    faceMarker.select(inFaceStackPosition);
    repositories::DataRepository::DataKey faceKeyOut(_spacetreeId, outFaceStack);

{%- for face in FACES | reverse %}
    // Store face {{face.logical_type_name}}
    {
      const auto flag       = {{face.full_qualified_type}}::loadStoreComputeFlag(faceMarker);
      const bool storeData  = ::peano4::grid::storePersistently(flag);

      if (dataShallBePushedOntoIntermediateStack or (isInOutStack and storeData)) {
        auto& data = popBlock{{face.logical_type_name}}.get(inFaceStackPosition);
        auto* _push = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPush(faceKeyOut);
        _push->push(std::move(data));
      } else {
        // Unused grid entity: Do nothing
      }
    }
{%- endfor %}
  }

  logTraceOutWith1Argument("storeFaceData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::sendFaceData(int position, int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("sendFaceData()", position, toStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::FaceMarker marker = event.getFaceMarker();
  marker.select(position);
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for face in FACES %}
  {
    /*const*/ auto& data = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPop(key)->top(TWO_TIMES_D - 1 - position);
    if (
      ::peano4::grid::storePersistently({{face.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == ::peano4::stacks::SendReceiveContext::ForkDomain or context == ::peano4::stacks::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::{{face.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data); // std::move(data)
    }
  }
{%- endfor %}

  logTraceOut("sendFaceData()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeFaceData(int position, int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("receiveAndMergeFaceData()", position, fromStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::FaceMarker marker = event.getFaceMarker();
  marker.select(position);
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for face in FACES %}
  {
    auto& data = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPush(key)->top(TWO_TIMES_D - 1 - position);
    if (::peano4::grid::loadPersistently({{face.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      const auto& incomingData = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
{%- endfor %}

  logTraceOut("receiveAndMergeFaceData()");
}
{% endif %}


{% if CELLS %}
void {{FULL_QUALIFIED_CLASSNAME}}::loadCellData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("loadCellData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyOut(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

  const int inCellStack = event.getCellData();
  ::peano4::datamanagement::CellMarker cellMarker = event.getCellMarker();
  repositories::DataRepository::DataKey cellKeyIn(_spacetreeId, inCellStack);

  const bool isCreateOrDestroy  = inCellStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
  const bool isNoData           = inCellStack == ::peano4::grid::GridTraversalObserver::NoData;

  const bool dataResidesOnInputStack              = not isCreateOrDestroy and not isNoData;
  const bool initialiseNewStackContentWithData    = isCreateOrDestroy;
  const bool initialiseNewStackContentWithoutData = isNoData;

{%- for cell in CELLS %}
  // Load cell {{cell.logical_type_name}}
  {
    auto pushBlock = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPush(keyOut)->pushBlock(1);

    const auto flag           = {{cell.full_qualified_type}}::loadStoreComputeFlag(cellMarker);
    const bool computeOnData  = ::peano4::grid::computeOnData(flag);
    const bool loadData       = ::peano4::grid::loadPersistently(flag);

    if (dataResidesOnInputStack and loadData) {
      auto* _pop = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPop(cellKeyIn);
      assertion(not _pop->empty());
      pushBlock.set(0, _pop->pop());
    } else if (initialiseNewStackContentWithoutData or not computeOnData) {
      // Do nothing as we have already pushed the block and emplaced entities without any data
    } else if (initialiseNewStackContentWithData or computeOnData) {
      pushBlock.emplace_default(0);
    } else {
      // Unused grid entity: Do nothing
    }
  }
{%- endfor %}

  logTraceOutWith1Argument("loadCellData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::storeCellData(const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("storeCellData()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyIn(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

  const int outCellStack = event.getCellData();
  const bool isInOutStack = ::peano4::grid::PeanoCurve::isInOutStack(outCellStack);
  ::peano4::datamanagement::CellMarker cellMarker = event.getCellMarker();
  repositories::DataRepository::DataKey cellKeyOut(_spacetreeId, outCellStack);

  const bool isCreateOrDestroy  = outCellStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity;
  const bool isNoData           = outCellStack == ::peano4::grid::GridTraversalObserver::NoData;

  const bool dataShallBePushedOntoIntermediateStack = not isInOutStack and not isCreateOrDestroy and not isNoData;

{%- for cell in CELLS | reverse %}
  // Store cell {{cell.logical_type_name}}
  {
    auto popBlock = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPop(keyIn)->popBlock(1);

    const auto flag       = {{cell.full_qualified_type}}::loadStoreComputeFlag(cellMarker);
    const bool storeData  = ::peano4::grid::storePersistently(flag);

    if (dataShallBePushedOntoIntermediateStack or (isInOutStack and storeData)) {
      auto& data = popBlock.get(0);
      auto* _push = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPush(cellKeyOut);
      _push->push(std::move(data));
    } else {
      // Unused grid entity: Do nothing
    }
  }
{%- endfor %}

  logTraceOutWith1Argument("storeCellData()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::sendCellData(int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith3Arguments("sendCellData()", toStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::CellMarker marker = event.getCellMarker();
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for cell in CELLS %}
  {
    /*const*/ auto& data = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPop(key)->top();
    if (
      ::peano4::grid::storePersistently({{cell.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == ::peano4::stacks::SendReceiveContext::ForkDomain or context == ::peano4::stacks::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data); // std::move(data)
    }
  }
{%- endfor %}

  logTraceOut("sendCellData()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeCellData(int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) {
  logTraceInWith3Arguments("receiveAndMergeCellData()", fromStack, event.toString(), _spacetreeId);

  ::peano4::datamanagement::CellMarker marker = event.getCellMarker();
  repositories::DataRepository::DataKey key(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for cell in CELLS %}
  {
    auto& data = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPush(key)->top();
    if (::peano4::grid::loadPersistently({{cell.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      const auto& incomingData = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
{%- endfor %}

  logTraceOut("receiveAndMergeCellData()");
}
{% endif %}


void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllHorizontalDataExchangeStacks(bool symmetricDataCardinality) {
  logTraceInWith2Arguments("exchangeAllHorizontalDataExchangeStacks()", symmetricDataCardinality, _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}

  logTraceOut("exchangeAllHorizontalDataExchangeStacks()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument("exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

  logTraceOut("exchangeAllPeriodicBoundaryDataStacks()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments("streamDataFromSplittingTreeToNewTree()", newWorker, _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId, newWorker);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId, newWorker);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId, newWorker);
{%- endfor %}

  logTraceOut("streamDataFromSplittingTreeToNewTree()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments("streamDataFromJoiningTreeToMasterTree()", master, _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId, master);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId, master);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId, master);
{%- endfor %}

  logTraceOut("streamDataFromJoiningTreeToMasterTree()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument("finishAllOutstandingSendsAndReceives()", _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

  logTraceOut("finishAllOutstandingSendsAndReceives()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::deleteAllStacks() {
  logTraceInWith1Argument("deleteAllStacks()", _spacetreeId);

{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}

  logTraceOut("deleteAllStacks()");
}
