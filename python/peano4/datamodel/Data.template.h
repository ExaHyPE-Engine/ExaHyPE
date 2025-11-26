// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/mpi.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/grid/LoadStoreComputeFlag.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "peano4/utils/Globals.h"

#include <string>
#include <memory>

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

public:
  explicit {{CLASSNAME}}();
  enum class ObjectConstruction { NoData };
  explicit {{CLASSNAME}}(ObjectConstruction) noexcept;
  ~{{CLASSNAME}}() noexcept;

  {{CLASSNAME}}(const {{CLASSNAME}}& other);
  {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) = delete;
  {{CLASSNAME}}({{CLASSNAME}}&& other) noexcept;
  {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) noexcept;

  INLINE_METHOD DoFType* data() noexcept {
    return _data;
  }
  INLINE_METHOD const DoFType* data() const noexcept {
    return _data;
  }

  void clone(const {{CLASSNAME}}& other);

{% if DATA_ASSOCIATION == 1 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::VertexMarker& marker, int spacetreeId);
  bool send(const ::peano4::datamanagement::VertexMarker& marker) const;
  bool receiveAndMerge(const ::peano4::datamanagement::VertexMarker& marker) const;
  static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const ::peano4::datamanagement::VertexMarker& marker);
{% endif -%}

{% if DATA_ASSOCIATION == 2 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::FaceMarker& marker, int spacetreeId);
  bool send(const ::peano4::datamanagement::FaceMarker& marker) const;
  bool receiveAndMerge(const ::peano4::datamanagement::FaceMarker& marker) const;
  static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const ::peano4::datamanagement::FaceMarker& marker);
{% endif -%}

{% if DATA_ASSOCIATION == 3 -%}
  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::CellMarker& marker, int spacetreeId);
  bool send(const ::peano4::datamanagement::CellMarker& marker) const;
  bool receiveAndMerge(const ::peano4::datamanagement::CellMarker& marker) const;
  static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const ::peano4::datamanagement::CellMarker& marker);
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
