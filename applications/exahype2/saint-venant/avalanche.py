import math

class Avalanche:

    def __init__(self, unknowns, auxiliary_variables, solver="FVRusanovGlobalAdaptive", Zeta=35.0, Zeta1=28.95, Zeta2=44.09, Zeta3=31.81, Beta=1.07, Gamma=2.01, BetaStar=0.06, Kappa=1.0, FrictionLengthScale=0.00035, Gravity=9.81):
        """
        Kappa in the friction law is assumed to be 1 and is just initialised as a const double in the source function.
        The user can apply std::pow((Fr/BetaStar),Kappa) for evaluating T in the source function in case the value of Kappa is different from 1.
        Units for input parameters in constructor are:
        Zeta, Zeta1, Zeta2, Zeta3 in degree.
        FrictionLengthScale in meter.
        Gravity in ms^-2.
        """
        self.unknowns = unknowns
        self.auxiliary_variables = auxiliary_variables
        self.solver = solver
        self.Pi = math.pi
        self.Zeta = Zeta*self.Pi/180 # coversion of degree to radian
        self.Zeta1 = Zeta1*self.Pi/180
        self.Zeta2 = Zeta2*self.Pi/180
        self.Zeta3 = Zeta3*self.Pi/180
        self.Beta = Beta
        self.Gamma = Gamma
        self.BetaStar = BetaStar
        self.Kappa = Kappa
        self.FrictionLengthScale = FrictionLengthScale
        self.Gravity = Gravity
        self.Mu1 = math.tan(self.Zeta1)
        self.Mu2 = math.tan(self.Zeta2)
        self.Mu3 = math.tan(self.Zeta3)
        self.Nu = (2.0/9.0)*(self.FrictionLengthScale/self.Beta)*(self.Gravity*math.sin(self.Zeta)/math.sqrt(self.Gravity*math.cos(self.Zeta)))*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu1))-1)
        self.Hstart = self.FrictionLengthScale*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu3))-1)
        self.Hstop = self.FrictionLengthScale*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu1))-1)

    def implementation_of_initial_conditions(self):
        initial_condition_string_template =  """
        const tarch::la::Vector<Dimensions, double>& granularMaterialCenter = {{0.15,0.35}};
        const bool nearCentre = tarch::la::norm2({volumeCentre}-granularMaterialCenter) < 0.1;

        Q[0] = (nearCentre ? 0.008 : 0.0 ); // Granular material height (h)
        Q[1] = 0.0; // x-momentum (hu)
        Q[2] = 0.0; // y-momentum (hv)
        Q[3] = 0.0;
        Q[4] = 0.0;
        Q[5] = 0.0;
        Q[6] = 0.0;
        Q[7] = 0.0;
        Q[8] = 0.0;
        Q[9] = 0.0;
        Q[10] = 0.0;
        Q[11] = 0.0;
        Q[12] = 0.0;
        Q[13] = 0.0;
        Q[14] = 0.0;
        Q[15] = 0.0;
        """
        if "FV" in self.solver:
            return initial_condition_string_template.format(volumeCentre="volumeCentre")
        elif "ADERDG" in self.solver:
            return initial_condition_string_template.format(volumeCentre="x")
    
    def implementation_of_boundary_conditions(self):
        return """
        ::exahype2::saintvenant::boundaryConditions(Qinside, Qoutside, faceCentre, t, normal, {NumberOfUnknowns}, {NumberOfAuxiliaryVariables});
        """.format(NumberOfUnknowns=self.unknowns,NumberOfAuxiliaryVariables=self.auxiliary_variables)
    
    def implementation_of_flux(self):

        flux_string_template = """ 
        ::exahype2::saintvenant::flux(Q, {faceCentre}, t, dt, normal, F, {NumberOfUnknowns}, {NumberOfAuxiliaryVariables},{Zeta}, {Gravity});
        """
        
        if "FV" in self.solver:
            return flux_string_template.format(faceCentre="faceCentre", NumberOfUnknowns=self.unknowns,NumberOfAuxiliaryVariables=self.auxiliary_variables,Zeta=self.Zeta, Gravity=self.Gravity)
        elif "ADERDG" in self.solver:
            return flux_string_template.format(faceCentre="volumeX", NumberOfUnknowns=self.unknowns,NumberOfAuxiliaryVariables=self.auxiliary_variables, Zeta=self.Zeta, Gravity=self.Gravity)
        
    
    def implementation_of_max_eigenvalue(self):
        max_eigenvalue_string_template =  """
        return ::exahype2::saintvenant::maxEigenvalue(Q, {faceCentre}, t, dt, normal, {NumberOfUnknowns}, {NumberOfAuxiliaryVariables}, {Zeta}, {Gravity});
        """

        if "FV" in self.solver:
            return max_eigenvalue_string_template.format(faceCentre="faceCentre", NumberOfUnknowns=self.unknowns,NumberOfAuxiliaryVariables=self.auxiliary_variables,Zeta=self.Zeta, Gravity=self.Gravity)
        elif "ADERDG" in self.solver:
            return max_eigenvalue_string_template.format(faceCentre="volumeX", NumberOfUnknowns=self.unknowns,NumberOfAuxiliaryVariables=self.auxiliary_variables, Zeta=self.Zeta, Gravity=self.Gravity)

    

    def implementation_of_ncp(self):
        implementation_of_ncp_string_template = """ 
        if(tarch::la::greater(Q[0],0.0)){{
            const double ih = 1.0 / Q[0];
            const double Nu = {Nu}; 
            const int NumberOfUnknowns = {NumberOfUnknowns};

            // Partial Derivatives of unknowns
            // Retrieving true value from average value and deltaQ

            //h
            const double hx  = (Q[NumberOfUnknowns + h_x] - 0.5*deltaQ[NumberOfUnknowns + h_x]) / (1 * volumeH(0));
            const double hy  = (Q[NumberOfUnknowns + h_y] - 0.5*deltaQ[NumberOfUnknowns + h_y]) / (1 * volumeH(1));

            // u
            const double ux  = (Q[NumberOfUnknowns + u_x] - 0.5*deltaQ[NumberOfUnknowns + u_x]) / (1 * volumeH(0));
            const double uy  = (Q[NumberOfUnknowns + u_y] - 0.5*deltaQ[NumberOfUnknowns + u_y]) / (1 * volumeH(1));
            const double uxx = (Q[NumberOfUnknowns + u_xx] - 0.5*deltaQ[NumberOfUnknowns + u_xx]) / (volumeH(0) * volumeH(0));
            const double uyy = (Q[NumberOfUnknowns + u_yy] - 0.5*deltaQ[NumberOfUnknowns + u_yy]) / (volumeH(1) * volumeH(1));
            const double uxy = (Q[NumberOfUnknowns + u_xy] - 0.5*deltaQ[NumberOfUnknowns + u_xy]) / (4 * volumeH(0) * volumeH(1));

            // v
            const double vx  = (Q[NumberOfUnknowns + v_x] - 0.5*deltaQ[NumberOfUnknowns + v_x]) / (1 * volumeH(0));
            const double vy  = (Q[NumberOfUnknowns + v_y] - 0.5*deltaQ[NumberOfUnknowns + v_y]) / (1 * volumeH(1));
            const double vxx = (Q[NumberOfUnknowns + v_xx] - 0.5*deltaQ[NumberOfUnknowns + v_xx]) / (volumeH(0) * volumeH(0));
            const double vyy = (Q[NumberOfUnknowns + v_yy] - 0.5*deltaQ[NumberOfUnknowns + v_yy]) / (volumeH(1) * volumeH(1));
            const double vxy = (Q[NumberOfUnknowns + v_xy] - 0.5*deltaQ[NumberOfUnknowns + v_xy]) / (4 * volumeH(0) * volumeH(1));

            switch (normal) {{
                case 0: // x
                    {BTimesDeltaQ}[0] = -(volumeH(0) * 0.0);
                    {BTimesDeltaQ}[1] = -(volumeH(0) * Nu * sqrt(Q[0]) * (1.5 * hx * ux + Q[0] * uxx));
                    {BTimesDeltaQ}[2] = -(volumeH(0) * 0.5 * Nu * sqrt(Q[0]) * (1.5 * hx * (uy + vx) + Q[0] * (uxy + vxx)));
                break;
                case 1: // y
                    {BTimesDeltaQ}[0] = -(volumeH(1) * 0.0);
                    {BTimesDeltaQ}[1] = -(volumeH(1) * 0.5 * Nu * sqrt(Q[0]) * (1.5 * hy * (uy + vx) + Q[0] * (uyy + vxy)));
                    {BTimesDeltaQ}[2] = -(volumeH(1) * Nu * sqrt(Q[0]) * (1.5 * hy * vy + Q[0] * vyy));
                break;
            }}
        }}

        else{{
            switch (normal) {{
                case 0: // x
                    {BTimesDeltaQ}[0] = 0.0;
                    {BTimesDeltaQ}[1] = 0.0;
                    {BTimesDeltaQ}[2] = 0.0;
                break;
                case 1: // y
                    {BTimesDeltaQ}[0] = 0.0;
                    {BTimesDeltaQ}[1] = 0.0;
                    {BTimesDeltaQ}[2] = 0.0;
                break;
            }}
        }}
        """
        if "FV" in self.solver:
            return implementation_of_ncp_string_template.format(BTimesDeltaQ="BTimesDeltaQ",Nu=self.Nu, NumberOfUnknowns=self.unknowns)
        elif "ADERDG" in self.solver:
            return implementation_of_ncp_string_template.format(BTimesDeltaQ="BgradQ",Nu=self.Nu, NumberOfUnknowns=self.unknowns)
        


    def implementation_of_sources(self):
        """
        Kappa in the friction law is assumed to be 1 and is just initialised as a const double here using self.Kappa.
        The user can apply std::pow((Fr/BetaStar),Kappa) for evaluating T here in case the value of Kappa is different from 1.
        """
        return """
        if(tarch::la::greater(Q[0],0.0)){{    
            const double ih = 1.0 / Q[0];
            const double u  = ih * Q[1];
            const double v  = ih * Q[2];
            const double ubar = sqrt(Q[1] * Q[1] + Q[2] * Q[2]) * ih;
            const double Gravity = {Gravity};
            const double Zeta = {Zeta};
            const double Zeta1 = {Zeta1};
            const double Zeta2 = {Zeta2};
            const double Zeta3 = {Zeta3};
            const double Beta = {Beta};
            const double Gamma = {Gamma};
            const double BetaStar = {BetaStar};
            const double Kappa = {Kappa};
            const double FrictionLengthScale = {FrictionLengthScale};
            const double Mu1 = {Mu1};
            const double Mu2 = {Mu2};
            const double Mu3 = {Mu3};
            const double Nu = {Nu};
            const double Hstart = {Hstart};
            const double Hstop = {Hstop}; 

            const double Fr       = ubar / sqrt(Gravity * Q[0] * cos(Zeta));
            const double Hstar    = (BetaStar + Gamma) * Hstop / Beta;
            double       Mu       = 1.0;

            if (Fr > BetaStar) {{
                const double vstop  = (Q[0] * Beta) / (Fr + Gamma);
                const double Mustop = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
                Mu                  = Mustop;
            }}

            else if (Fr > 0 && Fr <= BetaStar) {{
                const double vstart  = Q[0];
                const double vstop   = Q[0] * Beta / (BetaStar + Gamma);
                const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
                const double Mustop  = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
                const double T       = Mustart + (Mustop - Mustart) * std::pow((Fr / BetaStar),Kappa);
                Mu                   = T;
            }}

            else if (Fr == 0) {{
                const double vstart  = Q[0];
                const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
                Mu                   = Mustart;
            }}

            if(tarch::la::equals(ubar,0.0)){{
                const int NumberOfUnknowns = {NumberOfUnknowns};
                const double hx = Q[NumberOfUnknowns + h_x]/volumeH(0);
                const double hy = Q[NumberOfUnknowns + h_y]/volumeH(1);
                const tarch::la::Vector<Dimensions, double> frictionVector = {{tan(Zeta)-hx,-hy}};
                const double frictionVectorMagnitude = tarch::la::norm2(frictionVector);
                if(tarch::la::greater(Mu,frictionVectorMagnitude)){{
                    Mu = frictionVectorMagnitude;
                }}
                if(tarch::la::equals(Mu,0.0)){{
                    S[0] = 0.0;
                    S[1] = Q[0] * Gravity * sin(Zeta);
                    S[2] = 0.0;
                }}
                else{{
                    S[0] = 0.0;
                    S[1] = Q[0] * Gravity * sin(Zeta) - Mu * (frictionVector(0)/frictionVectorMagnitude) * Q[0] * Gravity * cos(Zeta);
                    S[2] = -Mu * (frictionVector(1)/frictionVectorMagnitude) * Q[0] * Gravity * cos(Zeta);
                }}
            }}
            else if(tarch::la::greater(ubar,0.0)){{
                S[0] = 0.0;
                S[1] = Q[0] * Gravity * sin(Zeta) - Mu * (u / ubar) * Q[0] * Gravity * cos(Zeta);
                S[2] = -Mu * (v / ubar) * Q[0] * Gravity * cos(Zeta);
            }}
        }}

        else{{
            S[0] = 0.0;
            S[1] = 0.0;
            S[2] = 0.0;
        }}
        """.format(Gravity=self.Gravity, Zeta=self.Zeta, Zeta1=self.Zeta1, Zeta2=self.Zeta2, Zeta3=self.Zeta3, Beta=self.Beta, Gamma=self.Gamma, BetaStar=self.BetaStar, Kappa=self.Kappa, FrictionLengthScale=self.FrictionLengthScale, Mu1=self.Mu1, Mu2=self.Mu2, Mu3=self.Mu3, Nu=self.Nu, Hstart=self.Hstart, Hstop=self.Hstop, NumberOfUnknowns=self.unknowns)


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;
        if(volumeCentre(0)<0.79){
            result = ::exahype2::RefinementCommand::Refine;
        }
        else{
            result = ::exahype2::RefinementCommand::Erase;
        }
        // return result;
        return ::exahype2::RefinementCommand::Keep;
        """

        

