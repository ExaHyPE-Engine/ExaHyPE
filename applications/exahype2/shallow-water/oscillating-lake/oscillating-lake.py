# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

"""!
    Implementation of the planar surface in a paraboloid Thacker's test.
    This implementation is mainly based on the Planar surface in a paraboloid case in section 4.2.2 Two dimensional cases of the paper:

    @article{Delestre2013,
      title={SWASHES: a compilation of shallow water analytic solutions for hydraulic and environmental studies},
      author={Delestre, Olivier and Lucas, Carine and Ksinant, Pierre-Antoine and Darboux, Fr{\'e}d{\'e}ric and Laguerre, Christian and Vo, T-N-Tuoi and James, Francois and Cordier, St{\'e}phane},
      journal={International Journal for Numerical Methods in Fluids},
      volume={72},
      number={3},
      pages={269--300},
      year={2013},
      publisher={Wiley Online Library}
    }

    It was first described by William Carlisle Thacker in:

    @article{Thacker1981,
      title={Some exact solutions to the nonlinear shallow-water wave equations},
      author={Thacker, William Carlisle},
      journal={Journal of Fluid Mechanics},
      volume={107},
      pages={499--508},
      year={1981},
      publisher={Cambridge University Press}
    }
"""
initial_conditions = """
  // Assumption: square domain is centered at 0.0, L/2 == 0.
  // Values chosen in accordance to the selected values in the SWASHES-framework paper
  constexpr double a = 1.0;
  constexpr double hZero = 0.1;
  constexpr double eta = 0.5;

  const double r = std::sqrt(std::pow(x[0], 2) + std::pow(x[1], 2));
  const double omega = std::sqrt(2 * g * hZero) / a;

  // As in the paper, the initial condition is the analytical solution at time stamp zero
  // Initialise the paraboloid bathymetry
  Q[Shortcuts::z] = -hZero * (1.0 - (std::pow(r, 2))/(std::pow(a, 2)));
  // Initialise the material height (The analytical solution at time step zero might contain values below zero, but those
  // will be set to zero here for physical feasibility)
  Q[Shortcuts::h] = ((eta * hZero) / std::pow(a, 2)) * (2 * (x[0]) * std::cos(omega * 0) + 2 * (x[1]) * std::sin(omega * 0) - eta) - Q[Shortcuts::z];
  if (Q[Shortcuts::h] >= 0) {
    Q[Shortcuts::hu] = (-eta * omega * std::sin(omega * 0)) * Q[Shortcuts::h];
    Q[Shortcuts::hv] = (eta * omega * std::cos(omega * 0)) * Q[Shortcuts::h];
  } else {
    Q[Shortcuts::h] = 0.0;
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
  }

  //Q[Shortcuts::z] = 0.1 * (std::pow(x[0], 2) + std::pow(x[1], 2));
  //Q[Shortcuts::h] = std::fmax(0.0, 0.05 * (2 * x[0] * std::cos(omega * 0) + 2 * x[1] * std::sin(omega * 0)) + 0.075 - Q[Shortcuts::z]);
  //Q[Shortcuts::hu] = 0.5 * omega * std::sin(omega * 0) * Q[Shortcuts::h];
  //Q[Shortcuts::hv] = 0.5 * omega * std::cos(omega * 0) * Q[Shortcuts::h];

  //Q[Shortcuts::h] = 0.025 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1))));
  //Q[Shortcuts::z] = -0.2 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1)))) - 0.1;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

refinement_criterion = """
  return std::sqrt((x(0) * x(0)) + (x(1) * x(1))) <= 1.0
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
"""

"""
The analytical solution implementation according to Delestre2013, similar to the initial condition but now with
the value for t substituted into the formula instead of zero.
"""
analytical_solution = """
  constexpr double a = 1.0;
  constexpr double hZero = 0.1;
  constexpr double eta = 0.5;

  const double r = std::sqrt(std::pow(x[0], 2) + std::pow(x[1], 2));
  const double omega = std::sqrt(2 * g * hZero) / a;

  const double currentHeight = ((eta * hZero) / std::pow(a, 2)) * (2 * (x[0]) * std::cos(omega * t) + 2 * (x[1]) * std::sin(omega * t) - eta) - Q[Shortcuts::z];
  // Again, only values h > 0 will be considered, the rest is assumed to be dry-state.
  if (currentHeight > 0) {
    solution[0] = ((eta * hZero) / std::pow(a, 2)) * (2 * (x[0]) * std::cos(omega * t) + 2 * (x[1]) * std::sin(omega * t) - eta) - Q[Shortcuts::z];
    solution[1] = (-eta * omega * std::sin(omega * t)) * Q[Shortcuts::h];
    solution[2] = (eta * omega * std::cos(omega * t)) * Q[Shortcuts::h];
  } else {
    solution[0] = 0.0;
    solution[1] = 0.0;
    solution[2] = 0.0;
  }
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=5,
    degrees_of_freedom=16,
    end_time=10.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [4.0, 4.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    riemann_solver="return augmentedStateRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.set_plotter(args.plotter)
fv_solver.add_user_solver_includes(
    """
#include "../AugmentedStateRiemannSolver.h"
"""
)

exahype2.solvers.fv.ErrorMeasurement(
    fv_solver,
    error_measurement_implementation=analytical_solution,
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="OscillatingLake",
    directory=".",
    executable="ExaHyPE",
)
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=[-2.0, -2.0],
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
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
