// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include <sstream>

#ifdef WITH_MPI
MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Datatype = MPI_DATATYPE_NULL;
#endif
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
  bool printSeparator = false;
{% for attribute in ATTRIBUTE_TO_STRING_PAIRS %}
  if (printSeparator) {
    out << ",";
  }
  out << "{{attribute.name}}=" << {{attribute.expression}};
  printSeparator = true;
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

#ifdef WITH_MPI
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_contiguous(sizeof({{NAMESPACE | join("::")}}::{{CLASSNAME}}), MPI_BYTE, &Datatype);
  MPI_Type_commit(&Datatype);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getForkDatatype() {
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getJoinDatatype() {
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryExchangeDatatype() {
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunicationDatatype() {
  return Datatype;
}
#endif
