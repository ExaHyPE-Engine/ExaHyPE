namespace {
  template <typename Block>
  inline void resetDataInBlock(
    Block& block,
    int    blockPosition
  ) {
    // pushBlock() reuses physical stack slots. Without this reset, the object of
    // an earlier traversal stays visible in a slot that this traversal does not
    // load and reaches user callbacks as stale mesh data.
    block.emplaceDefault(blockPosition);
  }

  template <typename Data, typename Block, typename Container, typename Marker>
  inline void loadDataIntoBlock(
    Block&        block,
    int           blockPosition,
    Container&    container,
    int           poolIndex,
    const Marker& marker,
    bool          isFirstTouch
  ) {
    // A repeat touch always moves the slot back into the block; the first
    // touch respects the storage guard, so a guard-off entity keeps its slot
    // untouched.
    if (not isFirstTouch or Data::load(marker)) {
      block.set(blockPosition, std::move(container.getPoolSlot(poolIndex)));
    } else {
      resetDataInBlock(block, blockPosition);
    }
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::loadNodeData(const ::peano4::grid::GridTraversalEvent& event) {
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for vertex in VERTICES %}
  auto pushBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(TWO_POWER_D);
{%- endfor %}
{%- for face in FACES %}
  auto pushBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(TWO_TIMES_D);
{%- endfor %}
{%- for cell in CELLS %}
  auto pushBlock{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_treeId, callStack).pushBlock(1);
{%- endfor %}

{%- if VERTICES %}
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto inVertexStack          = event.getVertexDataFrom(i);
    const auto outVertexStackPosition = event.getVertexDataTo(i);
    const auto isCreateOrDestroy      = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData               = inVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(inVertexStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto       vertexMarker = ::peano4::datamanagement::VertexMarker(event, outVertexStackPosition);
      const auto isFirstTouch = ::peano4::grid::StackTopology::isInOutStack(inVertexStack);

{%- for vertex in VERTICES %}
      loadDataIntoBlock<{{ vertex.full_qualified_type }}>(
        pushBlock{{ vertex.logical_type_name }},
        outVertexStackPosition,
        repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer,
        event.getVertexPoolIndex(outVertexStackPosition),
        vertexMarker,
        isFirstTouch
      );
{%- endfor %}
    } else {
{%- for vertex in VERTICES %}
      resetDataInBlock(pushBlock{{ vertex.logical_type_name }}, outVertexStackPosition);
{%- endfor %}
    }
  }
{%- endif %}

{%- if FACES %}
  for (int i = 0; i < TWO_TIMES_D; i++) {
    const auto inFaceStack          = event.getFaceDataFrom(i);
    const auto outFaceStackPosition = event.getFaceDataTo(i);
    const auto isCreateOrDestroy    = inFaceStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData             = inFaceStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(inFaceStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto faceMarker = event.getFaceMarker();
      faceMarker.select(outFaceStackPosition);
      const auto isFirstTouch = ::peano4::grid::StackTopology::isInOutStack(inFaceStack);

{%- for face in FACES %}
      loadDataIntoBlock<{{ face.full_qualified_type }}>(
        pushBlock{{ face.logical_type_name }},
        outFaceStackPosition,
        repositories::DataRepository::{{ face.logical_type_name }}StackContainer,
        event.getFacePoolIndex(outFaceStackPosition),
        faceMarker,
        isFirstTouch
      );
{%- endfor %}
    } else {
{%- for face in FACES %}
      resetDataInBlock(pushBlock{{ face.logical_type_name }}, outFaceStackPosition);
{%- endfor %}
    }
  }
{%- endif %}

{%- if CELLS %}
  {
    const auto inCellStack          = event.getCellData();
    const auto outCellStackPosition = 0;
    const auto isCreateOrDestroy    = inCellStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData             = inCellStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(inCellStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto       cellMarker   = event.getCellMarker();
      const auto isFirstTouch = ::peano4::grid::StackTopology::isInOutStack(inCellStack);

{%- for cell in CELLS %}
      loadDataIntoBlock<{{ cell.full_qualified_type }}>(
        pushBlock{{ cell.logical_type_name }},
        outCellStackPosition,
        repositories::DataRepository::{{ cell.logical_type_name }}StackContainer,
        event.getCellPoolIndex(),
        cellMarker,
        isFirstTouch
      );
{%- endfor %}
    } else {
{%- for cell in CELLS %}
      resetDataInBlock(pushBlock{{ cell.logical_type_name }}, outCellStackPosition);
{%- endfor %}
    }
  }
{%- endif %}
}
