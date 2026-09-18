# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from abc import abstractmethod

import mpmath as mp
from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto

from exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix import (
    LagrangeBasisWithDiagonalMassMatrix,
)

PREC_DIGITS = 256
mp.dps = PREC_DIGITS
mp.pretty = False


class LagrangeBasis(LagrangeBasisWithDiagonalMassMatrix):
    """Represents a nodal Lagrange basis over the unit interval."""

    def __init__(self, number_of_points):
        super().__init__(number_of_points - 1)
        (
            self.__quadrature_points,
            self.__quadrature_weights,
        ) = self._compute_quadrature_points_and_weights(number_of_points)

    @abstractmethod
    def _compute_quadrature_points_and_weights(self, number_of_points):
        """Computes quadrature points and weights over the unit interval."""
        raise NotImplementedError

    def _transform_and_sort_points_and_weights(
        self, quadrature_points, quadrature_weights
    ):
        """Maps quadrature data from [-1, 1] to [0, 1] and sorts it."""
        points_and_weights = []
        for point, weight in zip(quadrature_points, quadrature_weights):
            transformed_point = (mp.mpf(point) + mp.mpf(1.0)) / mp.mpf(2.0)
            transformed_weight = mp.mpf(weight) / mp.mpf(2.0)
            points_and_weights.append((transformed_point, transformed_weight))

        sorted_points_and_weights = sorted(points_and_weights)
        sorted_points, sorted_weights = zip(*sorted_points_and_weights)
        return list(sorted_points), list(sorted_weights)

    @property
    def quadrature_points(self):
        return self.__quadrature_points

    @property
    def quadrature_weights(self):
        return self.__quadrature_weights


class GaussLegendreBasis(LagrangeBasis):
    def _compute_quadrature_points_and_weights(self, number_of_points):
        quadrature_points, quadrature_weights = gauss_legendre(
            number_of_points, PREC_DIGITS
        )
        return self._transform_and_sort_points_and_weights(
            quadrature_points, quadrature_weights
        )

    def __str__(self):
        return "GaussLegendreBasis"


class GaussLobattoBasis(LagrangeBasis):
    def _compute_quadrature_points_and_weights(self, number_of_points):
        quadrature_points, quadrature_weights = gauss_lobatto(
            number_of_points, PREC_DIGITS
        )
        return self._transform_and_sort_points_and_weights(
            quadrature_points, quadrature_weights
        )

    def __str__(self):
        return "GaussLobattoBasis"
