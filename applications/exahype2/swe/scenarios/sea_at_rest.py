import scenarios.template_scenario, sympy

class Sea_At_Rest(scenarios.template_scenario.Template_Scenario):
    """
    There should not be any waves being artificially created in this scenario iff the shallow water equations are 
    implemented and solved correctly. 
    """
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[0] = sympy.Piecewise(
            (1.1, self.x[0] <= width[0]/2),
            (1.0, True)
        )
        self.initial_values[3] = -self.initial_values[0]