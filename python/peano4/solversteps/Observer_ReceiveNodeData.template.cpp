void {{FULL_QUALIFIED_CLASSNAME}}::receiveNodeData(
  ::peano4::grid::GridTraversalObserver::NodeData nodeData,
  int                                             position,
  int                                             fromStack,
  ::peano4::stacks::SendReceiveContext            context,
  const ::peano4::grid::GridTraversalEvent&       event
) {
  using NodeData = ::peano4::grid::GridTraversalObserver::NodeData;
  const auto key = repositories::DataRepository::DataKey(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

  switch (nodeData) {
  case NodeData::Vertex:
    {
{%- if VERTICES %}
      auto marker = ::peano4::datamanagement::VertexMarker(event, position);
{%- for vertex in VERTICES %}
      {
        auto& data = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPush(key)->top(TWO_POWER_D - 1 - position);
        if (data.receiveAndMerge(marker, context)) {
          auto incomingData = repositories::DataRepository::{{vertex.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
          data.merge(context, std::move(incomingData), marker, _spacetreeId);
        }
      }
{%- endfor %}
{%- endif %}
    }
    break;
  case NodeData::Face:
    {
{%- if FACES %}
      auto marker = event.getFaceMarker();
      marker.select(position);
{%- for face in FACES %}
      {
        auto& data = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPush(key)->top(TWO_TIMES_D - 1 - position);
        if (data.receiveAndMerge(marker, context)) {
          auto incomingData = repositories::DataRepository::{{face.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
          data.merge(context, std::move(incomingData), marker, _spacetreeId);
        }
      }
{%- endfor %}
{%- endif %}
    }
    break;
  case NodeData::Cell:
    {
{%- if CELLS %}
      auto marker = event.getCellMarker();
{%- for cell in CELLS %}
      {
        auto& data = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPush(key)->top();
        if (data.receiveAndMerge(marker, context)) {
          auto incomingData = repositories::DataRepository::{{cell.logical_type_name}}Stack.getForPop(_spacetreeId, fromStack)->pop();
          data.merge(context, std::move(incomingData), marker, _spacetreeId);
        }
      }
{%- endfor %}
{%- endif %}
    }
    break;
  }
}
