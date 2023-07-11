import scenarios.template_scenario, sympy

class Artificial_Tsunami(scenarios.template_scenario.Template_Scenario):
    """
    This class provides an artificial tsunami scenario. 
    The equation detailing the displacement is taken from the materials of the IN0012 Tsunami Simulation lab course at TUM held in WS21/22.
    """
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)
        self.initial_values[0] = 100.0
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        _x = 10000.0/width[0] * self.x[0] - 5000.0
        _y = 10000.0/width[1] * self.x[1] - 5000.0
        self.initial_values[3] = sympy.Piecewise(
            (-100.0 + 5.0 * sympy.sin((_x / 500.0 + 1) * sympy.pi) * (-(_y * _y / 250000.0) + 1), sympy.And(sympy.Abs(_x) <= 500.0, sympy.Abs(_y) <= 500.0)),
            (-100.0, True)
        )