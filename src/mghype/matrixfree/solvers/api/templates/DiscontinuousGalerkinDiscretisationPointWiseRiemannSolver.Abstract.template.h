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

#include "mghype/matrixfree/solvers/Solver.h"

{{SOLVER_INCLUDES}}

{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public mghype::matrixfree::solvers::Solver {

  public:
    enum class State {
      UpdateAndProjectImmediately,
      ProjectOnly,
      Suspend
    };

    static constexpr double     MinH = {{MIN_H}};
    static constexpr double     MaxH = {{MAX_H}};

    static constexpr double     OmegaCell              = {{CELL_RELAXATION}};
    static constexpr double     OmegaFace              = {{FACE_RELAXATION}};
    static constexpr int        VertexUnknowns         = {{VERTEX_CARDINALITY}};
    static constexpr int        CellUnknowns           = {{CELL_CARDINALITY}};
    static constexpr int        FaceUnknownsSolution   = {{FACE_CARDINALITY_SOLUTION}};
    static constexpr int        FaceUnknownsProjection = {{FACE_CARDINALITY_PROJECTION}};

    static constexpr int        NodesPerCell           = {{NODES_PER_CELL}};
    static constexpr int        NodesPerFace           = {{NODES_PER_FACE}};

    static constexpr int        UnknownsPerCellNode    = {{UNKNOWNS_PER_CELL_NODE}};
    static constexpr int        SolutionsPerFaceNode   = {{SOLUTIONS_PER_FACE_NODE}};
    static constexpr int        ProjectionsPerFaceNode = {{PROJECTIONS_PER_FACE_NODE}};

    static constexpr int        PolyDegree             = {{POLY_DEGREE}};
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
    ) {% if ASSEMBLY_MATRIX is not defined %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getMassMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) {% if MASS_MATRIX is not defined %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getInvertedApproxSystemMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) {% if APPROX_SYSTEM_MATRIX is not defined %} = 0 {% endif %};

    // no arguments needed
    virtual tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > getCellFromFaceMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    );

    // no arguments needed
    virtual tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > getFaceFromCellMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    );

    virtual tarch::la::Matrix< FaceUnknownsSolution, FaceUnknownsProjection, double > getRiemannMatrix()
      {% if RIEMANN_MATRIX is not defined %} = 0 {% endif %};

    virtual tarch::la::Matrix< FaceUnknownsSolution, FaceUnknownsSolution, double >   getBoundaryConditionMatrix()
      {% if BOUNDARY_MATRIX is not defined %} = 0 {% endif %};

    virtual void beginMeshSweep() override;

    /**
     * End the mesh sweep
     *
     * Toggle the solver's state into the next valid one.
     */
    virtual void endMeshSweep() override;

    bool updateCell() const;
    bool updateFace() const;
    bool projectOntoFaces() const;

    /**
     * Suspends solver for one sweep
     *
     * After this sweep, the solver will toggle back to the normal solver
     * state.
     */
    void suspend(bool projectOntoFaces);
  protected:
    static tarch::logging::Log  _log;

    State _state;
};

