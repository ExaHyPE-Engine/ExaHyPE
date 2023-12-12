# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2


class DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver:
    """!
       
    Discontinuous Galerking Solver
    
    This solver holds @f$ N(p+1)^d @f$ unknowns (doubles) per cell for the
    solution, @f$ N(p+1)^d @f$ per cell for the right-hand side, and it 
    holds @f$ 3NK(p+1)^{d-1} @f$ unknowns per face.
    
    The internal functionality relies on the break-down of the per-cell
    operations into 
    
    @f$
     A u|_c - P \mathcal{R}(u^-,u^+) = A u|_c - P \mathcal{R}(u|_f) = A u|_c - P \varphi|_f = Mf
    @f$  

    and it realises a Block-Jacobi smoother subject to two relaxation 
    parameters @f$ \omega _c, \omega _f \in [0,1] @f$. Conceptually,
    the solver starts from the representation
    
    @f$
    \left(
      \begin{array}{ccccc}
         A &   &   &   & ... \\
           & A &   &   & ... \\
           &   & A &   & ... \\
           &   &   & A & ... \\
         . & . & . & . & \hat A
      \end{array}
    \right)
    @f$
    
    of the overall left-hand side above. Here @f$ A @f$ acts on the individual 
    cell, and the block entries with a dot couple the cell to the faces and vice
    versa. We extract the matrix 
    
    @f$
    \left(
      \begin{array}{ccccc}
         A &   &   &   &  \\
           & A &   &   &  \\
           &   & A &   &  \\
           &   &   & A &  \\
           &   &   &   & diag( \hat A )
      \end{array}
    \right)
    @f$
    
    from this system matrix, and use this one within an (outer) Jacobi scheme.
    This yields a block-Jacobi solver, where individual cells (matrix blocks)
    are explicitly inverted.
    
    We end up with an iterative scheme where two phases take turns:
    
    - Per cell, we solve the equation
      @f$ 
      A u|_c + P q|_f = M b
      @f$ 
      which yields the update rule
      \f{eqnarray*}{ 
      u|_c & \gets & u|_c + \omega _c A^{-1}\left( Mb - A u|_c - P q|_f \right) \\
           &   =   & (1-\omega _c) u|_c + \omega _c A^{-1}\left( Mb - P q|_f \right)
      \f}
      Here, the @f$ q @f$ is the solution of the PDE term on the faces which we
      assume to be fixed. Obviously, @f$ \omega _c > 0 @f$ whilst @f$ \omega _c=1 @f$
      solves the problem per cell exactly (and might still be a bad smoother).
    - Per face, we use a plain Jacobi. We assume that we have an explicit
      expression of the Riemann solution (simple averaging would be an example)
      and hence @f$ \hat A = id @f$. We consequently get
      \f{eqnarray*}{ 
        q|_f & \gets & q|_f + \omega _f \left( \mathcal{R}(u^-, u^+) - q|_f \right) \\
            & = & (1-\omega _f) q|_f + \omega _f \mathcal{R}(u^-, u^+).
      \f}
    
      
    
    ## Algorithmical blueprint
    
    1. Enter cell: Whenever a cell is entered, we create a temporary 
       @f$ N(p+1)^d @f$ array (on the heap). 
       
       1. Into this temporary array
          @f$ r @f$, we write @f$ Mb @f$. This is the mass matrix (right-hand side
          matrix of the class constructor, as we are not restricted to a 
          traditional mass matrix) times the right-hand sides within the 
          cell. So
          @f$
          r \gets Mb.
          @f$
       2. Once complete, we substract @f$ Pq|_f @f$. That is, we take the 
          solution on the face and we project it (cell from face) onto the 
          cell, where we substract it from the previous @f$ r @f$ value:
          @f$
          r \gets Mb + Pq|_f.
          @f$
       3. The third step is where the MFlops come from: We take the cell
          matrix and explicitly invert it. For a linear, invariant matrix
          @f$ A @f$, we could do so once, but the solver itself makes no
          assumption if the matrix is invariant or not. So it inverts it
          every step and uses BLAS routines or this. The result is multiplied
          with the residual as accumulated so far:
          @f$
          r \gets A^{-1} \left( Mb + Pq|_f \right).
          @f$
       4. We run over the solution within the cell and scale each entry 
          with @f$ (1-\omega _c) @f$. If @f$ \omega _c =1 @f$, this means we
          set the entries to zero. In general, the entries however will 
          only be damped. After that, we add the @f$ r @f$ values to each
          point. This gives us a new solution guess.
       5. We project the updated solution @f$ u|_c @f$ onto the faces, i.e.
          obtain a new @f$ u|_f^{\pm } @f$. This is realised within the 
          action set ProjectOntoFaces.
    2. touchFaceFirstTime(): Whenever a face is touched, we solve the Riemann
       problem and update the Riemann guess accordingly. Initially, we assume
       that the @f$ q @f$ entries on the face are all set to zero.
       
       1. We evaluate per node on the face the expression @f$ \mathcal{R}(u^-,u^+) @f$.
          For this, we loop over the @f$ (p+1)^{d-1} @f$ points and call the 
          C++ solver's routine. The user can thus implement the actual 
          Riemann solver themself.
       2. We take the result and update the value on the face according to
          @f$
          q|_f gets (1-\omega _f) q|_f + \omega _f \mathcal{R}(u^-, u^+).
          @f$

          
       
       All of this functionality can be found in the subpackage actionsets.UpdateCell.
       
    """
    