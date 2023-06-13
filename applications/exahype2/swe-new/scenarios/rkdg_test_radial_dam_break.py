import scenarios.rkdg_template_scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class RKDG_Test_Radial_Dam_Break(scenarios.rkdg_template_scenario.RKDG_Template_Scenario, ABC):
    """
    This whole class is only necessary because the RKDG code generator differs from the FV code generator
    """
    def __init__(self):
        super().__init__()

    def implementation_of_initial_conditions(self):
        return """
        const double x_0 = x(0);
        const double x_1 = x(1);
        
        Q[0] = std::sqrt(std::pow(DomainSize(0)/2 - x_0, 2) + std::pow(DomainSize(1)/2 - x_1, 2)) < DomainSize(0)/10 ? 1.1000000000000001 : 1.0;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[3] = -1.0;
        """