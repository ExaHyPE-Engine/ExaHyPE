import scenarios.template_scenario, sympy


class Radial_Dam_Break(scenarios.template_scenario.Template_Scenario):
    """
    A radial dam break scenario, where within the center of the domain, there is a perturbation in the free surface.
    This perturbation leads to a circular wave moving from the center to the edges of the domain.
    """
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )
        
        self.initial_values[0] = sympy.Piecewise(
            (1.1, dam <= width[0] / 10), 
            (1.0, True)
        )
        
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = 0.0
        
        pass