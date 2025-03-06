#pragma once


#include "Abstract{{CLASSNAME}}.h"


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
     * Initialise a vertex
     *
     * This routine is called once throughout the mesh construction. Users
     * are supposed to set an initial value and the right-hand side. This
     * routine is only called for real degrees of freedom, i.e. for inner
     * vertices on the finest mesh. Boundary vertices are initialised through
     * setBoundaryConditions() once per mesh traversal (so you can realise
     * non-Dirichlet conditions). The domain is determined by getVertexType()
     * which you find in the superclass. You might however want to overwrite it.
     * If you alter getVertexType(), you should also overwrite getCellType()
     * such that the two routines return consistent results.
     */
    {%if CUSTOM_INIT_VERTEX=="" %}
    virtual void initVertex(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
    ) override;
    {% endif %}

    {%if CUSTOM_BOUNDARY_CONDITIONS=="" %}
    virtual void setBoundaryConditions(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value
    ) override;
    {% endif %}
};

