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

// for openblas
#include <cblas.h>
#include <lapacke.h>

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

    static constexpr double     OmegaCell = 0.6;
    static constexpr double     OmegaFace = 0.6;
    static constexpr int        VertexUnknowns         = {{VERTEX_CARDINALITY}};
    static constexpr int        CellUnknowns           = {{CELL_CARDINALITY}};
    static constexpr int        FaceUnknownsSolution   = {{FACE_CARDINALITY_SOLUTION}};
    static constexpr int        FaceUnknownsProjection = {{FACE_CARDINALITY_PROJECTION}};

    static constexpr int        NodesPerCell           = {{NODES_PER_CELL}};
    static constexpr int        NodesPerFace           = {{NODES_PER_FACE}};

    static constexpr int        UnknownsPerCellNode    = {{UNKNOWNS_PER_CELL_NODE}};
    static constexpr int        UnknownsPerFaceNode    = {{UNKNOWNS_PER_FACE_NODE}};

  private:
    // make this a variable class member, so we can modify it later.
    tarch::la::Matrix< CellUnknowns, CellUnknowns, double > AssemblyMatrix = 
    {
      {{ASSEMBLY_MATRIX[0]| join(", ")}}
      {% for ROW in ASSEMBLY_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  public:
    {{CLASSNAME}}();
    virtual ~{{CLASSNAME}}();

    /**
     * Initialise a cell degree of freedom
     */
    virtual void initCell(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< CellUnknowns, double >&  solution,
      tarch::la::Vector< CellUnknowns, double >&  rhs
    ) = 0;

    /**
     * Initialise a face degree of freedom
     */
    virtual void initFace(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< FaceUnknownsSolution  ,double >&  solution,
      tarch::la::Vector< FaceUnknownsProjection,double >& projection
    ) = 0;

    virtual void initVertex(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< VertexUnknowns, double >&  solution,
      tarch::la::Vector< VertexUnknowns, double >&  rhs
    ) = 0;

    virtual vertexdata::{{SOLVER_NAME}}::Type getVertexDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    ) final;

    virtual celldata::{{SOLVER_NAME}}::Type getCellDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual facedata::{{SOLVER_NAME}}::Type getFaceDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getLocalAssemblyMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    );

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getInvertedAssemblyMatrix(
      // const tarch::la::Vector<Dimensions, double>&  cellCentre,
      // const tarch::la::Vector<Dimensions, double>&  cellSize
    );

    // virtual void updateAssemblyMatrix();

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getMassMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    );

    // no arguments needed
    virtual tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > getCellFromFaceMatrix(
    );

    // no arguments needed
    virtual tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > getFaceFromCellMatrix(
    );

    virtual tarch::la::Matrix< FaceUnknownsSolution, FaceUnknownsProjection, double > getRiemannMatrix();

  protected:
    static tarch::logging::Log  _log;

    double     _minH;
    double     _maxH;

    tarch::multicore::BooleanSemaphore  _semaphore;
};

