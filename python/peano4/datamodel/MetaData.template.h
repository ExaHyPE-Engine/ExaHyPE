// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/mpi.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/stacks/SendReceiveContext.h"

#include <bitset>
#include <cstdint>
#include <string>

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
  using DoFType = unsigned char;
  static constexpr int Cardinality = 1;
{{ENUM_DECLARATIONS}}

private:
{{ATTRIBUTES}}
{{STATIC_ATTRIBUTES}}

#ifdef WITH_MPI
  static MPI_Datatype Datatype;
#endif

public:
  {{CLASSNAME}}();
  ~{{CLASSNAME}}() noexcept = default;

{{ATTRIBUTE_METHOD_DECLARATIONS}}

  std::string toString() const;
  INLINE_METHOD {{CLASSNAME}} createExchangeView() const {
    return *this;
  }

{% if DATA_ASSOCIATION == 1 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::VertexMarker& marker, int spacetreeId);
  INLINE_METHOD bool send(
    [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext          context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{SEND_CONDITION}};
  }
  INLINE_METHOD bool receiveAndMerge(
    [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext          context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{RECEIVE_AND_MERGE_CONDITION}};
  }
  INLINE_METHOD static bool load([[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker) {
    return true;
  }
  INLINE_METHOD static bool store([[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker) {
    return true;
  }
{% endif -%}

{% if DATA_ASSOCIATION == 2 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::FaceMarker& marker, int spacetreeId);
  INLINE_METHOD bool send(
    [[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{SEND_CONDITION}};
  }
  INLINE_METHOD bool receiveAndMerge(
    [[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{RECEIVE_AND_MERGE_CONDITION}};
  }
  INLINE_METHOD static bool load([[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker) {
    return true;
  }
  INLINE_METHOD static bool store([[maybe_unused]] const ::peano4::datamanagement::FaceMarker& marker) {
    return true;
  }
{% endif -%}

{% if DATA_ASSOCIATION == 3 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::CellMarker& marker, int spacetreeId);
  INLINE_METHOD bool send(
    [[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{SEND_CONDITION}};
  }
  INLINE_METHOD bool receiveAndMerge(
    [[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker,
    [[maybe_unused]] ::peano4::stacks::SendReceiveContext        context
  ) const {
    using enum ::peano4::stacks::SendReceiveContext;
    if ((context == ForkDomain) or (context == JoinDomain)) {
      return true;
    }
    return {{RECEIVE_AND_MERGE_CONDITION}};
  }
  INLINE_METHOD static bool load([[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker) {
    return true;
  }
  INLINE_METHOD static bool store([[maybe_unused]] const ::peano4::datamanagement::CellMarker& marker) {
    return true;
  }
{% endif -%}

#ifdef WITH_MPI
  static void initDatatype();
  static void shutdownDatatype();

  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();
#endif
};
