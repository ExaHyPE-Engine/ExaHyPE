# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.operator_tables import add_operator_tables_to_dictionary
from exahype2.dg.operator_tables import generate_operator_tables


def generate_kernels(solver, dictionary, output):
    add_operator_tables_to_dictionary(
        solver.basis, dictionary["DIMENSIONS"], dictionary
    )
    generate_operator_tables(solver, output, dictionary, dictionary["TABLE_PRECISIONS"])
