# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.operator_tables import add_operator_tables_to_dictionary
from exahype2.dg.operator_tables import flatten
from exahype2.dg.operator_tables import generate_operator_tables
from exahype2.dg.operator_tables import table


def generate_kernels(solver, dictionary, output):
    add_operator_tables_to_dictionary(
        solver._basis, dictionary["DIMENSIONS"], dictionary
    )
    dictionary["OPERATOR_TABLES"][1]["tables"].append(
        table(
            "InversePicardOperator1D",
            flatten(solver._basis.inverse_picard_operator),
            solver._basis.InversePicardOperator1DDocumentation,
        )
    )

    generate_operator_tables(solver, output, dictionary, dictionary["TABLE_PRECISIONS"])
