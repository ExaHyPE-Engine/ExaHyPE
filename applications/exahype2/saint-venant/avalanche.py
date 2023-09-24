import math

class Avalanche:

    

    def __init__(self, Zeta=23.0, Zeta1=21.0, Zeta2=30.7, Zeta3=22.2, Beta=0.136, FrictionLengthScale=0.00065, Gravity=9.81):
        self.Pi = math.pi
        self.Zeta = Zeta*self.Pi/180
        self.Zeta1 = Zeta1*self.Pi/180
        self.Zeta2 = Zeta2*self.Pi/180
        self.Zeta3 = Zeta3*self.Pi/180
        self.Beta = Beta
        self.FrictionLengthScale = FrictionLengthScale
        self.Gravity = Gravity
        self.Mu1 = math.tan(self.Zeta1)
        self.Mu2 = math.tan(self.Zeta2)
        self.Mu3 = math.tan(self.Zeta3)
        self.Nu = (2.0/9.0)*(self.FrictionLengthScale/self.Beta)*(self.Gravity*math.sin(self.Zeta)/math.sqrt(self.Gravity*math.cos(self.Zeta)))*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu1))-1)
        self.Hstart = self.FrictionLengthScale*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu3))-1)
        self.Hstop = self.FrictionLengthScale*(((self.Mu2-self.Mu1)/(math.tan(self.Zeta)-self.Mu1))-1)

    def implementation_of_initial_conditions(self):
        return """
        const tarch::la::Vector<Dimensions, double>& granularMaterialCenter = {0.15,1.0};
        const bool nearCentre = tarch::la::norm2(volumeCentre-granularMaterialCenter) < 0.1;

        Q[0] = (nearCentre ? 0.008 : 0.0 ); // Water height (h)
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
    
    def implementation_of_boundary_conditions(self):
        return """
        ::exahype2::saintvenant::boundaryConditions(Qinside, Qoutside, faceCentre, t, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
        """
    def implementation_of_flux(self):

        return """ 
        ::exahype2::saintvenant::flux(Q, faceCentre, t, dt, normal, F, NumberOfUnknowns, NumberOfAuxiliaryVariables,{Zeta}, {Gravity});
        """.format(Zeta=self.Zeta, Gravity=self.Gravity)
    
    def implementation_of_max_eigenvalue(self):
        return """
        return ::exahype2::saintvenant::maxEigenvalue(Q, faceCentre, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables, {Zeta}, {Gravity});
        """.format(Zeta=self.Zeta, Gravity=self.Gravity)
    

    def implementation_of_ncp(self):
        return """ 
        if(tarch::la::greater(Q[0],0.0)){{
            const double ih = 1.0 / Q[0];
            const double Nu = {Nu};

            // Partial Derivatives of unknowns
            // Retrieving true value from average value and deltaQ

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
            BTimesDeltaQ[0] = -(volumeH(0) * 0.0);
            BTimesDeltaQ[1] = -(volumeH(0) * Nu * sqrt(Q[0]) * (1.5 * deltaQ[0] * ux + Q[0] * uxx));
            BTimesDeltaQ[2] = -(volumeH(0) * 0.5 * Nu * sqrt(Q[0]) * (1.5 * deltaQ[0] * (uy + vx) + Q[0] * (uxy + vxx)));
            break;
            case 1: // y
            BTimesDeltaQ[0] = -(volumeH(1) * 0.0);
            BTimesDeltaQ[1] = -(volumeH(1) * 0.5 * Nu * sqrt(Q[0]) * (1.5 * deltaQ[0] * (uy + vx) + Q[0] * (uyy + vxy)));
            BTimesDeltaQ[2] = -(volumeH(1) * Nu * sqrt(Q[0]) * (1.5 * deltaQ[0] * vy + Q[0] * vyy));
            break;
            }}
        }}

        else{{
            // std::cout<<"ncp h=0 at t="<<t<<std::endl;
            switch (normal) {{
            case 0: // x
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = 0.0;
            BTimesDeltaQ[2] = 0.0;
            break;
            case 1: // y
            BTimesDeltaQ[0] = 0.0;
            BTimesDeltaQ[1] = 0.0;
            BTimesDeltaQ[2] = 0.0;
            break;
            }}
        }}
        """.format(Nu=self.Nu)


    def implementation_of_sources(self):
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
            const double FrictionLengthScale = {FrictionLengthScale};
            const double Mu1 = {Mu1};
            const double Mu2 = {Mu2};
            const double Mu3 = {Mu3};
            const double Nu = {Nu};
            const double Hstart = {Hstart};
            const double Hstop = {Hstop};
            // std::cout<<"sourceTerm h>0 at t="<<t<<std::endl;


            const double Fr       = ubar / sqrt(Gravity * Q[0] * cos(Zeta));
            const double Tau      = Beta * Q[0] / Hstop - Fr;
            const double Betastar = Beta * (Mu2 + Mu3 - 2 * Mu1) / ((Mu2 - Mu3) * (Mu2 - Mu3)) - Tau;
            const double Hstar    = (Betastar + Tau) * Hstop / Beta;
            double       Mu       = 1.0;

            if (Fr > Betastar) {{
                const double vstop  = (Q[0] * Beta) / (Fr + Tau);
                const double Mustop = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
                Mu                  = Mustop;
            }}

            else if (Fr > 0 && Fr <= Betastar) {{
                const double vstart  = Q[0];
                const double vstop   = Q[0] * Beta / (Betastar + Tau);
                const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
                const double Mustop  = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
                const double T       = Mustart + (Mustop - Mustart) * (Fr / Betastar);
                Mu                   = T;
            }}

            else if (Fr == 0) {{
                const double vstart  = Q[0];
                const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
                Mu                   = Mustart;
            }}

            if(tarch::la::equals(ubar,0.0)){{
                // std::cout<<"sourceTerm h>0 and ubar=0 at t="<<t<<std::endl;
                const double hx = Q[NumberOfUnknowns + h_x]/volumeH(0);
                const double hy = Q[NumberOfUnknowns + h_y]/volumeH(1);
                const tarch::la::Vector<Dimensions, double> frictionVector = {{tan(Zeta)-hx,-hy}};
                const double frictionVectorMagnitude = tarch::la::norm2(frictionVector);
                if(tarch::la::greater(Mu,frictionVectorMagnitude)){
                    Mu = frictionVectorMagnitude;
                }
                S[0] = 0.0;
                S[1] = Q[0] * Gravity * sin(Zeta) - Mu * (frictionVector(0)/frictionVectorMagnitude) * Q[0] * Gravity * cos(Zeta);
                S[2] = -Mu * (frictionVector(1)/frictionVectorMagnitude) * Q[0] * Gravity * cos(Zeta);
            }}
            else if(tarch::la::greater(ubar,0.0)){{
                // std::cout<<"sourceTerm h>0 ubar>0 at t="<<t<<std::endl;
                S[0] = 0.0;
                S[1] = Q[0] * Gravity * sin(Zeta) - Mu * (u / ubar) * Q[0] * Gravity * cos(Zeta);
                S[2] = -Mu * (v / ubar) * Q[0] * Gravity * cos(Zeta);
            }}
        }}

        else{{
            // std::cout<<"sourceTerm h=0 at t="<<t<<std::endl;
            S[0] = 0.0;
            S[1] = 0.0;
            S[2] = 0.0;
        }}
        """.format(Gravity=self.Gravity, Zeta=self.Zeta, Zeta1=self.Zeta1, Zeta2=self.Zeta2, Zeta3=self.Zeta3, Beta=self.Beta, FrictionLengthScale=self.FrictionLengthScale, Mu1=self.Mu1, Mu2=self.Mu2, Mu3=self.Mu3, Nu=self.Nu, Hstart=self.Hstart, Hstop=self.Hstop)


    def implementation_of_refinement_criterion(self):
        return """auto result = ::exahype2::RefinementCommand::Keep;
        return result;
        """

        

