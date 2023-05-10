import scenarios.template_scenario, sympy


class Radial_Dam_Break(scenarios.template_scenario.Template_Scenario):
    def __init__(self, width):
        super().__init__()

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )

        # (sqrt((x - 500.f) * (x - 500.f) + (y - 500.f) * (y - 500.f)) < 100.f) ? 15.f : 10.0f;
        self.initial_values[0] = sympy.Piecewise(
            (1.1, dam < width[0] / 10), (1.0, True)
        )
        
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = 0.0 
        
        pass
