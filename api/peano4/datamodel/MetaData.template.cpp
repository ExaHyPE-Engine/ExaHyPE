// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{{STATIC_ATTRIBUTE_DEFINITIONS}}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(){% if CONSTRUCTOR_INITIALISERS %}:
  {{CONSTRUCTOR_INITIALISERS}}
{% endif %}
{}

{{ATTRIBUTE_METHOD_DEFINITIONS}}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream out;
  out << std::boolalpha;
  out << "(";
{% for attribute in ATTRIBUTE_TO_STRING_PAIRS %}
  out << "{% if not loop.first %},{% endif %}{{attribute.name}}=" << {{attribute.expression}};
{% endfor %}
  out << ")";
  return out.str();
}

{% if DATA_ASSOCIATION == 1 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext           context,
  [[maybe_unused]] const {{CLASSNAME}}&                           neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker&  marker,
  [[maybe_unused]] int                                            spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}

{% if DATA_ASSOCIATION == 2 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext         context,
  [[maybe_unused]] const {{CLASSNAME}}&                         neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::FaceMarker&  marker,
  [[maybe_unused]] int                                          spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}

{% if DATA_ASSOCIATION == 3 %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext         context,
  [[maybe_unused]] const {{CLASSNAME}}&                         neighbour,
  [[maybe_unused]] const ::peano4::datamanagement::CellMarker&  marker,
  [[maybe_unused]] int                                          spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}
{% endif %}
