# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4, exahype2
import math

initial_conditions = """
  // init all to 0.0
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  // landslide model params for granular material
  double initCenterX = 4.0;
  double initCenterY = 30;
  double initRadius = 3.5;
  double initHeight = 8.0;

  const tarch::la::Vector<DIMENSIONS, double>& granularMaterialCenter = {initCenterX, initCenterY};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < initRadius;
  Q[Shortcuts::lsh] = (nearCentre ? initHeight : 0.0); // Granular material height (h)

  // swe equation initial conditions
  Q[Shortcuts::hu] = 0.0;
  Q[Shortcuts::hv] = 0.0;

  if (x[0] >= 19.5) { // made a smoother transition zone - some water movement at the start
    Q[Shortcuts::h] = 4.0;
  } else if (x[0] >= 17.5) {
      Q[Shortcuts::h] = 4.0 * (x[0] - 17.5) / 2.0;
  } else {
      Q[Shortcuts::h] = 0.0;
  }

  Q[Shortcuts::z] = std::max(20.0 - x[0], 1.0);
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];

  Qoutside[Shortcuts::lsh]   = 0.;
  Qoutside[Shortcuts::lshu]  = 0.;
  Qoutside[Shortcuts::lshv]  = 0.;
"""

project = exahype2.Project(
    namespace=["applications", "exahype2", "landslide"],
    project_name="FVLandslideTsunami",
    directory=".",
    executable="ExaHyPE",
)

project.set_output_path("solutions")

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=30.0,
)
args = parser.parse_args()

dimensions = 2
size = [60.0, 60.0]
dg_order = args.degrees_of_freedom - 1
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

constants = {
    "g": [9.81, "double"],
    "phi": [25.0, "double"],
    "invXi": [1.0 / 200.0, "double"],
    "hThreshold": [1e-1, "double"],
}
constants["mu"] = [
    math.tan(math.pi / 180.0 * constants["phi"][0]),
    "double",
]

### DEFINE ALL VARIABLES FOR COUPLED MODEL
unknowns = {"h": 1, "hu": 1, "hv": 1, "lsh": 1, "lshu": 1, "lshv": 1}
auxiliary_variables = {"z": 1}

my_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    min_volume_h=min_h,
    max_volume_h=max_h,
    patch_size=dg_order * 2 + 1,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    time_step_relaxation=0.45,
)

my_solver.set_implementation(
    flux="""
      ShallowWater::flux<double, Shortcuts, NumberOfUnknowns>(
        Q,x,h,t,dt,normal,F
      );

      landslide::flux<double, Shortcuts>(
        Q,x,h,t,dt,normal,F
      );""",
    ncp="""
      ShallowWater::nonconservativeProduct<double, Shortcuts, NumberOfUnknowns>(
        Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
      );
      landslide::nonconservativeProduct<double, Shortcuts>(
        Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
      );""",
    max_eigenvalue="""
      return std::fmax(
        landslide::maxEigenvalue<double, Shortcuts>(
          Q, x, h, t, dt, normal
        ),
        ShallowWater::maxEigenvalue<double, Shortcuts>(
          Q, x, h, t, dt, normal
        )
      );""",
    boundary_conditions=boundary_conditions,
    initial_conditions=initial_conditions,
    diffusive_source_term="""
landslide::diffusiveSourceTerm<double, Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        Q, deltaQ, x, h, t, dt, S
      );""",
    riemann_solver="""
return landslide::rusanovRiemannSolverFV<double, Shortcuts, NumberOfUnknowns, NumberOfAuxiliaryVariables>(
        QL, QR, x, h, t, dt, normal, FL, FR
      );""",
)
my_solver.add_user_solver_includes(
    """
#include "../RusanovRiemannSolver.h"
"""
)

my_solver.set_plotter(args.plotter)

my_solver.plot_description = ", ".join(unknowns.keys())
project.add_solver(my_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=dimensions,
    size=size,
    offset=[0.0, 0.0],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[False, False],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
