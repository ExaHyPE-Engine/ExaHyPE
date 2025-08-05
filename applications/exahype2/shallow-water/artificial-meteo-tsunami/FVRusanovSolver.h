// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractFVRusanovSolver.h"

namespace applications {
  namespace exahype2 {
    namespace swe {
      class FVRusanovSolver;
    } // namespace swe
  } // namespace exahype2
} // namespace applications

class applications::exahype2::swe::FVRusanovSolver: public applications::exahype2::swe::AbstractFVRusanovSolver {
private:
  static tarch::logging::Log _log;

public:
  void initialCondition(
    double* const NOALIAS                        Q, // Q[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const bool                                   gridIsConstructed
  ) override final;


  void boundaryConditions(
    const double* const NOALIAS                  Qinside,  // Qinside[3+1]
    double* const NOALIAS                        Qoutside, // Qoutside[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const int                                    normal
  ) override final;


  ::exahype2::RefinementCommand refinementCriterion(
    const double* const NOALIAS                  Q, // Q[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t
  ) override final;


  double maxEigenvalue(
    const double* const NOALIAS                  Q, // Q[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal
  ) override final;


  void flux(
    const double* const NOALIAS                  Q, // Q[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const NOALIAS                        F // F[3]
  ) override final;


  void nonconservativeProduct(
    const double* const NOALIAS                  Q,      // Q[3+1]
    const double* const NOALIAS                  deltaQ, // deltaQ[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const NOALIAS                        BTimesDeltaQ // BTimesDeltaQ[3]
  ) override final;


  void sourceTerm(
    const double* const NOALIAS                  Q, // Q[3+1]
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    double* const NOALIAS                        S // S[3]
  ) override final;
};
