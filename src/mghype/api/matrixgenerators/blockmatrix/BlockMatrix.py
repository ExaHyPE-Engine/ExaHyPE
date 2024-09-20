from abc import ABC
import numpy as np


class BlockMatrix(ABC):
    """Base class for block matrix storage format, defining the interface"""

    def __init__(self, dim, basis): # ! pass degree instead of basis, and construct basis inside
        """Initialise instance
        :arg dim: dimension
        :arg basis: nodal basis
        """
        self.dim = dim
        self._L_C = []
        self._L_F = []
        self._A_CC = {}
        self._A_CF = {}
        self._A_FC = {}
        self._A_FF = {}
        self._fs_labels_C = []
        self._fs_labels_F = []
        self._Xi_C = []
        self._Xi_F = []

    @property
    def L_C(self):
        """Return list L_C defining the cell function spaces"""
        return self._L_C

    @property
    def L_F(self):
        """Return list L_F defining the facet function spaces"""
        return self._L_F

    @property
    def fs_labels_C(self):
        """Return list of cell function space labels"""
        return self._fs_labels_C

    @property
    def fs_labels_F(self):
        """Return list of facet function space labels"""
        return self._fs_labels_F

    def n_C(self, ell):
        """number of unknowns per cell for the ell-th cell function space
        :arg ell: index of function space
        """
        return (self._L_C[ell] + 1) ** self.dim

    def n_F(self, ell):
        """number of unknowns per cell for the ell-th facet function space
        :arg ell: index of function space
        """
        return (self._L_F[ell] + 1) ** (self.dim - 1)

    def A_CC(self, k, ell, gamma):
        """Return matrix hat(A)_CC^{(k,ell); gamma}
        :arg ell: second function space (from-space)
        :arg k: first function space (to-space)
        :arg gamma: power gamma, tuple of size dim
        """
        return self._A_CC[(k, ell)][gamma]

    def A_CF(self, k, ell, beta_ref, gamma):
        """Return matrix hat(A)_CF^{(k,ell); beta_ref; gamma}
        :arg ell: second function space (from-space)
        :arg k: first function space (to-space)
        :arg beta_ref: type of facet
        :arg gamma: power gamma, tuple of size dim
        """
        return self._A_CF[(k, ell)][beta_ref][gamma]

    def A_FC(self, k, ell, beta_ref, gamma):
        """Return matrix hat(A)_FC^{(k,ell); beta_ref; gamma}
        :arg ell: second function space (from-space)
        :arg k: first function space (to-space)
        :arg beta_ref: type of facet
        :arg gamma: power gamma, tuple of size dim
        """
        return self._A_FC[(k, ell)][beta_ref][gamma]
    
    def A_FF(self, k, ell, beta_ref, gamma):
        """Return matrix hat(A)_FF^{(k,ell); beta_ref; gamma}
        :arg ell: second function space (from-space)
        :arg k: first function space (to-space)
        :arg beta_ref: type of facet
        :arg gamma: power gamma, tuple of size dim
        """
        return self._A_FF[(k, ell)][beta_ref][gamma]

    def Xi_C(self, ell):
        """return nodal points of ell-th cell function space
        :arg ell: index of function space
        """
        return self._Xi_C[ell]

    def Xi_F(self, ell, orientation):
        """return nodal points of ell-th cell function space
        :arg ell: index of function space
        :arg orientation: orientation of reference element
        """
        return self._Xi_F[ell][orientation]
