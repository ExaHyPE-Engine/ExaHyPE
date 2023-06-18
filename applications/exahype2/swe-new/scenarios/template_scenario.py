import sympy

import pde

import scenarios.scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class Template_Scenario(pde.PDE, scenarios.scenario.Scenario, ABC):
    """
    This class provides a template flux implementation that relies on a non-conservative product to model the bed slope.
    This class also provides an implementation for boundary condition that just call the homogeneous Neumann boundary conditions of PDE.py.
    All scenarios that do not rely on their own source terms and boundary condition may extend this class.
    In all other cases, please extend scenario.py and pde.py, and implement the required given methods.
    """
    @abstractmethod
    def __init__(self):
        super().__init__()
        self.definition_of_ncp()
        super().outflow_boundary_conditions()
    
    def definition_of_ncp(self):
        grad_b = self.grad(self.b)

        # NCP [unkowns, dimensions]
        self.ncp[0, 0] = 0.0
        self.ncp[1, 0] = self.g * self.h * grad_b
        self.ncp[2, 0] = 0.0

        self.ncp[0, 1] = 0.0
        self.ncp[1, 1] = 0.0
        self.ncp[2, 1] = self.g * self.h * grad_b
        
    def implementation_of_refinement_criterion(self):
        # Implementing https://doi.org/10.1007%2Fs00024-016-1412-y could be interesting
        # Maybe also Richardson Extrapolation Error?
        return """
        return ::exahype2::RefinementCommand::Keep;
        """
        """
        ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
        if (Q[0] > 1e-8 && std::abs(Q[0] + Q[3]) >= 0.05) { 
            result = ::exahype2::RefinementCommand::Refine;
        }
        //else if(Q[0] > 1e-8 && std::abs(Q[0] + Q[3]) <= 0.01) {
            //result = ::exahype2::RefinementCommand::Erase;
        //}
        return result;
        """
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation