# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.ButcherTableau import ButcherTableau
from exahype2.solvers.GenerateKernels import (
    add_operator_tables_to_dictionary,
    flatten,
    generate_operator_tables,
    table,
)


def generate_kernels(solver, output):
    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)

    add_operator_tables_to_dictionary(solver._basis, d["DIMENSIONS"], d)
    butcher_tableau = ButcherTableau(solver._rk_order)
    d["OPERATOR_TABLES"].append(
        {
            "name": "ButcherTableau",
            "documentation": " * Butcher tableau of the solver's Runge-Kutta scheme.\n",
            "tables": [
                table(
                    "ButcherMatrix",
                    flatten(butcher_tableau.weight_matrix()),
                    """
  Weights of the completed stages in every stage, packed row major.
""",
                ),
                table(
                    "FinalEstimateWeights",
                    flatten(butcher_tableau.final_estimate_weights()),
                    """
  Weights of the stages in the final estimate.
""",
                ),
                table(
                    "RelativeTimeStepSizes",
                    flatten(butcher_tableau.time_step_sizes()),
                    """
  Fraction of the time step each stage advances to.
""",
                ),
            ],
        }
    )

    generate_operator_tables(solver, output, d, ["double"])

    solver._user_action_set_includes += """
#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/rk/LinearCombination.h"
#include "exahype2/dg/rk/VolumeIntegral.h"
"""
