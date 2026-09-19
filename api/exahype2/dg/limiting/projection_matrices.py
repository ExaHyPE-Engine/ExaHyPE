# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.matrices import dg_to_fv as average_onto_subcells
from exahype2.dg.matrices import fv_to_dg as project_from_subcells
from exahype2.dg.matrices import quadrature_conversion
from exahype2.dg.matrices import render_vector


def render_matrix_table(name, matrix, precision):
    """Returns the definition of one constant projection table of the given ``precision``.

    The table is flattened row by input degree of freedom, the layout
    ``exahype2::dg::projectTensorProduct`` reads.
    """
    values = [value for row in matrix for value in row]
    return "  constexpr {} {}[] = {};\n".format(precision, name, render_vector(values))


def create_subcell_projection_matrices(basis):
    """Returns ``(dg_to_fv, fv_to_dg)`` for one axis of the given DG ``basis``.

    ``dg_to_fv`` averages the discontinuous Galerkin polynomial onto each subcell,
    and ``fv_to_dg`` projects the subcell averages back while conserving the cell
    mean.
    """
    nodes = basis.quadrature_points
    weights = basis.quadrature_weights
    number_of_nodes = len(nodes)
    number_of_subcells = 2 * number_of_nodes - 1
    dg_to_fv = average_onto_subcells(nodes, weights, number_of_subcells)
    fv_to_dg = project_from_subcells(dg_to_fv, weights, number_of_subcells)
    _symmetrise_mirror_pairs(dg_to_fv)
    _symmetrise_mirror_pairs(fv_to_dg)
    return dg_to_fv, fv_to_dg


def create_dmp_sampling_matrices(basis):
    """Returns the sampling matrices of the discrete maximum principle.

    Sampling the polynomial on the Lobatto grid and through its subcell averages
    exposes boundary and subcell-scale extrema to the admissibility test.
    """
    nodes = basis.quadrature_points
    weights = basis.quadrature_weights
    number_of_nodes = len(nodes)
    number_of_subcells = 2 * number_of_nodes - 1
    lobatto_nodes = [
        float(node) for node in GaussLobattoBasis(number_of_nodes).quadrature_points
    ]
    dg_to_lobatto = quadrature_conversion(nodes, lobatto_nodes)
    dg_to_subcells = average_onto_subcells(nodes, weights, number_of_subcells)
    _symmetrise_mirror_pairs(dg_to_lobatto)
    _symmetrise_mirror_pairs(dg_to_subcells)
    return dg_to_lobatto, dg_to_subcells


def _symmetrise_mirror_pairs(matrix):
    """Overwrites each entry and its point mirror with their common average.

    Averaging each mirror pair makes the stored doubles identical under reflection, so the
    operators add no mirror bias to the projections and admissibility samplings they drive.
    """
    number_of_rows = len(matrix)
    number_of_columns = len(matrix[0])
    for row in range(number_of_rows):
        for column in range(number_of_columns):
            mirror_row = number_of_rows - 1 - row
            mirror_column = number_of_columns - 1 - column
            average = 0.5 * (matrix[row][column] + matrix[mirror_row][mirror_column])
            matrix[row][column] = average
            matrix[mirror_row][mirror_column] = average
