import pde
from abc import ABC, abstractmethod
from exahype2.solvers.fv.PDETerms import PDETerms

class Scenario(pde.PDE, ABC):
    """
    This class provides an interface all scenarios should derive and implement.
    If some methods are to be implemented via sympy in the constructor,
    a simple super call as shown in this class is enough to fully implement the functionality.
    """
    @abstractmethod
    def __init__(self, dimensions, auxiliary_variables):
        super().__init__(dimensions=dimensions, auxiliary_variables=auxiliary_variables)
        pass

    @abstractmethod
    def implementation_of_flux(self):
        return super().implementation_of_flux()

    @abstractmethod
    def implementation_of_boundary_conditions(self):
        return super().implementation_of_boundary_conditions()
    
    @abstractmethod
    def implementation_of_initial_conditions(self):
        return super().implementation_of_initial_conditions()

    @abstractmethod
    def implementation_of_sources(self):
        return super().implementation_of_sources()
    
    @abstractmethod
    def implementation_of_ncp():
        return PDETerms.None_Implementation
    
    @abstractmethod
    def implementation_of_refinement_criterion():
        return "return ::exahype2::RefinementCommand::Keep;"