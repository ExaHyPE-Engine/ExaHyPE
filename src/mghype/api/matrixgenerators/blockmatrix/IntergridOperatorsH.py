import numpy as np

class IntergridOperatorsH:
    """!
    Class to produce restriction/prolongation matrices for multigrid tasks.
    
    """

    def __init__(self, dim=2, refinement_order=1):
        self.dim = dim
        self.refinement_order = refinement_order

        assert dim < 3
        assert refinement_order == 1

    def prolongation(self):
        prolong1d = np.asarray([[1,   0],
                                [2/3, 1/3],
                                [1/3, 2/3],
                                [0,   1]])
        
        if self.dim == 1:
            result = prolong1d
        elif self.dim == 2:
            result = np.kron(prolong1d, prolong1d)
        
        return result
    
    def restriction(self):
        return self.prolongation().T
    

if __name__ == "__main__":
    operators = IntergridOperatorsH(dim=2, refinement_order=1)
    print(f'{operators.prolongation() = }')
    print(f'{operators.restriction()  = }')