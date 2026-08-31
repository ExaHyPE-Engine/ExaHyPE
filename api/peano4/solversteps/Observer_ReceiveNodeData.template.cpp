void {{FULL_QUALIFIED_CLASSNAME}}::receiveNodeData(
  ::peano4::grid::GridTraversalObserver::NodeData nodeData,
  int                                             position,
  int                                             fromStack,
  ::peano4::stacks::SendReceiveContext            context,
  const ::peano4::grid::GridTraversalEvent&       event
) {
  using NodeData = ::peano4::grid::GridTraversalObserver::NodeData;

  // The live pool slot of the subtree task merges the frozen copy that fromStack
  // addresses, and the context says whether that copy comes from a partner task
  // or from a neighbour rank. The other partners read the frozen copy afterwards,
  // so the merge works on a copy of it.
  switch (nodeData) {
  case NodeData::Vertex:
    {
{%- if VERTICES %}
      assertion(event.getVertexPoolIndex(position) >= 0);
      auto marker = ::peano4::datamanagement::VertexMarker(event, position);
{%- for vertex in VERTICES %}
      {
        auto& data = repositories::DataRepository::{{vertex.logical_type_name}}StackContainer.getPoolSlot(event.getVertexPoolIndex(position));
        ::peano4::stacks::applyIncomingCopy(data, context, marker, _treeId, [&]() {
          return repositories::DataRepository::{{vertex.logical_type_name}}StackContainer.getPoolSlot(fromStack);
        });
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
        auto& data = repositories::DataRepository::{{face.logical_type_name}}StackContainer.getPoolSlot(event.getFacePoolIndex(position));
        ::peano4::stacks::applyIncomingCopy(data, context, marker, _treeId, [&]() {
          return repositories::DataRepository::{{face.logical_type_name}}StackContainer.getPoolSlot(fromStack);
        });
      }
{%- endfor %}
{%- endif %}
    }
    break;
  case NodeData::Cell:
    assertion(false, "Cell data never travels between subtree tasks or ranks");
    break;
  }
}
