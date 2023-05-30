import scenarios.template_scenario
import sympy


class Oscillating_Lake(scenarios.template_scenario.Template_Scenario):
    def __init__(self):
        super().__init__()

        _x = sympy.sqrt(self.x[0] * self.x[0] + self.x[1] * self.x[1])
        _z = 0.5 * (1 - 0.5 * (sympy.cos(sympy.pi * (_x - 0.5) / 0.5) + 1))
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = _z
        self.initial_values[0] = sympy.Max(0.0, 
                                           0.4 - _z + 0.04 * sympy.sin((_x - 0.5) / 0.25) - sympy.Max(0,
                                                                                     -0.4 + _z))