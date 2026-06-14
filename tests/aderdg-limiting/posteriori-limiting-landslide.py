# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

use_aderdg = True
use_fv = True
use_limiting = use_aderdg and use_fv
with_limiting = 1  # 0 - all ADER, 1 - limit, 2 - all FV

dimensions = 2
order = args.degrees_of_freedom - 1
CFL = 0.2
unknowns = {"h": 1, "hu": 1, "hv": 1, "z": 1}
auxiliary = {}

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="PosterioriLimitingLandslide",
    dimensions=dimensions,
    size=[1.58, 1.58, 1.58],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(
        f"Posteriori limiting landslide is a {dimensions}D testcase, but the "
        f"project was configured for {project.dimensions}D."
    )

g = 9.81
hThreshold = 1e-5
number_of_dmp_observables = 0
dmp_differences_scaling = 0.01  # 0.01 how the diff is scaled
dmp_relaxation_parameter = 0.02  # the diff scaling

landslide_zoo = {
    # avalanche-down-the-W
    "DownTheWForm": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = (0.5 + (0.5 * DomainSize[0] - x[0]) * (0.5 * DomainSize[0] - x[0]) / ((0.5 * DomainSize[0] - DomainSize[0]) * (0.5 * DomainSize[0] - DomainSize[0]))) * {amplitude} * std::cos({frequency} * x[0]);
""".format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "amplitude": 0.75,
            "frequency": 8.0,
            "initRadius": 0.10,
            "initHeight": 0.008,
        }
    ),
    # avalanche-down-the-hyperbolic-par
    "DownTheHyperbolicPar": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = {initElevationParA} * (x[0] - {initElevationCenterX}) * (x[0] - {initElevationCenterX}) - {initElevationParB} * (x[1] - {initElevationCenterY}) * (x[1] - {initElevationCenterY});
""".format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "initElevationCenterX": 0.79,
            "initElevationCenterY": 0.79,
            "initElevationParA": 1.0,
            "initElevationParB": 1.0,
            "initRadius": 0.10,
            "initHeight": 0.008,
        }
    ),
    # avalanche-down-the-revolve-par
    "DownTheRevolvePar": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = {initElevationParA} * (x[0] - {initElevationCenterX}) * (x[0] - {initElevationCenterX}) + {initElevationParB} * (x[1] - {initElevationCenterY}) * (x[1] - {initElevationCenterY});
""".format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "initElevationCenterX": 0.79,
            "initElevationCenterY": 0.79,
            "initElevationParA": 1.0,
            "initElevationParB": 1.0,
            "initRadius": 0.10,
            "initHeight": 0.008,
        }
    ),
    # avalanche-down-the-slope-with-jump
    "DownTheSlopeWithJump": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  const double zeta{{ {initZetaDegree} * tarch::la::PI / 180.0}}; // translation from degrees to radians
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = (DomainSize[0] - x[0]) * std::tan(zeta);
  if (tarch::la::greater({jumpX}, x[0])) {{
    Q[3] += {jumpSize};
  }}
  """.format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "jumpSize": 1.0,
            "jumpX": 0.79,
            "initZetaDegree": 35,
            "initHeight": 0.008,
            "initRadius": 0.10,
        }
    ),
    # avalanche-down-the-slope-with-plateau
    "DownTheSlopeWithPlateau": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  const double zeta{{{initZetaDegree} * tarch::la::PI / 180.0}}; // translation from degrees to radians
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  if (tarch::la::greater({plateauStartX}, x[0])) {{
    Q[3] = (DomainSize[0] - x[0]) * std::tan(zeta);
  }}
  else
  if (tarch::la::greater({plateauEndX}, x[0])) {{
    Q[3] = (DomainSize[0] - {plateauStartX}) * std::tan(zeta);
  }}
  else {{
    Q[3] = (DomainSize[0] - (x[0] - {plateauEndX} + {plateauStartX})) * std::tan(zeta);
  }}
""".format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "initElevationCenterX": 0.79,
            "initElevationCenterY": 0.79,
            "plateauStartX": 0.395,
            "plateauEndX": 1.185,
            "initZetaDegree": 35,
            "initHeight": 0.008,
            "initRadius": 0.10,
        }
    ),
    # avalanche-down-the-slope
    "DownTheSlope": """
  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {{ {initCenterX}, {initCenterY} }};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < {initRadius};
  const double zeta{{{initZetaDegree} * tarch::la::PI / 180.0}}; // translation from degrees to radians
  Q[0] = (nearCentre ? {initHeight} : 0.0); // Granular material height (h)
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = (DomainSize[0] - x[0]) * std::tan(zeta);
""".format_map(
        {
            "initCenterX": 0.15,
            "initCenterY": 0.79,
            "initZetaDegree": 35,
            "initHeight": 0.008,
            "initRadius": 0.10,
        }
    ),
}

landslide_case = "DownTheSlope"
initial_conditions = landslide_zoo[landslide_case]

boundary_conditions = f"""
  std::copy_n(Qinside, 4, Qoutside);
  Qoutside[1] = -Qoutside[1];
  Qoutside[2] = -Qoutside[2];
