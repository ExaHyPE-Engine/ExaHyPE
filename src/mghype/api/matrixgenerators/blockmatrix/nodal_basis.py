from abc import ABC, abstractmethod
import itertools
import numpy as np
from scipy.interpolate import lagrange
from scipy.special import legendre
from numpy.polynomial.legendre import leggauss
from functools import lru_cache

class NodalBasis(ABC):
    """Abstract base class for nodal basis functions"""

    def __init__(self, dim):
        """Initialise a new instance
        :arg dim: dimension
        """
        self.dim = dim

    @abstractmethod
    def node1d(self, degree, j):
        """Return the j-th nodal point of a Lagrange polynomial of given degree
        :arg degree: polynomial degree
        :arg j: index of node
        """


class NodalBasisGaussLegendre(NodalBasis):
    """Concrete implementation for Gauss Legendre nodes"""

    def nodal_points(self, degree):
        """Set nodal points xi_0, xi_1, ..., xi_p"""

        assert degree >= 0, "Negative degree"

        # Equidistant points
        # return np.linspace(-1.0, 1.0, degree + 1)

        # Gauss-Legendre points
        # print("Gauss-Legendre points are used")
        # return leggauss(degree + 1)[0]

        # Gauss-Lobatto points: 
        # roots of deriv(P_{n-1}), where P_{n-1} is the Legendre polynomial and n is the number of nodal points; 
        # plus endpoints of the interval

        if degree > 0:
            poly = legendre(degree) # "n-1" = (degree + 1) - 1
            deriv = poly.deriv()
            inter_points = np.sort(np.roots(deriv)) # interior nodal points 

            # result = [-1, inter_points, +1]
            result = np.zeros(len(inter_points) + 2)
            result[1:-1] = inter_points
            result[0]  = -1.0
            result[-1] = +1.0

        elif degree == 0:
            result = np.asarray([0.0])

        return result

    def node1d(self, degree, j):
        """Return the j-th nodal point of a Lagrange polynomial of given degree
        :arg degree: polynomial degree
        :arg j: index of node
        """
        assert j in range(degree+1), f'j must be in range [0, m], m = {degree}'
        return self.nodal_points(degree)[j]

    @lru_cache(maxsize=None)
    def lagrange_polynomial(self, degree, k):
        """Returns k-th basis function L^{(m)}_k(xi) of degree m as a numpy polynomial object
        :arg degree: polynomial degree
        :arg k: index of basis function (= index of node)
        """
        assert k in range(degree+1), f'k must be in [0, m], m = {degree}'

        x = self.nodal_points(degree)
        y = np.zeros(degree+1)
        y[k] = 1.0

        return lagrange(x, y) # scipy.interpolate
    
    @lru_cache(maxsize=None)
    def evaluate(self, degree, k, xi):
        """Evaluate the k-th basis function L^{(m)}_k(xi) of degree m at point xi in [-1,+1],
        return L^{(m)}_k(xi)
        :arg degree: polynomial degree
        :arg k: index of basis function
        :arg xi: position xi
        """
        # assert -1.0 <= xi.all() <= 1.0, f'xi must be in [-1,+1]'
        poly = self.lagrange_polynomial(degree, k)
        return poly(xi)

    @lru_cache(maxsize=None)
    def evaluate_derivative(self, degree, k, xi):
        """Evaluate the derivative of the k-basis function L^{(m)}_k(xi) of degree m at point xi in [-1,+1]
        return dL^{(m)}_k(xi)/dxi
        :arg degree: polynomial degree
        :arg k: index of basis function
        :arg xi: position xi
        """
        # assert -1.0 <= xi.all() <= 1.0, f'xi must be in [-1,+1]'

        poly = self.lagrange_polynomial(degree, k)
        deriv = np.polyder(poly)
        return deriv(xi)
    
    def plot_basis_functions(self):
        """Implement the below code in a method"""
        # ### Plot basis funtions ###
        # import matplotlib.pyplot as plt
        # import matplotlib
        # matplotlib.use('TkAgg')

        # m = 1
        # L = Basis1d(m)
        # x = np.arange(-1.0, 1.0, 0.01)

        # for i in range(m+1):
        #     plt.scatter(x, L.evaluate(i, x), label = f'L_{i}')
        #     #plt.scatter(x, L.evaluate_derivative(i, x), label = f'L_{i}')

        # plt.legend()
        # plt.show()
        # pass
        # ############################