# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""One-dimensional projections used by the discontinuous Galerkin limiter.

A troubled cell is projected onto ``2 * order + 1`` finite-volume subcells, evolved
there, and projected back. Both projections are separable, so one matrix per axis
drives ``exahype2::dg::projectTensorProduct``. ``MathUtils`` supplies the
averaging and conservation-constrained least-squares matrices.
"""
from exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix import GaussLobattoBasis
from exahype2.solvers.MathUtils import MathUtils


def create_subcell_projection_matrices(basis):
    """Returns ``(dg_to_fv, fv_to_dg)`` for one axis of the given DG ``basis``.

    ``dg_to_fv`` averages the discontinuous Galerkin polynomial onto each subcell,
    and ``fv_to_dg`` projects the subcell averages back while conserving the cell
    mean. Each is stored row by input degree of freedom, the layout
    ``projectTensorProduct`` reads: ``dg_to_fv`` by DG node, ``fv_to_dg`` by subcell.
    """
    nodes = basis.quadrature_points
    weights = basis.quadrature_weights
    number_of_nodes = len(nodes)
    number_of_subcells = 2 * number_of_nodes - 1
    dg_to_fv = MathUtils.assembleDGToFV(
        nodes,
        weights,
        number_of_nodes,
        number_of_subcells,
    )
    fv_to_dg = MathUtils.assembleFVToDG(
        dg_to_fv,
        weights,
        number_of_nodes,
        number_of_subcells,
    )
    _symmetrise_mirror_pairs(dg_to_fv)
    _symmetrise_mirror_pairs(fv_to_dg)
    return dg_to_fv, fv_to_dg


def create_dmp_sampling_matrices(basis):
    """Returns the sampling matrices of the discrete maximum principle.

    The native DG nodes alone do not bound a polynomial between those nodes.
    Sampling the same polynomial on the Lobatto grid and through its subcell
    averages exposes boundary and subcell-scale extrema to the admissibility test.
    Both matrices use the input-node-major layout read by the tensor projection.
    """
    nodes = basis.quadrature_points
    weights = basis.quadrature_weights
    number_of_nodes = len(nodes)
    number_of_subcells = 2 * number_of_nodes - 1
    lobatto_nodes = [
        float(node) for node in GaussLobattoBasis(number_of_nodes).quadrature_points
    ]
    dg_to_lobatto = MathUtils.assembleQuadratureConversion(
        nodes,
        lobatto_nodes,
        number_of_nodes,
    )
    dg_to_subcells = MathUtils.assembleDGToFV(
        nodes,
        weights,
        number_of_nodes,
        number_of_subcells,
    )
    _symmetrise_mirror_pairs(dg_to_lobatto)
    _symmetrise_mirror_pairs(dg_to_subcells)
    return dg_to_lobatto, dg_to_subcells


def _symmetrise_mirror_pairs(matrix):
    """Overwrites each entry and its point mirror with their common average.

    Reversing both indices maps each operator onto itself because the node and
    subcell layouts are symmetric, yet a raw assembly satisfies the relation
    only to rounding accuracy since its sums accumulate in a fixed order.
    Averaging each mirror pair makes the stored doubles identical under
    reflection at a perturbation of ~1e-16, so the operators themselves add no
    mirror bias to the projections and admissibility samplings they drive.
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
