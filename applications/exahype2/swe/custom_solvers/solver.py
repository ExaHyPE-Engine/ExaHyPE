from abc import abstractmethod, ABC
from exahype2.solvers.PDETerms import PDETerms

class Solver(ABC):
    """
    This class provides an interface all custom solvers should derive and implement.
    Flux and eigenvalues, which are implemented in PDE do not have to be implemented again if they are to be reused.
    """

    @abstractmethod
    def __init__():
        pass
    
    @abstractmethod
    def implementation_of_riemann_solver(self):
        pass
    
    def implementation_of_flux(self):
        return PDETerms.None_Implementation

    def implementation_of_eigenvalues(self):
        return PDETerms.None_Implementation

    def implementation_of_ncp(self):
        return PDETerms.None_Implementation
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation
