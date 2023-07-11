import scenarios.template_scenario, sympy

class Sea_At_Rest(scenarios.template_scenario.Template_Scenario):
    """
    There should not be any waves being artificially created in this scenario iff the shallow water equations are 
    implemented and solved correctly. 
    """
    def __init__(self, width, refinement_tolerance):
        super().__init__(refinement_tolerance)
        self.hu = 0.0
        self.hv = 0.0
        self.h = sympy.Piecewise(
            (10.0, self.x[0] <= width[0]/2),
            (15.0, True)
        )
        self.b = -self.h