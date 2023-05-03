import pde
from abc import ABC, abstractmethod

class Scenario(ABC):
    """
    This class provides an interface all scenarios should derive and implement.
    If some methods are to be implemented via sympy in the constructor,
    a simple super call as shown in this class is enough to fully implement the functionality.
    """
    @abstractmethod
    def __init__(self):
        pass

    @abstractmethod
    def implementation_of_flux(self):
        pass

    @abstractmethod
    def implementation_of_boundary_conditions(self):
        pass
    
    @abstractmethod
    def implementation_of_initial_conditions(self):
        pass

    @abstractmethod
    def implementation_of_sources(self):
        pass
    
    @abstractmethod
    def implementation_of_ncp():
        pass
    
    @abstractmethod
    def implementation_of_refinement_criterion():
        pass