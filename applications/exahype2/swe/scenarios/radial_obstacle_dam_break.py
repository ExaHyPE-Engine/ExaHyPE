import scenarios.template_scenario, sympy


class Radial_Obstacle_Dam_Break(scenarios.template_scenario.Template_Scenario):
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )
        
        self.initial_values[0] = sympy.Piecewise(
            (1.1, dam <= width[0] / 10), 
            (0.0, sympy.And(dam >= 2 * width[0] / 10, dam <= 3 * width[0] / 10)),
            (1.0, True)
        )
        
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = sympy.Piecewise(
            (2.0, sympy.And(dam >= 2 * width[0] / 10, dam <= 3 * width[0] / 10)),
            (-1.0, True)
        )
        
        pass