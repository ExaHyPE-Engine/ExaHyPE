// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% set MARKER_NAME = {1: "VertexMarker", 2: "FaceMarker", 3: "CellMarker"}[DATA_ASSOCIATION] -%}
{% set MARKER_TYPE = "::peano4::dataview::" + MARKER_NAME -%}
#include "peano4/dataview/MetaDataRecord.h"
#include "peano4/dataview/{{MARKER_NAME}}.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

{% if INCLUDES -%}
{{INCLUDES}}
{% endif -%}
namespace {{NAMESPACE | join("::")}} {
  struct {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

struct {{NAMESPACE | join("::")}}::{{CLASSNAME}} final :
  public ::peano4::dataview::MetaDataRecord<{{NAMESPACE | join("::")}}::{{CLASSNAME}}, {{MARKER_TYPE}}> {
public:
{%- if ENUM_DECLARATIONS %}
{{ENUM_DECLARATIONS}}
{% endif %}
  {{CLASSNAME}}() = default;
  {{CLASSNAME}}(const {{CLASSNAME}}&) = default;
  {{CLASSNAME}}({{CLASSNAME}}&&) noexcept = default;
  {{CLASSNAME}}& operator=(const {{CLASSNAME}}&) = default;
  {{CLASSNAME}}& operator=({{CLASSNAME}}&&) noexcept = default;
  ~{{CLASSNAME}}() noexcept = default;

{{ATTRIBUTE_METHOD_DECLARATIONS}}

  std::string toString() const;

  void merge(
    ::peano4::grid::SendReceiveContext  context,
    const {{CLASSNAME}}&                neighbour,
    const {{MARKER_TYPE}}&              marker,
    int                                 treeId
  );

  INLINE_METHOD bool sendCondition(const {{MARKER_TYPE}}& marker) const;
  INLINE_METHOD bool receiveAndMergeCondition(const {{MARKER_TYPE}}& marker) const;

private:
{{ATTRIBUTES}}
};

#include "{{NAMESPACE[-1]}}/{{CLASSNAME}}.cpph"
