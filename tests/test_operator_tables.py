# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Checks that every tensor product of the operator tables returns the same bits for every order
of the axes and matches the product of its weights. Checks that the rounded quadrature points
mirror exactly at one half."""

import itertools
import sys

from exahype2.dg.basis import GaussLegendreBasis
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.operator_tables import tensor_product

WEIGHTS = [
    0.11846344252809454,
    0.23931433524968324,
    0.28444444444444444,
    0.23931433524968324,
    0.11846344252809454,
]


def count_order_dependent_products():
    """Returns how many tensor products depend on the order of the axes or miss their weights."""
    nodes = len(WEIGHTS)
    products = tensor_product(WEIGHTS, 3)
    failures = 0
    for indices in itertools.product(range(nodes), repeat=3):
        index = (indices[0] * nodes + indices[1]) * nodes + indices[2]
        for permutation in itertools.permutations(indices):
            permuted = (permutation[0] * nodes + permutation[1]) * nodes + permutation[
                2
            ]
            if products[permuted] != products[index]:
                failures += 1
        expected = WEIGHTS[indices[0]] * WEIGHTS[indices[1]] * WEIGHTS[indices[2]]
        if abs(products[index] - expected) > 1.0e-15 * abs(expected):
            failures += 1
    return failures


def count_asymmetric_points():
    """Returns how many rounded quadrature points mirror inexactly or miss their exact point."""
    failures = 0
    for basis_type in (GaussLegendreBasis, GaussLobattoBasis):
        for number_of_points in range(2, 9):
            basis = basis_type(number_of_points)
            points = basis.rounded_quadrature_points
            last = len(points) - 1
            for node, point in enumerate(points):
                if point - 0.5 != -(points[last - node] - 0.5):
                    failures += 1
                if abs(point - float(basis.quadrature_points[node])) > 2.0e-16:
                    failures += 1
    return failures


def main():
    products = count_order_dependent_products()
    if products:
        print(
            f"{products} tensor products depend on the order of the axes or miss their weights."
        )
    points = count_asymmetric_points()
    if points:
        print(
            f"{points} rounded quadrature points mirror inexactly or miss their exact point."
        )
    if products or points:
        sys.exit(1)
    print("Every tensor product returns the same bits for every order of the axes.")
    print("Every rounded quadrature point mirrors exactly at one half.")


if __name__ == "__main__":
    main()
