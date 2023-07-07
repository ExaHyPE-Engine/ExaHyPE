import pde
from abc import ABC, abstractmethod

class Scenario(ABC):
    """
    This class provides an interface all scenarios should derive and implement.
    Methods, whose functionality is implemented via sympy, e. g. in the constructor, must not be implemented again.
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
    def implementation_of_ncp(self):
        pass
    
    @abstractmethod
    def implementation_of_refinement_criterion(self):
        pass