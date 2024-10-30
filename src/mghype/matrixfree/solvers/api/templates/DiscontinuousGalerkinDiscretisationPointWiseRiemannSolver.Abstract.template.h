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
 * The flow of the solver states is controlled by its internal state _state, which has 4
 * possible values. A discussion of them is incomplete without reviewing the 4 fundamental
 * operations that the DG Solver itself performs. The fact that there are 4 solver states
 * and 4 separate action sets is unfortunately a coincidence. 
 * 
 * When each action set is entered, the non-trivial operations are guarded by an if statement,
 * which checks:
 *  - Whether we are on the correct level of refinement (typically the finest level is all we care about)
 *  - Whether the Abstract DG Solver (this class) is in the correct programme phase to perform the given operation.
 * 
 * In brief, the 4 fundamental operations are as follows (mentioned in the order in which they are encountered):
 *  1. UpdateFaceSolution - this takes projected values (initially 0) on the faces and averages them, to get a true
 *      picture of the current solution on faces
 *  2. ProjectIntoCellAndUpdateCellSolution - this takes the newly  updated face solution and uses them to update the
 *      solutions in the cells
 *  3. ProjectOntoFaces - this takes the newly updated cell solutions and projects them back onto the faces
 *  4. UpdateResidual - this resets the internally-held residual vector to be a linear mixture of the current cell solution and the RHS.
 * 
 * Note that a full picture of the residual from sweep N is not complete until sweep N+1 - this is because the residual update
 * begins at stage 4 of sweep N and is completed at stage 2 of sweep N+1.
 * 
 * When this solver is used as a standalone, we begin in phase ProjectOntoFacesOnly. This means we perform stages 3 and 4 only, meaning that
 * at the end of the first sweep, we have put values in the residual that correspond to the RHS, and all projections/solutions on the
 * face/cell are 0 (or still at whatever value they were initially). We then move on to the Solve phase. The solver will remain in the Solve
 * phase, performing all operations in order until the Peano backend decides its time to stop, by polling the superclass function
 * terminationCriterionHolds().
 * 
 * When used as a two-grid solver, the following happens:
 * 
 * - We begin in phase ProjectOntoFacesOnly. This sets an initial value for the residual. At the end of the sweep we move onto the solve
 *    phase.
 * - Next, we remain in the Solve phase for a pre-determined number of sweeps, which is controlled by the DGCGCoupling action sets.
 *    As can be seen from endMeshSweep(), this solver will remain in the Solve phase indefinitely, so it is the responsibility of the
 *    coupler to modify this behaviour
 * - When the suspend() routine is called correctly, we move into phase computeAndRestrictResidual. This will allow us only to update the
 *    face solutions. This means we have current face solutions which match the projections from last phase, and we are ready to restrict
 *    onto the correction solver, which we do by computing on the fly.
 * - When the suspend() routine is called correctly, we move into phase ProjectOntoFacesOnly. This will allow us only to take the updated
 *    cell solutions (which were modified during prolongation by the coupling solver) and project those onto the faces. We then reset the
 *    cell's internal residual based on these updated cell solutions. From there we move back to the Solve phase.
 *
 * 
 */
class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public mghype::matrixfree::solvers::Solver {

  public:
    enum class State {
      Solve,                    // for use in middle DG stage
      ProjectOntoFacesOnly,     // for use in first DG stage
      Suspend,
      computeAndRestrictResidual // for use in final DG stage
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

    bool updateResidual() const;
    bool updateFace() const;
    bool projectOntoFaces() const;
    bool projectOntoCells() const;

    bool isSuspended() const;
    
    /**
     * Suspends solver for one sweep
     *
     * After this sweep, the solver will toggle back to the normal solver
     * state.
     */
    void suspend(bool projectOntoFaces, bool computeAndRestrictResidual);

    State getState() const;

  protected:
    static tarch::logging::Log  _log;

    State _state;
};

