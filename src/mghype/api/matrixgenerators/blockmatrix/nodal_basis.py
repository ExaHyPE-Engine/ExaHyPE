from abc import ABC, abstractmethod
import numpy as np
from scipy.interpolate import lagrange
from numpy.polynomial.legendre import leggauss
from functools import lru_cache

__all__ = ["NodalBasisGaussLobatto","NodalBasisGaussLegendre","NodalBasisEquidistant"]

class NodalBasis(ABC):
    """Abstract base class for nodal basis functions"""

    def __init__(self, dim):
        """Initialise a new instance
        :arg dim: dimension
        """
        self.dim = dim

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
        return lagrange(x, y)
    
    @lru_cache(maxsize=None)
    def evaluate(self, degree, k, xi):
        """Evaluate the k-th basis function L^{(m)}_k(xi) of degree m at point xi in [-1,+1],
        return L^{(m)}_k(xi)
        :arg degree: polynomial degree
        :arg k: index of basis function
        :arg xi: position xi
        """        
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
        poly = self.lagrange_polynomial(degree, k)
        deriv = np.polyder(poly)
        return deriv(xi)
    
    @abstractmethod
    def nodal_points(self, degree):
        """Return nodal points xi_0, xi_1, ..., xi_p for a particular polynomial degree
        
        :arg degree: polynomial degree
        """

class NodalBasisGaussLobatto(NodalBasis):
    """Nodal basis with Gauss-Lobatto nodes"""

    def __init__(self,dim):
        super().__init__(dim)
        self._nodal_point_dict = {0: [0.0],
                                  1: [-1.0, 1.0],
                                  2: [-1.0, 0.0, 1.0],
                                  3: [-1.0, -0.4472135954999579, 0.4472135954999579, 1.0],
                                  4: [-1.0, -0.6546536707079771, 0.0, 0.6546536707079771, 1.0],
                                  5: [-1.0, -0.7650553239294647, -0.28523151648064504, 0.28523151648064504, 0.7650553239294647, 1.0],
                                  6: [-1.0, -0.8302238962785669, -0.4688487934707142, 0.0, 0.4688487934707142, 0.8302238962785669, 1.0],
                                  7: [-1.0, -0.87174014850960668, -0.59170018143314218, -0.20929921790247885, 0.20929921790247885, 0.59170018143314218, 0.87174014850960668, 1.0] }
        
    def nodal_points(self, degree):
        """Return nodal points xi_0, xi_1, ..., xi_p for a particular polynomial degree
        
        :arg degree: polynomial degree
        """

        assert degree >= 0, "Negative degree"
        if degree <= 7:
            return np.asarray(self._nodal_point_dict[degree])        
        else:
            p_legendre = np.polynomial.legendre.Legendre(degree*[0]+[1])
            return np.asarray([-1.0]+list(p_legendre.deriv().roots())+[+1.0])
            

class NodalBasisGaussLegendre(NodalBasis):
    """Nodal basis with Gauss-Legendre nodes"""

    def nodal_points(self, degree):
        """Return nodal points xi_0, xi_1, ..., xi_p for a particular polynomial degree p
        
        :arg degree: polynomial degree
        """
        assert degree >= 0, "Negative degree"
        return leggauss(degree + 1)[0]
        
        
class NodalBasisEquidistant(NodalBasis):
    """Nodal basis with equidistant nodes"""

    def nodal_points(self, degree):
        """Return nodal points xi_0, xi_1, ..., xi_p for a particular polynomial degree p
        
        :arg degree: polynomial degree
        """
        assert degree >= 0, "Negative degree"
        if degree == 0:
            return np.asarray([0.0])
        else:
            h = 2/degree
            return np.arange(-1,+1+h/2,h)

