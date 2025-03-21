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


class NodalBasisGaussLobatto(NodalBasis):
    """Concrete implementation for given type of nodes"""

    def get_Gauss_Lobatto_nodes(self, poly_degree):
        """Return Gauss-Lobatto nodes coordinates in [-1, 1] for given polynomial degree"""

        if poly_degree == 0:
            return np.asarray([0.0])
        elif poly_degree == 1:
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
            raise ValueError(f"Add Gauss-Lobatto nodes for polynomial degree {poly_degree}")
        
    def get_Gauss_Legendre_nodes(self, poly_degree):
        """Return Gauss-Legendre nodes coordinates in [-1, 1] for given polynomial degree"""

        return leggauss(poly_degree + 1)[0]

    def nodal_points(self, degree):
        """Set nodal points xi_0, xi_1, ..., xi_p"""

        assert degree >= 0, "Negative degree"

        nodes_distribution = 'GaussLobatto'

        if nodes_distribution   == 'GaussLobatto':
            return self.get_Gauss_Lobatto_nodes(degree)
        elif nodes_distribution == 'GaussLegendre':
            return self.get_Gauss_Legendre_nodes(degree)
        elif nodes_distribution == 'Equidistant':
            return np.linspace(-1.0, 1.0, degree + 1)
        else:
            raise ValueError(f"Unknown nodes distribution {nodes_distribution}")

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


class NodalBasisGaussLegendre(NodalBasisGaussLobatto):
    """
    @todo: Move particular nodes distributions to a common superclass
    """

    def nodal_points(self, degree):
        """Set nodal points xi_0, xi_1, ..., xi_p"""

        assert degree >= 0, "Negative degree"

        nodes_distribution = 'GaussLegendre'

        if nodes_distribution   == 'GaussLobatto':
            return self.get_Gauss_Lobatto_nodes(degree)
        elif nodes_distribution == 'GaussLegendre':
            return self.get_Gauss_Legendre_nodes(degree)
        elif nodes_distribution == 'Equidistant':
            return np.linspace(-1.0, 1.0, degree + 1)
        else:
            raise ValueError(f"Unknown nodes distribution {nodes_distribution}")


