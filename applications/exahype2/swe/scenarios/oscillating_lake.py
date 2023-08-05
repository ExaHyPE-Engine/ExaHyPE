import scenarios.inundation_scenario
import sympy


class Oscillating_Lake(scenarios.inundation_scenario.Inundation_Scenario): 
    """
    Taken from https://doi.org/10.48550/arXiv.1607.04547
    """
    def __init__(self, refinement_tolerance, epsilon=1e-4):
        super().__init__(refinement_tolerance, epsilon=epsilon)
        super().reflective_boundary_conditions()

        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = -0.2*(1.0 - sympy.sqrt((self.x[0] - 2.0)**2 + (self.x[1] - 2.0)**2))-0.1
        _h = 0.025*(1-sympy.sqrt((self.x[0] - 2)**2 + (self.x[1] - 2)**2))
        self.initial_values[0] = sympy.Piecewise(
            (_h, _h > 0),
            (0.0, True)
        )