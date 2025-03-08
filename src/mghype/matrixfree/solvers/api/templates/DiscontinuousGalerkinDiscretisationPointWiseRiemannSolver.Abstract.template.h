{% include 'DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.Abstract.generic.template.h' %}

  protected:
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
    enum class State {
      Solve,                    // for use in middle DG stage
      Suspend,
      /**
       * Used in very first DG stage after interpolation. Not used
       * without multigrid.
       */
      ProjectOntoFaces,
      /**
       * Used by multigrid algorithms in the last DG step to determine the
       * residual to restrict.
       */
      ComputeResidualAndSuspend
    };

    State _state;
};

