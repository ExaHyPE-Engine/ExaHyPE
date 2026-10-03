# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .equation import Equation


class NavierStokes(Equation):
    def __init__(
        self,
        dimensions,
        use_advection=True,
        use_background_state=True,
        use_gravity=True,
        gamma=1.4,
        cv=1.0,
        gas_constant=287.058,
        reference_viscosity=0.1,
        prandtl_number=0.71,
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

        self._gamma = gamma  # Ratio of specific heats
        self._cv = cv  # Heat capacity at constant volume
        self._gas_constant = gas_constant  # Specific gas constant of a fluid
        self._reference_viscosity = reference_viscosity
        self._prandtl_number = prandtl_number
        self._molecular_diffusion_coeff = molecular_diffusion_coeff
        self._q0 = q0

        self.use_diffusive_flux = True

    def evaluate_pressure(self):
        return (
            """
    auto chemicalEnergy = HeatRelease * Z;
    """
            + (
                """
    chemicalEnergy += rho * Gravity * height;
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

    const auto pressure = (AdiabaticIndex-1) * (E - 0.5 * (1/rho) * jDot - chemicalEnergy);
"""
        )

    def evaluate_energy(self):
        return (
            """
    auto chemicalEnergy = HeatRelease * Z;
    """
            + (
                """
    chemicalEnergy += rho * Gravity * height;
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
    const auto E = pressure/(AdiabaticIndex - 1) + 0.5 * (jDot/rho) + chemicalEnergy;
"""
        )

    def eigenvalue_functions(self):
        return (
            """
  constexpr auto AdiabaticIndex = """
            + str(self._gamma)
            + """;"""
            + (
                """
  constexpr auto Gravity = 9.81;"""
                if self._use_gravity
                else ""
            )
            + """
  constexpr auto HeatRelease = """
            + str(self._q0)
            + """;
  constexpr auto ReferenceViscosity = """
            + str(self._reference_viscosity)
            + """;
  constexpr auto PrandtlNumber = """
            + str(self._prandtl_number)
            + """;
  constexpr auto MolecularDiffusionCoefficient = """
            + str(self._molecular_diffusion_coeff)
            + """;

  auto maxEval = [&](
    const auto* const NOALIAS Q,
    const int normal
  ){
    const auto* const j = &Q[Shortcuts::j];
    const auto E = Q[Shortcuts::E];
    const auto rho = Q[Shortcuts::rho];"""
            + (
                """
    const auto height = Q[Shortcuts::height];"""
                if self._use_gravity
                else ""
            )
            + """
    const auto Z = """
            + ("Q[Shortcuts::Z];" if self._use_advection else "0;")
            + self.evaluate_pressure()
            + """
    const auto normalVelocity = Q[Shortcuts::j+normal]/rho;
    const auto c = std::sqrt(AdiabaticIndex * (pressure/rho));
    const double lambda = std::fmax(
      std::abs(normalVelocity-c), std::abs(normalVelocity+c)
    );
    return lambda;
  };

  auto diffusiveEval = [&](
    const auto* const NOALIAS Q,
    [[maybe_unused]] const int normal
  ){
    double smaxDiffusive = MolecularDiffusionCoefficient;
    smaxDiffusive = std::fmax(
      smaxDiffusive, (4./3.) * (ReferenceViscosity/Q[Shortcuts::rho])
    );
    return std::fmax(
      smaxDiffusive, (AdiabaticIndex * ReferenceViscosity) / (PrandtlNumber * Q[Shortcuts::rho])
    );
  };
"""
        )

    def eigenvalues(self):
        return self.eigenvalue_functions() + """
    // The ADER-DG solver takes the order factor from PNPM, the RKDG solver
    // from the Cockburn-Shu bound of the Runge-Kutta order.
    {% if SOLVER_FAMILY == "ader" %}
    constexpr double OrderFactor = PNPM;
    {% else %}
    constexpr double OrderFactor = 1.0 / (2.0 * RungeKuttaOrder - 1.0);
    {% endif %}
    return maxEval(Q, normal)
      + diffusiveEval(Q, normal)*2/(h[normal]*OrderFactor);
"""

    def flux(self):
        flux_init = (
            """  const auto rho = Q[Shortcuts::rho];
  const auto* const j = &Q[Shortcuts::j];
  const auto E = Q[Shortcuts::E];
  const auto Z = """
            + ("Q[Shortcuts::Z]" if self._use_advection else "0")
            + """;"""
            + (
                """
  const auto height = Q[Shortcuts::height];"""
                if self._use_gravity
                else ""
            )
            + """
  const auto backgroundPressure = """
            + ("Q[Shortcuts::backgroundstate+1]" if self._use_background_state else "0")
            + """;
"""
            + (
                """
  constexpr auto Gravity = 9.81;"""
                if self._use_gravity
                else ""
            )
            + """
  constexpr auto AdiabaticIndex = """
            + str(self._gamma)
            + """;"""
            + (
                """
  constexpr auto GasConstant = """ + str(self._gas_constant) + """;"""
                if self._use_gravity
                else ""
            )
            + """
  constexpr auto IsochoricHeatCapacity = """
            + str(self._cv)
            + """;
  constexpr auto HeatRelease = """
            + str(self._q0)
            + """;
  const auto invRho = 1./rho; """
            + self.evaluate_pressure()
        )

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
  constexpr auto ReferenceViscosity = """
            + str(self._reference_viscosity)
            + """;
  constexpr auto Viscosity = ReferenceViscosity;
  constexpr auto PrandtlNumber = """
            + str(self._prandtl_number)
            + """;
  constexpr auto ThermalConductivity = 1./PrandtlNumber * ReferenceViscosity * AdiabaticIndex * IsochoricHeatCapacity;

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
  const auto wwx  = invRho * (deltaQ[0][Shortcuts::j+2] - ww * deltaQ[0][Shortcuts::rho]);
  const auto wwy  = invRho * (deltaQ[1][Shortcuts::j+2] - ww * deltaQ[1][Shortcuts::rho]);
  const auto wwz  = invRho * (deltaQ[2][Shortcuts::j+2] - ww * deltaQ[2][Shortcuts::rho]);
  const auto divV23 = 2./3. * (uux + vvy + wwz);
#endif

  double Fj[DIMENSIONS];
  switch (normal) {
    case 0:
      Fj[0] = Viscosity * (2 * uux - divV23);
      Fj[1] = Viscosity * (uuy + vvx);
#if DIMENSIONS == 3
      Fj[2] = Viscosity * (uuz + wwx);
#endif
      break;
    case 1:
      Fj[0] = Viscosity * (uuy + vvx);
      Fj[1] = Viscosity * (2 * vvy - divV23);
#if DIMENSIONS == 3
      Fj[2] = Viscosity * (vvz + wwy);
      break;
    case 2:
      Fj[0] = Viscosity * (uuz + wwx);
      Fj[1] = Viscosity * (vvz + wwy);
      Fj[2] = Viscosity * (2 * wwz - divV23);
#endif
      break;
    default:
      assertion(false, normal);
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
  const auto invCv = 1/IsochoricHeatCapacity;
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
  // Gravity acts in y direction in 2D and in z direction in 3D.
  if (normal == DIMENSIONS - 1) {
    T -= (Gravity * (AdiabaticIndex - 1)) / GasConstant;
  } """
                if self._use_gravity
                else ""
            )
            + """
  F[Shortcuts::E] -= ThermalConductivity * T;
"""
        )

        flux_advection = (
            """
  F[Shortcuts::Z] = j[normal] * Z * invRho;

  const auto factor = invCv * HeatRelease * invRho2;
  T = -factor *
          (Q[Shortcuts::rho] * deltaQ[normal][Shortcuts::Z] -
            Q[Shortcuts::Z] * deltaQ[normal][Shortcuts::rho]);
  F[Shortcuts::E] -= ThermalConductivity * T;

  constexpr auto MolecularDiffusionCoefficient = """
            + str(self._molecular_diffusion_coeff)
            + """;
  F[Shortcuts::rho] -= MolecularDiffusionCoefficient * deltaQ[normal][Shortcuts::rho];
  F[Shortcuts::Z] -= MolecularDiffusionCoefficient * deltaQ[normal][Shortcuts::Z];
"""
        )

        return (
            flux_init
            + flux_euler
            + flux_viscous
            + flux_heat
            + (flux_advection if self._use_advection else "")
        )

    def source(self):
        if not (self._use_gravity or self._use_background_state):
            return None
        return (
            """
  std::fill_n(S, NumberOfUnknowns, 0);

  auto rhoPerturbation = Q[Shortcuts::rho];
"""
            + (
                """
  const auto backgroundRho = Q[Shortcuts::backgroundstate+0];
  rhoPerturbation -= backgroundRho;
"""
                if self._use_background_state
                else ""
            )
            + (
                """
  constexpr auto Gravity = 9.81;
  // Gravity acts in y direction in 2D and in z direction in 3D.
  S[Shortcuts::j+DIMENSIONS-1] = -1 * rhoPerturbation * Gravity;
"""
                if self._use_gravity
                else ""
            )
        )

    def riemann_solver(self):
        return (
            """
  constexpr int ValuesPerNode = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  using FaceValue = std::remove_cvref_t<decltype(QL[0])>;
  FaceValue QavL[ValuesPerNode] __attribute__((aligned(16))) = {0};
  FaceValue QavR[ValuesPerNode] __attribute__((aligned(16))) = {0};

#if DIMENSIONS==3
  constexpr int NodesPerFace = (DGOrder+1)*(DGOrder+1);
#else
  constexpr int NodesPerFace = (DGOrder+1);
#endif

  const auto faceWeights = Quadrature<FaceValue>::getQuadratureWeightsOnFace();
  for (int xy = 0; xy < NodesPerFace; xy++) {
    for (int n = 0; n < ValuesPerNode; n++) {
      QavL[n] += faceWeights[xy] * QL[xy * ValuesPerNode + n];
      QavR[n] += faceWeights[xy] * QR[xy * ValuesPerNode + n];
    }
  }

  """
            + self.eigenvalue_functions()
            + """

  auto smax = std::fmax(
    maxEval(&QavL[0], normal),
    maxEval(&QavR[0], normal)
  );

  const auto factor = 2 * (2 * DGOrder + 1) / (h[normal] * std::sqrt(0.5 * std::numbers::pi));
  smax += factor*std::fmax(
    diffusiveEval(&QavL[0], normal), diffusiveEval(&QavR[0], normal)
  );

  for (int xy = 0; xy < NodesPerFace; xy++) {
    for (int n = 0; n < NumberOfUnknowns; n++) {
      FL[xy*NumberOfUnknowns+n] = 0.5 * (FL[xy * NumberOfUnknowns + n] + FR[xy * NumberOfUnknowns + n]
        + smax * (QL[xy * ValuesPerNode + n] - QR[xy * ValuesPerNode + n]) );
      FR[xy * NumberOfUnknowns + n] = FL[xy * NumberOfUnknowns + n];
    }
  }
  return smax;
"""
        )
