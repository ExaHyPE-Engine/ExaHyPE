// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
{%- set MARKER = "VertexMarker" if DATA_ASSOCIATION == 1 else ("FaceMarker" if DATA_ASSOCIATION == 2 else "CellMarker") %}

#include "peano4/data/PooledRecord.h"
#include "peano4/dataview/{{MARKER}}.h"
#include "peano4/grid/SendReceiveContext.h"
#include "tarch/la/Vector.h"

{% if FLOAT_TYPE=="std::float16_t" or FLOAT_TYPE=="std::bfloat16_t" %}
#include <stdfloat>
{% endif %}

{{INCLUDES}}

namespace {{NAMESPACE | join("::")}} {
  struct {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

/**
 * Stores patch data for one grid entity in the pool of this type and decides
 * which entities cross a rank or task boundary.
 */
struct {{NAMESPACE | join("::")}}::{{CLASSNAME}} final
  : ::peano4::data::PooledRecord<{{NAMESPACE | join("::")}}::{{CLASSNAME}}, {{FLOAT_TYPE}}, {{CARDINALITY_2D}}, {{CARDINALITY_3D}}> {
  bool send(const ::peano4::dataview::{{MARKER}}& marker, ::peano4::grid::SendReceiveContext context) const;
  bool receiveAndMerge(const ::peano4::dataview::{{MARKER}}& marker, ::peano4::grid::SendReceiveContext context) const;
  void merge(
    ::peano4::grid::SendReceiveContext    context,
    const {{CLASSNAME}}&                  neighbour,
    const ::peano4::dataview::{{MARKER}}& marker,
    int                                   treeId
  );
};
