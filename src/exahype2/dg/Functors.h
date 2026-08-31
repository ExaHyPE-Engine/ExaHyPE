// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg {
  struct PointSource {
    const tarch::la::Vector<DIMENSIONS, double>& x;
    const double* NOALIAS                        values;
  };

  /** Evaluates the physical flux at one DG quadrature point. */
  typedef std::function<void(
    const double* NOALIAS                        Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* NOALIAS                              F
  )>
    Flux;

  /** Evaluates one directional non-conservative product at a DG quadrature point. */
  typedef std::function<void(
    const double* NOALIAS                        Q,
    const double* NOALIAS                        dQdx,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* NOALIAS                              F
  )>
    NonConservativeProduct;

  /** Evaluates the algebraic source at one DG quadrature point. */
  typedef std::function<void(
    const double* NOALIAS                        Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    double* NOALIAS                              S
  )>
    Source;

  /** Returns the point sources associated with one complete cell. */
  typedef std::function<std::vector<PointSource>(
    const double* NOALIAS                        Q,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt
  )>
    PointSources;

  /** Returns the maximum characteristic speed in one coordinate direction. */
  typedef std::function<
    double(const double* NOALIAS Q, const tarch::la::Vector<DIMENSIONS, double>& x, double t, double dt, int normal)>
    MaxEigenvalue;

  /** Writes an exterior boundary trace from the corresponding interior trace. */
  typedef std::function<void(
    const double* NOALIAS                        Qinside,
    double* NOALIAS                              Qoutside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    int                                          normal
  )>
    BoundaryConditions;
} // namespace exahype2::dg
