#pragma once

#include "tarch/la/Vector.h"
#include "tarch/la/Matrix.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/accelerator/accelerator.h"

#include "peano4/utils/Globals.h"

#include "Constants.h"
#include "vertexdata/{{SOLVER_NAME}}.h"
#include "celldata/{{SOLVER_NAME}}.h"
#include "facedata/{{SOLVER_NAME}}.h"

{{SOLVER_INCLUDES}}

{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} {

  public:
    static constexpr double     MinH = {{MIN_H}};
    static constexpr double     MaxH = {{MAX_H}};

    static constexpr double     Omega = 0.6;
    // static constexpr int        VertexUnknowns = {{VERTEX_CARDINALITY}};

    {{CLASSNAME}}();
    virtual ~{{CLASSNAME}}();

    /**
     * Initialise a cell degree of freedom
     */
    virtual void initCell(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  solution,
      tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  rhs
    ) = 0;

    /**
     * Initialise a face degree of freedom
     */
    virtual void initCell(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  solution,
      tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  rhs
    ) = 0;

    virtual vertexdata::{{SOLVER_NAME}}::Type getVertexDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    ) final;

    virtual celldata::{{SOLVER_NAME}}::Type getCellDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    // virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getLocalAssemblyMatrix(
    //   const tarch::la::Vector<Dimensions, double>&  cellCentre,
    //   const tarch::la::Vector<Dimensions, double>&  cellSize
    // );

    // virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getMassMatrix(
    //   const tarch::la::Vector<Dimensions, double>&  cellCentre,
    //   const tarch::la::Vector<Dimensions, double>&  cellSize
    // );

  protected:
    static tarch::logging::Log  _log;

    double     _minH;
    double     _maxH;

    tarch::multicore::BooleanSemaphore  _semaphore;
};

