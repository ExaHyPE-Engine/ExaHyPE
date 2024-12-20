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

    /**
     * During solution updates, we update the solution via
     * \f$ u = u + \omega * du \f$. Its value is passed in to the python class
     * and is in the range [0,1]. A value 0 will mean that no updates are performed. 
     * 0.5 is a typical value.
     * 
     */
    static constexpr double     OmegaCell              = {{CELL_RELAXATION}};

    /**
     * During solution updates, we update the solution via
     * \f$ u = (1-\omega)*u + \omega * du \f$. Its value is passed in to the DG Solver class
     * and is in the range [0,1]. A value 0 will mean that no updates are performed. 
     * 1.0 is a typical value.
     * 
     */  
    static constexpr double     OmegaFace              = {{FACE_RELAXATION}};

    /**
     * This is the total number of unknowns we store in the cell, taking into account
     * dimension, polynomial degree and the number of variables that we care about in
     * this setup.
     * 
     * This is set in the python script and sent over via a template. Its value
     * is equal to UnknownsPerCellNode * ( PolyDegree + 1 ) ^ Dimensions, which
     * is in turn equal to UnknownsPerCellNode * NodesPerCell.
     */
    static constexpr int        CellUnknowns           = {{CELL_CARDINALITY}};

    /**
     * This is the total number of unknowns we store in the face's solution vector, taking
     * into account dimension, polynomial degree and the number of variables that we care
     * about in this setup.
     * 
     * Its value is equal to SolutionsPerFaceNode * NodesPerFace.
     * 
     */
    static constexpr int        FaceUnknownsSolution   = {{FACE_CARDINALITY_SOLUTION}};

    /**
     * This is the total number of unknowns we store in the face's projection vector, taking
     * into account dimension, polynomial degree and the number of variables that we care
     * about in this setup.
     * 
     * Its value is equal to ProjectionsPerFaceNode * NodesPerFace, which is twice as long
     * as the solution vector. This is to take into account projections from the left and
     * right sides of a face.
     */
    static constexpr int        FaceUnknownsProjection = {{FACE_CARDINALITY_PROJECTION}};

    /**
     * This is the total number of nodes we store logically in the cell, taking into account
     * polynomial degree and dimensions.
     * 
     * Its value is set in the DG Solver class, and is equal to (PolyDegree+1)^Dimensions
     * 
     */
    static constexpr int        NodesPerCell           = {{NODES_PER_CELL}};

    /**
     * This is the total number of nodes we store logically in the face, taking into account
     * polynomial degree and dimensions.
     * 
     * Its value is set in the DG Solver class, and is equal to (PolyDegree+1)^(Dimensions-1)
     * 
     */  
    static constexpr int        NodesPerFace           = {{NODES_PER_FACE}};

    /**
     * This is the number of unknowns we store per cell node. For almost all purposes, 
     * this number is 1 (ie the solution of the poisson problem in the cell).
     * 
     */
    static constexpr int        UnknownsPerCellNode    = {{UNKNOWNS_PER_CELL_NODE}};

    /**
     * This is the number of unknowns we store per face node for the solution. This
     * is typically 2, ie the true solution as well as its gradient.
     */
    static constexpr int        SolutionsPerFaceNode   = {{SOLUTIONS_PER_FACE_NODE}};

    /**
     * This is the number of unknowns we store per face node for the solution. This
     * is typically 2, ie the true solution as well as its gradient. 
     *
     * @todo Kick this out and replace it with SolutionsPerFaceNode, since it doesn't
     *  make sense if these two numbers differ.
     * 
     */
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
      tarch::la::Vector< FaceUnknownsSolution,double >&    projection
    ) = 0;

    /**
     * Initialise a cell degree of freedom
     * 
     */
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

