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
 *
 * @f$ \int  _c \left( F(Q(t)), \nabla \phi \right) dx @f$
 *
 * per cell:
 *
 * ### Volumetric contributions
 *
 * @todo This part has yet to be written; we also need links where this can be found in the source code documentation
 *
 * ### Riemann problem (Rusanov)
 *
 * @todo This part has yet to be written; we also need links where this can be found in the source code documentation
 *
 *
 * ## Projection onto faces
 *
 * The switch from a face representation to a volumetric representation and back is
 * a key ingredient of any DG scheme. In my formulae above, the projection of the
 * polynomial onto the
 *
 * @todo This part has yet to be written; we also need links where this can be found in the source code documentation
 *
 *
 * ## Non-conservative product
 *
 * ### Volumetric part
 *
 * For the solution of the non-conservative product, we first note that the evaluation
 * within the cell is pretty straightforward:
 *
 * @f$ \int  _c \Big( B(Q(t)) \nabla Q(t), \phi \Big) dx @f$
 *
 * is something we can treat directly:
 *
 * - We first run over all the individual quadrature points within the cell
 * - Per point, compute $ \nabla Q(t)$. This is trivial, as we represent Q with our
 *   shape functions, i.e. we can precompute the derivatives and simply scale them
 *   with the weights of Q's shape functions at the integration points: As Q is a
 *   linear combination of shape functions, the derivative is a linear combination
 *   of the derivatives of the shape functions.
 * - Ask the user's non-conservative product to give you the results of @f$ B(Q(t)) \cdot \nabla Q(t) @f$.
 *   We assume that we can repressent the result again with shape functions. This is
 *   a numerical approximation - in general the result of this will not be representable
 *   by our polynomials.
 * - As the result is tested against @f$ \phi @f$, we get a mass matrix over the
 *   outcome, but the time stepping then immediately multiplies the outcome with the
 *   inverse of the mass matrix, so these contributions kick out.
 *
 * Our calculation can be written down as
 *
 * @f$ Q^{(new)} \gets Q^{(new)} + dt \cdot ncp \Big( D\cdot Q \Big) @f$
 *
 * where the D is the precomputed derivative operator over the solution. This
 * instruction adds the copied over old solution subject to an additional source
 * term. We have to apply the update per dimension.
 *
 *
 *
 * ### Impact on Riemann problem
 *
 * ## Add Riemann solution to volumetric data
 *
 * @todo This part has yet to be written; we also need links where this can be found in the source code documentation
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

