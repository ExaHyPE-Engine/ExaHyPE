// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

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
