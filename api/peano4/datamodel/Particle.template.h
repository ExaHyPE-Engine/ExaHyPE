// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

{% if INCLUDES -%}
{{INCLUDES}}
{% endif -%}
namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final {
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

#if PEANO_DEBUG
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getDebugX() const;
  INLINE_METHOD double getDebugX(int index) const;
  INLINE_METHOD void setDebugX(const tarch::la::Vector<DIMENSIONS, double>& value);
  INLINE_METHOD void setDebugX(double value);
  INLINE_METHOD void setDebugX(int index, double value);
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getDebugH() const;
  INLINE_METHOD double getDebugH(int index) const;
  INLINE_METHOD void setDebugH(const tarch::la::Vector<DIMENSIONS, double>& value);
  INLINE_METHOD void setDebugH(double value);
  INLINE_METHOD void setDebugH(int index, double value);
#endif

  std::string toString() const;

private:
{{ATTRIBUTES}}

#if PEANO_DEBUG
  tarch::la::Vector<DIMENSIONS, double> _debugX{};
  tarch::la::Vector<DIMENSIONS, double> _debugH{};
#endif
};

#include "{{NAMESPACE[-1]}}/{{CLASSNAME}}.cpph"
