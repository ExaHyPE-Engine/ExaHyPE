void {{FULL_QUALIFIED_CLASSNAME}}::receiveNodeData(
  ::peano4::grid::GridTraversalObserver::NodeData nodeData,
  int                                             position,
  int                                             fromStack,
  ::peano4::stacks::SendReceiveContext            context,
  const ::peano4::grid::GridTraversalEvent&       event
) {
  using NodeData = ::peano4::grid::GridTraversalObserver::NodeData;
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

  auto incomingStack = [&](auto& container) -> typename std::remove_reference_t<decltype(container)>::Stack& {
    switch (context) {
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertion(fromStack >= 0 and fromStack < ::peano4::grid::StackTopology::TotalNumberOfLocalExchangeStacks, fromStack, _spacetreeId);
      return container.getLocalExchangeStack(_spacetreeId, fromStack); // local stack id
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      container.resizeParallelExchangeStacks(_spacetreeId, fromStack + 1);
      return container.getParallelExchangeStack(_spacetreeId, fromStack, ::peano4::grid::StackTopology::ParallelExchangeStack::HorizontalInput);
    case ::peano4::stacks::SendReceiveContext::ForkDomain:
    case ::peano4::stacks::SendReceiveContext::JoinDomain:
      container.resizeParallelExchangeStacks(_spacetreeId, fromStack + 1);
      return container.getParallelExchangeStack(_spacetreeId, fromStack, ::peano4::grid::StackTopology::ParallelExchangeStack::VerticalInput);
    default:
      assertion(false, "Unsupported receive context");
      return container.getLocalExchangeStack(_spacetreeId, callStack);
    }
  };

  switch (nodeData) {
  case NodeData::Vertex:
    {
{%- if VERTICES %}
      auto marker = ::peano4::datamanagement::VertexMarker(event, position);
{%- for vertex in VERTICES %}
      {
        auto& data = repositories::DataRepository::{{vertex.logical_type_name}}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).top(TWO_POWER_D - 1 - position);
        if (data.receiveAndMerge(marker, context)) {
          auto& stack = incomingStack(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer);
          assertion(not stack.empty());
          auto incomingData = stack.pop();
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
        auto& data = repositories::DataRepository::{{face.logical_type_name}}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).top(TWO_TIMES_D - 1 - position);
        if (data.receiveAndMerge(marker, context)) {
          auto& stack = incomingStack(repositories::DataRepository::{{face.logical_type_name}}StackContainer);
          assertion(not stack.empty());
          auto incomingData = stack.pop();
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
        auto& data = repositories::DataRepository::{{cell.logical_type_name}}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).top();
        if (data.receiveAndMerge(marker, context)) {
          auto& stack = incomingStack(repositories::DataRepository::{{cell.logical_type_name}}StackContainer);
          assertion(not stack.empty());
          auto incomingData = stack.pop();
          data.merge(context, std::move(incomingData), marker, _spacetreeId);
        }
      }
{%- endfor %}
{%- endif %}
    }
    break;
  }
}