"""

flux = f"""
  constexpr double g = {g};
  constexpr double hThreshold = {hThreshold};
  if (Q[0] > hThreshold) {{
    const double un = Q[1 + normal] / Q[0];
    F[0]            = Q[1 + normal];
    F[1]            = un * Q[1];
    F[2]            = un * Q[2];
    F[3]            = 0.0;
    F[1 + normal]  += 0.5 * g * Q[0] * Q[0];
  }}
  else {{
    F[0]            = 0.0;
    F[1]            = 0.0;
    F[2]            = 0.0;
    F[3]            = 0.0;
  }}
"""

ncp = f"""
  constexpr double g = {g};
  constexpr double hThreshold = {hThreshold};
  if (Q[0] > hThreshold) {{
    BTimesDeltaQ[0]           = 0.0;
    BTimesDeltaQ[1]           = 0.0;
    BTimesDeltaQ[2]           = 0.0;
    BTimesDeltaQ[3]           = 0.0;
    BTimesDeltaQ[1 + normal]  = g * Q[0] * deltaQ[3];
  }} else {{
    BTimesDeltaQ[0]           = 0.0;
    BTimesDeltaQ[1]           = 0.0;
    BTimesDeltaQ[2]           = 0.0;
    BTimesDeltaQ[3]           = 0.0;
  }}
"""

source_term = f"""
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = 0.0;
  S[3] = 0.0;
"""

eigenvalues = f"""
  constexpr double g = {g};
  constexpr double hThreshold = {hThreshold};
  if(Q[0] > hThreshold) {{
    const double un = Q[normal+1] / Q[0];
    const double c  = std::sqrt(g * Q[0]);
    return std::max(std::abs(un - c), std::abs(un + c));
  }}
  return 0.0;
"""

adjust_solution = f"""
  constexpr double hThreshold = {hThreshold};
  if (Q[0] < hThreshold) {{
    Q[0] = 0.0;
    Q[1] = 0.0;
    Q[2] = 0.0;
  }}
"""

riemann_solver_aderdg = f"""
  double smax = 0.0;
  constexpr double g = {g};
  constexpr double hThreshold = {hThreshold};
  for(int xy=0; xy<Order+1; xy++){{
    smax = std::max(smax, maxEigenvalue(&QL[xy*{len(unknowns)}], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*{len(unknowns)}], x, h, t, dt, direction));
  }}

  for(int xy=0; xy<Order+1; xy++){{
    double fluxLeft[{len(unknowns)}];
    double fluxRight[{len(unknowns)}];

    flux(&QL[xy*{len(unknowns)}], x, h, t, dt, direction, fluxLeft);
    flux(&QR[xy*{len(unknowns)}], x, h, t, dt, direction, fluxRight);
    for (int varId = 0; varId < {len(unknowns) - 1}; ++varId) {{
      FL[xy*{len(unknowns)}+varId] = 0.5*(fluxLeft[varId]+fluxRight[varId] + smax*(QL[xy*{len(unknowns)}+varId]-QR[xy*{len(unknowns)}+varId]));
    }}

    for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
      FR[xy*{len(unknowns)}+varId] = FL[xy*{len(unknowns)}+varId];
    }}

    //Contribution from NCP
    double AveragedQ[{len(unknowns)}];
    double DeltaQ[{len(unknowns)}];
    for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
      AveragedQ[varId]  = 0.5 * (QL[xy*{len(unknowns)} + varId] + QR[xy*{len(unknowns)} + varId]);
      DeltaQ[varId]     = QR[xy*{len(unknowns)} + varId] - QL[xy*{len(unknowns)} + varId];
    }}
    double ncp[{len(unknowns)}];
    nonconservativeProduct(AveragedQ, DeltaQ, x, h, t, dt, direction, ncp);
    for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
      FL[xy*{len(unknowns)}+varId] += 0.5 * ncp[varId];
      FR[xy*{len(unknowns)}+varId] -= 0.5 * ncp[varId];
    }}
    FL[xy*{len(unknowns)}+{len(unknowns) - 1}] = 0.0;
    FR[xy*{len(unknowns)}+{len(unknowns) - 1}] = 0.0;
  }}
