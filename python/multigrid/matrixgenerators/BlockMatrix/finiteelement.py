"""Define various DG finite elements. These finite elements essentially allow us to evaluate 
the local basis functions at an arbitrary point of the referent-domain for the respective element.
This reference element can be either a reference cell [-1,+1] x [-1,+1] or a reference 
interval [-1,+1].

We use the following elements:

    * DGScalarElement: scalar-valued functions such as pressure defined in cells
    * DGVectorElement: vector-valued functions such as velocity or momentum defined in cells: to be implemented later. Now use scalar elements for individual components of a vector
    * DGFacetElement: scalar-valued functions such as the function lambda that arises from hyrbidisation

"""

from abc import ABC, abstractmethod
import numpy as np
#from basis import Basis1d
from .nodal_basis import NodalBasisGaussLegendre
from .quadrature import GaussianQuadrature, Quadrature2d

class FiniteElement(ABC):
    """Defines a finite element which describes the local function space on a reference element"""

    def __init__(self):
        """Initialise object"""

    @property
    @abstractmethod
    def ndof(self):
        """return number of unknowns"""
        # implement in derived classes

    @abstractmethod
    def evaluate(self, k, x_local):
        """Evaluate the k-th basis function at the point x_local

        :arg k: index of basis function
        :arg x_local: local position in cell or on facet
        """
        # implement in derived classes

    @abstractmethod
    def evaluate_derivative(self, k, direction, x_local):
        """Evaluate the derivatibe k-th basis function at the point x_local  in the
        direction dir which can be 0 (x-direction) or 1 (y-direction)

        :arg k: index of basis function
        :arg direction: direction (0 or 1)
        :arg x_local: local position in cell or on facet
        """
        # implement in derived class

    # @property
    # def qudrature_rule(self):
    #     """Define set of quadrarute points and weights for the element
    #     :arg order: order of the quadrature rule
    #     """


class DGScalarElement(FiniteElement):
    """Scalar DG finite element on cells

    Scalar valued Discontinuous Galerkin functions on reference cell [-1,+1] x [-1,+1]
    """

    def __init__(self, dim, degree):
        """Initialise object

        :arg degree: polynomial degree
        """
        #self.basis = Basis1d(degree)
        self.dim = dim
        self.degree = degree
        self.basis = NodalBasisGaussLegendre(dim)

    @property
    def ndof(self):
        """return number of unknowns"""
        return (self.degree + 1) ** self.dim

    def evaluate(self, k, x_local):
        """Evaluate the k-th basis function at the point x_local = (xi,eta)
        2D basis functions are enumerated as follows
        !!! So far, for 2D only

            y
            |
          2 | 6  7  8       k = i + j * (self.basis.degree+1)
        j 1 | 3  4  5       => i = k % (self.basis.degree+1)
          0 | 0  1  2          j = k // (self.basis.degree+1)
              -  -  -  - x
              0  1  2
                 i

        :arg k: index of basis function
        :arg x_local: position (xi,eta) in [-1,+1] x [-1,+1]
        """
        i, j = k % (self.degree + 1), k // (self.degree + 1)
        return self.basis.evaluate(self.degree, i, x_local[0]) * self.basis.evaluate(self.degree, j, x_local[1])
        # i, j = k % (self.basis.degree + 1), k // (self.basis.degree + 1)
        # return self.basis.evaluate(i, x_local[0]) * self.basis.evaluate(j, x_local[1])

    def evaluate_derivative(self, k, direction, x_local):
        """Evaluate the derivative of k-th basis function at the point x_local = (xi,eta) in the
        direction dir which can be 0 (x-direction) or 1 (y-direction)

        :arg k: index of basis function
        :arg direction: direction (0 or 1)
        :arg x_local: position (xi,eta) in [-1,+1] x [-1,+1]
        """
        i, j = k % (self.degree + 1), k // (self.degree + 1)
        if direction == 0:
            return self.basis.evaluate_derivative(self.degree, i, x_local[0]) * self.basis.evaluate(self.degree, j, x_local[1])
        elif direction == 1:
            return self.basis.evaluate(self.degree, i, x_local[0]) * self.basis.evaluate_derivative(self.degree, j, x_local[1])
        else:
            raise ValueError("Direction can only be 0 (x-direction) or 1 (y-direction)")

    # @property 
    # def qudrature_rule(self):
    #     """Return set of quadrarute points and weights for the element.
    #     Number of points for the 1D quadrature rule is set self.degree + 1
    #     """
    #     quad = GaussianQuadrature(self.degree + 1)
    #     quad2d = Quadrature2d(quad)
    #     return quad2d


