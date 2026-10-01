



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

            //get density and temperature from csv file 

            auto interpolateBetweenRows = [](const std::string bottomRow, const std::string topRow, const int csvIndex, const double currentHeight) {
                const auto bottomRowValue = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, bottomRow);
                const auto topRowValue    = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, topRow);
                const auto topRowHeight   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, topRow) * 1000.0;
                const auto bottomRowHeight   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, bottomRow) * 1000.0;

                return bottomRowValue + (currentHeight - bottomRowHeight) * (topRowValue - bottomRowValue) / (topRowHeight - bottomRowHeight);
            };

             
            [[maybe_unused]] auto slopeBetweenRows = [](const std::string bottomRow, const std::string topRow, const int csvIndex) {
                const auto bottomRowValue  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, bottomRow);
                const auto topRowValue     = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, topRow);
                const auto bottomRowHeight = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, bottomRow) * 1000.0;
                const auto topRowHeight    = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, topRow) * 1000.0;
                return (topRowValue - bottomRowValue) / (topRowHeight - bottomRowHeight);
            };

            
            std::fill_n(Q, Shortcuts::NumberOfDistinctVariables, 0.0);

            // initialize CSV reader and skip first line of file, which contains column names
            static tarch::reader::CSVReader csvReader("./thunderstorm_model.csv", 1);
            
  

            const int csvNumberOfRows = csvReader.getNumberOfRows();

            // height in CSV is in km, so convert to m
            const double csvMaxHeight = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, csvReader.getRow(csvNumberOfRows - 1)) * 1000.0;
            
            // height of x is in m
            const int lastRowIndex = csvNumberOfRows - 1;
            const int rowIndex = static_cast<int>(std::floor(x(1) / (csvMaxHeight) * (csvNumberOfRows - 1)));

            // Two regimes for the vertical coordinate x(1):
            //  - inside the table (rowIndex < lastRowIndex): interpolate between the two bracketing rows.
            //  - at or above the top of the table (x(1) >= csvMaxHeight): the CSV stops at csvMaxHeight,
            //    so there is no data. Hold every quantity constant at its top-row (220 km) value.
            const bool holdConstant = rowIndex >= lastRowIndex;

            if (holdConstant) {
                const std::string row = csvReader.getRow(lastRowIndex);

                // density is in g/cm^3, needs conversion into kg/m^3 = 1000 * g/cm^3
                Q[Shortcuts::backgroundstate + 0] = 1000.0 * tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvDensityIndex, row);
                assertion(!std::isnan(Q[Shortcuts::backgroundstate + 0]), Q[Shortcuts::backgroundstate + 0]);
                assertion(Q[Shortcuts::backgroundstate + 0] > 0, Q[Shortcuts::backgroundstate + 0]);

                // populate major species
                // all number densities retreived from csv are cm^-3
                Q[Shortcuts::O]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvAtomicOxygenIndex, row);
                Q[Shortcuts::O2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvMolecularOxygenIndex, row);
                Q[Shortcuts::N2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvNitrogenIndex, row);
                // temperature is in Kelvin
                Q[Shortcuts::T]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvTemperatureIndex, row);
            }
            else {
                const std::string row     = csvReader.getRow(rowIndex);
                const std::string nextRow = csvReader.getRow(rowIndex + 1);

                Q[Shortcuts::backgroundstate + 0 ] = 1000.0 * interpolateBetweenRows(row, nextRow, csvDensityIndex, x(1));
                assertion(!std::isnan(Q[Shortcuts::backgroundstate + 0]), Q[Shortcuts::backgroundstate + 0]);
                assertion(Q[Shortcuts::backgroundstate + 0] > 0, Q[Shortcuts::backgroundstate + 0]);

                // populate major species
                // all number densities retreived from csv are cm^-3
                // all production rates are also in cm^-3 s^-1
                Q[Shortcuts::O]   = interpolateBetweenRows(row, nextRow, csvAtomicOxygenIndex, x(1));
                Q[Shortcuts::O2]  = interpolateBetweenRows(row, nextRow, csvMolecularOxygenIndex, x(1));
                Q[Shortcuts::N2]  = interpolateBetweenRows(row, nextRow, csvNitrogenIndex, x(1));
                // temperature is in Kelvin
                Q[Shortcuts::T]   = interpolateBetweenRows(row, nextRow, csvTemperatureIndex, x(1));
            }

        //zero denisty and E pertubations

        Q[Shortcuts::rho]   = 0;
        Q[Shortcuts::E]   = 0;
            
        //zero velocity field 
        Q[Shortcuts::j]   = 0;
        Q[Shortcuts::j+1]   = 0;
        //set initial Energy density

        """ 
        + self.compute_gamma()
        + """
        const auto meanMolarMass = (Q[Shortcuts::O]  * 0.016
                           + Q[Shortcuts::O2] * 0.032
                           + Q[Shortcuts::N2] * 0.028) / totalNumberDensity;   

        constexpr auto universalGasConstant = 8.314;  
        const auto gasConstant = universalGasConstant / meanMolarMass;   
        //const auto rho = Q[Shortcuts::rho]; 

        const auto pressure = Q[Shortcuts::backgroundstate + 0 ] * gasConstant * Q[Shortcuts::T];

        Q[Shortcuts::height] = x(1); 
        Q[Shortcuts::backgroundstate + 1 ] = pressure; 
        Q[Shortcuts::backgroundstate + 2 ] = ( pressure / (gamma -1)) ; 



        """ )


    def boundary_conditions(self):
        return (
            """
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == 1;
            const bool isTop = x[DIMENSIONS-1] > domainMidHeight && normal == 1;

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
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == 1;
            const bool isTop = x[DIMENSIONS-1] > domainMidHeight && normal == 1;

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
                const auto jo0 = Qoutside[Shortcuts::j+0];
                const auto jo1 = Qoutside[Shortcuts::j+1];
                Qoutside[Shortcuts::E] = Qinside[Shortcuts::backgroundstate + 2]
                                    + 0.5 * (jo0*jo0 + jo1*jo1) / Qoutside[Shortcuts::rho];
            } 
            
            if (isInflow){
                Qoutside[Shortcuts::j+0] = 0.0;
                Qoutside[Shortcuts::j+1] = 0.0;
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
                    Qoutside[Shortcuts::j+0] = 0.0;
                    Qoutside[Shortcuts::j+1] = 0.0;
                    Qoutside[Shortcuts::E]   = 0.0;
                } else {
                    // outflow: extrapolate rho', j' (already copied), pin p' = 0
                    // p' = (gamma-1)(E' - 0.5*jDot/rhoFull) = 0  ->  E' = 0.5*jDot/rhoFull
                    const auto rhoFull = Qoutside[Shortcuts::rho] + Qoutside[Shortcuts::backgroundstate+0];
                    const auto jo0 = Qoutside[Shortcuts::j+0];
                    const auto jo1 = Qoutside[Shortcuts::j+1];
                    Qoutside[Shortcuts::E] = 0.5 * (jo0*jo0 + jo1*jo1) / rhoFull;
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
            const auto jDot = j[0]*j[0]+j[1]*j[1];


            const auto backgroundPressure = Q[Shortcuts::backgroundstate + 1];
            const auto backgroundE = Q[Shortcuts::backgroundstate + 2];

            const auto invRho = 1./rho_full; """
                        )

        # part of the flux which corresponds to the Euler equations
        flux_euler = """
            F[Shortcuts::rho] = j[normal];

            F[Shortcuts::j+0] = invRho * j[normal] * j[0];
            F[Shortcuts::j+1] = invRho * j[normal] * j[1];

            auto pressurePertubation = (gamma - 1)*(E - 0.5* (jDot / (rho_full)));
            F[Shortcuts::j+normal] += pressurePertubation;

            auto pressure = pressurePertubation + backgroundPressure; 
            auto Efull = E + backgroundE;
            F[Shortcuts::E] = invRho * j[normal] * (Efull + pressure);



            """
    


        return (
            flux_init
            + flux_euler

        )

    

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
            const auto jDot = j[0]*j[0]+j[1]*j[1];

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

            const auto nu = kinematicViscosity(Q[Shortcuts::height]);
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
            return maxEval(Q, normal);
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

            // Compute max eigenvalue over face
            auto smax = std::fmax(
                maxEval(&QavL[0], direction), 
                maxEval(&QavR[0], direction)
            );



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

            constexpr auto gravity = 9.81;
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
            const double exponentialYTerm = tarch::la::pow((x(1) - zZero), 2) / (2 * sigmaZSq);
            const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
            //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);

            const double cosTerm = std::cos(kX * (x(0)))
                     * std::cos(omega * (simulationTime));

            // 0: rho, 2: rhov
            S[Shortcuts::j +1 ] += (Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0]) * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
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
                S[Shortcuts::j+0] -= sigma * Q[Shortcuts::j+0];
                S[Shortcuts::j+1] -= sigma * Q[Shortcuts::j+1];
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

            constexpr auto gravity = 9.81;
            S[Shortcuts::j+DIMENSIONS-1] = -1 * Q[Shortcuts::rho] * gravity;
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
            const double exponentialYTerm = tarch::la::pow((x(1) - zZero), 2) / (2 * sigmaZSq);
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
            S[Shortcuts::j +1 ] += (Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0]) * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
                * cosTerm * ramp;




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
                S[Shortcuts::j+0] -= sigma * Q[Shortcuts::j+0];
                S[Shortcuts::j+1] -= sigma * Q[Shortcuts::j+1];
                S[Shortcuts::E]   -= sigma * Q[Shortcuts::E];
            }


            """
                    )

    # def source(self):
    #     return (
    #     """
    #     std::fill_n(S, NumberOfUnknowns, 0);

    #     constexpr auto gravity = 9.81;

    #     // ---- gravity (well-balanced splitting) ----------------------------
    #     const auto rhoPertubation = Q[Shortcuts::rho] - Q[Shortcuts::backgroundstate+0];
    #     S[Shortcuts::j+DIMENSIONS-1] = -1 * rhoPertubation * gravity;
    #     S[Shortcuts::E]              = -gravity * Q[Shortcuts::j + DIMENSIONS-1];

    #     // ---- Rayleigh damping sponge --------------------------------------
    #     // Relaxes the state toward the stored background near the outer
    #     // boundaries. Exactly zero at equilibrium, so it does not disturb
    #     // the well-balanced steady state.
    #     constexpr auto spongeRate     = 0.01;   // [1/s] peak relaxation rate
    #     constexpr auto spongeTopFrac  = 0.20;   // fraction of domain height
    #     constexpr auto spongeSideFrac = 0.10;   // fraction of width, each side

    #     const auto xMin = DomainOffset[0];
    #     const auto xMax = DomainOffset[0] + DomainSize[0];
    #     const auto zMax = DomainOffset[DIMENSIONS-1] + DomainSize[DIMENSIONS-1];

    #     const auto topThickness  = spongeTopFrac  * DomainSize[DIMENSIONS-1];
    #     const auto sideThickness = spongeSideFrac * DomainSize[0];

    #     // s in [0,1] inside the layer, 0 outside; s*s is C1 at the inner edge
    #     auto ramp = [](const double s) {
    #         return (s <= 0.0) ? 0.0 : std::fmin(1.0, s * s);
    #     };

    #     double sigma = 0.0;
    #     sigma = std::fmax(sigma, ramp((x(DIMENSIONS-1) - (zMax - topThickness)) / topThickness));
    #     sigma = std::fmax(sigma, ramp(((xMin + sideThickness) - x(0)) / sideThickness));
    #     sigma = std::fmax(sigma, ramp((x(0) - (xMax - sideThickness)) / sideThickness));
    #     sigma *= spongeRate;

    #     //currently remove damping
    #     sigma = 0; 

    #     if (sigma > 0.0) {
    #         S[Shortcuts::rho] -= sigma * (Q[Shortcuts::rho] - Q[Shortcuts::backgroundstate+0]);
    #         S[Shortcuts::j+0] -= sigma *  Q[Shortcuts::j+0];   // background momentum is zero
    #         S[Shortcuts::j+1] -= sigma *  Q[Shortcuts::j+1];
    #         S[Shortcuts::E]   -= sigma * (Q[Shortcuts::E]   - Q[Shortcuts::backgroundstate+2]);
    #     }
    #     """
    # )


    def map_observerables(self): 

        return (""" 
                const auto rhoFull = Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate+0];
                observables[0] = std::sqrt(rhoFull / 1.16) *( Q[Shortcuts::j +1] / rhoFull);
                
        """)





    def physical_admissibility_criterion(self):

        return("""
        // Static limiting test: force the top half of the domain onto the FV
        // subgrid, regardless of the solution. x is the cell/volume centre.
        const double domainMidHeight =
            DomainOffset[DIMENSIONS-1] + 0.7 * DomainSize[DIMENSIONS-1];
        if (x[DIMENSIONS-1] > domainMidHeight) {
            return false;   // inadmissible -> recomputed with FV
        }
        return true;
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
