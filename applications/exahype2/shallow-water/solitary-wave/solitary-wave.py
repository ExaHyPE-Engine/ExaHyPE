# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

"""!
    A solitary wave case implementation based on the papers listed below. the wave shape and initial velocity
    are taken from the paper "Non-breaking and breaking solitary wave run-up" section 2.5.1.
    Some more properties, like the characteristic length of the wave, are taken from the paper
    "Solitary wave runup on plane slopes".

    @article{Li2002,
      title={Non-breaking and breaking solitary wave run-up},
      author={Li, Ying and Raichlen, Fredric},
      journal={Journal of Fluid Mechanics},
      volume={456},
      pages={295--318},
      year={2002},
      publisher={Cambridge University Press}
    }

    @article{Li2001,
      title={Solitary wave runup on plane slopes},
      author={Li, Ying and Raichlen, Fredric},
      journal={Journal of Waterway, Port, Coastal, and Ocean Engineering},
      volume={127},
      number={1},
      pages={33--44},
      year={2001},
      publisher={American Society of Civil Engineers}
    }

    Also interesting is one of the original papers which introduced many of such cases:

    @article{Synolakis1987,
      title={The runup of solitary waves},
      author={Synolakis, Costas Emmanuel},
      journal={Journal of Fluid Mechanics},
      volume={185},
      pages={523--545},
      year={1987},
      publisher={Cambridge University Press}
    }
"""

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double Depth = 1.0;
  constexpr double BeachSlopeAngle = 30 * std::numbers::pi / 180;
  constexpr double WaveAmplitude = 0.5;

  const double gamma = std::sqrt((3 * WaveAmplitude) / (4 * Depth));

  // The wave crest is located at L/2 away from the foot of the
  // slope xZero, where L is the characteristic length of the solitary wave.
  const double wavePos = 0.5 * ((2/gamma) * (std::acosh(std::sqrt(20))));

  const double xZero = Depth / std::tan(BeachSlopeAngle);

  // Set up the bathymetry, extend the beach a bit in negative x direction
  if (x[0] < 0) { // Beach slope for negative x[0] values
    Q[Shortcuts::z] = std::fabs(x[0]) * std::tan(BeachSlopeAngle);
  } else if (x[0] < xZero) { // Initialise the beach slope for x[0] >= 0
    Q[Shortcuts::z] = -(x[0] * std::tan(BeachSlopeAngle));
  } else { // Initialise the constant ocean depth
    Q[Shortcuts::z] = -Depth;
  }

  // Set up the wave profile
  const double innerTerm = gamma * ((x[0] - (xZero + wavePos)) / Depth);
  const double waveProfile = WaveAmplitude * std::pow((1 / (std::cosh(innerTerm))), 2);
  if (x[0] >= xZero) {
    Q[Shortcuts::h] = Depth + waveProfile;
  } else if (x[0] < xZero && x[0] >= 0){
    Q[Shortcuts::h] = std::fabs(Q[Shortcuts::z]);
  } else if (x[0] >= (xZero + 2 * wavePos)) { // Coordinate outside the characteristic wave region
    Q[Shortcuts::h] = Depth;
  }

  // Set up the velocity
  // Velocity taken from the paper "Non-breaking and breaking solitary wave run-up"
  const double waveCelerity = std::sqrt(g * (WaveAmplitude + Depth));
  Q[Shortcuts::hu] = -((waveCelerity * waveProfile) / (Depth + waveProfile)) * Q[Shortcuts::h];
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=6,
    degrees_of_freedom=16,
    end_time=5.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

offset = [-5, 0]
size = [15.0, 15.0]
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
    riemann_solver="return llfRiemannSolver<double, Shortcuts>(QL, QR, x, h, t, dt, normal, FL, FR);",
)

fv_solver.set_plotter(args.plotter)
fv_solver.add_user_solver_includes(
    """
#include "../LLFRiemannSolver.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="SolitaryWave",
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
    offset=offset,
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
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})"
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
