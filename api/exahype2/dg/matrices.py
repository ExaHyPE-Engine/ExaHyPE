# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math


def render_vector(entries, entry_separator=",", brackets=("{", "}")):
    """Returns the entries as one C++ initialiser list."""
    return (
        brackets[0]
        + " "
        + entry_separator.join(str(entry) for entry in entries)
        + brackets[1]
    )


def inverse(matrix):
    """Returns the inverse of a square matrix through Gauss elimination with row pivoting."""
    size = len(matrix)
    augmented = [[0.0 for _ in range(2 * size)] for _ in range(size)]
    for row in range(size):
        for column in range(size):
            augmented[row][column] = matrix[column][row]
        augmented[row][row + size] = 1.0

    for pivot_row in range(size):
        largest_row = pivot_row
        largest_value = math.fabs(augmented[pivot_row][pivot_row])
        for row in range(pivot_row + 1, size):
            if math.fabs(augmented[row][pivot_row]) > largest_value:
                largest_row = row
                largest_value = math.fabs(augmented[row][pivot_row])
        augmented[largest_row], augmented[pivot_row] = (
            augmented[pivot_row],
            augmented[largest_row],
        )
        if augmented[pivot_row][pivot_row] == 0.0:
            raise ValueError("The DG basis cannot invert a singular matrix.")
        pivot = 1.0 / augmented[pivot_row][pivot_row]
        for column in range(2 * size):
            augmented[pivot_row][column] *= pivot
        for row in range(pivot_row + 1, size):
            factor = augmented[row][pivot_row]
            for column in range(2 * size):
                augmented[row][column] -= factor * augmented[pivot_row][column]

    for pivot_row in range(size - 1, -1, -1):
        for row in range(pivot_row - 1, -1, -1):
            factor = augmented[row][pivot_row]
            for column in range(2 * size):
                augmented[row][column] -= factor * augmented[pivot_row][column]

    return [
        [augmented[row][column + size] for row in range(size)] for column in range(size)
    ]


def lagrange_basis_values(position, nodes):
    """Returns the values of the Lagrange basis polynomials of ``nodes`` at ``position``."""
    values = []
    for node in range(len(nodes)):
        value = 1.0
        for other in range(len(nodes)):
            if other != node:
                value = value * (position - nodes[other]) / (nodes[node] - nodes[other])
        values.append(value)
    return values


def quadrature_conversion(from_nodes, to_nodes):
    """Returns the matrix that evaluates a polynomial on ``from_nodes`` at ``to_nodes``."""
    size = len(from_nodes)
    result = [[0.0 for _ in range(size)] for _ in range(size)]
    for column in range(size):
        values = lagrange_basis_values(to_nodes[column], from_nodes)
        for row in range(size):
            result[row][column] = values[row]
    return result


def dg_to_fv(nodes, weights, subcells):
    """Returns the matrix that averages a polynomial on ``nodes`` over ``subcells`` equal
    intervals of the unit interval, with the quadrature ``weights`` of the nodes."""
    size = len(nodes)
    result = [[0.0 for _ in range(subcells)] for _ in range(size)]
    width = 1.0 / float(subcells)
    for subcell in range(subcells):
        left = subcell * width
        for node in range(size):
            values = lagrange_basis_values(left + width * nodes[node], nodes)
            for row in range(size):
                result[row][subcell] += weights[node] * values[row]
    return result


def fv_to_dg(averaging, weights, subcells):
    """Returns the conservation-constrained least-squares inverse of a ``dg_to_fv`` matrix."""
    size = len(weights)
    result = [[0.0 for _ in range(size)] for _ in range(subcells)]
    system = [[0.0 for _ in range(size + 1)] for _ in range(size + 1)]
    right_hand_side = [[0.0 for _ in range(subcells)] for _ in range(size + 1)]
    width = 1.0 / float(subcells)

    for row in range(size):
        for column in range(size):
            for subcell in range(subcells):
                system[column][row] += (
                    2 * averaging[row][subcell] * averaging[column][subcell]
                )
        system[size][row] = weights[row]
    for row in range(size):
        system[row][size] = -weights[row]
    system[size][size] = 0.0

    inverted = inverse(system)

    for subcell in range(subcells):
        for row in range(size):
            right_hand_side[row][subcell] = 2 * averaging[row][subcell]
        right_hand_side[size][subcell] = width

    for subcell in range(subcells):
        for row in range(size):
            for column in range(size + 1):
                result[subcell][row] += (
                    inverted[row][column] * right_hand_side[column][subcell]
                )

    return result
