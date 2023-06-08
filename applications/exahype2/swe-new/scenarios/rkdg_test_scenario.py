import pde

import scenarios.scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class RKDG_Test_Scenario(pde.PDE, scenarios.scenario.Scenario, ABC):
    """
    This whole class is only necessary because the RKDG code generator differs from the FV code generator
    """
    def __init__(self):
        super().__init__()

    def implementation_of_initial_conditions(self):
        return """
        const double x_0 = x(0);
        const double x_1 = x(1);
        
        Q[0] = 5.0*std::sqrt(std::pow(1 - 0.20000000000000001*x_0, 2) + std::pow(1 - 0.20000000000000001*x_1, 2)) < 1.0 ? 1.1000000000000001 : 1.0;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[3] = -1.0;
        """

    def implementation_of_flux(self):
        return """
        [[maybe_unused]] const double h = Q[0];
        [[maybe_unused]] const double hu = Q[1];
        [[maybe_unused]] const double hv = Q[2];
        switch( normal ) {
            case 0:
                F[0] = hu;
                F[1] = 4.9050000000000002*std::pow(h, 2) + hu * (h > 0.0 ? hu/h : 0.0);
                F[2] = h * (h > 0.0 ? hu/h : 0.0) * (h > 0.0 ? hv/h : 0.0);
                break;
            case 1:
                F[0] = hv;
                F[1] = h * (h > 0.0 ? hu/h : 0.0) * (h > 0.0 ? hv/h : 0.0);
                F[2] = 4.9050000000000002*std::pow(h, 2) + hv * (h > 0.0 ? hv/h : 0.0);
                break;
        }
        """
    
    def implementation_of_max_eigenvalue(self):
        return """
        [[maybe_unused]] const double h = Q[0];
        [[maybe_unused]] const double hu = Q[1];
        [[maybe_unused]] const double hv = Q[2];
        double lambda[3];
        switch (normal) {
            case 0:
                lambda[0] = h > 0.0 ?  hu/h : 0.0;
                lambda[1] = 3.1320919526731652*std::sqrt(h) + (h > 0.0 ? hu/h : 0.0);
                lambda[2] = -3.1320919526731652*std::sqrt(h) + (h > 0.0 ? hu/h : 0.0);
                break;
            case 1:
                lambda[0] = h > 0.0 ? hv/h : 0.0;
                lambda[1] = 3.1320919526731652*std::sqrt(h) + (h > 0.0 ? hv/h : 0.0);
                lambda[2] = -3.1320919526731652*std::sqrt(h) + (h > 0.0 ? hv/h : 0.0);
                break;
        }
        double result = 0.0;
        result = std::max( result, std::abs(lambda[0]) );
        result = std::max( result, std::abs(lambda[1]) );
        result = std::max( result, std::abs(lambda[2]) );
        return result;
        """

    def implementation_of_ncp(self):
        return """
        [[maybe_unused]] const double g = 9.81;
        [[maybe_unused]] const double h = Q[0];
        [[maybe_unused]] const double b_d = dQdx[3];
  
        switch(normal){
        case 0: //x
            BgradQ[0] = 0.0;
            BgradQ[1] = g * h * b_d;
            BgradQ[2] = 0.0;
            break;
        case 1: //y
            BgradQ[0] = 0.0;
            BgradQ[1] = 0.0;
            BgradQ[2] = g * h * b_d;
            break;
        }"""
    
    def implementation_of_sources(self):
        return PDETerms.None_Implementation
    
    def implementation_of_refinement_criterion(self):
        # Implementing https://doi.org/10.1007%2Fs00024-016-1412-y could be interesting
        # Maybe also Richardson Extrapolation Error?
        return """
        return ::exahype2::RefinementCommand::Keep;
        """
    
    def implementation_of_boundary_conditions(self):
        return """
        Qoutside[0] = Qinside[0];
        Qoutside[1] = Qinside[1];
        Qoutside[2] = Qinside[2];
        Qoutside[3] = Qinside[3];
        """