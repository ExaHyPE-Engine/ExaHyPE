import sympy

import pde

import scenarios.scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class Template_Scenario(pde.PDE, scenarios.scenario.Scenario, ABC):
    """
    This class provides a template swe implementation that relies on a non-conservative product to model the bed slope.
    All scenarios that do not rely on their own source terms and boundary condition may extend this class.
    In all other cases, please extend scenario.py and pde.py, and implement the required given methods.
    """
    @abstractmethod
    def __init__(self, refinement_tolerance:float, unkowns=3, auxilliary_variables=1):
        super().__init__(unkowns=unkowns, auxiliary_variables=auxilliary_variables)
        self.definition_of_ncp()
        super().outflow_boundary_conditions()
        self.amr_tol = refinement_tolerance
    
    def definition_of_ncp(self):
        grad_b = self.grad(self.b)

        # NCP [unknowns, dimensions]
        self.ncp[0, 0] = 0.0
        self.ncp[1, 0] = sympy.Piecewise(
                                            (self.g * self.h * grad_b, self.h > 0.0),
                                            (0.0, True)
                                        )
        self.ncp[2, 0] = 0.0

        self.ncp[0, 1] = 0.0
        self.ncp[1, 1] = 0.0
        self.ncp[2, 1] = sympy.Piecewise(
                                            (self.g * self.h * grad_b, self.h > 0.0),
                                            (0.0, True)
                                        )
        
    def implementation_of_refinement_criterion(self):
        return """
        ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
        if (Q[0] > 0.0 && std::abs(Q[0] + Q[3]) >= %f) { 
            result = ::exahype2::RefinementCommand::Refine;
        }
        else {
            result = ::exahype2::RefinementCommand::Erase;
        }
        return result;
        """ % (self.amr_tol)
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation