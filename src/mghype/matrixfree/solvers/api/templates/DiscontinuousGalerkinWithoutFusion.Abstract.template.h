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

/**
 * Abstract template class for Discontinuous Galerkin solver.
 * 
 * This serves as a copy of the original, but we only perform one action set per sweep
 * 
 */
class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public mghype::matrixfree::solvers::Solver {

  public:
    enum class Action {
      ProjectOntoFaces,
      ProjectIntoCellAndUpdateCellSolution,
      UpdateResidual,
      UpdateFaceSolution
    };

    //! Only necessary to ensure that we can overload toString
    using mghype::matrixfree::solvers::Solver::toString;

    std::string toString(Action state) const;

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

    virtual vertexdata::{{SOLVER_NAME}}::Type getVertexType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    ) final;

    virtual celldata::{{SOLVER_NAME}}::Type getCellType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual facedata::{{SOLVER_NAME}}::Type getFaceType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getLocalAssemblyMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) {% if ASSEMBLY_MATRIX is not defined %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > getRhsMatrix(
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

    bool updateResidual() const;
    bool updateFace() const;
    bool projectOntoFaces() const;
    bool projectOntoCells() const;

    Action getState() const;

  protected:
    static tarch::logging::Log  _log;

    Action _action;
};

