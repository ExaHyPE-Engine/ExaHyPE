// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/mpi.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/SwapHelper.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "peano4/Globals.h"

{% if FLOAT_TYPE=="std::float16_t" or FLOAT_TYPE=="std::bfloat16_t" %}
#include <stdfloat>
{% endif %}

{{INCLUDES}}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  struct {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

struct {{NAMESPACE | join("::")}}::{{CLASSNAME}} {
public:
  using DoFType = {{FLOAT_TYPE}};

#if DIMENSIONS == 2
  static constexpr int Cardinality = {{CARDINALITY_2D}};
#else
  static constexpr int Cardinality = {{CARDINALITY_3D}};
#endif

private:
  DoFType* _data;

  template <typename LeftData, typename RightData>
  friend struct ::peano4::datamanagement::SwapHelper;

public:
  explicit {{CLASSNAME}}() noexcept;
  ~{{CLASSNAME}}() noexcept;

  INLINE_METHOD {{CLASSNAME}}(const {{CLASSNAME}}& other):
    _data(nullptr) {
    *this = other;
  }

  INLINE_METHOD {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) {
    if (this == &other) {
      return *this;
    }

    if (other._data == nullptr) {
      // A null pointer is the only empty-state marker. Clear the target so
      // reused stack slots cannot expose data from an earlier traversal.
      // This is semantically required for stack reuse, but it gives up the
      // target's reusable heap buffer.
      // TODO: Remove, once we have memory pools.
      if (_data != nullptr) {
        tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
        _data = nullptr;
      }
      return *this;
    }

    // Reuse existing memory
    // If we already have memory, don't free and malloc again. Just overwrite.
    if (_data == nullptr) {
      _data = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
    }

    std::memcpy(_data, other._data, Cardinality * sizeof(DoFType));
    return *this;
  }

  INLINE_METHOD {{CLASSNAME}}({{CLASSNAME}}&& other) noexcept:
    _data(std::exchange(other._data, nullptr)) {}

  INLINE_METHOD {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) noexcept {
    std::swap(_data, other._data);
    return *this;
  }

  {{CLASSNAME}} createExchangeView();

  DoFType* data();

  INLINE_METHOD const DoFType* data() const noexcept {
    assertion(_data != nullptr);
    return _data;
  }

  template <typename OtherData>
  INLINE_METHOD void swap(OtherData& other) noexcept {
    ::peano4::datamanagement::SwapHelper<{{NAMESPACE | join("::")}}::{{CLASSNAME}}, OtherData>::apply(*this, other);
  }

{% if DATA_ASSOCIATION == 1 -%}
  bool send(const ::peano4::datamanagement::VertexMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  bool receiveAndMerge(const ::peano4::datamanagement::VertexMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  void merge(::peano4::stacks::SendReceiveContext context, {{CLASSNAME}}&& neighbour, const ::peano4::datamanagement::VertexMarker& marker, int spacetreeId);

  INLINE_METHOD static bool load(const ::peano4::datamanagement::VertexMarker& marker) {
    // Local persistence is intentionally topology-driven. Solver-state guards
    // belong to send()/receiveAndMerge() and only affect exchange.
    return not marker.hasBeenRefined();
  }
  INLINE_METHOD static bool store(const ::peano4::datamanagement::VertexMarker& marker) {
    return not marker.willBeRefined();
  }
{% endif -%}

{% if DATA_ASSOCIATION == 2 -%}
  bool send(const ::peano4::datamanagement::FaceMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  bool receiveAndMerge(const ::peano4::datamanagement::FaceMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  void merge(::peano4::stacks::SendReceiveContext context, {{CLASSNAME}}&& neighbour, const ::peano4::datamanagement::FaceMarker& marker, int spacetreeId);

  INLINE_METHOD static bool load(const ::peano4::datamanagement::FaceMarker& marker) {
    // Local persistence is intentionally topology-driven. Solver-state guards
    // belong to send()/receiveAndMerge() and only affect exchange.
    return not marker.hasBeenRefined();
  }
  INLINE_METHOD static bool store(const ::peano4::datamanagement::FaceMarker& marker) {
    return not marker.willBeRefined();
  }
{% endif -%}

{% if DATA_ASSOCIATION == 3 -%}
  bool send(const ::peano4::datamanagement::CellMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  bool receiveAndMerge(const ::peano4::datamanagement::CellMarker& marker, ::peano4::stacks::SendReceiveContext context) const;
  void merge(::peano4::stacks::SendReceiveContext context, {{CLASSNAME}}&& neighbour, const ::peano4::datamanagement::CellMarker& marker, int spacetreeId);

  INLINE_METHOD static bool load(const ::peano4::datamanagement::CellMarker& marker) {
    // Local persistence is intentionally topology-driven. Solver-state guards
    // belong to send()/receiveAndMerge() and only affect exchange.
    return not marker.hasBeenRefined();
  }
  INLINE_METHOD static bool store(const ::peano4::datamanagement::CellMarker& marker) {
    return not marker.willBeRefined();
  }
{% endif -%}

#ifdef PEANO_WITH_MPI
  static void initDatatype();
  static void shutdownDatatype();

  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();
#endif
};
