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
    
    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()

    def implementation_of_ncp(self):
        return """
        double g = 9.81;
        double h = Q[0];
        double b_d = deltaQ[3];
  
        switch(normal){
        case 0: //x
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = g * h * b_d;
            BTimesDeltaQ[2] = 0.0;
            break;
        case 1: //y
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = 0.0;
            BTimesDeltaQ[2] = g * h * b_d;
            break;
        }"""
        
    def implementation_of_refinement_criterion(self):
        # Implementing https://doi.org/10.1007%2Fs00024-016-1412-y could be interesting
        return """
        return ::exahype2::RefinementCommand::Keep;
        """
        """
        ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
        if (tarch::la::greaterEquals(Q[0] > 1e-8 && std::abs(Q[0] + Q[3]), 0.05)) { 
            result = ::exahype2::RefinementCommand::Refine;
        }
        else if(tarch::la::greaterEquals(Q[0] > 1e-8 && 0.01), std::abs(Q[0] + Q[3])) {
            result = ::exahype2::RefinementCommand::Erase;
        }
        logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
        return result;
        """
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation