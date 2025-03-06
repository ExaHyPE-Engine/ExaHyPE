#pragma once


#include "Abstract{{CLASSNAME}}.h"
#include <mutex>

{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}


class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public {{NAMESPACE | join("::")}}::Abstract{{CLASSNAME}} {
  public:

    /**
     * Default constructor
     *
     * @todo Please add your documentation here.
     */
    {{CLASSNAME}}();

    /**
     * Destructor
     *
     * Has to be virtual, as there is a superclass with virtual functions.
     *
     * @todo Please add your documentation here.
     */
    virtual ~{{CLASSNAME}}();

    {% if CUSTOM_INIT_CELL=="" %}
    virtual void initCellDoF(
        const tarch::la::Vector<Dimensions, double>&  x,
        const tarch::la::Vector<Dimensions, double>&  h,
        tarch::la::Vector< UnknownsPerCellNode, double >&    value,
        tarch::la::Vector< UnknownsPerCellNode, double >&    rhs,
        tarch::la::Vector< Dimensions, int >&         node
    ) override;
    {% endif %}
};
