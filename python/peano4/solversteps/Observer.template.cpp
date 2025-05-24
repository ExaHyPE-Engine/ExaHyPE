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

tarch::logging::Log {{FULL_QUALIFIED_CLASSNAME}}::_log("{{FULL_QUALIFIED_CLASSNAME}}");


{{FULL_QUALIFIED_CLASSNAME}}::{{CLASSNAME}}(int spacetreeId):
  _spacetreeId(spacetreeId)
  {%- for action_set in ACTIVE_ACTION_SETS %}
  , {{ action_set.name }}(spacetreeId)
  {%- endfor %}
  {}


peano4::grid::TraversalObserver* {{FULL_QUALIFIED_CLASSNAME}}::clone(int spacetreeId) {
  return new {{CLASSNAME}}(spacetreeId);
}


std::vector<peano4::grid::GridControlEvent> {{FULL_QUALIFIED_CLASSNAME}}::getGridControlEvents() const {
  std::vector<peano4::grid::GridControlEvent> result;

  std::vector<std::vector<peano4::grid::GridControlEvent>> allActionResults = {
    {%- for action_set in ACTIVE_ACTION_SETS %}
    {{ action_set.name }}.getGridControlEvents(){{ "," if not loop.last }}
    {%- endfor %}
  };

  std::size_t totalSize = 0;
  for (const auto& v : allActionResults) {
    totalSize += v.size();
  }
  result.reserve(totalSize);

  for (auto it = allActionResults.rbegin(); it != allActionResults.rend(); ++it) {
    result.insert(result.begin(), it->begin(), it->end());
  }

  return result;
}


void {{FULL_QUALIFIED_CLASSNAME}}::prepareTraversal() {
  logTraceIn("prepareTraversal()");
  {%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.full_qualified_name }}::prepareTraversal();
  {%- endfor %}
  logTraceOut("prepareTraversal()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::beginTraversal(const tarch::la::Vector<Dimensions,double>& x, const tarch::la::Vector<Dimensions,double>& h) {
  logTraceInWith2Arguments("beginTraversal()", x, h);
  {%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.beginTraversal();
  {%- endfor %}

  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for cell in CELLS %}
  repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPush(key)->push({{ cell.full_qualified_type }}());
  {%- endfor %}

  {%- if FACES %}
  for (int i = 0; i < TwoTimesD; i++) {
    {%- for face in FACES %}
    repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPush(key)->push({{ face.full_qualified_type }}());
    {%- endfor %}
  }
  {%- endif %}

  {%- if VERTICES %}
  for (int i = 0; i < TwoPowerD; i++) {
    {%- for vertex in VERTICES %}
    repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPush(key)->push({{ vertex.full_qualified_type }}());
    {%- endfor %}
  }
  {%- endif %}

  logTraceOutWith2Arguments("beginTraversal()", x, h);
}


