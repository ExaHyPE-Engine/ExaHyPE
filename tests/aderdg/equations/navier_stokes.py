# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .equation import Equation

from exahype2.solvers.PDETerms import PDETerms


class NavierStokes(Equation):
    def __init__(
        self,
        dimensions,
        use_advection=True,
        use_background_state=True,
        use_gravity=True,
        use_viscosity=True,
        gamma=1.4,
        cv=1.0,
        gas_constant=287.058,
        reference_viscosity=0.1,
        Pr=0.71,
        molecular_diffusion_coeff=0.0,
        q0=0.0,
    ):
        self.dimensions = dimensions

        self.num_unknowns = {
            "rho": 1,
            "j": dimensions,
            "E": 1,
        }
        if use_advection:
            self.num_unknowns["Z"] = 1

        self.num_auxiliary_variables = {
            key: value
            for key, value in {
                "height": 1 if use_gravity else 0,
                "backgroundstate": 2 if use_background_state else 0,
            }.items()
            if value > 0
        }

        self._use_advection = use_advection
        self._use_background_state = use_background_state
        self._use_gravity = use_gravity
        self._use_viscosity = use_viscosity

        self._gamma = gamma  # ratio of specific heats
        self._cv = cv  # heat capacity at constant volume
        self._gas_constant = gas_constant  # specific gas constant of a fluid
        self._reference_viscosity = reference_viscosity
        self._Pr = Pr  # Prandtl number
        self._molecular_diffusion_coeff = molecular_diffusion_coeff
        self._q0 = q0  # q0 not explained in the old version, merely marked with ???

        self.use_diffusive_flux = True

    def evaluate_pressure(self):
        return (
            """
    auto chemicalEnergy = q0 * Z;
    """
            + (
                """
    // add gravity Potential to the chemical energy for simplicity, 
    // but it is technically a separate term
    chemicalEnergy += rho * gravity * height;
"""
                if self._use_gravity
                else ""
            )
            + """

    const auto jDot = j[0]*j[0]+j[1]*j[1]
#if DIMENSIONS==3
      + j[2]*j[2]
#endif
    ;

    const auto pressure = (gamma-1) * (E - 0.5 * (1.0/rho) * jDot - chemicalEnergy);
"""
        )

    def evaluate_energy(self):
        return (
            """
    auto chemicalEnergy = q0 * Z;
    """
            + (
                """
    // add gravity Potential to the chemical energy for simplicity, 
    // but it is technically a separate term
    chemicalEnergy += rho * gravity * height;
"""
                if self._use_gravity
                else ""
            )
            + """

    const auto jDot = j[0]*j[0]+j[1]*j[1]
#if DIMENSIONS==3
      + j[2]*j[2]
#endif
    ;
    const auto E = pressure/(gamma - 1) + 0.5 * (jDot/rho) + chemicalEnergy;
"""
        )

    # these get used both here and in the Riemann solver, but need
    # different factors in their sum (i.e lambda + a * lambda_diff)
    # in an effort to minimize code duplication we therefore define
    # both of these once and just refer to them
    def eigenvalue_functions(self):
        return (
            """
  constexpr auto gamma = """
            + str(self._gamma)
            + """;
  constexpr auto gravity = 9.81;
  constexpr auto q0 = """
            + str(self._q0)
            + """;
  constexpr auto referenceViscosity = """
            + str(self._reference_viscosity)
            + """;
  constexpr auto Pr = """
            + str(self._Pr)
            + """; //Prandtl number
  constexpr auto molecularDiffusionCoeff = """
            + str(self._molecular_diffusion_coeff)
            + """;

  auto maxEval = [&](
    const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q,
    const int normal
  ){
    const auto j = &Q[Shortcuts::j];
    const auto E = Q[Shortcuts::E];
    const auto rho = Q[Shortcuts::rho];
    const auto height = """
            + ("Q[Shortcuts::height]" if self._use_gravity else "0.0")
            + """;
    const auto Z = """
            + ("Q[Shortcuts::Z];" if self._use_advection else "0.0;")
            + self.evaluate_pressure()
            + """
    const auto u_n = Q[Shortcuts::j+normal]/rho;
    const auto c = std::sqrt(gamma * (pressure/rho));
    double lambda = std::fmax(
      std::abs(u_n-c), std::abs(u_n+c)
    );
    return lambda;
  };

  auto diffusiveEval = [&](
    const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS  Q,
    const int normal
  ){
    double smaxDiffusive = molecularDiffusionCoeff;
    smaxDiffusive = std::fmax(
      smaxDiffusive, (4./3.) * (referenceViscosity/Q[Shortcuts::rho])
    );
    return std::fmax(
      smaxDiffusive, (gamma * referenceViscosity) / (Pr * Q[Shortcuts::rho])
    );

  };
"""
        )

    def eigenvalues(self):
        return (
            self.eigenvalue_functions()
            + """
    return maxEval(Q, normal) 
      + diffusiveEval(Q, normal)*2.0/(h[normal]*PNPM);
"""
        )

    def flux(self):
        # define parameters that will be used in the flux
        flux_init = (
            """  const auto rho = Q[Shortcuts::rho];
  const auto j = &Q[Shortcuts::j];
  const auto E = Q[Shortcuts::E];
  const auto Z = """
            + ("Q[Shortcuts::Z]" if self._use_advection else "0.0")
            + """;
  const auto height = """
            + ("Q[Shortcuts::height]" if self._use_gravity else "0.0")
            + """;
  const auto backgroundPressure = """
            + (
                "Q[Shortcuts::backgroundstate+1]"
                if self._use_background_state
                else "0.0"
            )
            + """;

  constexpr auto gravity = 9.81;
  constexpr auto gamma = """
            + str(self._gamma)
            + """;
  constexpr auto gasConstant = """
            + str(self._gas_constant)
            + """;
  constexpr auto cv = """
            + str(self._cv)
            + """;
  constexpr auto q0 = """
            + str(self._q0)
            + """;
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
        flux_viscous = (
            """
  // TODO Support different visc. models?
  constexpr auto referenceViscosity = """
            + str(self._reference_viscosity)
            + """;
  constexpr auto mu = referenceViscosity;
  constexpr auto Pr = """
            + str(self._Pr)
            + """; //Prandtl number
  constexpr auto kappa = 1./Pr * referenceViscosity * gamma * cv;

  const auto uu   = invRho * Q[Shortcuts::j];
  const auto uux  = invRho * (deltaQ[0][Shortcuts::j+0] - uu * deltaQ[0][Shortcuts::rho]);
  const auto uuy  = invRho * (deltaQ[1][Shortcuts::j+0] - uu * deltaQ[1][Shortcuts::rho]);

  const auto vv   = invRho * Q[Shortcuts::j+1];
  const auto vvx  = invRho * (deltaQ[0][Shortcuts::j+1] - vv * deltaQ[0][Shortcuts::rho]);
  const auto vvy  = invRho * (deltaQ[1][Shortcuts::j+1] - vv * deltaQ[1][Shortcuts::rho]);

#if DIMENSIONS == 2
  const auto divV23 = 2./3. * (uux + vvy);
#else
  const auto ww   = invRho * Q[Shortcuts::j+2];
  const auto uuz  = invRho * (deltaQ[2][Shortcuts::j+0] - uu * deltaQ[2][Shortcuts::rho]);
  const auto vvz  = invRho * (deltaQ[2][Shortcuts::j+1] - vv * deltaQ[2][Shortcuts::rho]);
  const auto wwx  = invRho * (deltaQ[2][Shortcuts::j+2] - ww * deltaQ[0][Shortcuts::rho]);
  const auto wwy  = invRho * (deltaQ[2][Shortcuts::j+2] - ww * deltaQ[1][Shortcuts::rho]);
  const auto wwz  = invRho * (deltaQ[2][Shortcuts::j+2] - ww * deltaQ[2][Shortcuts::rho]);
  const auto divV23 = 2./3. * (uux + vvy + wwz);
#endif

  double Fj[DIMENSIONS];
  switch(normal){
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
"""
        )
        flux_heat = (
            """
  const auto invCv = 1/cv;
  const auto invRho2 = invRho * invRho;
  const auto invRho3 = invRho2 * invRho;

#if DIMENSIONS == 2
  const auto dTdW1 = -1 * Q[Shortcuts::E] * invRho2 +
          invRho3 * (Q[Shortcuts::j]*Q[Shortcuts::j] + Q[Shortcuts::j+1] * Q[Shortcuts::j+1]);
#else
  const auto dTdW1 = -1 * Q[Shortcuts::E] * invRho2 +
          invRho3 * (Q[Shortcuts::j]*Q[Shortcuts::j] + Q[Shortcuts::j+1] * Q[Shortcuts::j+1] + Q[Shortcuts::j+2] * Q[Shortcuts::j+2]);
#endif
  const auto dTdW2 = -1 * Q[Shortcuts::j] * invRho2;
  const auto dTdW3 = -1 * Q[Shortcuts::j+1] * invRho2;
#if DIMENSIONS == 3
  const auto dTdW4 = -1 * Q[Shortcuts::j+2] * invRho2;
#endif

  auto T = invCv * (dTdW1 * deltaQ[normal][Shortcuts::rho] +
            dTdW2 * deltaQ[normal][Shortcuts::j+0] +
            dTdW3 * deltaQ[normal][Shortcuts::j+1] +
#if DIMENSIONS == 3
            dTdW4 * deltaQ[normal][Shortcuts::j+2] +
#endif
            invRho * deltaQ[normal][Shortcuts::E]); 
            """
            + (
                """
  // Including the gravitational potential in the pressure
  // implies that we need to change the derivative of the temperature as well,
  // but only in z direction
  // Gravity happens in y direction in 2D and in z direction in 3D
  if(normal==DIMENSIONS-1){
    T -= (gravity * (gamma - 1)) / gasConstant;
  } """
                if self._use_gravity
                else ""
            )
            + """
  // Heat flux
  F[Shortcuts::E] -= kappa * T;
"""
        )

        flux_advection = (
            """
  // Euler flux
  F[Shortcuts::Z] = j[normal] * Z * invRho;
  // Contribution to the Heat flux
  const auto factor = invCv * q0 * invRho2;
  T = -factor *
          (Q[Shortcuts::rho] * deltaQ[normal][Shortcuts::Z] -
            Q[Shortcuts::Z] * deltaQ[normal][Shortcuts::rho]);
  F[Shortcuts::E] -= kappa * T;

  // Molecular diffusion for Advection-Reaction-Diffusion part.
  constexpr auto molecularDiffusionCoeff = """
            + str(self._molecular_diffusion_coeff)
            + """;
  F[Shortcuts::rho] -= molecularDiffusionCoeff * deltaQ[normal][Shortcuts::rho];
  F[Shorcuts::Z] -= molecularDiffusionCoeff * deltaQ[normal][Shortcuts::Z];
"""
        )

        return (
            flux_init
            + flux_euler
            + (flux_viscous if self._use_viscosity else "")
            + flux_heat
            + (flux_advection if self._use_advection else "")
        )

    def source(self):
        # If both terms are 0, no need for a source
        if not (self._use_gravity or self._use_background_state):
            return PDETerms.None_Implementation
        return (
            """
  std::fill_n(S, NumberOfUnknowns, 0.0);

  auto rhoPertubation = Q[Shortcuts::rho];
"""
            + (
                """
  auto backgroundRho  = Q[Shortcuts::backgroundstate+0];
  rhoPertubation      -= backgroundRho;
"""
                if self._use_background_state
                else ""
            )
            + """
  constexpr auto gravity = 9.81;
  //vertical direction is either y or z
  S[Shortcuts::j+DIMENSIONS-1] = -1 * rhoPertubation * gravity;
"""
            + ("" if self._use_gravity else "")
        )

    def riemann_solver(self):
        return (
            """
  constexpr int strideQ = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  {{CORRECTOR_COMPUTATION_PRECISION}} QavL[strideQ] __attribute__((aligned(16))) = {0.0};
  {{CORRECTOR_COMPUTATION_PRECISION}} QavR[strideQ] __attribute__((aligned(16))) = {0.0};

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

  // Add viscous eigenvalue term to max eigenvalue
  const auto factor = 2 * (2 * Order + 1) / (h[direction] * std::sqrt(0.5 * std::numbers::pi));
  smax += factor*std::fmax(
    diffusiveEval(&QavL[0], direction), diffusiveEval(&QavR[0], direction)
  );

  for (int xy = 0; xy < faceSize; xy++) {
    for (int n = 0; n < NumberOfUnknowns; n++) {
      FL[xy*NumberOfUnknowns+n] = 0.5 * (FL[xy * NumberOfUnknowns + n] + FR[xy * NumberOfUnknowns + n] 
        + smax * (QL[xy * strideQ + n] - QR[xy * strideQ + n]) );
    }
  }
  std::copy_n(FL, faceSize*NumberOfUnknowns, FR);
"""
        )
