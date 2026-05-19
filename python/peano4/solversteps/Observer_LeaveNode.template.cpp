void {{FULL_QUALIFIED_CLASSNAME}}::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {
  const auto key = repositories::DataRepository::DataKey(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

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
  auto       cellMarker  = event.getCellMarker();
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
  for (int i = 0; i < TWO_TIMES_D; i++) {
    const auto outFaceStack = event.getFaceDataTo(i);
    const auto pick         = event.getFaceDataFrom(i);
    faceMarker.select(pick);

    const auto isLocal = faceMarker.isLocal();
    if (not isLocal) { continue; }

    const auto isCreateOrDestroy = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = faceMarker.isHanging();
    const auto isInOut           = ::peano4::grid::PeanoCurve::isInOutStack(outFaceStack);

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
    if (isCreateOrDestroy) {
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
  auto vertexMarker = ::peano4::datamanagement::VertexMarker(event);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto outVertexStack = event.getVertexDataTo(i);
    const auto pick           = event.getVertexDataFrom(i);
    vertexMarker.select(pick);

    const auto isLocal = vertexMarker.isLocal();
    if (not isLocal) { continue; }

    const auto isCreateOrDestroy = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isHanging         = vertexMarker.isHanging();
    const auto isInOut           = ::peano4::grid::PeanoCurve::isInOutStack(outVertexStack);

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
    if (isCreateOrDestroy) {
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
}
