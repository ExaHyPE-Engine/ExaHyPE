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

    def get_Gauss_Lobatto_nodes(self, poly_degree):
        """
        Return coordinates of Gauss-Lobatto nodes in [-1, 1] for given polynomial degree.
        Roots of deriv(P_{n-1}), where P_{n-1} is the Legendre polynomial and n is the number of nodal points; 
        plus endpoints of the interval.
        """

        use_explicit_nodes = False # set to True to use explicit nodes below, otherwise compute using Scipy's legendre

        if poly_degree == 0:
            return np.asarray([0.0])

        if not use_explicit_nodes:
            poly = legendre(poly_degree)
            deriv = poly.deriv()
            inter_points = np.sort(np.roots(deriv)) # interior nodal points excluding -1 and 1
            return np.concatenate(([-1.0], inter_points, [1.0]))
        
        else:
            if   poly_degree == 1:
                return np.asarray([-1.0, 1.0])
            elif poly_degree == 2:
                return np.asarray([-1.0, 0.0, 1.0])
            elif poly_degree == 3:
                return np.asarray([-1.0, -0.4472135954999579, 0.4472135954999579, 1.0])
            elif poly_degree == 4:
                return np.asarray([-1.0, -0.6546536707079771, 0.0, 0.6546536707079771, 1.0])
            elif poly_degree == 5:
                return np.asarray([-1.0, -0.7650553239294647, -0.28523151648064504, 0.28523151648064504, 0.7650553239294647, 1.0])
            elif poly_degree == 6:
                return np.asarray([-1.0, -0.8302238962785669, -0.4688487934707142, 0.0, 0.4688487934707142, 0.8302238962785669, 1.0])
            elif poly_degree == 7:
                return np.asarray([-1.0, -0.87174014850960668, -0.59170018143314218, -0.20929921790247885, 0.20929921790247885, 0.59170018143314218, 0.87174014850960668, 1.0])
            else:
                raise ValueError(f"Add Gauss-Lobatto nodes for polynomial degree {poly_degree} or set use_explicit_nodes to False to compute them")

    def nodal_points(self, degree):
        """Set nodal points xi_0, xi_1, ..., xi_p"""

        assert degree >= 0, "Negative degree"

        # Equidistant points
        # return np.linspace(-1.0, 1.0, degree + 1)

        # Gauss-Legendre points
        # print("Gauss-Legendre points are used")
        # return leggauss(degree + 1)[0]

        return self.get_Gauss_Lobatto_nodes(degree)

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