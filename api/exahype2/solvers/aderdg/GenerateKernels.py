# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.GenerateKernels import (
    add_operator_tables_to_dictionary,
    flatten,
    generate_operator_tables,
    table,
)


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

    solver._user_action_set_includes += """
#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/ader/CellErrorIntegral.h"
#include "exahype2/dg/ader/PointSources.h"
#include "exahype2/dg/ader/SpaceTimePredictor.h"
#include "tarch/accelerator/accelerator.h"
"""
