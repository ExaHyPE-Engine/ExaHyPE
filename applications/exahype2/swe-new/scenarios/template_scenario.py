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
    def __init__(self):
        super().__init__()
    
    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()
    
    def implementation_of_flux(self):        
        return """
        double g = 9.81;
        double h = Q[0];
        double hu = Q[1];
        double hv = Q[2];
        switch(normal) {
            case 0:
                F[0] = hu;
                F[1] = (hu * hu);// / h + (g * h * h) / 2;
                F[2] = (hu * hv) / h;
                break;
            case 1:
                F[0] = hv;
                F[1] = (hu * hv) / h;
                F[2] = (hv * hv) / h;// + (g * h * h) / 2;
                break;
        }
        """
    

    def implementation_of_ncp(self):
        return """
        double g = 9.81;
  
        switch(normal){
        case 0: //x
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
            BTimesDeltaQ[2] = 0.0;
            //BTimesDeltaQ[3] = 0.0;
            break;
        case 1: //y
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = 0.0;
            BTimesDeltaQ[2] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
            //BTimesDeltaQ[3] = 0.0;
            break;
        }"""
        return PDETerms.None_Implementation

    # TODO RefinementCommand::Erase implementieren
    def implementation_of_refinement_criterion(self):
        return """
        return ::exahype2::RefinementCommand::Keep;
        """
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation
        return """
        double g = 9.81;
        double h = Q[0];
        double b = Q[3];
        S[0] = 0;
        S[1] = -g * h * b;
        S[2] = -g * h * b;
        """