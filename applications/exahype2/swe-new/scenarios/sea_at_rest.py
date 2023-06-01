import scenarios.template_scenario, sympy

class Sea_At_Rest(scenarios.template_scenario.Template_Scenario):
    """
    There should not be any waves being artificially created in this scenario iff the shallow water equations are 
    implemented and solved correctly. 
    """
    def __init__(self):
        super().__init__()
    
    def implementation_of_initial_conditions(self):
        return """
        Q[1] = 0.0;
        Q[2] = 0.0;
        if(volumeCentre(0) <= DomainSize.begin()[0]/2){
            Q[0] = 10;
            Q[3] = -10;
        }else{
            Q[0] = 15;
            Q[3] = -15;
        }
        """