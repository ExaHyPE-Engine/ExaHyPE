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
 * that we use for the two of them.
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
 * To update the current solution with the source term thus can be combined with the
 * copying: We take the current solution, add the source term times the time step size,
 * and write it into the image.
 *
 * The copy-over can be found within exahype2::dg::cellIntegral_patchwise_computeAndUpdate_functors()
 * or variants thereof. It is a simple loop over all of the dofs within the cell.
 *
 *
 * ## Volume integral
 *
 * As any shape function has local support within one cell, we can evaluate
 *
 * @f$ \int  _c \left( F(Q(t)), \nabla \phi \right) dx @f$
 *
 * only within a cell:
 *
 * ## Projection from and onto faces
 *
 * The switch from a face representation to a volumetric representation and back is
 * a key ingredient of any DG scheme. In my formulae above, the projection of the
 * polynomial onto the
 *
 *
 * ## Non-conservative product
 *
 * Warum kommt das ueberhaupt in das Randproblem mit rein? Kann ich doch rein
 * ueber das Volumen abbilden.
 *
 *
 *
 *
 *
 *
 *
 */




#endif

