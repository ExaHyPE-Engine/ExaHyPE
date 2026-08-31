// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() noexcept {
  if (_data != nullptr) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    _data = nullptr;
  }
}

auto {{NAMESPACE | join("::")}}::{{CLASSNAME}}::data() -> DoFType* {
  if (_data == nullptr) {
    _data = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
    assertion(_data != nullptr);
    std::memset(_data, 0, Cardinality * sizeof(DoFType));
  }
  return _data;
}

auto {{NAMESPACE | join("::")}}::{{CLASSNAME}}::createExchangeView() -> {{CLASSNAME}} {
  if ({{USE_COPY_FOR_EXCHANGE}}) {
    {{CLASSNAME}} view = *this;
    return view;
  } else {
    return std::move(*this);
  }
}

{% if DATA_ASSOCIATION == 1 %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext          context
) const {
  if (not ({{CAN_SEND}})) {
    return false;
  }
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext          context
) const {
  if (not ({{CAN_RECEIVE}})) {
    return false;
  }
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::stacks::SendReceiveContext          context,
  {{CLASSNAME}}&&                               neighbour,
  const ::peano4::datamanagement::VertexMarker& marker,
  int                                           treeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}

{% if DATA_ASSOCIATION == 2 %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
) const {
  if (not ({{CAN_SEND}})) {
    return false;
  }
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
) const {
  if (not ({{CAN_RECEIVE}})) {
    return false;
  }
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::stacks::SendReceiveContext        context,
  {{CLASSNAME}}&&                             neighbour,
  const ::peano4::datamanagement::FaceMarker& marker,
  int                                         treeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}

{% if DATA_ASSOCIATION == 3 %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
) const {
  if (not ({{CAN_SEND}})) {
    return false;
  }
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
) const {
  if (not ({{CAN_RECEIVE}})) {
    return false;
  }
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::stacks::SendReceiveContext        context,
  {{CLASSNAME}}&&                             neighbour,
  const ::peano4::datamanagement::CellMarker& marker,
  int                                         treeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}
