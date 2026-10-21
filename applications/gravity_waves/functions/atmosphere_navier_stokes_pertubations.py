



class Atmosphere():

    def compute_gamma(self): 
        return (""" 

                const auto totalNumberDensity = Q[Shortcuts::O] + Q[Shortcuts::O2] + Q[Shortcuts::N2];
                const auto gamma = (1.4 * (totalNumberDensity - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) / totalNumberDensity;
        
                """)

    def compute_cv(self):
        return  ("""
                const auto totalNumberDensity = Q[Shortcuts::O] + Q[Shortcuts::O2] + Q[Shortcuts::N2];
                const auto gamma = (1.4 * (totalNumberDensity - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) / totalNumberDensity;

                const auto meanMolarMass = (Q[Shortcuts::O]  * 0.016
                                + Q[Shortcuts::O2] * 0.032
                                + Q[Shortcuts::N2] * 0.028) / totalNumberDensity;   

                constexpr auto universalGasConstant = 8.314;  
                const auto gasConstant = universalGasConstant / meanMolarMass;  
                [[maybe_unused]] const auto cv = gasConstant / (gamma -1);
                """)



    def initial_conditions(self):
        return ("""

            // linear interpolation of one csv column between two rows at height currentHeight
            auto interpolateBetweenRows = [](const std::string bottomRow, const std::string topRow, const int csvIndex, const double currentHeight) {
                const auto bottomRowValue  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, bottomRow);
                const auto topRowValue     = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, topRow);
                const auto topRowHeight    = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, topRow) * 1000.0;
                const auto bottomRowHeight = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, bottomRow) * 1000.0;

                return bottomRowValue + (currentHeight - bottomRowHeight) * (topRowValue - bottomRowValue) / (topRowHeight - bottomRowHeight);
            };

            std::fill_n(Q, Shortcuts::NumberOfDistinctVariables, 0.0);

            // initialize CSV reader and skip first line of file, which contains column names
            static tarch::reader::CSVReader csvReader("./thunderstorm_model_smooth.csv", 1);

            const int csvNumberOfRows = csvReader.getNumberOfRows();
            const int lastRowIndex    = csvNumberOfRows - 1;

            // height in CSV is in km, so convert to m
            const double csvMaxHeight = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, csvReader.getRow(lastRowIndex)) * 1000.0;

            struct BackgroundState { double rho, T, O, O2, N2, p, E; };

            // Background state at an arbitrary height z (in m). Inside the table the csv
            // columns are interpolated linearly between the two bracketing rows; at or
            // above the top row (csvMaxHeight) every quantity is held constant at its
            // top-row value. Heights outside [0, csvMaxHeight] are clamped, so the
            // function can be evaluated at z +- dz for finite-difference gradients.
            auto background = [&](const double zRequested) -> BackgroundState {
                const double z = std::max(0.0, std::min(zRequested, csvMaxHeight));
                const int rowIndex = static_cast<int>(std::floor(z / csvMaxHeight * lastRowIndex));

                BackgroundState s;
                if (rowIndex >= lastRowIndex) {
                    const std::string row = csvReader.getRow(lastRowIndex);
                    // density is in g/cm^3, needs conversion into kg/m^3 = 1000 * g/cm^3
                    s.rho = 1000.0 * tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvDensityIndex, row);
                    // number densities are in cm^-3
                    s.O   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvAtomicOxygenIndex, row);
                    s.O2  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvMolecularOxygenIndex, row);
                    s.N2  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvNitrogenIndex, row);
                    // temperature is in Kelvin
                    s.T   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvTemperatureIndex, row);
                }
                else {
                    const std::string row     = csvReader.getRow(rowIndex);
                    const std::string nextRow = csvReader.getRow(rowIndex + 1);
                    s.rho = 1000.0 * interpolateBetweenRows(row, nextRow, csvDensityIndex, z);
                    s.O   = interpolateBetweenRows(row, nextRow, csvAtomicOxygenIndex, z);
                    s.O2  = interpolateBetweenRows(row, nextRow, csvMolecularOxygenIndex, z);
                    s.N2  = interpolateBetweenRows(row, nextRow, csvNitrogenIndex, z);
                    s.T   = interpolateBetweenRows(row, nextRow, csvTemperatureIndex, z);
                }
                assertion(!std::isnan(s.rho), s.rho);
                assertion(s.rho > 0, s.rho);

                // same gamma, mean molar mass and gas constant as compute_gamma() / compute_cv()
                const double totalNumberDensity = s.O + s.O2 + s.N2;
                const double gamma = (1.4 * (totalNumberDensity - s.O) + 1.67 * s.O) / totalNumberDensity;
                const double meanMolarMass = (s.O * 0.016 + s.O2 * 0.032 + s.N2 * 0.028) / totalNumberDensity;
                constexpr double universalGasConstant = 8.314;
                const double gasConstant = universalGasConstant / meanMolarMass;

                s.p = s.rho * gasConstant * s.T;
                s.E = s.p / (gamma - 1);
                return s;
            };

            const auto b0 = background(x(DIMENSIONS-1));

            Q[Shortcuts::backgroundstate + 0] = b0.rho;
            Q[Shortcuts::backgroundstate + 1] = b0.p;
            Q[Shortcuts::backgroundstate + 2] = b0.E;
            Q[Shortcuts::O]  = b0.O;
            Q[Shortcuts::O2] = b0.O2;
            Q[Shortcuts::N2] = b0.N2;
            Q[Shortcuts::T]  = b0.T;
            Q[Shortcuts::height] = x(DIMENSIONS-1);

            // vertical gradients of the background by central differences of the very
            // same profile that was stored above (exact derivative of the stored state)
            constexpr double dz = 10.0;
            const double zPlus  = std::min(x(DIMENSIONS-1) + dz, csvMaxHeight);
            const double zMinus = std::max(x(DIMENSIONS-1) - dz, 0.0);
            const auto bPlus  = background(zPlus);
            const auto bMinus = background(zMinus);
            Q[Shortcuts::gradients + 0] = (bPlus.rho - bMinus.rho) / (zPlus - zMinus); // d rho0 / dz
            Q[Shortcuts::gradients + 1] = (bPlus.E   - bMinus.E  ) / (zPlus - zMinus); // d E0   / dz
            Q[Shortcuts::gradients + 2] = (bPlus.T   - bMinus.T  ) / (zPlus - zMinus); // d T0   / dz

            // zero density and energy perturbations
            Q[Shortcuts::rho] = 0;
            Q[Shortcuts::E]   = 0;

            // zero velocity field
            for (int d = 0; d < DIMENSIONS; d++) Q[Shortcuts::j + d] = 0;
        """ )


    def boundary_conditions(self):
        return (
            """
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == DIMENSIONS-1;
            const bool isTop = x[DIMENSIONS-1] > domainMidHeight && normal == DIMENSIONS-1;

            const bool isSide = (normal != DIMENSIONS-1);

            std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);

            // Normal velocity zero after Riemann.

            if (isBottom) Qoutside[Shortcuts::j+normal] = -Qinside[Shortcuts::j+normal];
            else Qoutside[Shortcuts::j+normal] = Qinside[Shortcuts::j+normal];


            // if (isTop || isSide){   //try alternatively copy from ins

            //      Qoutside[Shortcuts::j+0] = 0;
            //      Qoutside[Shortcuts::j+1] = 0;
            //      Qoutside[Shortcuts::rho] = Qinside[Shortcuts::rho];
            //      Qoutside[Shortcuts::E] = Qinside[Shortcuts::E];

            //  }



            """
        )

    def boundary_conditions_1_3(self):
        return (
            """
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == DIMENSIONS-1;
            const bool isTop = x[DIMENSIONS-1] > domainMidHeight && normal == DIMENSIONS-1;

            const bool isSide = (normal != DIMENSIONS-1);

            std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);

            // Normal velocity zero after Riemann.

            if (isBottom) Qoutside[Shortcuts::j+normal] = -Qinside[Shortcuts::j+normal];
            else Qoutside[Shortcuts::j+normal] = Qinside[Shortcuts::j+normal];

            const auto faceMid   = DomainOffset[normal] + 0.5 * DomainSize[normal];
            const auto outward   = (x[normal] > faceMid) ? +1.0 : -1.0;
            const bool isOutflow = outward * Qinside[Shortcuts::j + normal] > 0.0;
            const bool isInflow = outward * Qinside[Shortcuts::j + normal] < 0.0;

            if (isTop || isSide) {
            if (isOutflow) {
                // Three characteristics leave, one enters: one condition, and the
                // hydrostatic pressure is the natural choice. rho and j are already
                // extrapolated by the copy_n above.
                double joSq = 0.0;
                for (int d = 0; d < DIMENSIONS; d++) joSq += Qoutside[Shortcuts::j+d] * Qoutside[Shortcuts::j+d];
                Qoutside[Shortcuts::E] = Qinside[Shortcuts::backgroundstate + 2]
                                    + 0.5 * joSq / Qoutside[Shortcuts::rho];
            } 
            
            if (isInflow){
                for (int d = 0; d < DIMENSIONS; d++) Qoutside[Shortcuts::j+d] = 0.0;
                Qoutside[Shortcuts::rho] = Qinside[Shortcuts::backgroundstate + 0];


            }
            }


            //what if at equilibrium there is no velocity?

            """
        )

    def boundary_conditions_pertb_1_3(self):
        return (
            """
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == DIMENSIONS-1;

            std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);

            if (isBottom) {
                // solid ground: reflect the normal momentum only
                Qoutside[Shortcuts::j+normal] = -Qinside[Shortcuts::j+normal];
            }
            else {
                // top and sides: well-posed subsonic outflow.
                const auto faceMid  = DomainOffset[normal] + 0.5 * DomainSize[normal];
                const auto outward  = (x[normal] > faceMid) ? +1.0 : -1.0;
                const bool isInflow = outward * Qinside[Shortcuts::j + normal] < 0.0;

                if (isInflow) {
                    // no incoming wave energy: force ghost onto the background
                    // (zero perturbation)
                    Qoutside[Shortcuts::rho] = 0.0;
                    for (int d = 0; d < DIMENSIONS; d++) Qoutside[Shortcuts::j+d] = 0.0;
                    Qoutside[Shortcuts::E]   = 0.0;
                } else {
                    // outflow: extrapolate rho', j' (already copied), pin p' = 0
                    // p' = (gamma-1)(E' - 0.5*jDot/rhoFull) = 0  ->  E' = 0.5*jDot/rhoFull
                    const auto rhoFull = Qoutside[Shortcuts::rho] + Qoutside[Shortcuts::backgroundstate+0];
                    double joSq = 0.0;
                    for (int d = 0; d < DIMENSIONS; d++) joSq += Qoutside[Shortcuts::j+d] * Qoutside[Shortcuts::j+d];
                    Qoutside[Shortcuts::E] = 0.5 * joSq / rhoFull;
                }
            }
            """
        )


    
    
    def evaluate_pressure(self):
        return (
            """


            const auto jDot = j[0]*j[0]+j[1]*j[1]
            #if DIMENSIONS==3
                + j[2]*j[2]
            #endif
                ;

                const auto pressure = (gamma-1) * (E - 0.5 * (1/rho) * jDot );
            """
                )


    def flux(self):
        # define parameters that will be usechd in the flux
        flux_init = (
            self.compute_cv()+ 
            """
            
             
            const auto rho_full = Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate+0];
            const auto j = &Q[Shortcuts::j];
            
            const auto E = Q[Shortcuts::E];
            const auto jDot = j[0]*j[0]+j[1]*j[1]
            #if DIMENSIONS == 3
                + j[2]*j[2]
            #endif
                ;


            const auto backgroundPressure = Q[Shortcuts::backgroundstate + 1];
            const auto backgroundE = Q[Shortcuts::backgroundstate + 2];

            const auto invRho = 1./rho_full; 
            
            """
                        )

        # part of the flux which corresponds to the Euler equations
        flux_euler = ("""
            F[Shortcuts::rho] = j[normal];

            F[Shortcuts::j+0] = invRho * j[normal] * j[0];
            F[Shortcuts::j+1] = invRho * j[normal] * j[1];
            #if DIMENSIONS == 3
            F[Shortcuts::j+2] = invRho * j[normal] * j[2];
            #endif

            auto pressurePertubation = (gamma - 1)*(E - 0.5* (jDot / (rho_full)));
            F[Shortcuts::j+normal] += pressurePertubation;

            auto pressure = pressurePertubation + backgroundPressure; 
            auto Efull = E + backgroundE;
            F[Shortcuts::E] = invRho * j[normal] * (Efull + pressure);

            """)

        flux_viscous = ("""
            auto mu = eddy_viscosity*rho_full + mu_molecular; 
            
            // density gradient of the full state: the solver only differentiates the
            // pertubation, the background gradient (vertical only) is stored as auxiliary variable
            double rhoGrad[DIMENSIONS];
            for (int d = 0; d < DIMENSIONS; d++) rhoGrad[d] = deltaQ[d][Shortcuts::rho];
            rhoGrad[DIMENSIONS-1] += Q[Shortcuts::gradients + 0];

            const auto uu   = invRho * Q[Shortcuts::j];
            const auto uux  = invRho * (deltaQ[0][Shortcuts::j+0] - uu * rhoGrad[0]);
            const auto uuy  = invRho * (deltaQ[1][Shortcuts::j+0] - uu * rhoGrad[1]);

            const auto vv   = invRho * Q[Shortcuts::j+1];
            const auto vvx  = invRho * (deltaQ[0][Shortcuts::j+1] - vv * rhoGrad[0]);
            const auto vvy  = invRho * (deltaQ[1][Shortcuts::j+1] - vv * rhoGrad[1]);

            #if DIMENSIONS == 2
            const auto divV23 = 2./3. * (uux + vvy);
            #else
            const auto ww   = invRho * Q[Shortcuts::j+2];
            const auto uuz  = invRho * (deltaQ[2][Shortcuts::j+0] - uu * rhoGrad[2]);
            const auto vvz  = invRho * (deltaQ[2][Shortcuts::j+1] - vv * rhoGrad[2]);
            const auto wwx  = invRho * (deltaQ[0][Shortcuts::j+2] - ww * rhoGrad[0]);
            const auto wwy  = invRho * (deltaQ[1][Shortcuts::j+2] - ww * rhoGrad[1]);
            const auto wwz  = invRho * (deltaQ[2][Shortcuts::j+2] - ww * rhoGrad[2]);
            const auto divV23 = 2./3. * (uux + vvy + wwz);
            #endif


            double Fj[DIMENSIONS];
            switch (normal) {
                case 0:
                Fj[0] = mu * (2 * uux - divV23);
                Fj[1] = mu * (uuy + vvx);
            #if DIMENSIONS == 3
                Fj[2] = mu * (uuz + wwx);
            #endif
                break;
                case 1:
                Fj[0] = mu * (uuy + vvx);
                Fj[1] = mu * (2 * vvy - divV23);
            #if DIMENSIONS == 3
                Fj[2] = mu * (vvz + wwy);
                break;
                case 2:
                Fj[0] = mu * (uuz + wwx);
                Fj[1] = mu * (vvz + wwy);
                Fj[2] = mu * (2 * wwz - divV23);
            #endif
            }

            F[Shortcuts::j+0] -= Fj[0];
            F[Shortcuts::j+1] -= Fj[1];
            F[Shortcuts::E] -= Fj[0]*uu + Fj[1]*vv;
            #if DIMENSIONS == 3
            F[Shortcuts::j+2] -= Fj[2];
            F[Shortcuts::E] -= Fj[2] *ww;
            #endif
            """)
                        
        flux_heat = (
            """
                const auto cp = gasConstant + cv;

            const auto kappa = mu * 1.5 * cp;
            const auto invCv = 1/cv;
            const auto invRho2 = invRho * invRho;
            const auto invRho3 = invRho2 * invRho;

            #if DIMENSIONS == 2
            const auto dTdW1 = -1 * Efull * invRho2 +
                    invRho3 * (Q[Shortcuts::j]*Q[Shortcuts::j] + Q[Shortcuts::j+1] * Q[Shortcuts::j+1]);
            #else
            const auto dTdW1 = -1 * Efull * invRho2 +
                    invRho3 * (Q[Shortcuts::j]*Q[Shortcuts::j] + Q[Shortcuts::j+1] * Q[Shortcuts::j+1] + Q[Shortcuts::j+2] * Q[Shortcuts::j+2]);
            #endif
            const auto dTdW2 = -1 * Q[Shortcuts::j] * invRho2;
            const auto dTdW3 = -1 * Q[Shortcuts::j+1] * invRho2;
            #if DIMENSIONS == 3
            const auto dTdW4 = -1 * Q[Shortcuts::j+2] * invRho2;
            #endif

            auto gradT = invCv * (dTdW1 * deltaQ[normal][Shortcuts::rho] +
                        dTdW2 * deltaQ[normal][Shortcuts::j+0] +
                        dTdW3 * deltaQ[normal][Shortcuts::j+1] +
            #if DIMENSIONS == 3
                        dTdW4 * deltaQ[normal][Shortcuts::j+2] +
            #endif
                        invRho * deltaQ[normal][Shortcuts::E]); 



            // Heat flux
            if(normal == DIMENSIONS-1) {

                // exact perturbation form of  grad(T) - grad(T0): the background terms are
                // cancelled analytically, so every term carries a factor rho', E' or j'
                // and the heat flux is exactly zero at equilibrium (no roundoff seed).
                //   dTdW1 - dTdW1_0 = -E'/rho^2 + E0 rho' (rho+rho0)/(rho^2 rho0^2) + |j|^2/rho^3
                //   1/rho - 1/rho0  = -rho'/(rho rho0)
                const auto rho0 = Q[Shortcuts::backgroundstate+0];
                const auto E0   = Q[Shortcuts::backgroundstate+2];
                const auto rhoP = Q[Shortcuts::rho];
                const auto EP   = Q[Shortcuts::E];

                const auto dA   = -EP * invRho2
                                + E0 * rhoP * (rho_full + rho0) * invRho2 / (rho0 * rho0)
                                + jDot * invRho3;
                const auto dInv = -rhoP * invRho / rho0;

                gradT += invCv * (dA * Q[Shortcuts::gradients + 0] + dInv * Q[Shortcuts::gradients + 1]);

                F[Shortcuts::E] -= kappa * gradT;
            }

            else  F[Shortcuts::E] -= kappa * (gradT);
            """



        # NOTE: currently at a physical boundary the outside gradient is set to the jump between inside and outside;
        # this influences deltaQ values at the outside that influence the flux at the outside of the boundary
        # --> how does it impact the riemann solver?
        # TODO? maybe change so that boundary conditions also sets the gradients
        )


        return (
            flux_init
            + flux_euler
            + flux_viscous
            + flux_heat

        )

    

    def heat_flux_postprocess(self, order, number_of_unknowns, number_of_aux,
                              shortcuts="applications::gravity_waves::thunderstorm::VariableShortcutsADERDGSolver"):
        """C++ body for solver.postprocess_updated_patch that records the vertical heat flux.

        Needs the aux variables "heatflux" and "dTdz" (one each). For every node of the
        updated cell it evaluates the same perturbation-form grad(T') as flux_heat, using the
        nodal gradients of the cell polynomial (same operator the predictor uses), and writes
            dTdz     = d(T')/dz                      [K/m]
            heatflux = -kappa * d(T')/dz             [W/m^2], the vertical heat flux q_z
        (flux_heat subtracts kappa * dTdz from F[E], i.e. adds q_z). The values belong to the
        end of the step; at t=0 they are zero.
        """
        body = """
        {
        using Shortcuts = __SHORTCUTS__;
        using SolverClass = {{FULL_QUALIFIED_SOLVER_NAME}};
        constexpr int hfN     = __N__;
        constexpr int hfData  = __UNKNOWNS__ + __AUX__;
        constexpr int hfNodes = (DIMENSIONS == 3) ? hfN * hfN * hfN : hfN * hfN;
        constexpr int hfZ     = DIMENSIONS - 1;

        {{CORRECTOR_COMPUTATION_PRECISION}} hfGrad[DIMENSIONS * hfNodes * hfData];
        ::exahype2::dg::computeNodalGradients<
            hfN, __UNKNOWNS__, __AUX__,
            std::remove_cv_t<std::remove_pointer_t<decltype(QNew)>>,
            {{CORRECTOR_COMPUTATION_PRECISION}}>(
            hfGrad,
            QNew,
            kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::DerivativeOperator1D,
            marker.getH()
        );

        for (int hfNode = 0; hfNode < hfNodes; hfNode++) {
            auto* const hfQ = QNew + hfNode * hfData;
            const double* const hfD = hfGrad + hfZ * hfNodes * hfData + hfNode * hfData;   // d/dz of every entry
            const double* const Q = hfQ;                                                    // compute_cv reads Q
"""
        body += self.compute_cv()
        body += """
            const auto cp = gasConstant + cv;
            const auto invCv = 1 / cv;

            const auto rho0     = Q[Shortcuts::backgroundstate + 0];
            const auto E0       = Q[Shortcuts::backgroundstate + 2];
            const auto rhoP     = Q[Shortcuts::rho];
            const auto EP       = Q[Shortcuts::E];
            const auto rho_full = rhoP + rho0;
            const auto Efull    = EP + E0;
            const auto invRho   = 1. / rho_full;
            const auto invRho2  = invRho * invRho;
            const auto invRho3  = invRho2 * invRho;

            const auto jDot = Q[Shortcuts::j] * Q[Shortcuts::j] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]
            #if DIMENSIONS == 3
                + Q[Shortcuts::j + 2] * Q[Shortcuts::j + 2]
            #endif
                ;

            const auto dTdW1 = -1 * Efull * invRho2 + invRho3 * jDot;
            const auto dTdW2 = -1 * Q[Shortcuts::j]     * invRho2;
            const auto dTdW3 = -1 * Q[Shortcuts::j + 1] * invRho2;

            // perturbation gradient part (as in flux_heat)
            auto gradT = invCv * (dTdW1 * hfD[Shortcuts::rho]
                                + dTdW2 * hfD[Shortcuts::j + 0]
                                + dTdW3 * hfD[Shortcuts::j + 1]
            #if DIMENSIONS == 3
                                + (-1 * Q[Shortcuts::j + 2] * invRho2) * hfD[Shortcuts::j + 2]
            #endif
                                + invRho * hfD[Shortcuts::E]);

            // background-gradient correction, exact perturbation form (as in flux_heat)
            const auto dA   = -EP * invRho2
                            + E0 * rhoP * (rho_full + rho0) * invRho2 / (rho0 * rho0)
                            + jDot * invRho3;
            const auto dInv = -rhoP * invRho / rho0;
            gradT += invCv * (dA * Q[Shortcuts::gradients + 0] + dInv * Q[Shortcuts::gradients + 1]);

            const auto mu    = SolverClass::eddy_viscosity * rho_full + SolverClass::mu_molecular;
            const auto kappa = mu * 1.5 * cp;

            hfQ[Shortcuts::dTdz]     = gradT;
            hfQ[Shortcuts::heatflux] = -kappa * gradT;
        }
        }
"""
        return (body.replace("__SHORTCUTS__", shortcuts)
                    .replace("__N__", str(order + 1))
                    .replace("__UNKNOWNS__", str(number_of_unknowns))
                    .replace("__AUX__", str(number_of_aux)))


    def eigenvalue_functions(self):
        return (
        """

        constexpr auto molecularDiffusionCoeff = 0;

        auto maxEval = [&](
            const auto* const NOALIAS  Q,
            const int normal
        ){ """
                    + self.compute_gamma()
                    + """
            const auto j = &Q[Shortcuts::j];
            const auto E = Q[Shortcuts::E];
            const auto rho_full = Q[Shortcuts::rho] +Q[Shortcuts::backgroundstate+0];
            const auto height = Q[Shortcuts::height];
            const auto jDot = j[0]*j[0]+j[1]*j[1]
            #if DIMENSIONS == 3
                + j[2]*j[2]
            #endif
                ;

            auto pressurePertubation = (gamma - 1)*(E - 0.5*(jDot / rho_full));
            auto pressure = pressurePertubation + Q[Shortcuts::backgroundstate+1];
            const auto u_n = Q[Shortcuts::j+normal]/rho_full;

                        const auto radicand = gamma * (pressure/rho_full);
            if (not (radicand > 0.0)) {
                ::tarch::triggerNonCriticalAssertion(
                    __FILE__, __LINE__,
                    "gamma * pressure / rho > 0",
                    "radicand=" + std::to_string(radicand)
                        + ", pressure=" + std::to_string(pressure)
                        + ", rho="      + std::to_string(rho_full)
                        + ", E="        + std::to_string(E)
                        + ", height="   + std::to_string(height)
                        + ", normal="   + std::to_string(normal)
                );
            }
            
            const auto c = std::sqrt(gamma * (pressure/rho_full));
            double lambda = std::fmax(
            std::abs(u_n-c), std::abs(u_n+c)
            );
            return lambda;
        };



        
        [[maybe_unused]] auto diffusiveEval = [&](
            const auto* const NOALIAS  Q,
            const int normal
        ){ """
                    + self.compute_gamma()
                    + """
            auto rho_full = Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0];
            const auto nu = (mu_molecular / rho_full) + eddy_viscosity; 
            double smaxDiffusive = molecularDiffusionCoeff;
            smaxDiffusive = std::fmax(smaxDiffusive, (4./3.) * nu);
            return std::fmax(smaxDiffusive, (gamma * nu * 1.5));

        };
        """
        )

    
    
    def eigenvalues(self):
        return (
            self.eigenvalue_functions()
            + """
            return maxEval(Q, normal) + diffusiveEval(Q, normal)*2/(h[normal]*PNPM);
            """
        )

    def riemann_solver(self):
        return (
            """
            constexpr int strideQ = NumberOfUnknowns + NumberOfAuxiliaryVariables;
            {{CORRECTOR_COMPUTATION_PRECISION}} QavL[strideQ] __attribute__((aligned(16))) = {0};
            {{CORRECTOR_COMPUTATION_PRECISION}} QavR[strideQ] __attribute__((aligned(16))) = {0};

            #if DIMENSIONS==3
                constexpr int faceSize = (DGOrder+1)*(DGOrder+1);
            #else
                constexpr int faceSize = (DGOrder+1);
            #endif

            for (int xy = 0; xy < faceSize; xy++) {
                for (int n = 0; n < strideQ; n++) {
                QavL[n] += kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace[xy] * QL[xy * strideQ + n];
                QavR[n] += kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace[xy] * QR[xy * strideQ + n];                }
            } 

            """
                        + self.eigenvalue_functions()
                        + """

            // Compute max eigenvalue from teh convective flux
            auto smax_c = std::fmax(
                maxEval(&QavL[0], direction), 
                maxEval(&QavR[0], direction)
            );

            //compute the max eigenvalue from the diffusive flux 
            auto smax_d = std::fmax(
                diffusiveEval(&QavL[0], direction),
                diffusiveEval(&QavR[0], direction)
            ); 


            auto eta = (2*DGOrder+1) / (h[direction]* std::sqrt(0.5*M_PI)); 

            auto smax = smax_c + 2*eta*smax_d; 

            for (int xy = 0; xy < faceSize; xy++) {
                for (int n = 0; n < NumberOfUnknowns; n++) {
                    FL[xy*NumberOfUnknowns+n] = 0.5 * (FL[xy * NumberOfUnknowns + n] + FR[xy * NumberOfUnknowns + n]) + 0.5*smax*(QL[xy*strideQ + n] - QR[xy*strideQ + n]);  
                }
            }

            std::copy_n(FL, faceSize*NumberOfUnknowns, FR);
            """
        )


    def fv_riemann_solver(self):
        """Well-balanced Rusanov flux for the subcell limiter.

        The finite-volume subgrid sees one state per side, not a face of
        quadrature points, so the discontinuous-Galerkin body above does not fit
        its signature. The point that carries over is the dissipation: it has to
        act on the deviation from the hydrostatic background, because the plain
        jump QL - QR is dominated by the stratification and would drive the
        subgrid all by itself. Only the momentum, whose background is zero, uses
        the full jump.
        """
        return (
            self.eigenvalue_functions()
            + """

            const auto smax = std::fmax(maxEval(QL, normal), maxEval(QR, normal));

            double fluxL[NumberOfUnknowns];
            double fluxR[NumberOfUnknowns];
            flux(QL, x, h, t, dt, normal, fluxL);
            flux(QR, x, h, t, dt, normal, fluxR);

            // The background is a per-volume auxiliary variable, so a common
            // background cancels out of these differences and leaves the plain
            // Rusanov term on the pertubation.
            const auto rho_jump = QL[Shortcuts::rho] - QR[Shortcuts::rho];
            const auto E_jump   = QL[Shortcuts::E] - QR[Shortcuts::E];

            FL[Shortcuts::rho] = 0.5 * (fluxL[Shortcuts::rho] + fluxR[Shortcuts::rho])
                               + 0.5 * smax * rho_jump;
            FL[Shortcuts::E]   = 0.5 * (fluxL[Shortcuts::E] + fluxR[Shortcuts::E])
                               + 0.5 * smax * E_jump;
            for (int d = 0; d < DIMENSIONS; d++) {
              FL[Shortcuts::j + d] = 0.5 * (fluxL[Shortcuts::j + d] + fluxR[Shortcuts::j + d])
                                   + 0.5 * smax * (QL[Shortcuts::j + d] - QR[Shortcuts::j + d]);
            }

            std::copy_n(FL, NumberOfUnknowns, FR);
            return smax;
            """
        )


    def source(self):

        return (
            """
            std::fill_n(S, NumberOfUnknowns, 0);

            constexpr double gravitySurface = 9.782231107143115;   // [m/s^2]
            constexpr double effectiveEarthRadius = 6336531.01897801; // [m]
            const double ratio = effectiveEarthRadius / (effectiveEarthRadius + x(DIMENSIONS-1));
            const double gravity = gravitySurface * ratio * ratio;
            S[Shortcuts::j+DIMENSIONS-1] = -1 * Q[Shortcuts::rho] * gravity;
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1];


            """
                    )


    
    def source_with_TS_no_gravity(self, omega, k_x, x_zero, z_zero, t_zero, F_zero, sigma_x, sigma_z, sigma_t, time_offset=0.0):
        """
        time_offset shifts the thunderstorm forcing, which is the only
        explicitly time-dependent term, along the time axis. A run restarted
        from a snapshot at t=T starts its own clock at zero, so it has to pass
        time_offset=T to keep the forcing where it belongs.
        """

        return (
            """

            static constexpr double timeOffset = """ + str(time_offset) + """;
            const double simulationTime = t + timeOffset;

            std::fill_n(S, NumberOfUnknowns, 0);

 

            // frequency
            static constexpr double omega =""" + str(omega) + """;   // in rad/s
            static constexpr double xZero = """ + str(x_zero) + """; // in m
            static constexpr double zZero =""" + str(z_zero) + """;  // in m
            static constexpr double tZero = """ + str(t_zero) + """;   // in s

            static constexpr double kX = """ + str(k_x) + """;; // in rad/m

            static constexpr double sigmaXSq = """ + str(sigma_x) + """ * """ + str(sigma_x) + """; // in m
            static constexpr double sigmaZSq = """ + str(sigma_z) + """ * """ + str(sigma_z) + """;  // in m
            static constexpr double sigmaTSq = """ + str(sigma_t) + """ * """ + str(sigma_t) + """;       // in s

            static constexpr double FZero =""" + str(F_zero) + """;
            const double exponentialXTerm = tarch::la::pow((x(0) - xZero), 2) / (2 * sigmaXSq);
            const double exponentialYTerm = tarch::la::pow((x(DIMENSIONS-1) - zZero), 2) / (2 * sigmaZSq);
            const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
            //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);

            const double cosTerm = std::cos(kX * (x(0)))
                     * std::cos(omega * (simulationTime));

            // 0: rho, 2: rhov
            S[Shortcuts::j + DIMENSIONS-1 ] += (Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0]) * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
                * cosTerm;

            
                

            // ---- Rayleigh sponge: absorbs waves at the top --------------------
            // Relaxes the evolved perturbation back to zero (= the hydrostatic
            // background). Identically zero at equilibrium, so it preserves the
            // well-balanced state. Make it several vertical wavelengths thick and
            // ramp it smoothly, or the sponge edge reflects too.
            constexpr double spongeMaxRate = 0.3;   // [1/s] peak relaxation rate (tune)
            constexpr double spongeTopFrac = 0.25;   // top fraction of the domain height

            const double zTop         = DomainOffset[DIMENSIONS-1] + DomainSize[DIMENSIONS-1];
            const double topThickness = spongeTopFrac * DomainSize[DIMENSIONS-1];
            const double zSpongeStart = zTop - topThickness;

            double sigma = 0.0;
            if (x(DIMENSIONS-1) > zSpongeStart) {
                const double s = (x(DIMENSIONS-1) - zSpongeStart) / topThickness;  // 0 .. 1
                sigma = spongeMaxRate * std::pow(std::sin(0.5 * M_PI * s), 2);       // smooth 0 -> max
            }

            if (sigma > 0.0) {
                S[Shortcuts::rho] -= sigma * Q[Shortcuts::rho];
                for (int d = 0; d < DIMENSIONS; d++) S[Shortcuts::j+d] -= sigma * Q[Shortcuts::j+d];
                S[Shortcuts::E]   -= sigma * Q[Shortcuts::E];
            }


            """
                    )
    
    def source_with_TS(self, omega, k_x, x_zero, z_zero, t_zero, F_zero, sigma_x, sigma_z, sigma_t, time_offset=0.0, ramp_time=0.0):
        """
        time_offset shifts the thunderstorm forcing, which is the only
        explicitly time-dependent term, along the time axis. A run restarted
        from a snapshot at t=T starts its own clock at zero, so it has to pass
        time_offset=T to keep the forcing where it belongs.

        ramp_time > 0 switches the forcing on smoothly with sin^2 over
        [0, ramp_time]. With t_0 = 600 s and sigma_t = 2000 s the Gaussian
        envelope is already 0.96 at t = 0, so without a ramp the forcing jumps
        on and radiates a broadband (acoustic) transient ahead of the wave.
        """

        return (
            """
            // Time of the restart snapshot this run continues from; zero for a
            // run that starts from the analytical initial condition.
            static constexpr double timeOffset = """ + str(time_offset) + """;
            const double simulationTime = t + timeOffset;

            std::fill_n(S, NumberOfUnknowns, 0);

            constexpr double gravitySurface = 9.782231107143115;   // [m/s^2]
            constexpr double effectiveEarthRadius = 6336531.01897801; // [m]
            const double ratio = effectiveEarthRadius / (effectiveEarthRadius + x(DIMENSIONS-1));
            const double gravity = gravitySurface * ratio * ratio;            S[Shortcuts::j+DIMENSIONS-1] = -1 * Q[Shortcuts::rho] * gravity;
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1];


            //thunderstorm source


            // frequency
            static constexpr double omega =""" + str(omega) + """;   // in rad/s
            static constexpr double xZero = """ + str(x_zero) + """; // in m
            static constexpr double zZero =""" + str(z_zero) + """;  // in m
            static constexpr double tZero = """ + str(t_zero) + """;   // in s

            static constexpr double kX = """ + str(k_x) + """;; // in rad/m

            static constexpr double sigmaXSq = """ + str(sigma_x) + """ * """ + str(sigma_x) + """; // in m
            static constexpr double sigmaZSq = """ + str(sigma_z) + """ * """ + str(sigma_z) + """;  // in m
            static constexpr double sigmaTSq = """ + str(sigma_t) + """ * """ + str(sigma_t) + """;       // in s

            static constexpr double FZero =""" + str(F_zero) + """;
            const double exponentialXTerm = tarch::la::pow((x(0) - xZero), 2) / (2 * sigmaXSq);
            const double exponentialYTerm = tarch::la::pow((x(DIMENSIONS-1) - zZero), 2) / (2 * sigmaZSq);
            const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
            //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);

            const double cosTerm = std::cos(kX * (x(0)))
                     * std::cos(omega * (simulationTime));

            // smooth switch-on, 1 for ramp_time = 0
            static constexpr double rampTime = """ + str(ramp_time) + """; // in s
            double ramp = 1.0;
            if (simulationTime < rampTime) {
                const double r = std::sin(0.5 * M_PI * simulationTime / rampTime);
                ramp = r * r;
            }

            // 0: rho, 2: rhov
            S[Shortcuts::j + DIMENSIONS-1 ] += (Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0]) * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
                * cosTerm * ramp;


            """
                    )




    def source_with_TS_3D(self, omega, k_x, x_zero, z_zero, t_zero, F_zero, sigma_x, sigma_z, sigma_t, time_offset=0.0):


        return (
            """

            // Using 

            static constexpr double timeOffset = """ + str(time_offset) + """;
            const double simulationTime = t + timeOffset;

            std::fill_n(S, NumberOfUnknowns, 0);

            constexpr double gravitySurface = 9.782231107143115;   // [m/s^2]
            constexpr double effectiveEarthRadius = 6336531.01897801; // [m]
            const double ratio = effectiveEarthRadius / (effectiveEarthRadius + x(DIMENSIONS-1));
            const double gravity = gravitySurface * ratio * ratio;            S[Shortcuts::j+DIMENSIONS-1] = -1 * Q[Shortcuts::rho] * gravity;
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1];


            //thunderstorm source


            // frequency
            static constexpr double omega =""" + str(omega) + """;   // in rad/s
            static constexpr double xZero = """ + str(x_zero) + """; // in m
            static constexpr double zZero =""" + str(z_zero) + """;  // in m
            static constexpr double tZero = """ + str(t_zero) + """;   // in s

            static constexpr double kX = """ + str(k_x) + """;; // in rad/m

            static constexpr double sigmaXSq = """ + str(sigma_x) + """ * """ + str(sigma_x) + """; // in m
            static constexpr double sigmaZSq = """ + str(sigma_z) + """ * """ + str(sigma_z) + """;  // in m
            static constexpr double sigmaTSq = """ + str(sigma_t) + """ * """ + str(sigma_t) + """;       // in s

            static constexpr double FZero =""" + str(F_zero) + """;
            const double exponentialXTerm = tarch::la::pow((x(0) - xZero), 2) / (2 * sigmaXSq);
            const double exponentialYTerm = tarch::la::pow((x(1) - xZero), 2) / (2 * sigmaXSq);
            const double exponentialZTerm = tarch::la::pow((x(DIMENSIONS-1) - zZero), 2) / (2 * sigmaZSq);
            const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
            //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);

            const double cosTerm = std::cos(kX * (x(0))) * std::cos(kX * (x(1)))
                         * std::cos(omega * (simulationTime));



            // 0: rho, 2: rhov
            S[Shortcuts::j + DIMENSIONS-1 ] += (Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0]) * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialZTerm - exponentialTTerm)
                * cosTerm;


            """
                    )


        

    

    def map_observerables(self): 

        return (""" 
                const auto rhoFull = Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate+0];
                observables[0] = std::sqrt(rhoFull / 1.16) *( Q[Shortcuts::j + DIMENSIONS-1] / rhoFull);
                
        """)

    def physical_admissibility_criterion_true(self):
        return (""" 
        
        const double hlim =
        DomainOffset[DIMENSIONS-1] + 0.70 * DomainSize[DIMENSIONS-1];
        if (x[DIMENSIONS-1] > hlim) {
            return false;   // inadmissible -> recomputed with FV
        }
        
        return true;""" )




    def physical_admissibility_criterion(self):

        return("""
        // Static limiting test: force the top half of the domain onto the FV
        // subgrid, regardless of the solution. x is the cell/volume centre.
        const double domainMidHeight =
        DomainOffset[DIMENSIONS-1] + 0.7 * DomainSize[DIMENSIONS-1];
        if (x[DIMENSIONS-1] > domainMidHeight) {
            return false;   // inadmissible -> recomputed with FV
        }
        
        
        //return true;



        [[maybe_unused]] const double z = x[DIMENSIONS-1];
        const double t = limiterTime;
        [[maybe_unused]] const double limiterheight = t*55; 

         if (z > limiterheight) return false;



        return true;
        """)


    def modal_smoothness_admissibility(self, order, number_of_data, threshold=0.08,
                                       amplitude_floor=1e-10, static_fv_from=0.7):
        """Flags DG cells whose normalized vertical velocity carries grid-scale wiggles.

        Persson-Peraire type indicator, 2D only: along every line of Gauss-Legendre
        nodes, in x and in z, the coefficient of the highest Legendre mode P_N is
        computed. share = sqrt(energy in P_N / total energy) of the worse direction.
        A well-resolved wave has share ~1e-3, the wiggles ahead of the front 0.1-0.3,
        FV data projected back to DG ~0.05, hence the threshold 0.08.

        amplitude_floor (rms of the observable) keeps cells that hold nothing but a
        tiny tail from being flagged. It scales with the forcing amplitude F_0. The top static_fv_from fraction of the domain
        stays on the FV subgrid as before. Subcell and single-state checks only see
        the static layer, the indicator needs the DG candidate.
        """
        import numpy as np

        n = order + 1
        xi, wq = np.polynomial.legendre.leggauss(n)
        top_mode_weight = (2 * order + 1) / 2 * wq * np.polynomial.legendre.Legendre.basis(order)(xi)
        top_mode_norm = 2 / (2 * order + 1)

        def cpp_array(values):
            return "{" + ", ".join(repr(float(v)) for v in values) + "}"

        return ("""
        if (x[DIMENSIONS-1] > DomainOffset[DIMENSIONS-1] + """ + str(static_fv_from) + """ * DomainSize[DIMENSIONS-1]) {
            return false;
        }

        constexpr int n      = """ + str(n) + """;
        constexpr int stride = """ + str(number_of_data) + """;
        if (numberOfStates != n * n) {
            return true;
        }

        // Gauss-Legendre weights on [-1,1], and the row that maps nodal values
        // to the coefficient of P_N
        constexpr double weight[n]        = """ + cpp_array(wq) + """;
        constexpr double topModeWeight[n] = """ + cpp_array(top_mode_weight) + """;
        constexpr double topModeNorm      = """ + repr(top_mode_norm) + """;

        double u[n * n];
        for (int k = 0; k < n * n; k++) {
            const double* const q   = Q + k * stride;
            const double rhoFull    = q[Shortcuts::rho] + q[Shortcuts::backgroundstate + 0];
            u[k] = std::sqrt(rhoFull / 1.16) * q[Shortcuts::j + DIMENSIONS-1] / rhoFull;
        }

        double total = 0.0, topFast = 0.0, topSlow = 0.0;
        for (int a = 0; a < n; a++) {
            double coeffFast = 0.0, coeffSlow = 0.0;
            for (int b = 0; b < n; b++) {
                coeffFast += topModeWeight[b] * u[a * n + b];
                coeffSlow += topModeWeight[b] * u[b * n + a];
                total     += weight[a] * weight[b] * u[a * n + b] * u[a * n + b];
            }
            topFast += weight[a] * coeffFast * coeffFast;
            topSlow += weight[a] * coeffSlow * coeffSlow;
        }

        // total / 4 is the mean square over the reference cell
        constexpr double floor = """ + repr(float(amplitude_floor)) + """;
        if (total / 4.0 < floor * floor) {
            return true;
        }

        const double share = std::sqrt(std::fmax(topFast, topSlow) * topModeNorm / total);
        return share < """ + repr(float(threshold)) + """;
        """)



    CSV_CONSTANTS = """
    // CSV constants
    static const char csvSeparator            = ';';
    static const int csvHeightIndex           = 5;
    static const int csvAtomicOxygenIndex     = 8;
    static const int csvNitrogenIndex         = 9;
    static const int csvMolecularOxygenIndex  = 10;
    static const int csvDensityIndex          = 11;
    static const int csvTemperatureIndex      = 12;
    static const int csvHydrogenIndex         = 16;
  """

    FLUID_PROPERTIES = """
    static constexpr double mu_molecular         = 1e-5;
    static constexpr double eddy_viscosity         = 100;
  """