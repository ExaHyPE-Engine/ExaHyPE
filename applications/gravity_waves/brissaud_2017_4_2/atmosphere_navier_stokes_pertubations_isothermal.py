



class Atmosphere():



    def initial_conditions(self):
        return ("""

            // constant scale height 
                
                auto scale_height_value = std::exp(-x(DIMENSIONS-1)/scale_height);
                
                //perturbations as unknowns
                Q[Shortcuts::rho] = 0; 
                Q[Shortcuts::j] = 0; 
                Q[Shortcuts::j+1] = 0; 
                Q[Shortcuts::E] = 0; 
                

                //background
                Q[Shortcuts::backgroundstate + 0] = rho_0*scale_height_value; 
                Q[Shortcuts::backgroundstate + 1] = p_0*scale_height_value; 
                Q[Shortcuts::backgroundstate + 2] = E_0*scale_height_value;

                //gradients (since heat flux is zero as kappa = 0, only rho_background gradient needed)
                Q[Shortcuts::gradients + 0] = -(rho_0/scale_height)*scale_height_value; 

                Q[Shortcuts::height] = x(1); 


                    """ )


    def boundary_conditions(self):
        return (
            """


            //source at bottom
            //set the outside values in such a way that at the bottom there is p0, rho0, E0 and the given velocity
            //remembering to only set the perturbations
    

            //copy inside state to outside (TODO: check if this causes reflections)
            std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);
            
            
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == DIMENSIONS-1;

            auto exp_coeff1 = -1 * std::pow( (t-(t_0 - PT_div_4))/PT_div_4, 2) ;
            auto exp_coeff2 = -1 * std::pow( (t-(t_0 + PT_div_4))/PT_div_4, 2);
            auto vertical_velocity = 0.01*((-2/(PT_div_4*PT_div_4))*(t - (t_0-PT_div_4))*std::exp(exp_coeff1) - (-2/(PT_div_4*PT_div_4))*(t - (t_0+PT_div_4))*std::exp(exp_coeff2));
    
            if (isBottom){
                Qoutside[Shortcuts::j+1] = 2*(Qinside[Shortcuts::rho] + Qinside[Shortcuts::backgroundstate + 0])* vertical_velocity - Qinside[Shortcuts::j + 1]; 


                //setting gradients to ground base gradients 
                Qoutside[Shortcuts::gradients + 0] = -rho_0 / scale_height; 
            
            }         
        
            """
        )


    # def boundary_conditions(self):
    #     return (
    #         """
    #         const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
    #         const bool isBottom = x[DIMENSIONS-1] < domainMidHeight && normal == DIMENSIONS-1;


    #         //source at bottom
    #         //set the outside values in such a way that at the bottom there is p0, rho0, E0 and the given velocity
    #         //remembering to only set the perturbations
    
    #         auto exp_coeff1 = -1 * std::pow( (t-(t_0 - PT_div_4))/PT_div_4, 2);
    #         auto exp_coeff2 = -1 * std::pow( (t-(t_0 + PT_div_4))/PT_div_4, 2);
    #         auto vertical_velocity = 0.01*(std::exp(exp_coeff1) - std::exp(exp_coeff2));

    #         //copy inside state to outside (TODO: check if this causes reflections)
    #         std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);
            
            

    #         if (isBottom){
    #             Qoutside[Shortcuts::j+1] = (Qinside[Shortcuts::rho] + Qinside[Shortcuts::backgroundstate + 0])* vertical_velocity; 


    #             //setting gradients to ground base gradients 
    #             Qoutside[Shortcuts::gradients + 0] = -rho_0 / scale_height; 
            
    #         }         

    #         """
    #     )

    
    
    
    def source(self):
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
            std::fill_n(S, NumberOfUnknowns, 0);

            S[Shortcuts::j+DIMENSIONS-1] = -1 * Q[Shortcuts::rho] * gravity;
            S[Shortcuts::E] = -gravity * Q[Shortcuts::j + DIMENSIONS-1];
            
            
            //include a sponge layer at the top

            auto sponge_start = 0.7*domain_height;
            constexpr double spongeSigmaMax = 0.2;     // [1/s] damping rate at the very top

            const double zz = x(DIMENSIONS-1);
            if (zz > sponge_start){
                const double xi    = (zz - sponge_start) / (DomainOffset[DIMENSIONS-1] + DomainSize[DIMENSIONS-1] - sponge_start);
                const double sigma = spongeSigmaMax * xi * xi;       // smooth quadratic ramp

                S[Shortcuts::rho] -= sigma * Q[Shortcuts::rho];
                S[Shortcuts::E]   -= sigma * Q[Shortcuts::E];
                for (int d = 0; d < DIMENSIONS; d++) S[Shortcuts::j + d] -= sigma * Q[Shortcuts::j + d];
            }
            
            """
            )



    def flux(self):
        # define parameters that will be usechd in the flux
        flux_init = (
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

            const auto invRho = 1/rho_full; 
            
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
            //auto mu = nu * invRho ; 
            
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
           



        return (
            flux_init
            + flux_euler
            + flux_viscous

        )

    
    def eigenvalue_functions(self):
        return (
        """

        constexpr auto molecularDiffusionCoeff = 0;

        auto maxEval = [&](
            const auto* const NOALIAS  Q,
            const int normal
        ){ 
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
        ){ 
            auto rho_full = Q[Shortcuts::rho] + Q[Shortcuts::backgroundstate + 0];
            double smaxDiffusive = molecularDiffusionCoeff;
            auto nu = mu / rho_full;
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


            /*
            //check if the interface is the bottom boundary --> at bottom we want to prescribe a certain flux 
            const auto domainMidHeight = DomainOffset[DIMENSIONS-1] + 0.5 * DomainSize[DIMENSIONS-1];
            
            const bool isBottom = isBoundaryFace
                   && direction == DIMENSIONS-1
                   && x[DIMENSIONS-1] < domainMidHeight;

            
            if (!isBottom) { 
                
            */

            """
                        + self.eigenvalue_functions()
                        + """

            // Compute max eigenvalue from the convective flux
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

            //}

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




