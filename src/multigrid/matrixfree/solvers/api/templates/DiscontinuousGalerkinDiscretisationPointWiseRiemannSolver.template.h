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

    virtual void initVertex(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< VertexUnknowns, double >&  value,
      tarch::la::Vector< VertexUnknowns, double >&  rhs
    ) override;

    virtual void initCell(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< CellUnknowns, double >&  value,
      tarch::la::Vector< CellUnknowns, double >&  rhs,
      tarch::la::Vector< CellUnknowns, double >&  exact
    ) override;

    virtual void initFace(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< FaceUnknownsSolution  , double >&  solution,
      tarch::la::Vector< FaceUnknownsProjection, double >&  projection
    ) override;

    virtual bool absValueWithinTolerance() override;

    void setMaxAbs(double);
    void updateMaxAbs(double);

  private:
    double     currentMaxAbs;
    std::mutex maxAbsMtx;
};
