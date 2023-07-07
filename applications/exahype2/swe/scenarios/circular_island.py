import scenarios.template_scenario, sympy


class Circular_Island(scenarios.template_scenario.Template_Scenario):
    """
    Taken from https://doi.org/10.48550/arXiv.1607.04547
    """
    def __init__(self):
        super().__init__()
        super().reflective_boundary_conditions()
        
        _h_0 = 0.32
        _A = 0.064
        _x_c = 12.5
        _y_c = 15.0
        _r_c = 3.6
        _r = sympy.sqrt((self.x[0] - _x_c)**2 + (self.x[1] - _y_c)**2)
        _gamma = sympy.sqrt((3.0 * _A) / (4.0 * _h_0))

        self.initial_values[0] = _h_0 + (_A / _h_0 * 1.0 / (sympy.cosh(_gamma * (self.x[0] - 2.5))**2))
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0 
        self.initial_values[3] = sympy.Piecewise(
            (0.93 * (1 - (_r/_r_c)), _r <= _r_c),
            (0.0, True)
        )