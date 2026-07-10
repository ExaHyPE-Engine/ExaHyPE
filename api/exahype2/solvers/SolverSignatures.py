# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

# TODO: Maybe used these in the feature for all solvers that have these signatures in common
# TODO: Also, do not make them Python code snippets anymore, but just use real XXX.template.cpp files

max_eigenvalue_signature = """
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal
"""

flux_signature = """
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         F // F[{{NUMBER_OF_UNKNOWNS}}]
"""

ncp_signature = """
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q,           // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ,      // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
"""

source_term_signature = """
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S  // S[{{NUMBER_OF_UNKNOWNS}}]
"""

riemann_solver_signature = """
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
"""