void {{FULL_QUALIFIED_CLASSNAME}}::loadCell(const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("loadCell()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyOut(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for vertex in VERTICES %}
  auto view{{ vertex.logical_type_name }} = repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TwoPowerD);
  {%- endfor %}

  {%- if VERTICES %}
  for (int i = 0; i < TwoPowerD; i++) {
    int inVertexStack          = event.getVertexDataFrom(i);
    int outVertexStackPosition = event.getVertexDataTo(i);
    const bool isInOutStack    = peano4::grid::PeanoCurve::isInOutStack(inVertexStack);

    peano4::datamanagement::VertexMarker vertexMarker(event, outVertexStackPosition);
    repositories::DataRepository::DataKey vertexKeyIn(_spacetreeId, inVertexStack);

    {%- for vertex in VERTICES %}
    // Load vertex {{vertex.logical_type_name}}
    {
      const auto flag = {{vertex.full_qualified_type}}::loadStoreComputeFlag(vertexMarker);
      const bool dataResidesOnIntermediateStack   = not isInOutStack and
                                                    inVertexStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                    inVertexStack != peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity and
                                                    inVertexStack != peano4::grid::TraversalObserver::NoData;
      const bool initialiseStackContentForNewData = peano4::grid::computeOnData(flag) or
                                                    inVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity or
                                                    inVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
      const bool dataResidesOnInputStack          = isInOutStack and
                                                    peano4::grid::loadPersistently(flag);

      if (dataResidesOnIntermediateStack or dataResidesOnInputStack) {
        assertion(not repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPop(vertexKeyIn)->empty());
        view{{vertex.logical_type_name}}.set(outVertexStackPosition, std::move(repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPop(vertexKeyIn)->pop()));
      } else if (initialiseStackContentForNewData) {
        {{vertex.full_qualified_type}} data;
        view{{vertex.logical_type_name}}.set(outVertexStackPosition, data);
      } else {
        // Unused grid entity: Do nothing
      }
    }
    {%- endfor %}
  }
  {%- endif %}

  {%- for face in FACES %}
  auto view{{ face.logical_type_name }} = repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TwoTimesD);
  {%- endfor %}

  {%- if FACES %}
  for (int i = 0; i < TwoTimesD; i++) {
    const int inFaceStack           = event.getFaceDataFrom(i);
    const int outFaceStackPosition  = event.getFaceDataTo(i);
    const bool isInOutStack         = peano4::grid::PeanoCurve::isInOutStack(inFaceStack);

    peano4::datamanagement::FaceMarker faceMarker(event, outFaceStackPosition);
    repositories::DataRepository::DataKey faceKeyIn(_spacetreeId, inFaceStack);

    {%- for face in FACES %}
    // Load face {{face.logical_type_name}}
    {
      const auto flag = {{face.full_qualified_type}}::loadStoreComputeFlag(faceMarker);
      const bool dataResidesOnIntermediateStack   = not isInOutStack and
                                                    inFaceStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                    inFaceStack != peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity and
                                                    inFaceStack != peano4::grid::TraversalObserver::NoData;
      const bool initialiseStackContentForNewData = peano4::grid::computeOnData(flag) or
                                                    inFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity or
                                                    inFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
      const bool dataResidesOnInputStack          = isInOutStack and
                                                    peano4::grid::loadPersistently(flag);

      if (dataResidesOnIntermediateStack or dataResidesOnInputStack) {
        assertion(not repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPop(faceKeyIn)->empty());
        view{{face.logical_type_name}}.set(outFaceStackPosition, std::move(repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPop(faceKeyIn)->pop()));
      } else if (initialiseStackContentForNewData) {
        {{face.full_qualified_type}} data;
        view{{face.logical_type_name}}.set(outFaceStackPosition, data);
      } else {
        // Unused grid entity: Do nothing
      }
    }
    {%- endfor %}
  }
  {%- endif %}

  {%- if CELLS %}
  const int inCellStack  = event.getCellData();
  peano4::datamanagement::CellMarker cellMarker(event);
  repositories::DataRepository::DataKey cellKeyIn(_spacetreeId, inCellStack);

  {%- for cell in CELLS %}
  // Load cell {{cell.logical_type_name}}
  {
    auto view = repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPush(keyOut)->pushBlock(1);

    const auto flag = {{cell.full_qualified_type}}::loadStoreComputeFlag(cellMarker);
    const bool dataResidesOnInputStack          = inCellStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                  inCellStack != peano4::grid::TraversalObserver::NoData and
                                                  peano4::grid::loadPersistently(flag);
    const bool initialiseStackContentForNewData = peano4::grid::computeOnData(flag) or
                                                  inCellStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity;

    if (dataResidesOnInputStack) {
      assertion(not repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPop(cellKeyIn)->empty());
      view.set(0, std::move(repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPop(cellKeyIn)->pop()));
    } else if (initialiseStackContentForNewData) {
      {{cell.full_qualified_type}} data;
      view.set(0, data);
    } else {
      // Unused grid entity: Do nothing
    }
  }
  {%- endfor %}
  {%- endif %}

  logTraceOutWith1Argument("loadCell()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::enterCell(const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("enterCell()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for vertex in VERTICES %}
  peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TwoPowerD - 1));
  peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TwoPowerD * 2 - 1));
  {%- endfor %}

  {%- for face in FACES %}
  peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(&repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(key)->top(TwoTimesD - 1));
  peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(&repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(key)->top(TwoTimesD * 2 - 1));
  {%- endfor %}

  {%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPop(key)->top(0);
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPop(key)->top(1);
  {%- endfor %}

  {%- if VERTICES %}
  peano4::datamanagement::VertexMarker vertexMarker(event);
  for (int i = 0; i < TwoPowerD; i++) {
    const int inVertexStack = event.getVertexDataFrom(i);
    const int pick          = event.getVertexDataTo(i);
    vertexMarker.select(pick);

    if (vertexMarker.isLocal() and inVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createPersistentVertex(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );

      {{action_set.name}}.touchVertexFirstTime(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (vertexMarker.isLocal() and inVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createHangingVertex(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (vertexMarker.isLocal() and peano4::grid::PeanoCurve::isInOutStack(inVertexStack)) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchVertexFirstTime(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    }
  }
  {%- endif %}

  {%- if FACES %}
  peano4::datamanagement::FaceMarker faceMarker(event);
  for (int i = 0; i < TwoTimesD; i++) {
    const int inFaceStack   = event.getFaceDataFrom(i);
    const int pick          = event.getFaceDataTo(i);
    faceMarker.select(pick);

    if (faceMarker.isLocal() and inFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createPersistentFace(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );

      {{action_set.name}}.touchFaceFirstTime(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (faceMarker.isLocal() and inFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.createHangingFace(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (faceMarker.isLocal() and peano4::grid::PeanoCurve::isInOutStack(inFaceStack)) {
      {%- for action_set in ACTIVE_ACTION_SETS %}
      {{action_set.name}}.touchFaceFirstTime(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    }
  }
  {%- endif %}

  {% if CELLS %}
  peano4::datamanagement::CellMarker cellMarker(event);
  if (cellMarker.isLocal() and event.getCellData() == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
    {%- for action_set in ACTIVE_ACTION_SETS %}
    {{action_set.name}}.createCell(cellMarker,
      {%- for vertex in VERTICES %}
      fineGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      fineGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      fineGrid{{ cell.logical_type_name }},
      {%- endfor %}
      {%- for vertex in VERTICES %}
      coarseGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      coarseGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
      {%- endfor %}
    );
    {%- endfor %}
  }

  if (cellMarker.isLocal()) {
    {%- for action_set in ACTIVE_ACTION_SETS %}
    {{action_set.name}}.touchCellFirstTime(cellMarker,
      {%- for vertex in VERTICES %}
      fineGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      fineGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      fineGrid{{ cell.logical_type_name }},
      {%- endfor %}
      {%- for vertex in VERTICES %}
      coarseGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      coarseGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
      {%- endfor %}
    );
    {%- endfor %}
  }
  {%- endif %}

  logTraceOut("enterCell()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::leaveCell(const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("leaveCell()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for vertex in VERTICES %}
  peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> fineGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TwoPowerD - 1));
  peano4::datamanagement::VertexEnumerator<{{ vertex.full_qualified_type }}> coarseGrid{{ vertex.logical_type_name }}(&repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(key)->top(TwoPowerD * 2 - 1));
  {%- endfor %}

  {%- for face in FACES %}
  peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> fineGrid{{ face.logical_type_name }}(&repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(key)->top(TwoTimesD - 1));
  peano4::datamanagement::FaceEnumerator<{{ face.full_qualified_type }}> coarseGrid{{ face.logical_type_name }}(&repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(key)->top(TwoTimesD * 2 - 1));
  {%- endfor %}

  {%- for cell in CELLS %}
  auto& fineGrid{{ cell.logical_type_name }} = repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPop(key)->top(0);
  auto& coarseGrid{{ cell.logical_type_name }} = repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPop(key)->top(1);
  {%- endfor %}

  {%- if CELLS %}
  peano4::datamanagement::CellMarker cellMarker(event);
  if (cellMarker.isLocal()) {
    {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
    {{action_set.name}}.touchCellLastTime(cellMarker,
      {%- for vertex in VERTICES %}
      fineGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      fineGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      fineGrid{{ cell.logical_type_name }},
      {%- endfor %}
      {%- for vertex in VERTICES %}
      coarseGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      coarseGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
      {%- endfor %}
    );
    {%- endfor %}
  }

  if (cellMarker.isLocal() and event.getCellData() == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
    {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
    {{action_set.name}}.destroyCell(cellMarker,
      {%- for vertex in VERTICES %}
      fineGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      fineGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      fineGrid{{ cell.logical_type_name }},
      {%- endfor %}
      {%- for vertex in VERTICES %}
      coarseGrid{{ vertex.logical_type_name }},
      {%- endfor %}
      {%- for face in FACES %}
      coarseGrid{{ face.logical_type_name }},
      {%- endfor %}
      {%- for cell in CELLS %}
      coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
      {%- endfor %}
    );
    {%- endfor %}
  }
  {%- endif %}

  {%- if FACES %}
  peano4::datamanagement::FaceMarker faceMarker(event);
  for (int i = 0; i < TwoTimesD; i++) {
    const int outFaceStack  = event.getFaceDataTo(i);
    const int pick          = event.getFaceDataFrom(i);
    faceMarker.select(pick);

    if (faceMarker.isLocal() and outFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchFaceLastTime(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );

      {{action_set.name}}.destroyPersistentFace(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (faceMarker.isLocal() and outFaceStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyHangingFace(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (faceMarker.isLocal() and peano4::grid::PeanoCurve::isInOutStack(outFaceStack)) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchFaceLastTime(faceMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        fineGrid{{ face.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    }
  }
  {%- endif %}

  {%- if VERTICES %}
  peano4::datamanagement::VertexMarker vertexMarker(event);
  for (int i = 0; i < TwoPowerD; i++) {
    const int outVertexStack  = event.getVertexDataTo(i);
    const int pick            = event.getVertexDataFrom(i);
    vertexMarker.select(pick);

    if (vertexMarker.isLocal() and outVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchVertexLastTime(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );

      {{action_set.name}}.destroyPersistentVertex(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (vertexMarker.isLocal() and outVertexStack == peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.destroyHangingVertex(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    } else if (vertexMarker.isLocal() and peano4::grid::PeanoCurve::isInOutStack(outVertexStack)) {
      {%- for action_set in ACTIVE_ACTION_SETS | reverse %}
      {{action_set.name}}.touchVertexLastTime(vertexMarker,
        {%- for vertex in VERTICES %}
        fineGrid{{ vertex.logical_type_name }}(pick),
        {%- endfor %}
        {%- for vertex in VERTICES %}
        coarseGrid{{ vertex.logical_type_name }},
        {%- endfor %}
        {%- for face in FACES %}
        coarseGrid{{ face.logical_type_name }},
        {%- endfor %}
        {%- for cell in CELLS %}
        coarseGrid{{ cell.logical_type_name }}{{ "," if not loop.last else "" }}
        {%- endfor %}
      );
      {%- endfor %}
    }
  }
  {%- endif %}

  logTraceOutWith1Argument("leaveCell()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::storeCell(const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith2Arguments("storeCell()", _spacetreeId, event.toString());

  repositories::DataRepository::DataKey keyIn(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- if CELLS %}
  const int outCellStack  = event.getCellData();
  peano4::datamanagement::CellMarker cellMarker(event);
  repositories::DataRepository::DataKey cellKeyOut(_spacetreeId, outCellStack);

  {%- for cell in CELLS | reverse %}
  // Store cell {{cell.logical_type_name}}
  {
    auto view = repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPop(keyIn)->popBlock(1);
    const auto flag = {{cell.full_qualified_type}}::loadStoreComputeFlag(cellMarker);
    const bool dataShallBePushedOntoIntermediateStack = not peano4::grid::PeanoCurve::isInOutStack(outCellStack) and
                                                        outCellStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                        outCellStack != peano4::grid::TraversalObserver::NoData;
    const bool dataShallBePushedOntoOutputStack       = peano4::grid::PeanoCurve::isInOutStack(outCellStack) and
                                                        peano4::grid::storePersistently(flag);

    if (dataShallBePushedOntoIntermediateStack or dataShallBePushedOntoOutputStack) {
      repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPush(cellKeyOut)->push(view.get(0));
    } else {
      // Unused grid entity: Do nothing
    }
  }
  {%- endfor %}
  {%- endif %}

  {%- for face in FACES | reverse %}
  auto view{{ face.logical_type_name }} = repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TwoTimesD);
  {%- endfor %}

  {%- if FACES %}
  for (int i = 0; i < TwoTimesD; i++) {
    const int inFaceStackPosition  = event.getFaceDataFrom(i);
    const int outFaceStack         = event.getFaceDataTo(i);
    const bool isInOutStack        = peano4::grid::PeanoCurve::isInOutStack(outFaceStack);

    peano4::datamanagement::FaceMarker faceMarker(event, inFaceStackPosition);
    repositories::DataRepository::DataKey faceKeyOut(_spacetreeId, outFaceStack);

    {%- for face in FACES | reverse %}
    // Store face {{face.logical_type_name}}
    {
      {{face.full_qualified_type}}& data = view{{face.logical_type_name}}.get(inFaceStackPosition);
      const auto flag = {{face.full_qualified_type}}::loadStoreComputeFlag(faceMarker);
      const bool dataShallBePushedOntoIntermediateStack = not isInOutStack and
                                                          outFaceStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                          outFaceStack != peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity and
                                                          outFaceStack != peano4::grid::TraversalObserver::NoData;
      const bool dataShallBePushedOntoOutputStack       = isInOutStack and
                                                          peano4::grid::storePersistently(flag);

      if (dataShallBePushedOntoIntermediateStack or dataShallBePushedOntoOutputStack) {
        repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPush(faceKeyOut)->push(data);
      } else {
        // Unused grid entity: Do nothing
      }
    }
    {%- endfor %}
  }
  {%- endif %}

  {%- for vertex in VERTICES %}
  auto view{{ vertex.logical_type_name }} = repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TwoPowerD);
  {%- endfor %}

  {%- if VERTICES %}
  for (int i = 0; i < TwoPowerD; i++) {
    const int inVertexStackPosition   = event.getVertexDataFrom(i);
    const int outVertexStack          = event.getVertexDataTo(i);
    const bool isInOutStack           = peano4::grid::PeanoCurve::isInOutStack(outVertexStack);

    peano4::datamanagement::VertexMarker vertexMarker(event, inVertexStackPosition);
    repositories::DataRepository::DataKey vertexKeyOut(_spacetreeId, outVertexStack);

    {%- for vertex in VERTICES | reverse %}
    // Load vertex {{vertex.logical_type_name}}
    {
      {{vertex.full_qualified_type}}& data = view{{vertex.logical_type_name}}.get(inVertexStackPosition);
      const auto flag = {{vertex.full_qualified_type}}::loadStoreComputeFlag(vertexMarker);
      const bool dataShallBePushedOntoIntermediateStack = not isInOutStack and
                                                          outVertexStack != peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity and
                                                          outVertexStack != peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity and
                                                          outVertexStack != peano4::grid::TraversalObserver::NoData;
      const bool dataShallBePushedOntoOutputStack       = isInOutStack and
                                                          peano4::grid::storePersistently(flag);

      if (dataShallBePushedOntoIntermediateStack or dataShallBePushedOntoOutputStack) {
        repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPush(vertexKeyOut)->push(data);
      } else {
        // Unused grid entity: Do nothing
      }
    }
    {%- endfor %}
  }
  {%- endif %}

  logTraceOutWith1Argument("storeCell()", _spacetreeId);
}


void {{FULL_QUALIFIED_CLASSNAME}}::endTraversal(const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h) {
  logTraceInWith2Arguments("endTraversal()", x, h);
  {%- for action_set in ACTIVE_ACTION_SETS %}
  {{ action_set.name }}.endTraversal();
  {%- endfor %}

  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for cell in CELLS %}
  repositories::DataRepository::_{{ cell.logical_type_name }}Stack.getForPop(key)->pop();
  {%- endfor %}

  {%- if FACES %}
  for (int i = 0; i < TwoTimesD; i++) {
    {%- for face in FACES %}
    repositories::DataRepository::_{{ face.logical_type_name }}Stack.getForPop(key)->pop();
    {%- endfor %}
  }
  {%- endif %}

  {%- if VERTICES %}
  for (int i = 0; i < TwoPowerD; i++) {
    {%- for vertex in VERTICES %}
    repositories::DataRepository::_{{ vertex.logical_type_name }}Stack.getForPop(key)->pop();
    {%- endfor %}
  }
  {%- endif %}

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
void {{FULL_QUALIFIED_CLASSNAME}}::sendVertex(int position, int toStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("sendVertex()", position, toStack, event.toString(), _spacetreeId);

  peano4::datamanagement::VertexMarker marker(event, position);
  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for vertex in VERTICES %}
  {
    const {{vertex.full_qualified_type}}& data = repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPop(key)->top(TwoPowerD - 1 - position);
    if (
      peano4::grid::storePersistently({{vertex.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == TraversalObserver::SendReceiveContext::ForkDomain or context == TraversalObserver::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data);
    }
  }
  {%- endfor %}

  logTraceOut("sendVertex()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeVertex(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("receiveAndMergeVertex()", position, fromStack, event.toString(), _spacetreeId);

  peano4::datamanagement::VertexMarker marker(event, position);

  {%- for vertex in VERTICES %}
  {
    {{vertex.full_qualified_type}}& data = repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPush(
      repositories::DataRepository::DataKey(_spacetreeId, peano4::grid::PeanoCurve::CallStack)
    )->top(TwoPowerD - 1 - position);

    if (peano4::grid::loadPersistently({{vertex.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      auto incomingData = std::move(repositories::DataRepository::_{{vertex.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop());
      logDebug("receiveAndMergeVertex()", "Merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString());
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
  {%- endfor %}

  logTraceOut("receiveAndMergeVertex()");
}
{% endif %}


{% if FACES %}
void {{FULL_QUALIFIED_CLASSNAME}}::sendFace(int position, int toStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("sendFace()", position, toStack, event.toString(), _spacetreeId);

  peano4::datamanagement::FaceMarker marker(event, position);
  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for face in FACES %}
  {
    const {{face.full_qualified_type}}& data = repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPop(key)->top(TwoTimesD - 1 - position);
    if (
      peano4::grid::storePersistently({{face.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == TraversalObserver::SendReceiveContext::ForkDomain or context == TraversalObserver::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data);
    }
  }
  {%- endfor %}

  logTraceOut("sendFace()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeFace(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments("receiveAndMergeFace()", position, fromStack, event.toString(), _spacetreeId);

  peano4::datamanagement::FaceMarker marker(event, position);

  {%- for face in FACES %}
  {
    {{face.full_qualified_type}}& data = repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPush(
      repositories::DataRepository::DataKey(_spacetreeId, peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD - 1 - position);

    if (peano4::grid::loadPersistently({{face.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      auto incomingData = std::move(repositories::DataRepository::_{{face.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop());
      logDebug("receiveAndMergeFace()", "Merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString());
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
  {%- endfor %}

  logTraceOut("receiveAndMergeFace()");
}
{% endif %}


{% if CELLS %}
void {{FULL_QUALIFIED_CLASSNAME}}::sendCell(int toStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith3Arguments("sendCell(", toStack, event.toString(), _spacetreeId);

  peano4::datamanagement::CellMarker marker(event);
  repositories::DataRepository::DataKey key(_spacetreeId, peano4::grid::PeanoCurve::CallStack);

  {%- for cell in CELLS %}
  {
    const {{cell.full_qualified_type}}& data = repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPop(key)->top();
    if (
      peano4::grid::storePersistently({{cell.full_qualified_type}}::loadStoreComputeFlag(marker))
      and
      (data.send(marker) or context == TraversalObserver::SendReceiveContext::ForkDomain or context == TraversalObserver::SendReceiveContext::JoinDomain)
    ) {
      repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPush(_spacetreeId, toStack)->push(data);
    }
  }
  {%- endfor %}

  logTraceOut("sendCell()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::receiveAndMergeCell(int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith3Arguments("receiveAndMergeCell()", fromStack, event.toString(), _spacetreeId);

  peano4::datamanagement::CellMarker marker(event);

  {%- for cell in CELLS %}
  {
    {{cell.full_qualified_type}}& data = repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPush(
      repositories::DataRepository::DataKey(_spacetreeId, peano4::grid::PeanoCurve::CallStack)
    )->top();

    if (peano4::grid::loadPersistently({{cell.full_qualified_type}}::loadStoreComputeFlag(marker)) and data.receiveAndMerge(marker)) {
      auto incomingData = std::move(repositories::DataRepository::_{{cell.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop());
      logDebug("receiveAndMergeCell()", "Merge " << incomingData.toString() << " into " << data.toString());
      data.merge(context, incomingData, marker, _spacetreeId);
    }
  }
  {%- endfor %}

  logTraceOut("receiveAndMergeCell()");
}
{% endif %}


void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllVerticalDataExchangeStacks(int masterId) {
  logTraceInWith2Arguments("exchangeAllVerticalDataExchangeStacks()", masterId, _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId, masterId);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId, masterId);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId, masterId);
  {%- endfor %}

  logTraceOut("exchangeAllVerticalDataExchangeStacks()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllHorizontalDataExchangeStacks(bool symmetricDataCardinality) {
  logTraceInWith2Arguments("exchangeAllHorizontalDataExchangeStacks()", symmetricDataCardinality, _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
  {%- endfor %}

  logTraceOut("exchangeAllHorizontalDataExchangeStacks()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument("exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}

  logTraceOut("exchangeAllPeriodicBoundaryDataStacks()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments("streamDataFromSplittingTreeToNewTree()", newWorker, _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId, newWorker);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId, newWorker);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId, newWorker);
  {%- endfor %}

  logTraceOut("streamDataFromSplittingTreeToNewTree()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments("streamDataFromJoiningTreeToMasterTree()", master, _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId, master);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId, master);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId, master);
  {%- endfor %}

  logTraceOut("streamDataFromJoiningTreeToMasterTree()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument("finishAllOutstandingSendsAndReceives()", _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}

  logTraceOut("finishAllOutstandingSendsAndReceives()");
}


void {{FULL_QUALIFIED_CLASSNAME}}::deleteAllStacks() {
  logTraceInWith1Argument("deleteAllStacks()", _spacetreeId);

  {%- for cell in CELLS %}
  peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::_{{cell.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for face in FACES %}
  peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::_{{face.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}
  {%- for vertex in VERTICES %}
  peano4::parallel::SpacetreeSet::deleteAllStacks(repositories::DataRepository::_{{vertex.logical_type_name}}Stack, _spacetreeId);
  {%- endfor %}

  logTraceOut("deleteAllStacks()");
}
