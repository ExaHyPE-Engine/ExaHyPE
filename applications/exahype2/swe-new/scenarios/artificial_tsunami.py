import scenarios.template_scenario

class Artificial_Tsunami(scenarios.template_scenario.Template_Scenario):
    """
    This class provides an artificial tsunami scenario. 
    The equation detailing the displacement is taken from the materials of the IN0012 Tsunami Simulation lab course at TUM held in WS21/22.
    """
    def __init__(self):
        super().__init__()
    
    def implementation_of_initial_conditions(self):
        return """
        Q[0] = 100;
        Q[1] = 0.0;
        Q[2] = 0.0;
        double _x = 1000/DomainSize.begin()[0] * volumeCentre(0) - 500;
        double _y = 1000/DomainSize.begin()[1] * volumeCentre(1) - 500;
        Q[3] = -100;
        // Displacement
        if (abs(_x) <= 500 && abs(_y) <= 500)
            Q[3] += 5 * sin(((_x / 500) + 1) * M_PI) * (-((_y * _y) / 250000) + 1);
        """