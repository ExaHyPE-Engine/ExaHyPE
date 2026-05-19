namespace {
  template <typename Data, typename Block, typename StackContainer, typename StackKey, typename Marker>
  inline void storeDataFromBlock(
    Block&                block,
    int                   blockPosition,
    StackContainer&       stack,
    const StackKey&       keyOut,
    const Marker&         marker,
    bool                  isInOutStack
  ) {
    if (not isInOutStack or Data::store(marker)) {
      auto& data = block.get(blockPosition);
      stack.getForPush(keyOut)->push(std::move(data));
    }
  }

  template <typename Data, typename Block, typename StackContainer, typename StackKey, typename Marker>
  inline void storeDataFromBlock(
    Block&                block,
    int                   blockPosition,
    StackContainer&       stack,
    StackContainer&       hangingStack,
    const StackKey&       keyOut,
    const Marker&         marker,
    bool                  isInOutStack,
    bool                  isHanging
  ) {
    if (not isInOutStack or Data::store(marker)) {
      auto& data        = block.get(blockPosition);
      auto& targetStack = isHanging ? hangingStack : stack;
      targetStack.getForPush(keyOut)->push(std::move(data));
    }
  }
}

void {{FULL_QUALIFIED_CLASSNAME}}::storeNodeData(const ::peano4::grid::GridTraversalEvent& event) {
  const auto keyIn = repositories::DataRepository::DataKey(_spacetreeId, ::peano4::grid::PeanoCurve::CallStack);

{%- for cell in CELLS | reverse %}
  auto popBlock{{ cell.logical_type_name }} = repositories::DataRepository::{{ cell.logical_type_name }}Stack.getForPop(keyIn)->popBlock(1);
{%- endfor %}
{%- for face in FACES | reverse %}
  auto popBlock{{ face.logical_type_name }} = repositories::DataRepository::{{ face.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TWO_TIMES_D);
{%- endfor %}
{%- for vertex in VERTICES | reverse %}
  auto popBlock{{ vertex.logical_type_name }} = repositories::DataRepository::{{ vertex.logical_type_name }}Stack.getForPop(keyIn)->popBlock(TWO_POWER_D);
{%- endfor %}

{%- if CELLS %}
  {
    const auto outCellStack        = event.getCellData();
    const auto inCellStackPosition = 0;
    const auto isCreateOrDestroy   = outCellStack == ::peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    const auto isNoData            = outCellStack == ::peano4::grid::GridTraversalObserver::NoData;

    if (not isCreateOrDestroy and not isNoData) {
      assertion(outCellStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto cellKeyOut   = repositories::DataRepository::DataKey(_spacetreeId, outCellStack);
      auto       cellMarker   = event.getCellMarker();
      const auto isInOutStack = ::peano4::grid::PeanoCurve::isInOutStack(outCellStack);

{%- for cell in CELLS | reverse %}
      // Store cell {{ cell.logical_type_name }}
      storeDataFromBlock<{{ cell.full_qualified_type }}>(
        popBlock{{ cell.logical_type_name }},
        inCellStackPosition,
        repositories::DataRepository::{{ cell.logical_type_name }}Stack,
        cellKeyOut,
        cellMarker,
        isInOutStack
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
      assertion(outFaceStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto faceKeyOut = repositories::DataRepository::DataKey(_spacetreeId, outFaceStack);
      auto       faceMarker = event.getFaceMarker();
      faceMarker.select(inFaceStackPosition);
      const auto isInOutStack  = ::peano4::grid::PeanoCurve::isInOutStack(outFaceStack);
      const auto isHangingFace = faceMarker.isHanging();

{%- for face in FACES | reverse %}
      // Store face {{ face.logical_type_name }}
      storeDataFromBlock<{{ face.full_qualified_type }}>(
        popBlock{{ face.logical_type_name }},
        inFaceStackPosition,
        repositories::DataRepository::{{ face.logical_type_name }}Stack,
        repositories::DataRepository::{{ face.logical_type_name }}HangingStack,
        faceKeyOut,
        faceMarker,
        isInOutStack,
        isHangingFace
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
      assertion(outVertexStack >= ::peano4::grid::PeanoCurve::CallStack);

      const auto vertexKeyOut    = repositories::DataRepository::DataKey(_spacetreeId, outVertexStack);
      auto       vertexMarker    = ::peano4::datamanagement::VertexMarker(event, inVertexStackPosition);
      const auto isInOutStack    = ::peano4::grid::PeanoCurve::isInOutStack(outVertexStack);
      const auto isHangingVertex = vertexMarker.isHanging();

{%- for vertex in VERTICES | reverse %}
      // Store vertex {{ vertex.logical_type_name }}
      storeDataFromBlock<{{ vertex.full_qualified_type }}>(
        popBlock{{ vertex.logical_type_name }},
        inVertexStackPosition,
        repositories::DataRepository::{{ vertex.logical_type_name }}Stack,
        repositories::DataRepository::{{ vertex.logical_type_name }}HangingStack,
        vertexKeyOut,
        vertexMarker,
        isInOutStack,
        isHangingVertex
      );
{%- endfor %}
    }
  }
{%- endif %}
}
