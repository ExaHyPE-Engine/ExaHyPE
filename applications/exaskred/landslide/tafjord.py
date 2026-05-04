# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
Tafjord landslide-generated tsunami benchmark in the Tafjord region of Norway.

The setup models an idealised mass movement into the fjord with the nonlinear
shallow-water equations augmented by Voellmy-Salm friction. At ``t=0`` the flow
depth is initialised from the prescribed slide displacement, the bathymetry is
sampled from the digital elevation model, and all momenta are zero.

Open boundaries copy the interior state outward, mirroring the original Tafjord
configuration while the shared landslide PDE terms live in inline helper
headers at the subapplication root.
"""

import math

import exahype2
import peano4

DOMAIN_OFFSET = [414895.5, 6904495.5]
DOMAIN_SIZE = [1900.0, 1900.0]
BATHYMETRY_FILE = "bathymetry/Tafjord_5m_EPSG25832.nc"
DISPLACEMENT_FILE = "bathymetry/ini_3.0Mm3_5m_EPSG25832.nc"

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++i) {
    Q[i] = 0.0;
  }

  static tarch::reader::NetCDFFieldSampler fieldSampler(
    {
      {"bathymetry", RuntimeParameters::getBathymetryFile(), "Band1", "x", "y"},
      {"displacement", RuntimeParameters::getDisplacementFile(), "Band1", "x", "y"}
    },
    DomainSize(0),
    DomainSize(1),
    DomainOffset(0),
    DomainOffset(1)
  );

  Q[Shortcuts::h] = fieldSampler.sample("displacement", x(0), x(1));
  Q[Shortcuts::z] = fieldSampler.sample("bathymetry", x(0), x(1));
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++i) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  return ::exahype2::RefinementCommand::Keep;
"""

limiting_criterion = """
  const auto Qh = Q[0];
  if (!__builtin_isfinite(Qh)) {
    return false;
  }

  if ((Qh < hThreshold) and (Qh > -hThreshold)) {
    return true;
  }

  if (Qh <= -hThreshold) {
    return false;
  }

  const double leftDistanceX = x[0] - DomainOffset[0];
  const double rightDistanceX = x[0] - DomainOffset[0] - DomainSize[0];
  if (
    (leftDistanceX < h[0] and leftDistanceX > -h[0])
    or (rightDistanceX < h[0] and rightDistanceX > -h[0])
  ) {
    return false;
  }

  const double leftDistanceY = x[1] - DomainOffset[1];
  const double rightDistanceY = x[1] - DomainOffset[1] - DomainSize[1];
  if (
    (leftDistanceY < h[1] and leftDistanceY > -h[1])
    or (rightDistanceY < h[1] and rightDistanceY > -h[1])
  ) {
    return false;
  }

  return true;
"""

adjust_solution = """
  if (Q[0] < hThreshold) {
    Q[0] = (Q[0] < 0.0) ? 0.0 : Q[0];
    Q[1] = 0.0;
    Q[2] = 0.0;
  }
"""

flux = """
  ::applications::exahype2::exaskred::landslide::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::landslide::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

fv_nonconservative_product = """
  ::applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns, true>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

aderdg_nonconservative_product = """
  ::applications::exahype2::exaskred::landslide::nonconservativeProduct<Shortcuts, NumberOfUnknowns, false>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

fv_diffusive_source_term = """
  ::applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, false
  >(Q, deltaQ, x, h, t, dt, S);
"""

aderdg_diffusive_source_term = """
  ::applications::exahype2::exaskred::landslide::diffusiveSourceTerm<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables, true
  >(Q, deltaQ, x, h, t, dt, S);
"""

fv_riemann_solver = """
  return ::applications::exahype2::exaskred::landslide::solveRiemannProblem<
    Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables
  >(QL, QR, x, h, t, dt, normal, FL, FR);
"""

aderdg_riemann_solver = """
  ::applications::exahype2::exaskred::landslide::aderdgRiemannSolver<
    Shortcuts, NumberOfUnknowns, Order
  >(
    FL, FR, QL, QR, t, dt, x, h, direction, isBoundaryFace, faceIndex,
    kernels::ADERDGSolver::Quadrature<double>::weights2
  );
"""

parser = exahype2.ArgumentParser()
parser.add_argument(
    "--friction",
    type=float,
    help="Friction parameter.",
)
parser.set_defaults(
    min_depth=3,
    end_time=40.0,
    degrees_of_freedom=7,
    friction=200.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "phi": [25.0, "double"],
    "invXi": [1.0 / args.friction, "double"],
    "hThreshold": [1e-1, "double"],
}
constants["mu"] = [
    math.tan(math.pi / 180.0 * constants["phi"][0]),
    "double",
]

max_h = 1.1 * min(DOMAIN_SIZE) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
dg_order = args.degrees_of_freedom - 1

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.20,
)
aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=aderdg_nonconservative_product,
    diffusive_source_term=aderdg_diffusive_source_term,
    riemann_solver=aderdg_riemann_solver,
)
aderdg_solver.add_user_solver_includes(
    """
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "Riemann.h"
#include "SourceTerm.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)
aderdg_solver.add_kernel_optimisations(
    is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * dg_order + 1,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.20,
)
fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=fv_nonconservative_product,
    diffusive_source_term=fv_diffusive_source_term,
    riemann_solver=fv_riemann_solver,
    adjust_solution=adjust_solution,
)
fv_solver.add_user_solver_includes(
    """
#include "Flux.h"
#include "NonconservativeProduct.h"
#include "Riemann.h"
#include "SourceTerm.h"
#include "tarch/reader/NetCDFFieldSampler.h"
"""
)

limiter_solver = exahype2.solvers.limiting.PosterioriLimiting(
    name="LimiterSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=0.001,
    dmp_differences_scaling=0.0001,
    physical_admissibility_criterion=limiting_criterion,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="TafjordLandslide",
    directory=".",
    executable="ExaSkred-Landslide",
)

fv_solver.set_plotter(peano4.plotter.NetCDFPatchAMRPlotter)
aderdg_solver.set_plotter(peano4.plotter.NetCDFPatchNodalPlotter)

project.add_solver(aderdg_solver)
project.add_solver(fv_solver)
project.add_solver(limiter_solver)
project.add_runtime_parameter("bathymetryFile", "string", BATHYMETRY_FILE)
project.add_runtime_parameter("displacementFile", "string", DISPLACEMENT_FILE)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=DOMAIN_SIZE,
    offset=DOMAIN_OFFSET,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
peano_project = project.generate_Peano4_project(verbose=False)

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.output.makefile.set_target_device(args.target_device)
peano_project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
