// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  /**
   *
   * Implementation of a HLLC solver utilising Einfeldt speeds for the outer wave speed estimates. In order to account
   * for varying bathymetry and steady states, the solver also utilises a local hydrostatic reconstruction approach.
   *
   * The basic HLLC approach and the middle height based wave speed estimation process
   * is described in the following scientific sources:
   *
   * @article{Toro2019,
   *      title = {The {HLLC} {Riemann} solver},
   *      volume = {29},
   *      issn = {1432-2153},
   *      url = {https://doi.org/10.1007/s00193-019-00912-4},
   *      doi = {10.1007/s00193-019-00912-4},
   *      abstract = {The HLLC (Harten–Lax–van Leer contact) approximate Riemann solver for computing solutions
   *      to hyperbolic systems by means of finite volume and discontinuous Galerkin methods is reviewed.
   *      HLLC was designed, as early as 1992, as an improvement to the classical HLL (Harten–Lax–van Leer) Riemann
   * solver of Harten, Lax, and van Leer to solve systems with three or more characteristic fields, in order to avoid
   * the excessive numerical dissipation of HLL for intermediate characteristic fields. Such numerical dissipation is
   * particularly evident for slowly moving intermediate linear waves and for long evolution times. High-order accurate
   * numerical methods can, to some extent, compensate for this shortcoming of HLL, but it is a costly remedy and for
   * stationary or nearly stationary intermediate waves such compensation is very difficult to achieve in practice. It
   * is therefore best to resolve the problem radically, at the first-order level, by choosing an appropriate numerical
   * flux. The present paper is a review of the HLLC scheme, starting from some historical notes, going on to a
   * description of the algorithm as applied to some typical hyperbolic systems, and ending with an overview of some of
   * the most significant developments and applications in the last 25 years.}, number = {8}, journal = {Shock Waves},
   *      author = {Toro, E. F.},
   *      month = nov,
   *      year = {2019},
   *      pages = {1065--1082},
   * }
   *
   * In the following book, especially section 11.4 HLLC Riemann Solvers describes the basic HLLC solver:
   *
   * @book{Toro2024,
   *      title={Computational Algorithms for Shallow Water Equations},
   *      author={Toro, Eleuterio F},
   *      year={2024},
   *      publisher={Springer Cham}
   * }
   *
   * @article{Fraccarollo1995,
   *      author = {Luigi Fraccarollo and Eleuterio F. Toro},
   *      title = {Experimental and numerical assessment of the shallow water model for two-dimensional dam-break type
   * problems}, journal = {Journal of Hydraulic Research}, volume = {33}, number = {6}, pages = {843--864}, year =
   * {1995}, publisher = {IAHR Website}, doi = {10.1080/00221689509498555}, URL = {
   *      https://doi.org/10.1080/00221689509498555
   *      },
   *      eprint = {
   *      https://doi.org/10.1080/00221689509498555
   *      }
   * }
   *
   * The local hydrostatic reconstruction approach, used for:
   * - well-balancedness
   * - positivity preservation of the material height h,
   * - dealing with variable topography
   * is described in:
   *
   * @article{Clain2016,
   *      author = {Clain, S. and Reis, C. and Costa, R. and Figueiredo, J. and Baptista, M. A. and Miranda, J. M.},
   *      title = {Second-order finite volume with hydrostatic reconstruction for tsunami simulation},
   *      journal = {Journal of Advances in Modeling Earth Systems},
   *      volume = {8},
   *      number = {4},
   *      pages = {1691-1713},
   *      keywords = {second-order, finite volume scheme, hydrostatic reconstruction, nonconservative flux, application
   * to tsunami}, doi = {https://doi.org/10.1002/2015MS000603}, url =
   * {https://agupubs.onlinelibrary.wiley.com/doi/abs/10.1002/2015MS000603}, eprint =
   * {https://agupubs.onlinelibrary.wiley.com/doi/pdf/10.1002/2015MS000603}, abstract = {Abstract Tsunami modeling
   * commonly accepts the shallow water system as governing equations where the major difficulty is the correct
   * treatment of the nonconservative term due to bathymetry variations. The finite volume method for solving the
   * shallow water equations with such source terms has received great attention in the two last decades. The built-in
   * conservation property, the capacity to correctly treat discontinuities, and the ability to handle complex
   * bathymetry configurations preserving some steady state configurations (well-balanced scheme) make the method very
   * efficient. Nevertheless, it is still a challenge to build an efficient numerical scheme, with very few numerical
   * artifacts (e.g., small numerical diffusion, correct propagation of the discontinuities, accuracy, and robustness),
   * to be used in an operational environment, and that is able to better capture the dynamics of the wet-dry interface
   * and the physical phenomena that occur in the inundation area. In the first part of this paper, we present a new
   * second-order finite volume code. The code is developed for the shallow water equations with a nonconservative term
   * based on the hydrostatic reconstruction technology to achieve a well-balanced scheme and an adequate dry/wet
   * interface treatment. A detailed presentation of the numerical method is given. In the second part of the paper, we
   * highlight the advantages of the new numerical technique. We benchmark the numerical code against analytical,
   * experimental, and field results to assess the robustness and the accuracy of the numerical code. Finally, we use
   * the 28 February 1969 North East Atlantic tsunami to check the performance of the code with real data.}, year =
   * {2016}
   * }
   *
   * @article{Audusse2004,
   *      author = {Audusse, Emmanuel and Bouchut, Fran\c{c}ois and Bristeau, Marie-Odile and Klein, Rupert and
   * Perthame, Beno\i{}⁁t}, title = {A Fast and Stable Well-Balanced Scheme with Hydrostatic Reconstruction for Shallow
   * Water Flows}, journal = {SIAM Journal on Scientific Computing}, volume = {25}, number = {6}, pages = {2050-2065},
   *      year = {2004},
   *      doi = {10.1137/S1064827503431090},
   *      URL = {
   *      https://doi.org/10.1137/S1064827503431090
   *      },
   *      eprint = {
   *      https://doi.org/10.1137/S1064827503431090
   *      }
   *      ,
   *      abstract = { We consider the Saint-Venant system for shallow water flows, with nonflat bottom. It is
   *      a hyperbolic system of conservation laws that approximately describes various geophysical flows,
   *      such as rivers, coastal areas, and oceans when completed with a Coriolis term, or granular flows
   *      when completed with friction. Numerical approximate solutions to this system may be generated using
   *      conservative finite volume methods, which are known to properly handle shocks and contact discontinuities.
   *      However, in general these schemes are known to be quite inaccurate for near steady states, as the structure
   *      of their numerical truncation errors is generally not compatible with exact physical steady state conditions.
   *      This difficulty can be overcome by using the so-called well-balanced schemes. We describe a general strategy,
   *      based on a local hydrostatic reconstruction, that allows us to derive a well-balanced scheme from any given
   *      numerical flux for the homogeneous problem. Whenever the initial solver satisfies some classical
   *      stability properties, it yields a simple and fast well-balanced scheme that preserves the nonnegativity of the
   *      water height and satisfies a semidiscrete entropy inequality. }
   * }
   *
   * @Inbook{Ortleb2022,
   *      author="Ortleb, Sigrunand Lambrechts, Jonathan and K{\"a}rn{\"a}, Tuomas",
   *      editor="Schuttelaars, Henk and Heemink, Arnold and Deleersnijder, Eric",
   *      title="Wetting and drying Procedures for Shallow Water Simulations",
   *      bookTitle="The Mathematics of Marine Modelling: Water, Solute and Particle Dynamics in Estuaries and Shallow
   * Seas", year="2022", publisher="Springer International Publishing", address="Cham", pages="287--314", abstract="In
   * the coastal zone, the alternating exposure and submerging of the seabed is an important feature in coastal
   * engineering and marine ecosystems. The wetting and drying of shallow regions challenges the numerical simulation in
   * terms of robustness, efficiency, and the preservation of physical properties. Considering a physically sound
   * representation, desired numerical properties include positivity preservation with respect to the water depth, local
   * and global mass conservation, well-balancedness with respect to lake at rest steady states, and avoidance of
   * artificial pressure gradients. This chapter focuses on recent numerical methods on fixed computational grids based
   * on the depth-averaged shallow water equations where we review and discuss the classes of finite volume methods and
   * discontinuous Galerkin schemes and their respective wetting and drying treatment. In addition, the numerical
   * approaches are classified with respect to either explicit or implicit time integration, and we discuss the
   * advantages and disadvantages accompanying these two alternatives.", isbn="978-3-031-09559-7",
   *      doi="10.1007/978-3-031-09559-7_11",
   *      url="https://doi.org/10.1007/978-3-031-09559-7_11"
   * }
   *
   * The Kurganov desingularization is described in:
   * @article{Kurganov2007,
   *      title={A second-order well-balanced positivity preserving central-upwind scheme for the Saint-Venant system},
   *      author={Kurganov, Alexander and Petrova, Guergana},
   *      year={2007}
   * }
   *
   * Some details about more sophisticated middle height estimations processes can be found in:
   * @article{George2008,
   *      title = {Augmented Riemann solvers for the shallow water equations over variable topography with steady states
   * and inundation}, journal = {Journal of Computational Physics}, volume = {227}, number = {6}, pages = {3089-3113},
   *      year = {2008},
   *      issn = {0021-9991},
   *      doi = {https://doi.org/10.1016/j.jcp.2007.10.027},
   *      url = {https://www.sciencedirect.com/science/article/pii/S0021999107004767},
   *      author = {David L. George},
   *      keywords = {Shallow water equations, Hyperbolic conservation laws, Finite volume methods, Godunov methods,
   *      Riemann solvers, Wave propagation, Shock capturing methods, Tsunami modeling},
   *      abstract = {We present a class of augmented approximate Riemann solvers for the shallow water equations
   *      in the presence of a variable bottom surface. These belong to the class of simple approximate solvers that use
   *      a set of propagating jump discontinuities, or waves, to approximate the true Riemann solution. Typically,
   *      a simple solver for a system of m conservation laws uses m such discontinuities. We present a four wave solver
   *      for use with the the shallow water equations—a system of two equations in one dimension. The solver is based
   * on a decomposition of an augmented solution vector—the depth, momentum as well as momentum flux and bottom surface.
   *      By decomposing these four variables into four waves the solver is endowed with several desirable properties
   *      simultaneously. This solver is well-balanced: it maintains a large class of steady states by the use of a
   *      properly defined steady state wave—a stationary jump discontinuity in the Riemann solution that acts as a
   * source term. The form of this wave is introduced and described in detail. The solver also maintains depth
   *      non-negativity and extends naturally to Riemann problems with an initial dry state. These are important
   *      properties for applications with steady states and inundation, such as tsunami and flood modeling.
   * Implementing the solver with LeVeque’s wave propagation algorithm [R.J. LeVeque, Wave propagation algorithms for
   *      multi-dimensional hyperbolic systems, J. Comput. Phys. 131 (1997) 327–335] is also described. Several
   * numerical simulations are shown, including a test problem for tsunami modeling.}
   *  }
   *  and:
   *  https://github.com/clawpack/riemann/blob/master/src/rpn2_sw_aug.f90
   *
   */
  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD double hllcRiemannSolver(
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR
  );
} // namespace applications::exahype2::ShallowWater

#include "HLLCRiemannSolver.cpph"
