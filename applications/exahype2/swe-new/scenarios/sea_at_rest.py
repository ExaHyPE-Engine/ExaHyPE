import scenarios.template_scenario, sympy

class Sea_At_Rest(scenarios.template_scenario.Template_Scenario):
    """
    This class provides a sea at rest scenario based on the artificial tsunami scenario. 
    There should not be any waves being artificially created in this scenario iff the shallow water equations are 
    implemented correctly. 
    """
    def __init__(self):
        super().__init__()
    
    def implementation_of_initial_conditions(self):
        return """
        Q[1] = 0.0;
        Q[2] = 0.0;
        double _x = 1000/DomainSize.begin()[0] * volumeCentre(0) - 500;
        double _y = 1000/DomainSize.begin()[1] * volumeCentre(1) - 500;
        Q[3] = -100;
        // Displacement
        if (abs(_x) <= 500 && abs(_y) <= 500)
            Q[3] += 5 * sin(((_x / 500) + 1) * M_PI) * (-((_y * _y) / 250000) + 1);
        Q[0] = -Q[3];
        """