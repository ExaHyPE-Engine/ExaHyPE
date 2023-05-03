import scenarios.template_scenario, sympy


class Radial_Bathymetry_Dam_Break(scenarios.template_scenario.Template_Scenario):
    def __init__(self, dimensions, auxiliary_variables, width):
        super().__init__(dimensions=dimensions, auxiliary_variables=auxiliary_variables)

        self.initial_values[0] = 1.0 
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )

        self.initial_values[3] = sympy.Piecewise(
            (0.1, dam < width[0] / 10), (0.0, True)
        ) 

        pass
