import scenarios.rkdg_template_scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class RKDG_Test_Radial_Bathymetry_Dam_Break(scenarios.rkdg_template_scenario.RKDG_Template_Scenario, ABC):
    """
    This whole class is only necessary because the RKDG code generator differs from the FV code generator
    """
    def __init__(self):
        super().__init__()

    def implementation_of_initial_conditions(self):
        return """
        const double x_0 = x(0);
        const double x_1 = x(1);
        
        Q[3] = 5.0*std::sqrt(std::pow(1 - 0.20000000000000001*x_0, 2) + std::pow(1 - 0.20000000000000001*x_1, 2)) < 1.0 ? 0.1000000000000001 : 0.0;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[0] = 1.0;
        """