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

    /**
     * Initialise cell
     *
     * Set the initial value of a cell as well as its right-hand side. The
     * choice of an initial guess should not have an impact on the final
     * result, but obviously determines the number of iterations.
     */
    virtual void initCell(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< UnknownsPerCellNode, double >&  value,
      tarch::la::Vector< UnknownsPerCellNode, double >&  rhs
    ) override;

    /**
     * Set boundary values
     *
     * Boundary values are imposed through the boundary faces. Hence,
     * this routine is invoked over the points on a face.
     */
    virtual void setBoundaryConditions(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< UnknownsPerCellNode, double >&  solution
    ) override;
};
