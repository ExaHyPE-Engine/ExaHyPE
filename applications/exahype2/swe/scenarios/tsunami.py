import scenarios.template_scenario, sympy


class Tsunami(scenarios.template_scenario.Template_Scenario):
    """
    This class provides a tsunami scenario. 
    Bathymetry and displacement are read from files provided by the user.
    """
    def __init__(self, refinement_tolerance, use_guided_amr:bool=False):
        super().__init__(refinement_tolerance)
        self.guided_amr = use_guided_amr
        pass

    def implementation_of_initial_conditions(self):
        return """
        double bath = topology_parser.sample_bathymetry(volumeCentre(0), volumeCentre(1));
        double displ = topology_parser.sample_displacement(volumeCentre(0), volumeCentre(1));
        double combined_bathymetry = bath+displ;

        /* Might need this for proper boundary conditions 
        if (combined_bathymetry > -20.0 && combined_bathymetry < 0.0)
            combined_bathymetry = -20.0;
        else if (combined_bathymetry >= 0.0 && combined_bathymetry < 20.0)
            combined_bathymetry = 20.0;
        */
        
        Q[0] = bath > 0  || combined_bathymetry > 0 ? 0.0 : -bath;
        Q[1] = 0.0;
        Q[2] = 0.0;
        Q[3] = combined_bathymetry; 
        """
    
    def implementation_of_refinement_criterion(self):
        if not self.guided_amr:
            return super().implementation_of_refinement_criterion()
        else:
            return self.implementation_of_guided_amr_criterion()
        
    def implementation_of_guided_amr_criterion(self):
        return """
        ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
        double inner_product = adjoint_parser.max_inner_product(t, volumeCentre(0), volumeCentre(1), Q[0] + Q[3], Q[1], Q[2], Q[3]);
        if (Q[0] > 0.0 && inner_product >= %f) { 
            result = ::exahype2::RefinementCommand::Refine;
        }
        else {
            result = ::exahype2::RefinementCommand::Erase;
        }
        return result;
        """ % (self.amr_tol)