namespace {
  template <typename Stack, typename Data>
  inline void pushDataForExchange(
    Stack& stack,
    Data&  data
  ) {
    // stack.push(data);
    stack.push(data.createExchangeView());
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::sendNodeData(
  ::peano4::grid::GridTraversalObserver::NodeData nodeData,
  int                                             position,
  int                                             toStack,
  ::peano4::stacks::SendReceiveContext            context,
  const ::peano4::grid::GridTraversalEvent&       event
) {
  using NodeData = ::peano4::grid::GridTraversalObserver::NodeData;
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

  auto outgoingStack = [&](auto& container) -> typename std::remove_reference_t<decltype(container)>::Stack& {
    switch (context) {
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertion(toStack >= 0 and toStack < ::peano4::grid::StackTopology::TotalNumberOfLocalExchangeStacks, toStack, _spacetreeId);
      return container.getLocalExchangeStack(_spacetreeId, toStack); // local stack id
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      container.resizeParallelExchangeStacks(_spacetreeId, toStack + 1);
      return container.getParallelExchangeStack(_spacetreeId, toStack, ::peano4::grid::StackTopology::ParallelExchangeStack::HorizontalOutput);
    case ::peano4::stacks::SendReceiveContext::ForkDomain:
    case ::peano4::stacks::SendReceiveContext::JoinDomain:
      container.resizeParallelExchangeStacks(_spacetreeId, toStack + 1);
      return container.getParallelExchangeStack(_spacetreeId, toStack, ::peano4::grid::StackTopology::ParallelExchangeStack::VerticalOutput);
    default:
      assertion(false, "Unsupported send context");
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
        if (data.send(marker, context)) {
          pushDataForExchange(outgoingStack(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer), data);
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
        if (data.send(marker, context)) {
          pushDataForExchange(outgoingStack(repositories::DataRepository::{{face.logical_type_name}}StackContainer), data);
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
        if (data.send(marker, context)) {
          pushDataForExchange(outgoingStack(repositories::DataRepository::{{cell.logical_type_name}}StackContainer), data);
        }
      }
{%- endfor %}
{%- endif %}
    }
    break;
  }
}
