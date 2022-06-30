// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_DG_H_
#define _EXAHYPE2_DG_DG_H_



/**
 * @page DG Discontinuous Galerkin
 *
 * ExaHyPE's discontinuous Galerkin realisation provides support for plain
 * higher-order DG with generic, i.e. problem-unspecific, Riemann solvers.
 * The C++ code is a collection of routines, it is rather a toolbox than a
 * real solver. It is the Python code which serves as glue. It takes the
 * individual events triggered by Peano's mesh traversal, augments it with
 * ExaHyPE-specific data structure, and maps it onto DG routines. This
 * mapping is realised/called action sets.
 *
 * We rely on the generic DG formulation for
 *
 * @f$ \partial _t Q(t) + div F(Q(t)) = S @f$
 *
 * which is subject to a weak formulation.
 *
 * @f$ \int \left( \partial _t Q(t), \phi \right) dx + \int \left( div F(Q(t)), \phi \right) dx = \int \left(S, \phi \right) dx @f$
 *
 * We can roll over the divergence operator per cell and obtain
 *
 * @f$ \int _c \left( \partial _t Q(t), \phi \right) dx - \int  _c \left( F(Q(t)), \nabla \phi \right) dx + \int _{\partial c} \left( F(Q(t)), \phi \right) dS(x) = \int _c \left(S, \phi \right) dx @f$
 *
 *
 * The weak formulation is subject to a standard Runge-Kutta, i.e. we evaluate
 * a series of forward shots/trials and then combine these shots finally into
 * the new solution. However, the C++ toolbox is really independent of the
 * time integration scheme. Therefore, I decided to place it in a namespace
 * dg, whereas the Python glue code might have different namespaces reflecting
 * different timestepping schemes.
 *
 * For a single time step (or Runge-Kutta trial), we end up with a linear equation
 * system per cell. The individual steps and operators within this formula are subject of discussion
 * below.
 *
 *
 * ## Data format
 *
 * The main data format discussion can be found in the python subdirectory of
 * Peano. For Runge-Kutta, you have for example
 *
 *         exahype2.rkdg.RungeKuttaDG
 *
 * which serves as base class for different implementation variants. The base
 * class mainly creates the data structures (a polynomial per cell, the polynomial's
 * projections onto the face, all meta data, and the intermediate solutions for the
 * Runge-Kutta scheme) and defines all potential operations on these meta data,
 * i.e. all the glue code linked to the present C++ routines.
 *
 * It is however the job of the Python subclasses to switch the individual actions
 * on or off per grid sweep. While the Python docu provides more details on the
 * actual behaviour of the actions, the present file discusses some of the elementary
 * steps needed by any time stepping scheme, and which routines in C++ realise these
 * steps. That is, we discuss how things are implemented and what they do, but we
 * leave it to Python to orchestrate when these steps are actually done.
 *
 *
 * ## A single time step
 *
 * If we want to realise a single time step, the scheme above clarifies that we have
 * to copy over the solution and then progress along the temporal derivative for the
 * time step size @f$ dt @f$. Let
 *
 * @f$ \partial _t Q(t) \approx 1/dt \cdot \left( Q^{(new)} - Q^{(old)} \right) @f$
 *
 * So any time stepping scheme has to copy over the old solution and then add the
 * derivative subject to a scaling.
 *
 * We do not have the derivative in the Discontinuous Galerkin scheme. We only have
 * a weak representation of the derivative. As we assume that @f$ \partial _t Q(t) @f$
 * is constant over a time step and as we assume that this derivative can be represented
 * in the same basis as @f$ Q(t) @f$, the integral formulation above gives us an
 *
 * @f$ M \partial Q(T) @f$
 *
 * term, i.e. the derivative times a mass matrix. It makes sense to assume that
 * @f$ \partial _t Q(t) @f$ lives in Q's polynomial space: We start with a Q and we end
 * up in a Q, and the way how we can construct this Q representation is the same for
 * preimage and image. Therefore, also the difference can be represented by the polynomials
 * that we use for the two of them. In theory, we thus have to apply @f$ M^{-1} @f$ in
 * multiple steps later on. As we will see however, it falls together with the mass matrix
 * itself in most cases and thus does not enter the actual computation.
 *
 *
 * ## Source term
 *
 * When we integrate over @f$ (S,\phi ) @f$, we obtain a matrix-vector product
 *
 * @f$ M\dot S(Q) @f$
 *
 * However, we never use this term. Instead, we are interested in
 *
 * @f$ dt\cdot M^{-1}M\dot S(Q) = dt\cdot S(Q) @f$
 *
 * in line with the discussion above. The update the current solution with the source
 * term thus can be combined straightforwardly with the
 * copying: We take the current solution, add the source term times the time step size,
 * and write it into the image.
 * We can write this down as
 *
 * @f$ Q^{(new)} \gets Q^{(old)} + dt \cdot S(Q^{(old)}) @f$
 *
 * The copy-over can be found within exahype2::dg::cellIntegral_patchwise_computeAndUpdate_functors()
 * or variants thereof. It is a simple loop over all of the dofs within the cell.
 *
 * The source term solely enters the volumetric calculations.
 * It does not show up in any face integral.
 *
 *
 * ## Flux term
 *
 * As any shape function has local support within one cell, we can evaluate
 * per cell by employing integration by parts:
 *
 * @f$ \int  _c div F(Q(t))\ \phi dx = - \int  _c \Big( F(Q(t), \nabla \phi \Big) dx  + \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) @f$
 *
 * The volumetric part can be evaluated as it is: We compute the F weights and
 * multiply it directly with the matrix over the gradients. Actually, we are not
 * even interested in this matrix, but we are interested in the mass matrix inverse
 * times the derivative matrix. This can all be precomputed. The scaling of the
 * volumetric contribution is in @f$ 1/h @f$ - the integral over the cell times the inverse
 * of the mass matrix kick all of their derivative terms/scalings out, and we have
 * a remaining @f$ h^{-1} @f$ from @f$ \nabla \phi @f$.
 * We conclude that the volumetric term can be implemented generically.
 * All it needs is the flux terms supplemented by the user.
 *
 * The Riemann problem
 *
 * @f$ \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) @f$
 *
 * requires more work, as Q (and hence F) are not defined on the cell faces.
 * One generic way to tackle this is to exploit standard Rusanov, e.g.
 * In this case, we employ some averaging of the faces subject to additional
 * damping.
 *
 * @f$ \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) \mapsto  \int _{\partial c} \Big( \frac{1}{2}(F(Q^-(t))+F(Q^+(t))) - \frac{1}{2} \lambda _{max}(Q^-(t),Q^+(t)) (Q^+(t)-Q^-(t))  \Big) \ \phi \ n dS(x) @f$
 *
 *
 * ## Projection onto faces
 *
 * To apply any Riemann solver of our choice, we first have to project the current
 * solution @f$ Q(t) @f$ from within the cell onto the faces.
 * This projection is realised by a dedicated Python action set which is fed the
 * polynomial basis functions.
 *
 * We can exploit the fact that we employ Lagrangian basis functions and that we also
 * work with Cartesian tensor product structures.
 * Due to these choices, we know that any projection of the current solution representation
 * within the cell can be mapped onto a Lagrangian polynomial of exactly the same order
 * on the face.
 * Furthermore, everything is beautifully lined up:
 * In 2d, all the integration point within a cell along a horizontal line span up a
 * polynomial, and we can thus multiply them with this polynomial and thus reconstruct
 * the polynomials value on the left and right faces' sample point (which also is exactly
 * on this line).
 * We don't even have to use Gauss-Lobatto points.
 * It works with Gauss-Legendre just as well (with Lobatta, we could avoid the construction
 * of the polynomial and just read out the solution value right at the face; we do not
 * exploit this fact).
 *
 * @image html solution-projection.png
 *
 * In the 2d example above, the four bottom sample points and their weight define the
 * blue polynomial along the horizontal line. The other 12 integration points in the
 * cell have no influence on the solution value along this line. Consequently, we can
 * compute the left and right solution solely by these four weights.
 *
 * On the left face, we have a polynomial with the same order (3rd order here), but it
 * is - obviously - a 1d polynomial and not one resulting from a tensor product.
 * Once we know the the volumetric polynomials value on the left face, we can directly
 * set this value as weight on the face's quadrature point (green weight).
 *
 * Each face has a left and a right side, and the solutions left and right will not be the same
 * as we work in a Discontinuous Galerkin setup. We hence do not store four weights in the
 * example above, but we do hold eight weights, i.e. both the weights from the left and the
 * right side. These weights are stored lexicographically in line with the Finite Volume
 * solver. See the documentation of the face enumerator exahype2::enumerator::FaceAoSLexicographicEnumerator.
 * In the vanilla version of ExaHyPE, we do only store the solution left and right. For more
 * sophisticated Riemann solver, you can also store derivatives, e.g., from both sides.
 *
 * The projection onto the faces is an operation within the polynomial function space.
 * As there's no integral or something similar involved, it is a plain multiplication of
 * the sample points' weights within the cell with the corresponding weights of the
 * polynomial.
 *
 *
 *
 * ## Getting the face data back
 *
 * Let the outcome of a Riemann solve (the solution) be everything under the face integral
 * besides the @f$ \phi @f$ test function.
 * For the Rusanov example above, we would call the result of
 *
 * @f$ \frac{1}{2}(F(Q^-(t))+F(Q^+(t))) - \frac{1}{2} \lambda _{max}(Q^-(t),Q^+(t)) (Q^+(t)-Q^-(t)) @f$
 *
 * the solution to the Riemann problem.
 *
 *
 * Once we know the point-wise Riemann solution, we know that this point-wise solution
 * spans functions in the shape space restricted to the faces.
 * That is, the individual points of the Riemann solution serve as weights to shape functions
 * which live on the face only.
 * We can analytically integrate over those guys (we know the shapes and have a linear combination)
 * and then project the data back by applying the mass matrix.
 * As the mass matrix is scaled with @f$ h^d @f$, the impact of the Riemann solution is in @f$ h^{-1} @f$.
 *
 *
 *
 * ## Non-conservative product
 *
 * You find some discussions of the treatment of the non-conservative product in papers by Michael Dumbser
 * et al, e.g. I found the discussion around @f$ P_NP_M @f$ patch-conservative products in his paper
 * "Space-time adaptive ADER discontinuous Galerkin schemes for nonlinear hyperelasticity with material failure"
 * particularly helpsful (<a href="https://arxiv.org/abs/2003.02760>">arXiv:2003.02760</a>). Have a look
 * notably into (21).
 *
 * Personally, I always use the following formalism which might be formally slightly wrong (but it gives
 * the same result as Michael's work, so I'm happy):
 *
 * Our fundamental observation is that @f$ \nabla \phi @f$ is nicer than @f$ \nabla Q @f$ as we can
 * completely pre-compute it. Within the DG weak formulation, it is clear that we somehow have to roll
 * over the derivative through integration by parts. Just let's keep in mind that we really want to avoid
 * a term like @f$ \nabla B(Q) @f$ - this one would be nasty due to chain rule kicking in and the Hessian
 * (tensor) structure. So we end up with the following insights:
 *
 * @f$ \int _{\partial c} \Big( B(Q(t))\ Q(t), \phi \Big) \ n_x\ dS(x) =  \int _c \Big( B(Q(t)), \nabla _x \Big( Q(t)\phi \Big) \Big) dx =  \int _c \Big( B(Q(t)) \ \nabla _x Q(t), \phi \Big) dx + \int _c \Big( B(Q(t)) \ \nabla _x \phi, Q(t) \Big) dx @f$
 *
 * But this is a cell-wise expression. We have to take the weak ansatz over the whole domain @f$ \Omega _h = \sum c @f$.
 *
 * We look at the expression for one test function. We know that test functions have local support
 * on one cell. Let this cell be called c.
 *
 * @f$ \int _{\Omega } \Big( B(Q(t)) \ \nabla _x Q(t), \phi \Big) dx = \sum _{c^{\pm}} \int _{\partial c^{\pm}} \Big( B(Q(t))\ Q(t), \phi \Big) \ n_x\ dS(x) - \int _{c} \Big( B(Q(t)) \ \nabla _x \phi, Q(t) \Big) \ dx @f$
 *
 *
 * The volumetric integral reduces to an integral over c as the support disappears.
 * Each face integral enters the formula twice - once from the left and once from the right.
 * As we have discontinuous functions, these two contributions do not cancel each other out.
 * The Q values are different.
 *
 *
 * ### Volumetric part
 *
 * - We first run over all the individual quadrature points within the cell.
 * - Per point, we evaluate @f$ B(Q(t)) @f$.
 * - We scale this contribution with @f$ Q(t) @f$, i.e. we actually evaluate
 *   @f$ \int _{c} \Big( B(Q(t)) \ \nabla _x \phi, Q(t) \Big) \ dx = Q(t)^T B(Q(t)) \cdot ... @f$.
 *   We assume that this term can be represented within the same function space from which we
 *   took our shape functions. This is an approximation, but that's what we have currently
 *   baked into our Python scripts.
 * - Now we multiply the result vector of weights with the precomputed derivative shape functions
 *   over which we have integrated already. This gives us a @f$ h^{-1} @f$ scaling.
 * - The result is subject to a multiplication with an inverted mass matrix. There's no need
 *   to have this multiplication explicitly in the code. We can directly incorporate it within
 *   the derivative matrix. The overall scaling of the contributions from the volumetric part remain in @f$ h^{-1} @f$.
 *
 *
 * ### The Riemann problem
 *
 * We note that almost all face integrals disappear per test @f$ \phi @f$. Only those over
 * faces adjacent to the test support cell remain. But each face is there twice, as we had to
 * first split @f$ \Omega @f$ before we do the integration by parts. I try to denote this by
 * the symbol @f$ \partial c^{\pm} @f$. The left and right sum contribution collapse as follows:
 *
 * @f$ \int _{\partial c^{\pm}} \Big( B(Q(t))\ Q(t), \phi \Big) \ n_x\ dS(x) = \int _{\partial c^{+}} \Big( B(Q(t))\ Q(t), \phi \Big) \ n_x\ dS(x) - \int _{\partial c^{-}} \Big( B(Q(t))\ Q(t), \phi \Big) \ n_x\ dS(x) = \int _{\partial c} \Big( B(Q(t))\ (Q^+(t)-Q^-(t) ), \phi \Big) \ dS(x)  @f$
 *
 * The remaining question now is what @f$ B(Q(t)) | _{\partial c} @f$ looks like. After all,
 * this is where the solution is not defined. We simply use averaging in our code, i.e.
 *
 * @f$ B( \frac{1}{2} (Q^+(t)+Q^-(t) ) )\ (Q^+(t)-Q^-(t) ) @f$
 *
 * Once again, we have an ncp term here. Different to the one above, we apply it to the jump, i.e.
 * the solution difference, this time. We do not apply it to a gradient! In this way, the
 * Riemann problem here is more similar to the Finite Volume formulation. Note that the notion
 * of left and right depends on the test function of interest, i.e. the outcome of this Riemann
 * problems goes with a switched sign into the left and right adjacent cell.
 *
 * Remark: The work by Dumbser et al have an overall factor of 0.5 in the Riemann problem.
 * I don't see where this comes from (in the Finite Volume case, it is clear where it comes from).
 * So likely there's a term missing in this explanation.
 *
 *
 * ## Todo list
 *
 * - The file Riemann.h/Riemann.cpp should go away. However, we might need its computation
 *   of maximum eigenvalues over the faces - though I think it should be the maximum over
 *   the cell's eigenvalues instead. In this case, the file pair can completely go away.
 * - My Riemann solver atm is only working point-wisely. Marc made the point that you might
 *   want to use the face-wise maximum eigenvalue. We have to implement this variant.
 * - The generic Enumerator.h file lacks docu.
 * - Is the FaceAoSEnumerator really used and properly documented?
 * - 3d version of face to cell projection is missing
 * - 2d version of cell to face projection is buggy or not nice. Clean-up
 * - Look through DGUtils and tidy is up.
 * - Strides should completely go away and should be subsumed in the enumerators. So the
 *   enumerators is where all the logic sits and the logic is completely encapsulated there.
 * - Remove L and R basis functions as they are redundant?
 * - Investigate if we could remove a few other matrices from the generated Python code
 * - Point sources implement
 *
 *
 */




#endif