"""

riemann_solver_fv = f"""
  double smax = 0.0;
  constexpr double g = {g};
  constexpr double hThreshold = {hThreshold};
  smax = std::max(smax, maxEigenvalue(QL, x, h, t, dt, normal));
  smax = std::max(smax, maxEigenvalue(QR, x, h, t, dt, normal));

  double fluxLeft[{len(unknowns)}];
  double fluxRight[{len(unknowns)}];
  flux(QL, x, h, t, dt, normal, fluxLeft);
  flux(QR, x, h, t, dt, normal, fluxRight);
  for (int varId = 0; varId < {len(unknowns) - 1}; ++varId) {{
    FL[varId] = 0.5*(fluxLeft[varId]+fluxRight[varId] + smax*(QL[varId]-QR[varId]));
  }}

  for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
    FR[varId] = FL[varId];
  }}

  //Contribution from NCP
  double AveragedQ[{len(unknowns)}];
  double DeltaQ[{len(unknowns)}];
  for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
    AveragedQ[varId] = 0.5 * (QL[varId] + QR[varId]);
    DeltaQ[varId] = QR[varId]-QL[varId];
  }}
  double ncp[{len(unknowns)}];
  nonconservativeProduct(AveragedQ, DeltaQ, x, h, t, dt, normal, ncp);
  for (int varId = 0; varId < {len(unknowns)}; ++varId) {{
    FL[varId] += 0.5 * ncp[varId];
    FR[varId] -= 0.5 * ncp[varId];
  }}

  // the elevation should be constant
  FL[{len(unknowns) - 1}] = 0.0;
  FR[{len(unknowns) - 1}] = 0.0;

  return smax;
"""

isPhysicallyAdmissible = "return true;"
if with_limiting == 0:
    isPhysicallyAdmissible = "return true; //all ADERDG"
elif with_limiting == 1:
    isPhysicallyAdmissible = f"""
  // If h is close to zero, it is assumed that limiting is not required.
  // It allows to reduce the computational intensity, avoiding recalculation
  // cells with zero heights on the FV layer
  if (std::abs(Q[0]) < {hThreshold}) {{
    return true;
  }}

  // Height must be positive!
  if (Q[0] < {hThreshold}) {{
      return false;
  }}

  // If non-physical oscillations occur, the wave speed might receive a negative value under the square root.
  // It results in eigenvalue becoming NaN, which are not marked as TROUBLED, if no isfinte check is performed.
  for (int i = 0; i < {len(unknowns)}; ++i) {{
    if (!std::isfinite(Q[i])) {{
      return false;
    }}
  }}

  // Limit close to boundaries
  if (std::abs(x[0] - DomainOffset[0]) < h[0]
    || std::abs(x[0] - DomainOffset[0] - DomainSize[0]) < h[0]
    || std::abs(x[1] - DomainOffset[1]) < h[1]
    || std::abs(x[1] - DomainOffset[1] - DomainSize[1]) < h[1]
  ) {{
    return false;
  }}
  return true;
  """
else:
    isPhysicallyAdmissible = "return false; //all FV"

if use_aderdg:
    aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
        name="ADERDGSolver",
        order=order,
        unknowns=unknowns,
        auxiliary_variables=auxiliary,
        time_step_relaxation=CFL,
    )
    aderdg_solver.set_implementation(
        flux=flux,
        max_eigenvalue=eigenvalues,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        ncp=ncp,
        source_term=source_term,
        riemann_solver=riemann_solver_aderdg,
    )
    project.add_solver(aderdg_solver)

if use_fv:
    fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="FVSolver",
        patch_size=2 * order + 1,
        unknowns=unknowns,
        auxiliary_variables=auxiliary,
        time_step_relaxation=CFL,
    )
    fv_solver.set_implementation(
        flux=flux,
        max_eigenvalue=eigenvalues,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        ncp=ncp,
        source_term=source_term,
        riemann_solver=riemann_solver_fv,
        adjust_solution=adjust_solution,
    )
    project.add_solver(fv_solver)

if use_limiting:
    limiter = exahype2.solvers.limiting.PosterioriLimiting(
        name="LimitingSolver",
        regular_solver=aderdg_solver,
        limiting_solver=fv_solver,
        number_of_dmp_observables=number_of_dmp_observables,
        dmp_relaxation_parameter=dmp_relaxation_parameter,
        dmp_differences_scaling=dmp_differences_scaling,
        physical_admissibility_criterion=isPhysicallyAdmissible,
    )
    project.add_solver(limiter)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
