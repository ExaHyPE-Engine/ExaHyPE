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

  template <typename Data, typename Block, typename StackContainer, typename StackKey, typename Marker>
  inline void loadDataIntoBlock(
    Block&                block,
    int                   blockPosition,
    StackContainer&       stack,
    const StackKey&       keyIn,
    const Marker&         marker,
    bool                  isInOutStack
  ) {
    if (not isInOutStack) {
      // Intermediate stack
      auto* popStack = stack.getForPop(keyIn);
      assertion(not popStack->empty());
      block.set(blockPosition, popStack->pop());
      return;
    }

    if (Data::load(marker)) {
      if (stack.holdsStack(keyIn)) {
        if (auto* popStack = stack.getForPop(keyIn); not popStack->empty()) {
          block.set(blockPosition, popStack->pop());
          return;
        }
      }
    }

    // resetDataInBlock(block, blockPosition);
  }

  template <typename Data, typename Block, typename StackContainer, typename StackKey, typename Marker>
  inline void loadDataIntoBlock(
    Block&                block,
    int                   blockPosition,
    StackContainer&       stack,
    StackContainer&       hangingStack,
    const StackKey&       keyIn,
    const Marker&         marker,
    bool                  isInOutStack,
    bool                  isHanging
  ) {
    if (not isInOutStack) {
      assertion(not isHanging);
      auto* popStack = stack.getForPop(keyIn);
      assertion(not popStack->empty());
      block.set(blockPosition, popStack->pop());
      return;
    }

    if (Data::load(marker)) {
      auto& sourceStack = isHanging ? hangingStack : stack;
      if (sourceStack.holdsStack(keyIn)) {
        if (auto* popStack = sourceStack.getForPop(keyIn); not popStack->empty()) {
          block.set(blockPosition, popStack->pop());
          return;
        }
      }
    }

    // resetDataInBlock(block, blockPosition);
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::loadNodeData(const ::peano4::grid::GridTraversalEvent& event) {
  const auto keyOut = repositories::DataRepository::DataKey(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for vertex in VERTICES %}
  auto pushBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TWO_POWER_D);
{%- endfor %}
{%- for face in FACES %}
  auto pushBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(TWO_TIMES_D);
{%- endfor %}
{%- for cell in CELLS %}
  auto pushBlock{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPush(keyOut)->pushBlock(1);
{%- endfor %}

{%- if VERTICES %}
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto inVertexStack          = event.getVertexDataFrom(i);
    const auto outVertexStackPosition = event.getVertexDataTo(i);
    const auto isCreateOrDestroy      = inVertexStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData               = inVertexStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(inVertexStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto vertexKeyIn     = repositories::DataRepository::DataKey(_spacetreeId, inVertexStack);
      auto       vertexMarker    = ::peano4::datamanagement::VertexMarker(event, outVertexStackPosition);
      const auto isInOutStack    = ::peano4::grid::PeanoCurve::isInOutStack(inVertexStack);
      const auto isHangingVertex = vertexMarker.isHanging();

{%- for vertex in VERTICES %}
      // Load vertex {{ vertex.logical_type_name }}
      loadDataIntoBlock<{{ vertex.full_qualified_type }}>(
        pushBlock{{ vertex.logical_type_name }},
        outVertexStackPosition,
        repositories::DataRepository::{{ vertex.logical_type_name }}Stack,
        repositories::DataRepository::{{ vertex.logical_type_name }}HangingStack,
        vertexKeyIn,
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
      assertion(inFaceStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto faceKeyIn  = repositories::DataRepository::DataKey(_spacetreeId, inFaceStack);
      auto       faceMarker = event.getFaceMarker();
      faceMarker.select(outFaceStackPosition);
      const auto isInOutStack  = ::peano4::grid::PeanoCurve::isInOutStack(inFaceStack);
      const auto isHangingFace = faceMarker.isHanging();

{%- for face in FACES %}
      // Load face {{ face.logical_type_name }}
      loadDataIntoBlock<{{ face.full_qualified_type }}>(
        pushBlock{{ face.logical_type_name }},
        outFaceStackPosition,
        repositories::DataRepository::{{ face.logical_type_name }}Stack,
        repositories::DataRepository::{{ face.logical_type_name }}HangingStack,
        faceKeyIn,
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
      assertion(inCellStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto cellKeyIn    = repositories::DataRepository::DataKey(_spacetreeId, inCellStack);
      auto       cellMarker   = event.getCellMarker();
      const auto isInOutStack = ::peano4::grid::PeanoCurve::isInOutStack(inCellStack);

{%- for cell in CELLS %}
      // Load cell {{ cell.logical_type_name }}
      loadDataIntoBlock<{{ cell.full_qualified_type }}>(
        pushBlock{{ cell.logical_type_name }},
        outCellStackPosition,
        repositories::DataRepository::{{ cell.logical_type_name }}Stack,
        cellKeyIn,
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
