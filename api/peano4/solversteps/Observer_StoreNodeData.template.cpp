namespace {
  template <typename Data, typename Block, typename Container, typename Marker>
  inline void storeDataFromBlock(
    Block&        block,
    int           blockPosition,
    Container&    container,
    int           poolIndex,
    const Marker& marker,
    bool          isLastTouch
  ) {
    // A repeat touch always moves the block entry back into its slot; the
    // last touch respects the storage guard, so a guard-off entity leaves
    // its slot alone.
    if (not isLastTouch or Data::store(marker)) {
      container.getPoolSlot(poolIndex) = std::move(block.get(blockPosition));
    }
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::storeNodeData(const ::peano4::grid::GridTraversalEvent& event) {
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for cell in CELLS | reverse %}
  auto popBlock{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(1);
{%- endfor %}
{%- for face in FACES | reverse %}
  auto popBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(TWO_TIMES_D);
{%- endfor %}
{%- for vertex in VERTICES | reverse %}
  auto popBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).popBlock(TWO_POWER_D);
{%- endfor %}

{%- if CELLS %}
  {
    const auto outCellStack        = event.getCellData();
    const auto inCellStackPosition = 0;
    const auto isCreateOrDestroy   = outCellStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData            = outCellStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(outCellStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto       cellMarker  = event.getCellMarker();
      const auto isLastTouch = ::peano4::grid::StackTopology::isInOutStack(outCellStack);

{%- for cell in CELLS | reverse %}
      storeDataFromBlock<{{ cell.full_qualified_type }}>(
        popBlock{{ cell.logical_type_name }},
        inCellStackPosition,
        repositories::DataRepository::{{ cell.logical_type_name }}StackContainer,
        event.getCellPoolIndex(),
        cellMarker,
        isLastTouch
      );
{%- endfor %}
    }
  }
{%- endif %}

{%- if FACES %}
  for (int i = 0; i < TWO_TIMES_D; i++) {
    const auto outFaceStack         = event.getFaceDataTo(i);
    const auto inFaceStackPosition  = event.getFaceDataFrom(i);
    const auto isCreateOrDestroy    = outFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData             = outFaceStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(outFaceStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto faceMarker = event.getFaceMarker();
      faceMarker.select(inFaceStackPosition);
      const auto isLastTouch = ::peano4::grid::StackTopology::isInOutStack(outFaceStack);

{%- for face in FACES | reverse %}
      storeDataFromBlock<{{ face.full_qualified_type }}>(
        popBlock{{ face.logical_type_name }},
        inFaceStackPosition,
        repositories::DataRepository::{{ face.logical_type_name }}StackContainer,
        event.getFacePoolIndex(inFaceStackPosition),
        faceMarker,
        isLastTouch
      );
{%- endfor %}
    }
  }
{%- endif %}

{%- if VERTICES %}
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto outVertexStack         = event.getVertexDataTo(i);
    const auto inVertexStackPosition  = event.getVertexDataFrom(i);
    const auto isCreateOrDestroy      = outVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData               = outVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(outVertexStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto       vertexMarker = ::peano4::datamanagement::VertexMarker(event, inVertexStackPosition);
      const auto isLastTouch  = ::peano4::grid::StackTopology::isInOutStack(outVertexStack);

{%- for vertex in VERTICES | reverse %}
      storeDataFromBlock<{{ vertex.full_qualified_type }}>(
        popBlock{{ vertex.logical_type_name }},
        inVertexStackPosition,
        repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer,
        event.getVertexPoolIndex(inVertexStackPosition),
        vertexMarker,
        isLastTouch
      );
{%- endfor %}
    }
  }
{%- endif %}
}
