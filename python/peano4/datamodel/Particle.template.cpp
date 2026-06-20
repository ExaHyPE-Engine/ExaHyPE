// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#ifdef WITH_MPI
MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Datatype = MPI_DATATYPE_NULL;
#endif

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(){% if CONSTRUCTOR_INITIALISERS %}:
  {{CONSTRUCTOR_INITIALISERS}}
{% endif %}
{}

{{ATTRIBUTE_METHOD_DEFINITIONS}}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream out;
  out << "(";
  bool printSeparator = false;
{% for attribute in ATTRIBUTE_TO_STRING_PAIRS %}
  if (printSeparator) {
    out << ",";
  }
  out << "{{attribute.name}}=" << {{attribute.expression}};
  printSeparator = true;
{% endfor %}
#if PEANO_DEBUG
  if (printSeparator) {
    out << ",";
  }
  out << "debugX=" << getDebugX();
  out << ",debugH=" << getDebugH();
  printSeparator = true;
#endif
  out << ")";
  return out.str();
}

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
  initDatatype();
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getJoinDatatype() {
  initDatatype();
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryExchangeDatatype() {
  initDatatype();
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMultiscaleDataExchangeDatatype() {
  initDatatype();
  return Datatype;
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunicationDatatype() {
  initDatatype();
  return Datatype;
}
#endif