class DGFacetElement(FiniteElement):
    """Scalar DG finite element on facets

    Scalar valued Discontinuous Galerkin functions on reference interval [-1,+1]
    """

    def __init__(self, dim, degree):
        """Initialise object

        :arg degree: polynomial degree
        """
        # self.basis = Basis1d(degree)
        self.dim = dim
        self.degree = degree
        self.basis = NodalBasisGaussLegendre(dim)

    @property
    def ndof(self):
        """return number of unknowns"""
        return (self.degree + 1) ** (self.dim - 1)

    def evaluate(self, k, x_local):
        """Evaluate the k-th basis function at the point x_local = (xi,eta)

        :arg k: index of basis function
        :arg x_local: position xi in [-1,+1]
        """
        return self.basis.evaluate(self.degree, k, x_local)

    def evaluate_derivative(self, k, x_local):
        """Evaluate the derivatibe k-th basis function at the point x_local = xi

        :arg k: index of basis function
        :arg x_local: position xi in [-1,+1]
        """
        return self.basis.evaluate_derivative(self.degree, k, x_local)
    
    # @property 
    # def qudrature_rule(self):
    #     """Return set of quadrarute points and weights for the element.
    #     Number of points for the 1D quadrature rule is set self.degree + 1
    #     """
    #     return GaussianQuadrature(self.degree + 1)

# class DGVectorElement(FiniteElement): # to be implemented later
#     """Vector DG finite element on cells

#     Vector-valued Discontinuous Galerkin functions on reference cell [-1,+1] x [-1,+1]
#     """

#     def __init__(self, degree):
#         """Initialise object

#         :arg degree: polynomial degree
#         """
#         self.basis = Basis1d(degree)

#     @property
#     def ndof(self):
#         """return number of unknowns"""
#         return 2 * (self.basis.degree + 1) ** 2

#     def evaluate(self, k, x_local):
#         """Evaluate the k-th basis function at the point x_local = (xi,eta)

#         :arg k: index of basis function
#         :arg x_local: position (xi,eta) in [-1,+1] x [-1,+1]
#         """
#         #raise RuntimeError('Complete implementation of DGVectorElement or consider using scalar elements instead')

#         i, j = (k // 2) % (self.basis.degree + 1), (k // 2) // (self.basis.degree + 1)
#         print(f'{k}: {i}, {j}')
#         direction = np.asarray((1.0, 0.0)) if (k % 2) == 0 else np.asarray((0.0, 1.0))
#         #print(f'{direction}')
#         return (
#             direction
#             * self.basis.evaluate(i, x_local[0])
#             * self.basis.evaluate(j, x_local[1])
#         )
    

#     def evaluate_derivative(self, k, direction, x_local):
#         """Evaluate the derivative of the k-th basis function at the point
#         x_local = (xi,eta) in the direction dir which can be 0 (x-direction) or 1 (y-direction)

#         :arg k: index of basis function
#         :arg direction: direction (0 or 1)
#         :arg x_local: position (xi,eta) in [-1,+1] x [-1,+1]
#         """
#         # IMPLEMENT ME
#         raise RuntimeError('Complete implementation of DGVectorElement or consider using scalar elements instead')
#         pass


# dim = 2
# degree = 1
# fe = DGScalarElement(dim, degree)
# pass

# import matplotlib.pyplot as plt
# ScEl = DGVectorElement(2)
# #val = np.zeros(1,100)
# fig = plt.figure()
# ax = fig.add_subplot(111, projection='3d')
# for x in np.arange(-1.0, 1.0, 0.05):
#     for y in np.arange(-1.0, 1.0, 0.05):
#         val = ScEl.evaluate(7, [x,y])
#         #val = ScEl.evaluate_derivative(7, 2, [x,y])
#         ax.scatter(x, y, val, c='b', marker='.')
#         plt.show()