// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @page exahype2_dg Discontinuous Galerkin
 *
 * ExaHyPE's discontinuous Galerkin realisation provides support for plain
 * higher-order DG with Riemann solvers of your choice. The C++ code is a mere
 * collection of routines. It is rather a toolbox than a real solver. It is the
 * Python code which serves as glue. It takes the individual events triggered
 * by Peano's mesh traversal, augments it with ExaHyPE-specific data structure,
 * and maps it onto DG routines. Furthermore, the generated Python code holds
 * all the DG matrices and vectors. These are not hardwired into the C++
 * routines but passed in as arguments (pointers). The mapping from the
 * traversal events onto DG computations is realised via action sets. This is
 * the Peano terminology.
 *
 * We rely on the generic DG formulation for
 *
 * @f$ \partial _t Q(t) + div F(Q(t)) = S(Q(t)) @f$
 *
 * which is subject to a weak formulation. A discussion of non-conservative
 * terms can be found further down. For the time being, we focus on a
 * conservative formulation and examine the weak formulation which
 * has to hold for any reasonably smooth test function @f$ \phi = \phi (x)@f$:
 *
 * @f$ \int _{\Omega } \left( \partial _t Q(t), \phi \right) dx + \int _{\Omega } \left( div F(Q(t)), \phi \right) dx = \int _{\Omega } \left(S(Q(t)), \phi \right) dx @f$
 *
 * In the DG framework, any test function @f$ \phi @f$ has strict local support
 * which reduces the weak formulation to a cell expression: We know that
 * @f$ \phi \not= 0 @f$ inside one specific cell c, but it equals zero
 * everywhere else. Within this one cell, our shape function is continuous.
 * Therefore, we can apply integration by parts and the product rule for divergence and obtain
 *
 * @f$ \int _c \left( \partial _t Q(t), \phi \right) dx - \int  _c \left( F(Q(t)), \nabla \phi \right) dx + \int _{\partial c} \left( F(Q(t)), \phi \right) dS(x) = \int _c \left(S(Q(t)), \phi \right) dx @f$
 *
 * It doesn't really matter for the discussion here how we treat the time
 * derivative. Our discussion focuses on how we compute @f$ \partial _t Q @f$
 * and it is then up to your time integrator of choice how to process this
 * information further.
 * From hereon, any reference to @f$ \partial _t Q @f$ means we speak about
 * the change of the value in time for one particular quadrature point, i.e. one particular weight.
 *
 *
 * In ExaHyPE 2, we mainly use Runge-Kutta. If the
 * weak formulation is subject to Runge-Kutta, we evaluate
 * a series of forward shots/trials and then combine these shots finally into
 * the new solution. All we need is a description of the time derivative,
 * which we obtain from the weak formulation above.
 * As our C++ toolbox is independent of the
 * time integration scheme used, I decided to place it in a namespace
 * dg, whereas the Python glue code might have different namespaces reflecting
 * different timestepping schemes.
 *
 *
 * ## A single time step
 *
 * On this page, I do not discuss Discontinuous
 * Galerkin in detail, but I try to point out, how the individual steps are
 * integrated into the mesh traversal, and how I split them up into individual
 * routines.
 *
 * We solve
 *
 * @f$ \int _c \left( \partial _t Q, \phi \right) dx = \int _c \left( Q, \phi \right) dx + dt \cdot \left( \int  _c \left( F(Q), \nabla \phi \right) dx - \int _{\partial c} \left( F(Q), \phi \right) dS(x) + \int _c \left(S(Q), \phi \right) dx \right) @f$
 *
 * Which is just one step of the forward euler method with the time derivative from above.
 * ExaHyPE works with cubic or square cells and a Cartesian layout of the degrees
 * of freedom therein.
 * We thus have @f$ (p+1)^d @f$ degrees of freedom in our square/cube cells. Each carries
 * n unknowns, but they are all treated the same, i.e. independent of each other.
 * So it is sufficient to look at one unknown. We may assume that the Q is scalar.
 * The F and S term can potentially couple n unknowns per dof from the previous
 * time step with each other. But this is irrelevant to the formulation of the problem.
 *
 * @image html shape-functions.png
 *
 * Each of the @f$ (p+1)^d @f$ unknowns carries one shape function.
 * The picture above illustrates this: There's a polynomial which is tied to one
 * particular dof and this dof holds a weight, i.e. a scaling of this one shape
 * function. The test functions @f$ \phi @f$ are polynomials, too.
 * All of these polynomials live only within one cell.
 * The solution is then actually a combination of all of the polynomials scaled
 * by the dof weights.
 * In the sketch, the solution is a linear combination of 16 polyomials within
 * this very cell.
 * Then it might jump on the cell face, and in the next adjacent cell there are
 * another 16 dofs which determine the solution's shape.
 *
 * As the integrals of the weak formulation span one cell only, we get something
 * like
 *
 * @f$ M \Big( \partial _t Q \Big) =  \Big( ... \Big) Q^* @f$
 *
 * per cell.
 * There is a square mass matrix which results from the integral above which tells us
 * that a linear combination of the derivatives within a cell equals an expression over all previous
 * values of the time step. This is the reason I add the star. It is just there to
 * point out that we use a lot more values than only those within one cell.
 *
 * The M is the <b>MassMatrix</b>. Let all the dofs within a cell be enumerated
 * lexicographically. Each value scales the underlying shape function. The first
 * line of the mass matrix then tells us what the linear combination of all of these
 * shape functions scaled by the weights and tested against the first gives under the
 * integral. The second line of the @f$ M Q^{(new)} @f$ tells us what the test of the
 * linear combination against the second test function gives us.
 *
 * We can compute the mass matrix a priori. This is done
 * by the Python script exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix for example.
 * The computed matrix is then written into the abstract solver base class of the user
 * and can be handed over to any computational routine. We note that we will (more
 * often) need the inverse.
 *
 * @f$ \partial _t Q(T) = M^{-1} \Big( \dots \Big) Q^* =: Q + dt \cdot M^{-1} \hat Q@f$
 *
 * This is the last step of our algorithm: We assume that we have a quantity @f$ \hat Q @f$
 * in each and every sample point whose entries will be discussed later. We multiply these quantities with the inverse of the
 * mass matrix and add it, subject to a scaling with dt, to the old solution. This
 * operation is called exahype2::dg::multiplyWithInvertedMassMatrixAndAddToSolution_GaussLegendre() or variants thereof.
 * So far, the algorithm thus consists of two steps:
 *
 * - (unspecified) Compute the @f$ \hat Q @f$ value per degree of freedom in each and every cell somehow.
 * - (3) Run over each cell. Per cell, call exahype2::dg::multiplyWithInvertedMassMatrixAndAddToSolution_GaussLegendre() or variants thereof.
 *   This routine takes the Q values of the cell, and adds the contribution @f$ dt \cdot M^{-1} \hat Q @f$.
 *
 * You will find exahype2::dg::multiplyWithInvertedMassMatrixAndAddToSolution_GaussLegendre()
 * and its cousins in the file exahype2/dg/CellIntegral.h as they correspond, as name and
 * derivation suggest, to an integral formulation over integrals.
 *
 *
 * ## Mass matrix
 *
 * You can pick different shape functions. For different shape functions, you get
 * different mass matrices. @f$ M @f$ in general is a dense matrix.
 * Its entries stem from
 *
 * @f$ M_{ij} = \int _c \Big( \phi _i, \phi _j \Big) dx @f$
 *
 * For some (approximations of) mass matrices or particular choices of the shape
 * fuctions, we provide specialised versions of
 * DG's mathematical steps. All mass matrices have in common that the underlying
 * shape functions have local support. They never extend beyond a cell. Therefore,
 * mass matrices are always square and have exactly @f$ (p+1)^d @f$ rows/columns.
 *
 * Important special cases of mass matrices arise if some of the following
 * properties hold:
 *
 * - If we use a Ritz-Galerkin shape function choice, i.e. pick the test
 *   functions from the same function space as our shape functions, then
 *   the matrix is symmetric. We might want to exploit this, though there's
 *   limited gain if we make our abstract solver precompute the inverse.
 * - If we use Gauss-Legendre shape functions, the polynomials are orthogonal. Therefore,
 *   the mass matrix becomes a diagonal matrix. Its inversion is trivial. See
 *   exaype2.solvers.GaussLegendreBasis for details.
 * - Due to its construction, we expect M to be very diagonal dominant. Codes thus might
 *   want to use a lumped, i.e. diagonalised, approximation of M. See
 *   exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix for details.
 * - If our shape functions have tensor-product structure, i.e. can be
 *   written down as @f$ \phi (x) = \phi (x_1) \cdot \phi (x_2) \cdot \phi (x_3) @f$ we
 *   can run optimisations: A lot of non-mass matrices are constructed by
 *   tensor products of a 1d matrix with a 1d mass matrix or two 1d mass
 *   matrices.
 *
 * As we use Discontinuous Galerkin where the individual shape functions are
 * confined to a cell, M couples really only dofs of one cell with each other.
 * There is no reason to use a lumped matrix as the (global) inverse then also
 * has block structure, i.e. couples only dofs from within one cell. But some
 * codes might nevertheless prefer lumped (diagonalised) matrices for performance
 * reasons.
 *
 *
 * Purists might argue that most setups don't need the inverse of the mass
 * matrix, as the mass matrix inverse coincides with the mass matrix inside the
 * @f$ \hat Q @f$ term. However, there are cases where this is not the case,
 * and if you know that your mass matrix is a diagonal, then we
 * offer specialised versions for this anyway, and you can afford the few
 * additional multiplications that result from the fact that our @f$ \hat Q @f$
 * is the result of the test and we do not mangle in the inverse of the mass
 * matrix a priori.
 *
 *
 * ## The computation of the right-hand side that is multiplied with the inverse of the mass matrix
 *
 * From the definition above, we note that the ith entry of @f$ \hat Q @f$ denotes
 * what happens if we have a global solution Q and test it against the test function
 * @f$ \phi _i @f$ carried by this ith dof from a cell.
 * The computation of this intermediate value @f$ \hat Q @f$ consists of two types
 * of contributions:
 * Contributions from the face integrals and contributions from the volumetric terms.
 * I discuss the terms individually and split any term discussion again up into two
 * parts if a term arises under the surface and the volume integral.
 *
 * This is different to the implementation where you find a routine cellIntegral_patchwise_GaussLegendre_functors()
 * or any of its cousins
 * which does all the volumetric stuff in one rush, while the handling of the face contributions
 * is done differently.
 *
 *
 * ### Source terms
 *
 * In ExaHyPE's DG code, we distinguish two types of source terms: Volumetric
 * source terms and point sources. Usually, I mean volumetric source terms when
 * I speak of a source.
 *
 * Once we assume that S(Q) is something that exists in each and every point
 * within the domain, we have to make a decision how to represent it as a function.
 * It is convenient to assume that S can be represented in the same way as the
 * solution Q, i.e. as linear combination of the same type of shape functions.
 *
 * In this case, we obtain a matrix-vector product
 *
 * @f$ M\dot S(Q) @f$
 *
 * when we integrate over @f$ (S,\phi ) @f$. If we insert this formalism into the
 * equations above, we recognise that we would never need the outcome of the matrix-vector
 * product. Instead, we are interested in
 *
 * @f$ dt\cdot M^{-1}M\dot S(Q) = dt\cdot S(Q) @f$
 *
 * in the end. However, I made the design decision that I want to have these steps
 * as proper separate steps. The reason is that we have to treat some other terms
 * differently:
 *
 * Point sources are modelled as Dirac terms within S:
 *
 * @f$ \int _c \Big( \delta _p, \phi \Big) dx = \phi (p) @f$
 *
 * So we basically get, for a test with a test function @f$ \phi @f$, the value
 * of this test function at the point source (times a calibration if the point
 * source should not be one).
 * Our implementation runs over all point sources within a cell.
 * As we know that the test functions span only the current cell and disappear
 * everywhere else, we don't have to check all point sources out there.
 * Per point source, we test over all the @f$ (p+1)^d @f$ test functions
 * associated with the dofs of this cell, and add the corresponding values to
 * @f$ \hat Q @f$.
 *
 * The source term solely enters the volumetric calculations.
 * It does not show up in any face integral.
 * With source terms, our algorithm implementation consists of the following steps:
 *
 * - (1) Set @f$ \hat Q \gets 0 @f$ per degree of freedom in each and every cell.
 * - t.b.d.
 * - (3.1) Compute source term contribution to @f$ \hat Q @f$
 * - t.b.d.
 * - (5) Per cell, take the Q values and add the contribution @f$ dt \cdot M^{-1} \hat Q @f$.
 *
 *
 * ### Flux term
 *
 * As any shape function has local support within one cell, we can evaluate
 * per cell by employing integration by parts:
 *
 * @f$ \int  _c div F(Q(t))\ \phi dx = - \int  _c \Big( F(Q(t), \nabla \phi \Big) dx  + \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) @f$
 *
 * The volumetric integral looks complicated, but it can be broken down into
 * simpler steps. First, we consider a one-dimensional setup. Let us assume
 * that we can represent @f$ F(Q) @f$ once again with our shape functions.
 * That's not point-wisely true usually, but we can work with this assumption.
 * If this is the case, then the @f$ F(Q) @f$ can be written down as an evaluation
 * of the Fs in the quadrature point, and then we can take the outcome and
 * interpret it as weight of the shape functions. So we get expressions like
 *
 * @f$ \int \Big( F(Q(x_i)) \phi _i, \nabla \phi _j \Big) dx @f$
 *
 * which are then linearly combined to give us the integral. We now observe
 * that we know the test functions, i.e. we can also precompute the derivative.
 * Furthermore, we can move the weight out of the integral. This gives us
 *
 * @f$ F(Q(x_i)) \int \Big( \phi _i, \nabla \phi _j \Big) dx = D F(Q) @f$
 *
 * where the matrix D is the <b>StiffnessOperator</b> which is the product of quadrature weights and the integral over the test functions and its derivative. Q is the vector of
 * the unknowns.
 *
 * If we have a a two-dimensional problem, we note that we benefit from the
 * tensor product structure once again. As
 *
 * @f$ \phi (x,y,z) = \phi (x) \cdot \phi (y) \cdot \phi (z) @f$
 *
 * decomposes the function into a product of independent ones, we can also
 * decompose the integral:
 *
 * @f$ \int \Big( F_x(Q(x_i,y_i)) \phi _i, \nabla _x \phi _j \Big) d(x,y) = F_x(Q(x_i,y_i)) \int \Big( \phi _i(x)\phi _i(y), \nabla _x \phi _j(x)\phi _j(y) \Big) d(x,y) = F_x(Q(x_i,y_i)) \int \Big( \phi _i(x), \nabla _x \phi _j(x) \Big) dx \int \Big( \phi _i(y), \phi _j(y) \Big) dy @f$
 *
 * which are then linearly combined to give us the integral. In the generic
 * case, we will need to make D a @f$ (p+1)^d \times (p+1)^d @f$ matrix.
 * If we have Gauss-Legendre polynomials however, we can once again exploit
 * the fact that the polynomials are orthogonal. It is hence sufficient to
 * have a one-dimensional @f$ D_{1d} @f$ diffusion operator and the one-dimensional
 * mass entries to construct all diffusion operators.
 *
 * The scaling of the volumetric contribution is in @f$ h^{d-1} @f$. We have a
 * @f$ h^d @f$ from the integral, and we have
 * a @f$ h^{-1} @f$ from @f$ \nabla \phi @f$.
 * The source term solely enters the volumetric calculations.
 * It does not show up in any face integral.
 * With source terms, our algorithm implementation consists of the following steps:
 *
 * - (1) Set @f$ \hat Q \gets 0 @f$ per degree of freedom in each and every cell.
 * - t.b.d.
 * - (3.1) Compute source term contribution to @f$ \hat Q @f$
 * - (3.2) Compute volumetric flux contribution to @f$ \hat Q @f$
 * - t.b.d.
 * - (5) Per cell, take the Q values and add the contribution @f$ dt \cdot M^{-1} \hat Q @f$.
 *
 *
 * #### Projection onto faces
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
 * It works with Gauss-Legendre just as well (with Lobatto, we could avoid the construction
 * of the polynomial and just read out the solution value right at the face; we do not yet 
 * exploit this fact).
 *
 * @image html solution-projection.png
 *
 * In the 2d example above, the four bottom sample points and their weight define the
 * blue polynomial along the horizontal line. The other 12 integration points in the
 * cell have no influence on the solution value along this line because the Lagrange polynomials are 0 there. 
 * Consequently, we can compute the left and right solution solely by these four weights.
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
 * #### The Riemann problem
 *
 * @f$ \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) @f$
 *
 * requires work (and care), as Q (and hence F) are not defined on the cell faces.
 * We therefore have to approximate the physical flux F by a numerical flux, there are several
 * definitions for numerical fluxes, but one generic way to tackle this is to exploit standard 
 * Rusanov, e.g. In this case, we employ some averaging of the faces subject to additional
 * damping.
 *
 * @f$ \int _{\partial c} F(Q(t)) \ \phi \ n dS(x) \mapsto  \int _{\partial c} \Big( \frac{1}{2}(F(Q^-(t))+F(Q^+(t))) - \frac{1}{2} \lambda _{max}(Q^-(t),Q^+(t)) (Q^+(t)-Q^-(t))  \Big) \ \phi \ n dS(x) @f$
 *
 * This is what we do when we touch a face. We simply take the projected Q values
 * from left and right and compute what F looks like. In theory the solutions to the 
 * left and right of the face could be different. Therefore we hold both outputs.
 *
 * - (1) Set @f$ \hat Q \gets 0 @f$ per degree of freedom in each and every cell.
 * - (2) Project Q onto faces
 * - (3.1) Compute source term contribution to @f$ \hat Q @f$
 * - (3.2) Compute volumetric flux contribution to @f$ \hat Q @f$
 * - (4.1) Compute the solution to the Riemann problem, i.e. the effective F values.
 * - t.b.d.
 * - (5) Per cell, take the Q values and add the contribution @f$ dt \cdot M^{-1} \hat Q @f$.
 *
 *
 *
 * #### Getting the face data back
 *
 * Let the outcome of a Riemann solve (the solution) be the thing under the face integral
 * without the @f$ \phi @f$ test function.
 * For the Rusanov example above, we would call the result of
 *
 * @f$ \frac{1}{2}(F(Q^-(t))+F(Q^+(t))) - \frac{1}{2} \lambda _{max}(Q^-(t),Q^+(t)) (Q^+(t)-Q^-(t)) @f$
 *
 * the solution to the Riemann problem. This is essentially a numerical approximation of the physical flux
 * between neighbouring volumes (and therefore cells.)
 *
 *
 * Once we know the point-wise Riemann solution, we know that this point-wise solution
 * spans functions in the shape space restricted to the faces.
 * That is, the individual points of the Riemann solution serve as weights to shape functions
 * which live on the face only.
 * We can analytically integrate over these (we know the shapes and have a linear combination)
 * and then project the data back.
 * Once again, the integral is the mass matrix, though this time a mass matrix on the submanifold.
 * It has a scaling of @f$ h^{d-1} @f$.
 *
 * Every single test on the face is the result of a projection of a volumetric test function onto
 * the face.
 * This explains how the outcome affects the @f$ \hat Q @f$: We simply take the outcome and distribute
 * it according to the original projection matrix.
 * Mathematically this equals a multiplication with the transpose.
 * There's no scaling involved in this step anymore.
 *
 *
 * ### Summary
 *
 * The whole algorithm implementation hence reads as follows:
 *
 * - (1) Set @f$ \hat Q \gets 0 @f$ per degree of freedom in each and every cell.
 * - (2) Project Q onto faces
 * - (3.1) Compute source term contribution to @f$ \hat Q @f$
 * - (3.2) Compute volumetric flux contribution to @f$ \hat Q @f$
 * - (4.1) Compute the solution to the Riemann problem
 * - (4.2) Project Riemann solution back into cell and add it to @f$ \hat Q @f$
 * - (5) Per cell, take the Q values and add the contribution @f$ dt \cdot M^{-1} \hat Q @f$.
 *
 *
 * ## Non-conservative product
 *
 * You find some discussions of the treatment of the non-conservative product in papers by Michael Dumbser
 * et al, e.g. I found the discussion around @f$ P_NP_M @f$ patch-conservative products in his paper
 * "Space-time adaptive ADER discontinuous Galerkin schemes for nonlinear hyperelasticity with material failure"
 * particularly helpful (<a href="https://arxiv.org/abs/2003.02760">arXiv:2003.02760</a>). Have a look
 * notably into (21). Other useful sources of information can be found in this
 * <a href="https://www.math.u-bordeaux.fr/~rabgrall/dfg-cnrs/talks/diehl.pdf">presentation</a>, e.g.
 *
 * Personally, I always use the following hand-wavy explanation:
 *
 *
 * ### Volumentric part
 *
 * We usually keep the @f$ B_x(Q)\nabla _xQ @f$ term as it is and assume that we can represent the
 * outcome again in the same polynomial space that we used for the shape functions. This is quite a
 * crude approximation, as the ncp terms can be really non-linear and nasty, so it is unlikely that
 * the analytical solution can be represented by the same linear combination of shape functions as the
 * input. But we don't really know that much about the ncp term anyway, so any other choice of shape
 * functions to represent the outcome would be equally flawed.
 *
 * If the @f$ B_x(Q)\nabla _xQ @f$ term projects into our ansatz space, then the whole ncp contributions
 * enter our equations as one giant mass matrix times B(Q). However, that's only a part of the story.
 *
 * Whenever we study
 *
 * @f$ \int _\Omega \Big( B_x(Q)\nabla _xQ, \phi \Big) dx @f$
 *
 * we know that we can split up the integral into a sum over integrals over the cells but that
 * the faces require special care, as the solution jumps there. In the context of the ncp, we
 * follow the argument by Hulsen et al: The idea is that we split up the integral domain into
 * two areas: The face plus and @f$ \epsilon @f$ environment and the remainder of the cells. So we
 * get a sum over the faces (plus the tiny area around them) and the remaining cells. We have already
 * discussed the latter. Here, we obtain a mass matrix.
 *
 * ### The face contribution
 *
 * In this @f$ \epsilon @f$-environment around the face, now approximate @f$ Q @f$ with a continuous
 * function. So we interpolate between @f$ Q^- @f$ and @f$ Q^+ @f$. As we make @f$ \epsilon @f$ smaller
 * and smaller, the interpolation approximates the jump between @f$ Q^- @f$ and @f$ Q^+ @f$ better
 * and better. The literature makes quite a lot of fuzz about pathes here and path-conservative
 * integrals, but really most of the time it is sufficient to think of it as an interpolation which
 * smoothes out the jump.
 *
 * We end up with an integral
 *
 * @f$ \int _{\partial c^{\epsilon}} \Big( B_x(Q)\nabla _xQ, \phi \Big) dx @f$
 *
 * where the @f$ Q @f$ once more is not properly defined. In line with the Rusanov approximation,
 * we assume that Q is roughly the average when we evaluate B, i.e. we evaluate the term
 *
 * @f$ B_x(Q) \mapsto B_x \Big( \frac{1}{2}Q^- + \frac{1}{2}Q^+ \Big) @f$
 *
 * In the implementation, we note that we now use the term B in two different scenarios: As an
 * application to the gradient and as an application to the average. Obviously, it makes no sense
 * to ask users to provide B twice (they are exactly the same operator), so we call the argument
 * always gradient in the implementation but in the boundary context, we hand the average into
 * the function.
 *
 * The gradient at the cell interface is not known, but we can approximate it with a standard
 * finite differences formulation where h is the volume size along the coordinate axis:
 *
 * @f$ \nabla _xQ \mapsto \frac{1}{h} \Big( Q^+ - Q^- \Big) @f$
 *
 * So far, we still have a volumetric formulation, where the @f$ \phi @f$ term either picks the right
 * or the left half of the term. This yields another factor of 0.5. We finally integrate along the
 * normal to obtain a face integral in line with the flux term, i.e. we get a real integral over the
 * face but the 1/h term is eliminated by the integration. We end up with
 *
 * @f$ \int _{\partial c^{\epsilon}} \Big( B_x(Q)\nabla _xQ, \phi \Big) dx \mapsto \pm \int _{\partial c} B_x \Big( \frac{1}{2}Q^- + \frac{1}{2}Q^+ \Big) \frac{1}{2} \Big( Q^+ - Q^- \Big) \ n dS(x) @f$
 *
 * Usually, the
 *
 * @f$ B_x \Big( \frac{1}{2}Q^- + \frac{1}{2}Q^+ \Big) \frac{1}{2} \Big( Q^+ - Q^- \Big) @f$
 *
 * is expressed in terms of the shape functions projected onto the faces. As we follow
 * a tensor product ansatz, we obtain a d-1-dimensional mass matrix that has to be applied
 * to the face values.
 *
 *
 * ## Data format
 *
 * Even though we try to separate the spatial from the temporal discretisation, it is the
 * Runge-Kutta DG scheme which is currently the most mature version within ExaHyPE 2.
 * The main data format discussion, i.e. what do we store where and when, can thus be found
 * in the python subdirectory
 *
 *         exahype2.rkdg
 *
 * and the class
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
 *
 * ## Todo list
 *
 * - Strides should completely go away and should be subsumed in the enumerators. So the
 *   enumerators is where all the logic sits and the logic is completely encapsulated there.
 * - Investigate if we could remove a few other matrices from the generated Python code
 * - Point sources are missing
 *
 */
