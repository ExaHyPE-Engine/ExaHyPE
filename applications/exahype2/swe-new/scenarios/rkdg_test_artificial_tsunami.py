import scenarios.rkdg_template_scenario

from abc import ABC, abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class RKDG_Test_Artificial_Tsunami(scenarios.rkdg_template_scenario.RKDG_Template_Scenario, ABC):
    """
    This whole class is only necessary because the RKDG code generator differs from the FV code generator
    """
    def __init__(self):
        super().__init__()

    def implementation_of_initial_conditions(self):
        return """
        const double x_0 = x(0);
        const double x_1 = x(1);
        
        Q[0] = 100;
        Q[1] = 0.0;
        Q[2] = 0.0;
        double _x = 10000/DomainSize(0) * x_0 - 5000;
        double _y = 10000/DomainSize(1) * x_1 - 5000;
        Q[3] = -100;
        // Displacement
        if (abs(_x) <= 500 && abs(_y) <= 500)
            Q[3] += 5 * sin(((_x / 500) + 1) * M_PI) * (-((_y * _y) / 250000) + 1);
        """