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
    const auto& Q,
    const int normal
  ){
    const auto& j = Q.j;
    const auto E = Q.E;
    const auto rho = Q.rho;"""
            + (
                """
    const auto height = Q.height;"""
                if self._use_gravity
                else ""
            )
            + """
    const auto Z = """
            + ("Q.Z;" if self._use_advection else "0;")
            + self.evaluate_pressure()
            + """
    const auto normalVelocity = Q.j(normal)/rho;
    const auto c = std::sqrt(AdiabaticIndex * (pressure/rho));
    const double lambda = std::fmax(
      std::abs(normalVelocity-c), std::abs(normalVelocity+c)
    );
    return lambda;
  };

  auto diffusiveEval = [&](
    const auto& Q,
    [[maybe_unused]] const int normal
  ){
    double smaxDiffusive = MolecularDiffusionCoefficient;
    smaxDiffusive = std::fmax(
      smaxDiffusive, (4./3.) * (ReferenceViscosity/Q.rho)
    );
    return std::fmax(
      smaxDiffusive, (AdiabaticIndex * ReferenceViscosity) / (PrandtlNumber * Q.rho)
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
            """  const auto rho = Q.rho;
  const auto& j = Q.j;
  const auto E = Q.E;
  const auto Z = """
            + ("Q.Z" if self._use_advection else "0")
            + """;"""
            + (
                """
  const auto height = Q.height;"""
                if self._use_gravity
                else ""
            )
            + """
  const auto backgroundPressure = """
            + ("Q.backgroundstate(1)" if self._use_background_state else "0")
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
  F.rho = j[normal];
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    F.j(axis) = invRho * j[normal] * j[axis];
  }
  F.E = invRho * j[normal] * (E+pressure);

  F.j(normal) += (pressure-backgroundPressure);
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

  const auto uu   = invRho * Q.j(0);
  const auto uux  = invRho * (deltaQ[0].j(0) - uu * deltaQ[0].rho);
  const auto uuy  = invRho * (deltaQ[1].j(0) - uu * deltaQ[1].rho);

  const auto vv   = invRho * Q.j(1);
  const auto vvx  = invRho * (deltaQ[0].j(1) - vv * deltaQ[0].rho);
  const auto vvy  = invRho * (deltaQ[1].j(1) - vv * deltaQ[1].rho);

#if DIMENSIONS == 2
  const auto divV23 = 2./3. * (uux + vvy);
#else
  const auto ww   = invRho * Q.j(2);
  const auto uuz  = invRho * (deltaQ[2].j(0) - uu * deltaQ[2].rho);
  const auto vvz  = invRho * (deltaQ[2].j(1) - vv * deltaQ[2].rho);
  const auto wwx  = invRho * (deltaQ[0].j(2) - ww * deltaQ[0].rho);
  const auto wwy  = invRho * (deltaQ[1].j(2) - ww * deltaQ[1].rho);
  const auto wwz  = invRho * (deltaQ[2].j(2) - ww * deltaQ[2].rho);
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
      ASSERTION(false, normal);
  }

  F.j(0) -= Fj[0];
  F.j(1) -= Fj[1];
  F.E -= Fj[0]*uu + Fj[1]*vv;
#if DIMENSIONS == 3
  F.j(2) -= Fj[2];
  F.E -= Fj[2] *ww;
#endif
"""
        )
        flux_heat = (
            """
  const auto invCv = 1/IsochoricHeatCapacity;
  const auto invRho2 = invRho * invRho;
  const auto invRho3 = invRho2 * invRho;

#if DIMENSIONS == 2
  const auto dTdW1 = -1 * Q.E * invRho2 +
          invRho3 * (Q.j(0)*Q.j(0) + Q.j(1) * Q.j(1));
#else
  const auto dTdW1 = -1 * Q.E * invRho2 +
          invRho3 * (Q.j(0)*Q.j(0) + Q.j(1) * Q.j(1) + Q.j(2) * Q.j(2));
#endif
  const auto dTdW2 = -1 * Q.j(0) * invRho2;
  const auto dTdW3 = -1 * Q.j(1) * invRho2;
#if DIMENSIONS == 3
  const auto dTdW4 = -1 * Q.j(2) * invRho2;
