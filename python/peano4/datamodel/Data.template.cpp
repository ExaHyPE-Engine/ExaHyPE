// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

#include <cstring> // std::memcpy, std::memset
#include <utility> // std::exchange

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _data(tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap)) {

  std::memset(_data, 0, Cardinality * sizeof(DoFType));
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(ObjectConstruction) noexcept:
  _data(nullptr) {}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() noexcept {
  if (_data) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    _data = nullptr;
  }
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other):
  _data(nullptr) {
  clone(other);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}({{CLASSNAME}}&& other) noexcept:
  _data(std::exchange(other._data, nullptr)) {}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=({{CLASSNAME}}&& other) noexcept {
  if (this != &other) {
    if (_data) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap); // TODO: Is this needed? How often is this called?
    }
    _data = std::exchange(other._data, nullptr);
  }
  return *this;
}

// {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFType* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::data() {
//   if (not _data) {
//     _data = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
//   }
//   return _data;
// }

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone(const {{CLASSNAME}}& other) {
  if (this == &other) return;

  if (other._data == nullptr) {
    // other has no data: just reset
    if (_data) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
      _data = nullptr;
    }
    return;
  }

  DoFType* newbuf = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
  assertion(newbuf);
  std::memcpy(newbuf, other._data, Cardinality * sizeof(DoFType));

  // Commit
  if (_data) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
  }
  _data = newbuf;
}

{% if DATA_ASSOCIATION == 1 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send([[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker) const {
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge([[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag([[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif %}

{% if DATA_ASSOCIATION == 2 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send([[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker) const {
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge([[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag([[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif %}

{% if DATA_ASSOCIATION == 3 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send([[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker) const {
  return {{SEND_CONDITION}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge([[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}

::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag([[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif %}

#ifdef WITH_MPI
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getForkDatatype() {
  return {{MPI_FLOAT_TYPE}};
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getJoinDatatype() {
  return {{MPI_FLOAT_TYPE}};
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryExchangeDatatype() {
  return {{MPI_FLOAT_TYPE}};
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMultiscaleDataExchangeDatatype() {
  return {{MPI_FLOAT_TYPE}};
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunicationDatatype() {
  return {{MPI_FLOAT_TYPE}};
}
#endif
