import scenarios.inundation_scenario
import sympy


class Three_Mounds_Channel(scenarios.inundation_scenario.Inundation_Scenario): 
    """
    Taken from https://doi.org/10.48550/arXiv.1607.04547
    """
    def __init__(self, refinement_tolerance, epsilon=1e-4):
        super().__init__(refinement_tolerance, epsilon=epsilon)
        super().reflective_boundary_conditions()

        _m1 = 1.0 - 0.10 * sympy.sqrt((self.x[0] - 30.0)**2 + (self.x[1] - 22.5)**2)
        _m2 = 1.0 - 0.10 * sympy.sqrt((self.x[0] - 30.0)**2 + (self.x[1] - 7.50)**2)
        _m3 = 2.8 - 0.28 * sympy.sqrt((self.x[0] - 47.5)**2 + (self.x[1] - 15.0)**2)

        self.initial_values[0] = sympy.Piecewise(
            (0.9, sympy.And(self.x[0] <= 17.0, self.x[1] <= 30.0)), 
            (0.0, True)
            )
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = sympy.Max(0.0, _m1, _m2, _m3)