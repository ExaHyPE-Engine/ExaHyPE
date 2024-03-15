"""Quadrature rules"""
import numpy as np
from numpy.polynomial.legendre import leggauss # To calculate the Gauss-Legendre points and weights


class Quadrature:
    """Abstract base class for quadrature on the reference interval [-1,+1]"""

    def __init__(self, npoints):
        """Initialise object

        :arg npoints: number of quadrature points"""
        self.npoints = npoints


class GaussianQuadrature(Quadrature):
    """Gaussian quadrature on the reference interval [-1,+1]"""

    def __init__(self, npoints):
        """Initialise object

        :arg npoints: number of quadrature points"""
        super().__init__(npoints)
        self.points, self.weights = leggauss(npoints)


class Quadrature2d:
    """Product of two 1d quadrature rules"""

    def __init__(self, quadrature_1d):
        """Initialise object

        :arg underlying 1d quadrature
        """
        quadrature = quadrature_1d
        self.weights = [
            w1 * w2 for w1 in quadrature.weights for w2 in quadrature.weights
        ]
        self.points = [
            np.asarray([xi, eta]) for xi in quadrature.points for eta in quadrature.points
        ]

# N = 3
# Q = GaussianQuadrature(N)
# Q = Quadrature2d(Q)
# print(f'{N}: {Q.points}, {Q.weights}')
# pass