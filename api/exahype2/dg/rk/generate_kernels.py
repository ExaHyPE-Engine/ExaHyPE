# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.operator_tables import add_operator_tables_to_dictionary
from exahype2.dg.operator_tables import flatten
from exahype2.dg.operator_tables import generate_operator_tables
from exahype2.dg.operator_tables import table

from .butcher_tableau import ButcherTableau


def generate_kernels(solver, dictionary, output):
    add_operator_tables_to_dictionary(
        solver.basis, dictionary["DIMENSIONS"], dictionary
    )
    butcher_tableau = ButcherTableau(solver.rk_order)
    dictionary["OPERATOR_TABLES"].append(
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

    generate_operator_tables(solver, output, dictionary, ["double"])
