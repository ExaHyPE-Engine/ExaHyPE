import numpy as np
from .finiteelement import DGScalarElement
from .nodal_basis import NodalBasisGaussLobatto, NodalBasisGaussLegendre

class IntergridOperators:

    def __init__(self, dim, coarse_order, fine_order, nodes_distribution):
        self.coarse_order = coarse_order
        self.fine_order = fine_order

        self.coarse_basis = NodalBasisGaussLobatto(dim) # Coarse basis is always Gauss-Lobatto
        if nodes_distribution == 'GaussLobatto':
            self.fine_basis = NodalBasisGaussLobatto(dim)
        elif nodes_distribution == 'GaussLegendre':
            self.fine_basis = NodalBasisGaussLegendre(dim)
        else:
            raise ValueError(f"Unknown nodes distribution {nodes_distribution}")
        
        self.coarse_element = DGScalarElement(coarse_order, self.coarse_basis)
        self.fine_element   = DGScalarElement(fine_order,   self.fine_basis)

    def prolongation(self):
        """
        Return the local prolongation matrix of size fine_element.ndof x coarse_element.ndof
        """
        coarse_ndof = self.coarse_element.ndof
        fine_ndof = self.fine_element.ndof
        Prolongation_matrix = np.zeros((fine_ndof, coarse_ndof))
        for i in range(fine_ndof):
            for j in range(coarse_ndof):
                Prolongation_matrix[i, j] = self.coarse_element.evaluate(j, self.fine_element.nodes[i])

        return Prolongation_matrix
    
    def restriction(self):
        """
        Return the local restriction matrix of size coarse_element.ndof x fine_element.ndof
        This is the transposed prolongation matrix
        """
        return np.transpose(self.prolongation())
    
    def injection(self):
        """
        Return the local "injection" or "interpolation" matrix of size coarse_element.ndof x fine_element.ndof
        """
        coarse_ndof = self.coarse_element.ndof
        fine_ndof = self.fine_element.ndof
        Injection_matrix = np.zeros((coarse_ndof, fine_ndof))
        for i in range(coarse_ndof):
            for j in range(fine_ndof):
                Injection_matrix[i, j] = self.fine_element.evaluate(j, self.coarse_element.nodes[i])

        return Injection_matrix
