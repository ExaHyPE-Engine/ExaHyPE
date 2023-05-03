import sympy

import pde

import scenarios.scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class Template_Scenario(pde.PDE, scenarios.scenario.Scenario, ABC):
    """
    This class provides a template flux implementation that relies on a non-conservative product to work.
    This class also provides an implementation for boundary condition that just call the homogeneous Neumann boundary conditions of PDE.py.
    All scenarios that do not rely on their own source terms, fluxes and boundary condition may extend this class.
    In all other cases, please extend scenario.py and implement the required given methods.
    """
    @abstractmethod
    def __init__(self, dimensions, auxiliary_variables):
        super().__init__(dimensions=dimensions, auxiliary_variables=auxiliary_variables)

        # Flux [unknowns, dimensions]
        self.F[0, 0] = self.hu
        self.F[1, 0] = self.hu * self.u #+ (self.g * self.h**2)/2
        self.F[2, 0] = self.hu * self.v

        self.F[0, 1] = self.hv
        self.F[1, 1] = self.hu * self.v
        self.F[2, 1] = self.hv * self.v #+ (self.g * self.h**2)/2
    
    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()
    

    def implementation_of_ncp(self):
        return """
        double g = 9.81;
  
        switch(normal){
        case 0: //x
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
            BTimesDeltaQ[2] = 0.0;
            break;
        case 1: //y
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = 0.0;
            BTimesDeltaQ[2] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
            break;
        }"""

    def implementation_of_refinement_criterion(self):
        return """
        ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

        if ( tarch::la::greaterEquals(Q[0] > 1e-8 && std::abs(Q[0] + Q[3]), 0.05)) { 
            result = ::exahype2::RefinementCommand::Refine;
        }

        logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
        return result;
        """
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation