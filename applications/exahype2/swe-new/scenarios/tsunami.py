import scenarios.template_scenario, sympy


class Tsunami(scenarios.template_scenario.Template_Scenario):
    def __init__(self):
        super().__init__()
        pass

    def implementation_of_initial_conditions(self):
        return """
        double bath = parser.sample_bathymetry(volumeCentre(0), volumeCentre(1));
        double displ = parser.sample_displacement(volumeCentre(0), volumeCentre(1));
        combined_bathymetry = bath+displ;

        /* Might need this for proper boundary conditions
        if (combined_bathymetry > -20.0 && combined_bathymetry < 0.0)
            comined_bathymetry = -20.0;
        else if (combined_bathymetry >= 0.0 && combined_bathymetry < 20.0)
            combined_bathymetry = 20.0;
        */
        
        Q[0] = bath > 0  || combined_bathymetry > 0 ? 0.0 : -bath;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[3] = combined_bathymetry; 
        """