#endif

  auto T = invCv * (dTdW1 * deltaQ[normal].rho +
            dTdW2 * deltaQ[normal].j(0) +
            dTdW3 * deltaQ[normal].j(1) +
#if DIMENSIONS == 3
            dTdW4 * deltaQ[normal].j(2) +
#endif
            invRho * deltaQ[normal].E);
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
  F.E -= ThermalConductivity * T;
"""
        )

        flux_advection = (
            """
  F.Z = j[normal] * Z * invRho;

  const auto factor = invCv * HeatRelease * invRho2;
  T = -factor *
          (Q.rho * deltaQ[normal].Z -
            Q.Z * deltaQ[normal].rho);
  F.E -= ThermalConductivity * T;

  constexpr auto MolecularDiffusionCoefficient = """
            + str(self._molecular_diffusion_coeff)
            + """;
  F.rho -= MolecularDiffusionCoefficient * deltaQ[normal].rho;
  F.Z -= MolecularDiffusionCoefficient * deltaQ[normal].Z;
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
  S = {};

  auto rhoPerturbation = Q.rho;
"""
            + (
                """
  const auto backgroundRho = Q.backgroundstate(0);
  rhoPerturbation -= backgroundRho;
"""
                if self._use_background_state
                else ""
            )
            + (
                """
  constexpr auto Gravity = 9.81;
  // Gravity acts in y direction in 2D and in z direction in 3D.
  S.j(DIMENSIONS - 1) = -1 * rhoPerturbation * Gravity;
"""
                if self._use_gravity
                else ""
            )
        )

    def riemann_solver(self):
        return (
            """
  using FaceState = std::remove_cvref_t<decltype(QL[0])>;
  using FaceFlux  = std::remove_cvref_t<decltype(FL[0])>;
  using FaceValue = std::remove_cvref_t<decltype(QL[0].rho)>;
  FaceState QavL{};
  FaceState QavR{};

#if DIMENSIONS==3
  constexpr int NodesPerFace = (DGOrder+1)*(DGOrder+1);
#else
  constexpr int NodesPerFace = (DGOrder+1);
#endif

  // The weighted node values add in fixed point. A mirrored face then averages the same values in
  // another order and returns the same bits.
  const auto faceWeights = Quadrature<FaceValue>::getQuadratureWeightsOnFace();
  constexpr int          ValuesPerNode = sizeof(FaceState) / sizeof(FaceValue);
  const FaceValue* const leftValues    = reinterpret_cast<const FaceValue*>(&QL[0]);
  const FaceValue* const rightValues   = reinterpret_cast<const FaceValue*>(&QR[0]);
  FaceValue* const       leftAverage   = reinterpret_cast<FaceValue*>(&QavL);
  FaceValue* const       rightAverage  = reinterpret_cast<FaceValue*>(&QavR);
  for (int n = 0; n < ValuesPerNode; n++) {
    double weightedLeft[NodesPerFace];
    double weightedRight[NodesPerFace];
    for (int xy = 0; xy < NodesPerFace; xy++) {
      weightedLeft[xy]  = tarch::la::productRoundedOnce(
        static_cast<double>(faceWeights[xy]), static_cast<double>(leftValues[xy * ValuesPerNode + n])
      );
      weightedRight[xy] = tarch::la::productRoundedOnce(
        static_cast<double>(faceWeights[xy]), static_cast<double>(rightValues[xy * ValuesPerNode + n])
      );
    }
    leftAverage[n]  = static_cast<FaceValue>(tarch::la::sumInFixedPoint(weightedLeft, NodesPerFace));
    rightAverage[n] = static_cast<FaceValue>(tarch::la::sumInFixedPoint(weightedRight, NodesPerFace));
  }

  """
            + self.eigenvalue_functions()
            + """

  auto smax = std::fmax(
    maxEval(QavL, normal),
    maxEval(QavR, normal)
  );

  const auto factor = 2 * (2 * DGOrder + 1) / (h[normal] * std::sqrt(0.5 * std::numbers::pi));
  smax += factor*std::fmax(
    diffusiveEval(QavL, normal), diffusiveEval(QavR, normal)
  );

  for (int node = 0; node < NodesPerFace; node++) {
    const FaceFlux sumOfFluxes = FL[node] + FR[node];
    const FaceFlux jumpOfState = QL[node] - QR[node];
    FL[node] = transform(sumOfFluxes, jumpOfState, [&](const auto sum, const auto jump) {
      return 0.5 * (sum + smax * jump);
    });
    FR[node] = FL[node];
  }
  return smax;
"""
        )
