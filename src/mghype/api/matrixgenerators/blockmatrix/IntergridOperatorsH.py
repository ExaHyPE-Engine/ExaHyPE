import numpy as np

class IntergridOperatorsH:
    """
    Class to produce restriction/prolongation matrices for h-multigrid for CG.
    
    Note: nodes multiplicity does not need to be accounted for in neither prolongation nor restriction matrices.
          In both cases the target vertices are written to only once!

    The restriction matrix is the transpose of the prolongation matrix.
    """

    def __init__(self, dim=2, refinement_order=1):
        self.dim = dim
        self.refinement_order = refinement_order

        assert dim > 0
        assert dim < 3
        assert refinement_order == 1

    def prolongation_cell(self, dim=None):
        """
        Generate the corse-to-fine reference cell prolongation matrix, 
        without considering the nodes multiplicity.
        """

        if dim is None:
            dim = self.dim

        # Prolongate corse 2-node interval to the fine 4-node equispaced interval
        # Use this matrix to generate the matrices for higher dimensions using Kronecker product
        prolong_1d = np.asarray([[1,   0],
                                 [2/3, 1/3],
                                 [1/3, 2/3],
                                 [0,   1]])
        
        result = prolong_1d.copy()
        for _ in range(dim - 1):
            result = np.kron(result, prolong_1d)

        return result
    
    def prolongation(self):
        """
        Return the prolongation matrix.
        Note: It does not need to be scaled by the inverse of the multiplicity of the node, 
        as each node is only accessed once for prolongation.
        """                                                             
        return self.prolongation_cell()
    
    def restriction(self):
        """
        Return the restriction matrix accounting for the nodes multiplicity.
        Note: It does not need to be scaled by the inverse of the multiplicity of the node, 
        as each node is only accessed once.
        """
        return self.prolongation_cell().T
    

if __name__ == "__main__":
    operators = IntergridOperatorsH(dim=2, refinement_order=1)
