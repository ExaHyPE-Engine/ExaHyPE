



class Atmosphere():

    _reference_pressure = 100000.0
    _background_potential_temperature = 300.0
    _cv = 1 / (1.4 - 1) * 287.058
    _reference_viscosity=0.1

    VISCOSITY_MODEL = """
        // Altitude-dependent kinematic viscosity (m^2/s)
        static double kinematicViscosity(double height) {
            if (height < 110000.0) {
                return 100.0;
            } else {
                // exponential through (110000, 100) and (220000, 60000)
                return 0.16666666666666666 * std::exp(5.8153905956510424e-05 * height);
            }
        }
    """

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
                const auto cv = gasConstant / (gamma -1);
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

            auto slopeBetweenRows = [](const std::string bottomRow, const std::string topRow, const int csvIndex) {
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
            // const int rowIndex = tarch::la::round(x(1) / (csvMaxHeight) * (csvNumberOfRows - 1));
            const int rowIndex = static_cast<int>(std::floor(x(1) / (csvMaxHeight) * (csvNumberOfRows - 1)));
            
            // extract row
            const std::string row = csvReader.getRow(rowIndex);

            // if not last row, interpolate between rows for stratification in Euler solver
            const bool interpolate = rowIndex < csvNumberOfRows - 1;

            if (interpolate) {
                const std::string nextRow = csvReader.getRow(rowIndex + 1);

                Q[Shortcuts::rho] = 1000.0 * interpolateBetweenRows(row, nextRow, csvDensityIndex, x(1));
                assertion(!std::isnan(Q[Shortcuts::rho]), Q[Shortcuts::rho]);
                assertion(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
        
                // populate major species
                // all number densities retreived from csv are cm^-3
                // all production rates are also in cm^-3 s^-1 
                Q[Shortcuts::O]   = interpolateBetweenRows(row, nextRow, csvAtomicOxygenIndex, x(1));
                Q[Shortcuts::O2]  = interpolateBetweenRows(row, nextRow, csvMolecularOxygenIndex, x(1));
                Q[Shortcuts::N2]  = interpolateBetweenRows(row, nextRow, csvNitrogenIndex, x(1));
                // temperature is in Kelvin
                Q[Shortcuts::T]   = interpolateBetweenRows(row, nextRow, csvTemperatureIndex, x(1));
            




            } 
            
            else {

                // density is in g/cm^3, needs conversion into kg/m^3 = 1000 * g/cm^3
                Q[Shortcuts::rho] = 1000.0 * tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvDensityIndex, row); 
                assertion(!std::isnan(Q[Shortcuts::rho]), Q[Shortcuts::rho]);
                assertion(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
            
                // populate major species
                // all number densities retreived from csv are cm^-3
                // all production rates are also in cm^-3 s^-1 
                Q[Shortcuts::O2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvMolecularOxygenIndex, row);
                Q[Shortcuts::O]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvAtomicOxygenIndex, row);
                Q[Shortcuts::N2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvNitrogenIndex, row);

                // temperature is in Kelvin
                Q[Shortcuts::T]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvTemperatureIndex, row);
                


            }

            
        //zero velocity field 
        Q[Shortcuts::j]   = 0;
        Q[Shortcuts::j+1]   = 0;
        //set initial Energy density
        constexpr auto gravity = 9.81;

        """ 
        + self.compute_gamma()
        + """
        const auto meanMolarMass = (Q[Shortcuts::O]  * 0.016
                           + Q[Shortcuts::O2] * 0.032
                           + Q[Shortcuts::N2] * 0.028) / totalNumberDensity;   

        constexpr auto universalGasConstant = 8.314;  
        const auto gasConstant = universalGasConstant / meanMolarMass;   
        const auto rho = Q[Shortcuts::rho]; 

        const auto pressure = rho * gasConstant * Q[Shortcuts::T];



        auto j = &Q[Shortcuts::j];
        const auto j_squared = j[0]*j[0]+j[1]*j[1]; 

        Q[Shortcuts::E] =( pressure / (gamma -1)) + 0.5*j_squared*(1/rho) ; 

        Q[Shortcuts::height] = x(1); 
        Q[Shortcuts::backgroundstate + 0 ] = rho; 
        Q[Shortcuts::backgroundstate + 1 ] = pressure; 
        Q[Shortcuts::backgroundstate + 2 ] = Q[Shortcuts::E]; 
        Q[Shortcuts::viscosity] = kinematicViscosity(x(1));




        """ )

    def initial_conditions_from_file(self, path, step=-1):
        """
        Warm-start initial condition: samples every Q component (the four
        unknowns and all auxiliary variables) from the point nearest to x in a
        previous run's patch output, instead of deriving it from the CSV
        background profile.

        path is that run's "<name>.output/patch/ADERDGSolver.vtkhdf" if it was
        built with HDF5 support and "<name>.output/patch/ADERDGSolver.pvd" if it
        fell back to VTU, step the index of the snapshot to continue from (-1 is
        the last one, -2 the last but one, ...). See tarch::reader::VTKSampler,
        which reads either: the points the nodal patch plotter writes are the
        ADER-DG scheme's own quadrature points, i.e. exactly where the initial
        condition is evaluated, so an unchanged mesh and order reproduce the
        stored state one to one.

        The finite volume solver of the limiting setup reads the same file. Its
        volume centres are not the ADER-DG nodes, so it picks up the nearest
        nodal value - a piecewise-constant projection of the same state.

        Mind that the restarted run's clock starts at zero again: shift the
        thunderstorm forcing by the snapshot's time stamp, which the log line of
        the sampler reports, via source_with_TS(time_offset=...).
        """
        return ("""

            static tarch::reader::VTKSampler restartSampler(
                \"""" + path + """\",
                {"rho", "j", "E", "height", "backgroundstate", "O", "O2", "N2", "T", "viscosity"},
                """ + str(step) + """
            );

            // One search per degree of freedom, not one per component.
            const int restartPoint = restartSampler.getClosestPoint(
              x(0), x(1)
            #if DIMENSIONS == 3
              , x(2)
            #endif
            );

            Q[Shortcuts::rho] = restartSampler.getValue("rho", 0, restartPoint);
            Q[Shortcuts::j+0] = restartSampler.getValue("j", 0, restartPoint);
            Q[Shortcuts::j+1] = restartSampler.getValue("j", 1, restartPoint);
            Q[Shortcuts::E]   = restartSampler.getValue("E", 0, restartPoint);

            Q[Shortcuts::height] = restartSampler.getValue("height", 0, restartPoint);
            Q[Shortcuts::backgroundstate + 0] = restartSampler.getValue("backgroundstate", 0, restartPoint);
            Q[Shortcuts::backgroundstate + 1] = restartSampler.getValue("backgroundstate", 1, restartPoint);
            Q[Shortcuts::backgroundstate + 2] = restartSampler.getValue("backgroundstate", 2, restartPoint);

            Q[Shortcuts::O]   = restartSampler.getValue("O", 0, restartPoint);
            Q[Shortcuts::O2]  = restartSampler.getValue("O2", 0, restartPoint);
            Q[Shortcuts::N2]  = restartSampler.getValue("N2", 0, restartPoint);
            Q[Shortcuts::T]   = restartSampler.getValue("T", 0, restartPoint);
            Q[Shortcuts::viscosity] = restartSampler.getValue("viscosity", 0, restartPoint);

        """ )

    def record_troubled_marker(self, index, limiter_name="LimitingSolver"):
        """
        Postprocessing kernel that writes the limiter's verdict into the solution.

        Pass the result to the ADER-DG solver's DoFWisePostprocessSolution action
        set and the plotted state carries one more field, so ParaView shows which
        cells the finite volume solver ran in without any post-hoc inference:

            0  REGULAR             the ADER-DG solution was kept
            1  REGULAR_TO_LIMITER  flagged this step, handed to the limiter
            2  LIMITER_TO_REGULAR  neighbour of a troubled cell, so also limited
            3  TROUBLED            the limiter recomputed this cell

        index is the position of the "troubled" auxiliary variable in Q. The
        action set writes into the observer, where the solver's Shortcuts alias
        is not in scope, hence the raw index - let the setup compute it from its
        unknowns and auxiliary variables rather than hard-coding a number.
        """
        return ("""
      // A cell label is per cell, the auxiliary variable per degree of freedom,
      // so every degree of freedom of the cell gets the same verdict.
      values[""" + str(index) + """] = static_cast<double>(
        fineGridCell""" + limiter_name + """CellLabel.getTroubledMarker()
      );
        """)


    def record_pressure(self, index):
        """
        Postprocessing kernel writing p into an auxiliary variable for plotting.

        Raw indices throughout: the snippet lands in the observer, where the
        solver's Shortcuts alias is not in scope.
        """
        return ("""
        const double totalNumberDensity = values[8] + values[9] + values[10];
        const double gammaLocal =
            (1.4 * (totalNumberDensity - values[8]) + 1.67 * values[8]) / totalNumberDensity;
        const double jDotLocal  = values[1] * values[1] + values[2] * values[2];
        values[""" + str(index) + """] =
            (gammaLocal - 1.0) * (values[3] - 0.5 * jDotLocal / values[0]);
        """)


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


            if (isTop || isSide){   //try alternatively copy from ins

                 Qoutside[Shortcuts::j+0] = 0;
                 Qoutside[Shortcuts::j+1] = 0;
                 Qoutside[Shortcuts::rho] = Qinside[Shortcuts::backgroundstate + 0];
                 Qoutside[Shortcuts::E] = Qinside[Shortcuts::backgroundstate + 2]; ;

             }


            //if (isTop || isSide ){
            
            //    const auto jo0 = Qoutside[Shortcuts::j+0]; 
            //    const auto jo1 = Qoutside[Shortcuts::j+1];
            //    Qoutside[Shortcuts::E] = Qinside[Shortcuts::backgroundstate + 2]
            //               + 0.5 * (jo0*jo0 + jo1*jo1) / Qoutside[Shortcuts::rho];
            //            }



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



    def boundary_conditions_crash_replica(self):
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

            if (isTop || isSide) {
                // Three characteristics leave, one enters: one condition, and the
                // hydrostatic pressure is the natural choice. rho and j are already
                // extrapolated by the copy_n above.
                const auto jo0 = Qoutside[Shortcuts::j+0];
                const auto jo1 = Qoutside[Shortcuts::j+1];
                Qoutside[Shortcuts::E] = Qinside[Shortcuts::backgroundstate + 2]
                                    + 0.5 * (jo0*jo0 + jo1*jo1) / Qoutside[Shortcuts::rho];

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
            self.compute_cv()
            + """ 
            const auto rho = Q[Shortcuts::rho];
            const auto j = &Q[Shortcuts::j];
            
            const auto E = Q[Shortcuts::E];
            const auto height = Q[Shortcuts::height];
            // const auto backgroundPressure = Q[Shortcuts::backgroundstate+1];
            
            //here
            const auto backgroundPressure = (gamma - 1) * Q[Shortcuts::backgroundstate + 2];

            constexpr auto gravity = 9.81;
            const auto invRho = 1./rho; """
                        + self.evaluate_pressure()
                        )

        # part of the flux which corresponds to the Euler equations
        flux_euler = """
            F[Shortcuts::rho] = j[normal];
            F[Shortcuts::j+0] = invRho * j[normal] * j[0];
            F[Shortcuts::j+1] = invRho * j[normal] * j[1];
            #if DIMENSIONS==3
            F[Shortcuts::j+2] = invRho * j[normal] * j[2];
            #endif
            F[Shortcuts::E] = invRho * j[normal] * (E+pressure);

            F[Shortcuts::j+normal] += (pressure-backgroundPressure);
            """
    


        return (
            flux_init
            + flux_euler

        )

    

    def eigenvalue_functions(self):
        return (
        """

        constexpr auto gravity = 9.81;
        constexpr auto q0 = 0;
        constexpr auto Pr = 0.71; //Prandtl number
        constexpr auto molecularDiffusionCoeff = 0;

        auto maxEval = [&](
            const auto* const NOALIAS  Q,
            const int normal
        ){ """
                    + self.compute_gamma()
                    + """
            const auto j = &Q[Shortcuts::j];
            const auto E = Q[Shortcuts::E];
            const auto rho = Q[Shortcuts::rho];
            const auto height = Q[Shortcuts::height];
            const auto Z = 0; """
                    + self.evaluate_pressure()
                    + """
            const auto u_n = Q[Shortcuts::j+normal]/rho;

                        const auto radicand = gamma * (pressure/rho);
            if (not (radicand > 0.0)) {
                ::tarch::triggerNonCriticalAssertion(
                    __FILE__, __LINE__,
                    "gamma * pressure / rho > 0",
                    "radicand=" + std::to_string(radicand)
                        + ", pressure=" + std::to_string(pressure)
                        + ", rho="      + std::to_string(rho)
                        + ", E="        + std::to_string(E)
                        + ", height="   + std::to_string(height)
                        + ", normal="   + std::to_string(normal)
                );
            }
            
            const auto c = std::sqrt(gamma * (pressure/rho));
            double lambda = std::fmax(
            std::abs(u_n-c), std::abs(u_n+c)
            );
            return lambda;
        };



        auto diffusiveEval = [&](
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
                constexpr int faceSize = (Order+1)*(Order+1);
            #else
                constexpr int faceSize = (Order+1);
            #endif

            for (int xy = 0; xy < faceSize; xy++) {
                for (int n = 0; n < strideQ; n++) {
                QavL[n] += kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::weights2[xy] * QL[xy * strideQ + n];
                QavR[n] += kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::weights2[xy] * QR[xy * strideQ + n];
                }
            } 

            """
                        + self.eigenvalue_functions()
                        + """

            // Compute max eigenvalue over face
            auto smax = std::fmax(
                maxEval(&QavL[0], direction), 
                maxEval(&QavR[0], direction)
            );


            //     for (int xy = 0; xy < faceSize; xy++) {
            //     for (int n = 0; n < NumberOfUnknowns; n++) {
            //     FL[xy*NumberOfUnknowns+n] = 0.5 * (FL[xy * NumberOfUnknowns + n] + FR[xy * NumberOfUnknowns + n] 
            //         + smax * (QL[xy * strideQ + n] - QR[xy * strideQ + n]) );
            //     }
            // }

            for (int xy = 0; xy < faceSize; xy++) {
                for (int n = 0; n < NumberOfUnknowns; n++) {
                    FL[xy*NumberOfUnknowns+n] = 0.5 * (FL[xy * NumberOfUnknowns + n] + FR[xy * NumberOfUnknowns + n]);  
                }


                const auto rhoL_pertubation = QL[xy*strideQ + Shortcuts::rho] - QL[xy*strideQ + Shortcuts::backgroundstate + 0];
                const auto rhoR_pertubation = QR[xy*strideQ + Shortcuts::rho] - QR[xy*strideQ + Shortcuts::backgroundstate + 0];
                FL[xy*NumberOfUnknowns] += 0.5*smax*( (rhoL_pertubation - rhoR_pertubation) ); //setting rho pertubations
                
                //background momentum is zero
                FL[xy*NumberOfUnknowns + 1] += 0.5*smax*(QL[xy * strideQ + 1] - QR[xy * strideQ + 1]); 
                FL[xy*NumberOfUnknowns + 2] += 0.5*smax*(QL[xy * strideQ + 2] - QR[xy * strideQ + 2]); 

                //petubations in energy 
                const auto EL_pertubation = QL[xy*strideQ + Shortcuts::E] - QL[xy*strideQ + Shortcuts::backgroundstate + 2];
                const auto ER_pertubation = QR[xy*strideQ + Shortcuts::E] - QR[xy*strideQ + Shortcuts::backgroundstate + 2];
                FL[xy*NumberOfUnknowns + 3] += 0.5*smax*( (EL_pertubation - ER_pertubation) ); 
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
            const auto rho_jump = (QL[Shortcuts::rho] - QL[Shortcuts::backgroundstate + 0])
                                - (QR[Shortcuts::rho] - QR[Shortcuts::backgroundstate + 0]);
            const auto E_jump   = (QL[Shortcuts::E]   - QL[Shortcuts::backgroundstate + 2])
                                - (QR[Shortcuts::E]   - QR[Shortcuts::backgroundstate + 2]);

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

            auto rhoPertubation = Q[Shortcuts::rho];
            auto backgroundRho  = Q[Shortcuts::backgroundstate+0];
            rhoPertubation      -= backgroundRho;
            constexpr auto gravity = 9.81;
            //vertical direction is either y or z
            S[Shortcuts::j+DIMENSIONS-1] = -1 * rhoPertubation * gravity;
            
            
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1]; 

            """
                    )

    def thunderstorm_source(self, time_offset): 
        return (
                    """
                    // Time of the restart snapshot this run continues from; zero for a
                    // run that starts from the analytical initial condition.
                    static constexpr double timeOffset = """ + str(time_offset) + """;
                    const double simulationTime = t + timeOffset;
        
                    std::fill_n(S, NumberOfUnknowns, 0);
        
                    //thunderstorm source
        
        
                    // frequency
                    static constexpr double omega = 0.011;   // in rad/s
                    static constexpr double xZero = 450'000; // in m
                    static constexpr double zZero = 12'000;  // in m
                    static constexpr double tZero = 2'000;   // in s
        
                    static constexpr double kX = 0.00011; // in rad/m
        
                    static constexpr double sigmaXSq = 30'000 * 30'000; // in m
                    static constexpr double sigmaZSq = 4'000 * 4'000;   // in m
                    static constexpr double sigmaTSq = 600 * 600;       // in s
        
                    static constexpr double vZero = 0.33;          // forcing velocity amplitude [m/s]
                    static constexpr double FZero = vZero * omega; // [m/s^2] -> max induced v_z = vZero
                    const double exponentialXTerm = tarch::la::pow((x(0) - xZero), 2) / (2 * sigmaXSq);
                    const double exponentialYTerm = tarch::la::pow((x(1) - zZero), 2) / (2 * sigmaZSq);
                    const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
                    //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);
        
                    const double cosTerm = std::cos(kX * (x(0) - xZero))
                             * std::cos(omega * (simulationTime - tZero));
        
                    // 0: rho, 2: rhov
                    S[Shortcuts::j +1 ] += Q[Shortcuts::rho] * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
                        * cosTerm;
        
        
        
        
                    """
                            )

    
    def source_with_TS(self, time_offset=0.0):
        """
        time_offset shifts the thunderstorm forcing, which is the only
        explicitly time-dependent term, along the time axis. A run restarted
        from a snapshot at t=T starts its own clock at zero, so it has to pass
        time_offset=T to keep the forcing where it belongs.
        """

        return (
            """
            // Time of the restart snapshot this run continues from; zero for a
            // run that starts from the analytical initial condition.
            static constexpr double timeOffset = """ + str(time_offset) + """;
            const double simulationTime = t + timeOffset;

            std::fill_n(S, NumberOfUnknowns, 0);

            auto rhoPertubation = Q[Shortcuts::rho];
            auto backgroundRho  = Q[Shortcuts::backgroundstate+0];
            rhoPertubation      -= backgroundRho;
            constexpr auto gravity = 9.81;
            //vertical direction is either y or z
            S[Shortcuts::j+DIMENSIONS-1] = -1 * rhoPertubation * gravity;
            
            
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1]; 

            //thunderstorm source


            // frequency
            static constexpr double omega = 0.011;   // in rad/s
            static constexpr double xZero = 450'000; // in m
            static constexpr double zZero = 12'000;  // in m
            static constexpr double tZero = 2'000;   // in s

            static constexpr double kX = 0.00011; // in rad/m

            static constexpr double sigmaXSq = 30'000 * 30'000; // in m
            static constexpr double sigmaZSq = 4'000 * 4'000;   // in m
            static constexpr double sigmaTSq = 600 * 600;       // in s

            static constexpr double vZero = 0.33;          // forcing velocity amplitude [m/s]
            static constexpr double FZero = vZero * omega; // [m/s^2] -> max induced v_z = vZero
            const double exponentialXTerm = tarch::la::pow((x(0) - xZero), 2) / (2 * sigmaXSq);
            const double exponentialYTerm = tarch::la::pow((x(1) - zZero), 2) / (2 * sigmaZSq);
            const double exponentialTTerm = tarch::la::pow((simulationTime - tZero), 2) / (2 * sigmaTSq);
            //const double cosTerm          = std::cos(kX * x(0)) * std::cos(omega * simulationTime);

            const double cosTerm = std::cos(kX * (x(0) - xZero))
                     * std::cos(omega * (simulationTime - tZero));

            // 0: rho, 2: rhov
            S[Shortcuts::j +1 ] += Q[Shortcuts::rho] * FZero * tarch::la::pow(tarch::la::E, -exponentialXTerm - exponentialYTerm - exponentialTTerm)
                * cosTerm;




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



    def physical_admissibility_criterion(self):

        return("""


          constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;

            // Q holds numberOfStates consecutive states: every node of a candidate
            // polynomial, or a single state when one finite-volume value is checked.
            const double* state = Q;
            for (int stateNumber = 0; stateNumber < numberOfStates; ++stateNumber, state += NumberOfData) {

                // A negative radicand in the sound speed turns the eigenvalue into a
                // NaN. A NaN is not caufght by the discrete maximum principle, so it
                // would poison the global time step size instead of flagging the
                // cell as troubled. Reject non-finite states explicitly.

                for (int unknown = 0; unknown < NumberOfData; ++unknown) {
                    if (not std::isfinite(state[unknown])) {
                        return false;
                    }
                }


                if (state[0] <= 0.0 or state[3] <= 0.0) { // density and energy
                return false;
                }

                const double totalNumberDensity = state[8] + state[9] + state[10];
                const double gamma = (1.4 * (totalNumberDensity - state[8]) + 1.67 * state[8]) / totalNumberDensity;
                const double jDot = state[1] * state[1] + state[2] * state[2];
                const double pressure = (gamma - 1.0) * (state[3] - 0.5 * jDot / state[0]);

                if (!std::isfinite(pressure) or pressure <= 0.0) {
                return false;
                }

            }
            return true;
        """)

    # def physical_admissibility_criterion(self):
    # # Q is a single DG node of the cell, so this indexes point values.
    # # The limiter class has no VariableShortcuts in scope, hence raw indices:
    # # rho=0, j=1..2, E=3, height=4, backgroundstate=5..7, O=8, O2=9, N2=10.
    #     return("""

    #         return true;
    #     """)


    def map_observerables(self): 

        return (""" 
        // Observable 0: density perturbation w.r.t. the hydrostatic background,
        // in decades. Unlike (rho/bg - 1) this does not saturate at -1, so a
        // collapse well below background still registers proportionally.
        constexpr double RatioFloor = 1e-12;   // reads as -12; finite, so extrema stay usable
        const double densityRatio = Q[Shortcuts::rho] / Q[Shortcuts::backgroundstate];
        observables[0] = std::log10(std::fmax(densityRatio, RatioFloor));

        """)



    def refinement_criterion(self, refine_threshold=50.0, erase_threshold=30.0):

        return (f"""

            // Q is the whole DG polynomial of the cell, i.e. (Order+1)^DIMENSIONS
            // consecutive nodes, and nullptr while the grid is still constructed.
            // The vertical velocity is therefore reduced over all nodes: a wave
            // entering through any node has to refine the cell, and erasing is
            // only allowed once every node has fallen quiet.
            if (Q == nullptr) return ::exahype2::RefinementCommand::Keep;

            constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
            constexpr int NodesPerAxis = Order + 1;
            constexpr int NodesPerCell = DIMENSIONS == 2
              ? NodesPerAxis * NodesPerAxis
              : NodesPerAxis * NodesPerAxis * NodesPerAxis;
            constexpr double DensityFloor = 1.0e-12; // keeps the division finite

            double maxHorizontalVelocity = 0.0;
            const double* node = Q;
            for (int nodeNumber = 0; nodeNumber < NodesPerCell; ++nodeNumber, node += NumberOfData) {{
                const double density = std::fmax(node[Shortcuts::rho], DensityFloor);
                maxHorizontalVelocity  = std::fmax(
                  maxHorizontalVelocity, std::abs(node[Shortcuts::j] / density)
                );
            }}

            if (maxHorizontalVelocity > {refine_threshold}) return ::exahype2::RefinementCommand::Refine;
            if (maxHorizontalVelocity < {erase_threshold}) return ::exahype2::RefinementCommand::Erase;
            return ::exahype2::RefinementCommand::Keep;

        """)