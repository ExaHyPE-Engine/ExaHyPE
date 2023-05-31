import scenarios.template_scenario
import sympy


class Oscillating_Lake(scenarios.template_scenario.Template_Scenario): 
    """
    Taken from https://doi.org/10.48550/arXiv.1607.04547
    """
    def __init__(self):
        super().__init__()

        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = -0.2*(1.0 - sympy.sqrt((self.x[0] - 2.0)**2 + (self.x[1] - 2.0)**2))-0.1
        _h = 0.025*(1-sympy.sqrt((self.x[0] - 2)**2 + (self.x[1] - 2)**2))
        self.initial_values[0] = sympy.Piecewise(
            (_h, self.initial_values[3] < _h),
            (0.0, True)
        )

    
    def implementation_of_boundary_conditions(self):
        return self.implementation_of_reflective_BC()