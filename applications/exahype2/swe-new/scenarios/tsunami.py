import scenarios.template_scenario, sympy


class Tsunami(scenarios.template_scenario.Template_Scenario):
    def __init__(self, dimensions, auxiliary_variables, width):
        super().__init__(dimensions, auxiliary_variables)
        pass

    def implementation_of_initial_conditions(self):
        return """
        float bath = parser.sample_bathymetry(volumeCentre(0), volumeCentre(1));
        float displ = parser.sample_displacement(volumeCentre(0), volumeCentre(1));
        Q[0] = bath > 0  || bath+displ > 0 ? 0.0 : -bath;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[3] = bath + displ;
        """