# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.matrices import dg_to_fv as average_onto_subcells
from exahype2.dg.matrices import fv_to_dg as project_from_subcells
from exahype2.dg.matrices import quadrature_conversion


def create_subcell_projection_matrices(basis):
    """Returns ``(dg_to_fv, fv_to_dg)`` for one axis of the DG ``basis``.

    ``dg_to_fv`` averages the polynomial over each subcell. ``fv_to_dg`` projects the subcell
    averages back and conserves the cell mean.
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
    """Returns ``(dg_to_lobatto, dg_to_subcells)`` for the discrete maximum principle.

    ``dg_to_lobatto`` samples the polynomial at the Gauss-Lobatto nodes, which include the cell
    boundary. ``dg_to_subcells`` averages it over each subcell.
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
    """Overwrites each entry and its point mirror with their average.

    The rounded entries then are equal under reflection, so the matrix maps a mirror-symmetric
    solution to a mirror-symmetric result.
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
