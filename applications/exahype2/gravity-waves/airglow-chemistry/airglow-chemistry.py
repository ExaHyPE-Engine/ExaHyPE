import sys
import os

import exahype2

sys.path.insert(0, os.path.abspath(".."))
import GravityWaves
from AirglowPDE import (
    get_flux,
    get_max_eigenvalue,
    get_source_term,
    get_initial_tendency_postprocessing,
)
from AirglowInitialization import (
    get_initial_steady_state_for_airglow,
    get_steady_state_for_ozone_MSISE,
)

"""
This application demonstrates how to setup a scenario for airglow chemistry.
It shows steady states for chemicals and needed interpolations for coupling with the stratified atmosphere model.
Input files can be obtained from https://kauai.ccmc.gsfc.nasa.gov/instantrun/nrlmsis/

The application makes the following assumptions:
  - The height passed to the initial conditions is in [m];
  - The number densities in the input file are in [cm^-3];
  - The temperature in the input file is in [K];
  - The height in the input file is in [km];
  - The air density in the input file is in [g/cm^3];
  - The CSV file uses a delimiter as assigned in get_csv_constants();
  - The CSV file has values at indices assigned in get_csv_constants().
"""

if __name__ == "__main__":
    parser = exahype2.ArgumentParser(
        "ExaHyPE 2 Gravity Waves Airglow Chemistry Argument Parser"
    )
    parser.set_defaults(min_depth=5, degrees_of_freedom=16, end_time=500)
    parser.add_argument(
        "-ec",
        "--einstein-coefficients",
        choices=GravityWaves.EINSTEIN_COEFFICIENTS,
        default="goldman",
        help="|".join(GravityWaves.EINSTEIN_COEFFICIENTS),
    )
    parser.add_argument(
        "-csv",
        "--csv-file-path",
        default="./OH_and_OI_airglow_layer_modulation_model.csv",
        type=str,
        help="Path to CSV file for use as initial conditions for this project.",
    )
    args = parser.parse_args()

    size = [220e3, 220e3, 220e3]

    initial_conditions = (
        GravityWaves.get_CSV_initializer(
            csv_file_path=args.csv_file_path,
            initialize_density=False,
            initialize_hydrogen=True,
        )
        + """

      // create steady state in minor species 
      using ProductionRates = applications::exahype2::GravityWaves::Airglow::ProductionRates;
      const {{COMPUTE_PRECISION}} O = Q[Shortcuts::O];
      const {{COMPUTE_PRECISION}} O2 = Q[Shortcuts::O2];
      const {{COMPUTE_PRECISION}} N2 = Q[Shortcuts::N2];
      const {{COMPUTE_PRECISION}} M = Q[Shortcuts::O] + Q[Shortcuts::O2] + Q[Shortcuts::N2];
      const {{COMPUTE_PRECISION}} H = Q[Shortcuts::H];
      const {{COMPUTE_PRECISION}} T = Q[Shortcuts::T];
      constexpr int BANDWIDTH = EINSTEIN_BANDWIDTH;

      """
        + get_steady_state_for_ozone_MSISE(csv_file_path=args.csv_file_path)
        + """

      """
        + get_initial_steady_state_for_airglow()
        + """

      // initialize remaining coupling variables for advection
      Q[Shortcuts::u] = HORIZONTAL_WINDSPEED;
      Q[Shortcuts::v] = 0.0;
      #if DIMENSIONS == 3
      Q[Shortcuts::w] = 0.0;
      #endif"""
    )

    boundary_conditions = """
      // normal == 0 => left
      // normal == 1 => bottom
      // normal == 2 => right 
      // normal == 3 => top

      for (int unknown = 0; unknown < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++unknown) {
        Qoutside[unknown] = Qinside[unknown];
      }

      if (normal == 1) {
        // Reflective at bottom to keep atmosphere
        // from escaping the domain through the Earth's surface
        Qoutside[Shortcuts::u] = Qinside[Shortcuts::u];
        Qoutside[Shortcuts::v] = -Qinside[Shortcuts::v];
        #if DIMENSIONS == 3
        Qoutside[Shortcuts::w] = Qinside[Shortcuts::w];
        #endif
      }"""

    max_h = 1.1 * min(size) / (3.0**args.min_depth)
    min_h = max_h * 3.0 ** (-args.amr_levels)

    unknowns_list = ["O", "O2", "N2", "O3", "H", "O2cu", "OS"]
    auxiliary_list = ["u", "v"]
    if args.dimensions == 3:
        auxiliary_list.append("w")
    auxiliary_list.append("T")

    # ALWAYS last auxiliary variable
    auxiliary_list += ["dO3dt0"]

    for i in range(0, 10):
        unknowns_list.append("OH" + str(i))

    from collections import OrderedDict

    # order of parameters is VERY important here due to postprocessing
    # relying on the order of parameters to stay consistent

    chemical_unknowns = OrderedDict()
    chemical_auxiliary_variables = OrderedDict()

    for var in unknowns_list:
        chemical_unknowns[var] = 1

    for var in auxiliary_list:
        chemical_auxiliary_variables[var] = 1

    riemann_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="FVSolver",
        patch_size=args.degrees_of_freedom,
        unknowns=chemical_unknowns,
        auxiliary_variables=chemical_auxiliary_variables,
        min_volume_h=min_h,
        max_volume_h=max_h,
        time_step_relaxation=0.5,
    )

    riemann_solver.set_implementation(
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        source_term=get_source_term(),
        flux=get_flux(),
        max_eigenvalue=get_max_eigenvalue(),
    )

    riemann_solver.plot_description = ", ".join(chemical_unknowns.keys()) + ", "
    riemann_solver.plot_description += ", ".join(chemical_auxiliary_variables.keys())
    riemann_solver.add_user_solver_includes(f"""#include \"../ProductionRates.h\"\n""")
    riemann_solver.add_user_solver_includes('#include "tarch/reader/CSVReader.h"\n')

    # initialize initial tendency
    riemann_solver.postprocess_updated_patch = get_initial_tendency_postprocessing()

    riemann_solver.add_solver_constants(GravityWaves.CSV_CONSTANTS)

    project = GravityWaves.create_peano_project(
        atmospheric_solver=None,
        diffusion_solver=None,
        chemical_solver=riemann_solver,
        size=size[: args.dimensions],
        arguments=args,
    )

    project.output.makefile.add_CXX_flag(f"""-DHORIZONTAL_WINDSPEED=10.5""")
    project.output.makefile.add_CXX_flag(f"""-DCHEMISTRY_NUMERICAL_THRESHOLD=1E-5""")
    project.output.makefile.add_CXX_flag(
        f"""-DEINSTEIN_BANDWIDTH=applications::exahype2::GravityWaves::Airglow::ProductionRates::{args.einstein_coefficients.upper()}BANDWIDTH"""
    )
    project.output.makefile.add_CXX_flag(
        f"""-DEINSTEIN_COEFFICIENTS=applications::exahype2::GravityWaves::Airglow::ProductionRates::{args.einstein_coefficients}EinsteinCoefficients"""
    )

    project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
