// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/mpi.h"

{{INCLUDES}}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} {
public:
{{ENUM_DECLARATIONS}}

private:
{{ATTRIBUTES}}

#if PEANO_DEBUG
  tarch::la::Vector<DIMENSIONS,double> _debugX;
  tarch::la::Vector<DIMENSIONS,double> _debugH;
#endif

#ifdef PEANO_WITH_MPI
  static MPI_Datatype Datatype;
#endif

public:
  {{CLASSNAME}}();
  ~{{CLASSNAME}}() noexcept = default;

{{ATTRIBUTE_METHOD_DECLARATIONS}}

#if PEANO_DEBUG
  INLINE_METHOD tarch::la::Vector<DIMENSIONS,double> getDebugX() const {
    return _debugX;
  }
  INLINE_METHOD double getDebugX(int index) const {
    return _debugX(index);
  }
  INLINE_METHOD void setDebugX(const tarch::la::Vector<DIMENSIONS,double>& value) {
    _debugX = value;
  }
  INLINE_METHOD void setDebugX(double value) {
    _debugX = value;
  }
  INLINE_METHOD void setDebugX(int index, double value) {
    _debugX(index) = value;
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS,double> getDebugH() const {
    return _debugH;
  }
  INLINE_METHOD double getDebugH(int index) const {
    return _debugH(index);
  }
  INLINE_METHOD void setDebugH(const tarch::la::Vector<DIMENSIONS,double>& value) {
    _debugH = value;
  }
  INLINE_METHOD void setDebugH(double value) {
    _debugH = value;
  }
  INLINE_METHOD void setDebugH(int index, double value) {
    _debugH(index) = value;
  }
#endif

  std::string toString() const;

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
