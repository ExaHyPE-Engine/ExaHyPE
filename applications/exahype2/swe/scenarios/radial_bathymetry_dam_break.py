import scenarios.template_scenario, sympy


class Radial_Bathymetry_Dam_Break(scenarios.template_scenario.Template_Scenario):
    """
    Scenario similar to the radial dam break scenario, but creating the initial flux with a bathymetry discontinuity.
    """
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)

        self.initial_values[0] = 1.0 
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )

        self.initial_values[3] = sympy.Piecewise(
            (-0.9, dam <= width[0] / 10), (-1.0, True)
        ) 

        pass
