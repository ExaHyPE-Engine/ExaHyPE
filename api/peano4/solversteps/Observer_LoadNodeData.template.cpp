namespace {
  template <typename Block>
  inline void resetDataInBlock(
    Block& block,
    int    blockPosition
  ) {
    // pushBlock() reuses physical stack slots. If this traversal does not
    // load data for a slot, the old object in that reused slot would otherwise
    // remain visible and could later be passed to user callbacks, stored, or
    // exchanged as stale mesh data.
    //
    // This reset is therefore a correctness guard, but emplace_default()
    // destroys the old object. For generated patch data this frees reusable
    // heap storage and later data() calls allocate/zero it again. A faster
    // follow-up would be an explicit cheap semantic reset on the data object,
    // e.g. a validity flag or clearForReuse(), that invalidates stale content
    // while keeping reusable payload capacity alive.
    block.emplace_default(blockPosition);
  }

  template <typename Data, typename Block, typename Stack, typename Marker>
  inline void loadDataIntoBlock(
    Block&        block,
    int           blockPosition,
    Stack&        stack,
    const Marker& marker,
    bool          isInOutStack
  ) {
    if (not isInOutStack) {
      assertion(not stack.empty());
      block.set(blockPosition, stack.pop());
      return;
    }

    if (Data::load(marker) and not stack.empty()) {
      block.set(blockPosition, stack.pop());
      return;
    }

    // resetDataInBlock(block, blockPosition);
  }

  template <typename Data, typename Block, typename Stack, typename Marker>
  inline void loadDataIntoBlock(
    Block&        block,
    int           blockPosition,
    Stack&        stack,
    Stack&        hangingStack,
    const Marker& marker,
    bool          isInOutStack,
    bool          isHanging
  ) {
    if (not isInOutStack) {
      assertion(not isHanging);
      assertion(not stack.empty());
      block.set(blockPosition, stack.pop());
      return;
    }

    if (Data::load(marker)) {
      auto& sourceStack = isHanging ? hangingStack : stack;
      if (not sourceStack.empty()) {
        block.set(blockPosition, sourceStack.pop());
        return;
      }
    }

    // resetDataInBlock(block, blockPosition);
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::loadNodeData(const ::peano4::grid::GridTraversalEvent& event) {
  const auto callStack = static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call);

{%- for vertex in VERTICES %}
  auto pushBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(TWO_POWER_D);
{%- endfor %}
{%- for face in FACES %}
  auto pushBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(TWO_TIMES_D);
{%- endfor %}
{%- for cell in CELLS %}
  auto pushBlock{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, callStack).pushBlock(1);
{%- endfor %}

{%- if VERTICES %}
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto inVertexStack          = event.getVertexDataFrom(i);
    const auto outVertexStackPosition = event.getVertexDataTo(i);
    const auto isCreateOrDestroy      = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData               = inVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(inVertexStack >= static_cast<int>(::peano4::grid::StackTopology::BaseStack::Call));

      auto       vertexMarker    = ::peano4::datamanagement::VertexMarker(event, outVertexStackPosition);
      const auto isInOutStack    = ::peano4::grid::StackTopology::isInOutStack(inVertexStack);
      const auto isHangingVertex = vertexMarker.isHanging();

{%- for vertex in VERTICES %}
      // Load vertex {{ vertex.logical_type_name }}
      loadDataIntoBlock<{{ vertex.full_qualified_type }}>(
        pushBlock{{ vertex.logical_type_name }},
        outVertexStackPosition,
        repositories::DataRepository::{{ vertex.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, inVertexStack),
        repositories::DataRepository::{{ vertex.logical_type_name }}HangingStackContainer.getLocalExchangeStack(_spacetreeId, inVertexStack),
        vertexMarker,
        isInOutStack,
        isHangingVertex
      );
{%- endfor %}
    } else {
{%- for vertex in VERTICES %}
      // resetDataInBlock(pushBlock{{ vertex.logical_type_name }}, outVertexStackPosition);
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

      auto       faceMarker    = event.getFaceMarker();
      faceMarker.select(outFaceStackPosition);
      const auto isInOutStack  = ::peano4::grid::StackTopology::isInOutStack(inFaceStack);
      const auto isHangingFace = faceMarker.isHanging();

{%- for face in FACES %}
      // Load face {{ face.logical_type_name }}
      loadDataIntoBlock<{{ face.full_qualified_type }}>(
        pushBlock{{ face.logical_type_name }},
        outFaceStackPosition,
        repositories::DataRepository::{{ face.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, inFaceStack),
        repositories::DataRepository::{{ face.logical_type_name }}HangingStackContainer.getLocalExchangeStack(_spacetreeId, inFaceStack),
        faceMarker,
        isInOutStack,
        isHangingFace
      );
{%- endfor %}
    } else {
{%- for face in FACES %}
      // resetDataInBlock(pushBlock{{ face.logical_type_name }}, outFaceStackPosition);
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
      const auto isInOutStack = ::peano4::grid::StackTopology::isInOutStack(inCellStack);

{%- for cell in CELLS %}
      // Load cell {{ cell.logical_type_name }}
      loadDataIntoBlock<{{ cell.full_qualified_type }}>(
        pushBlock{{ cell.logical_type_name }},
        outCellStackPosition,
        repositories::DataRepository::{{ cell.logical_type_name }}StackContainer.getLocalExchangeStack(_spacetreeId, inCellStack),
        cellMarker,
        isInOutStack
      );
{%- endfor %}
    } else {
{%- for cell in CELLS %}
      // resetDataInBlock(pushBlock{{ cell.logical_type_name }}, outCellStackPosition);
{%- endfor %}
    }
  }
{%- endif %}
}
