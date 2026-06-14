import argparse
import sys
import os

import exahype2

sys.path.insert(0, os.path.abspath(".."))

from EulerPDE import get_flux, get_max_eigenvalue
from EulerFWave import fWave

from DiffusionPDE import stencil
import GravityWaves

"""This application is based on:

"Breaking of thunderstorm-generated gravity waves as a source of short-period ducted waves at mesopause altitudes", Snively and Pasko, 2003, doi:10.1029/2003GL018436
"""

if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-et",
        "--end-time",
        type=float,
        default=3,  # end time given in Snively and Pasko: 16000
    )
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=5)
    parser.add_argument(
        "-csv",
        "--csv-file-path",
        default="./thunderstorm_model.csv",
        type=str,
        help="Path to CSV file for use as initial conditions for this project.",
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()

    # adapt to [1_200e3, 180e3, 1_200e3] for reduced model
    size = [1_200e3, 220e3, 1_200e3]
    project = exahype2.Project(
        namespace=["applications", "exahype2", "GravityWaves"],
        project_name="GravityWaves",
        dimensions=2,
        offset=[0, 0, 0],
        size=size,
        periodic_BC=[False, False, False],
        directory=".",
        executable="ExaHyPE",
        min_end_time=args.end_time,
        max_end_time=args.end_time,
        min_grid_levels=args.grid_levels,
        max_grid_levels=args.grid_levels,
    )
    dimensions = project.dimensions

    #####################
    # ATMOSPHERE SOLVER #
    #####################

    atmosphere_initial_conditions = (
        GravityWaves.get_CSV_initializer(
            csv_file_path=args.csv_file_path,
            initialize_density=True,
            initialize_hydrogen=False,
        )
        + """
      // momenta are in m/s * kg/m^3 = kg/(m^2 * s)
      Q[Shortcuts::rhou]  = HORIZONTAL_WINDSPEED * Q[Shortcuts::rho];
      Q[Shortcuts::rhov]  = 0.0;
      #if DIMENSIONS == 3
      Q[Shortcuts::rhow]  = 0.0;
      #endif

      const auto M                  = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
      const auto iM                 = 1.0 / M;
      const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol
      const auto pressure           = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * Q[Shortcuts::T] / meanMolecularMass;
      #if !defined(WITH_GPU)
      assertion5(pressure > 0.0, M, meanMolecularMass, Q[Shortcuts::rho], Q[Shortcuts::T], pressure);
      #endif
    
      const auto gamma = (1.4 * (M - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) * iM;

      Q[Shortcuts::E] = pressure / (gamma - 1) + 0.5 * Q[Shortcuts::rho] * (HORIZONTAL_WINDSPEED * HORIZONTAL_WINDSPEED);

      Q[Shortcuts::backgroundDensity] = Q[Shortcuts::rho];
      Q[Shortcuts::backgroundPressure] = pressure;"""
    )

    atmosphere_boundary_conditions = """
      // normal == 0 => left
      // normal == 1 => bottom
      // normal == 2 => right 
      // normal == 3 => top

      if (normal == 0 || normal == 2) {
        // outflow at sides
        for (int unknown = 0; unknown < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++unknown) {
          Qoutside[unknown] = Qinside[unknown];
        }
      } else if (normal == 1 || normal == 3) {
        const auto M                  = Qinside[Shortcuts::O2] + Qinside[Shortcuts::O] + Qinside[Shortcuts::N2];
        const auto iM                 = 1.0 / M;
        const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Qinside[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Qinside[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Qinside[Shortcuts::O]) * iM; // in kg/mol
        const auto pressure           = Qinside[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * Qinside[Shortcuts::T] / meanMolecularMass;
        #if !defined(WITH_GPU)
        assertion5(pressure > 0.0, M, meanMolecularMass, Qinside[Shortcuts::rho], Qinside[Shortcuts::T], pressure);
        #endif
      
        const auto gamma  = (1.4 * (M - Qinside[Shortcuts::O]) + 1.67 * Qinside[Shortcuts::O]) * iM;

        // positive dy upwards, negative dz downwards
        const auto dy = normal == 3 ? h(1) : -h(1);

        const auto specificGasConstant = UNIVERSAL_GAS_CONSTANT / meanMolecularMass;
        const auto scaleHeight = specificGasConstant * Qinside[Shortcuts::T] / GRAVITY;
        const auto exponentialDistribution = tarch::la::pow(tarch::la::E, -dy / scaleHeight);

        const auto densityOutside   = Qinside[Shortcuts::rho] * exponentialDistribution;
        const auto pressureOutside  = pressure * exponentialDistribution;

        Qoutside[Shortcuts::rho] = densityOutside;
        const auto u = Qinside[Shortcuts::rhou] / Qinside[Shortcuts::rho];
        const auto v = Qinside[Shortcuts::rhov] / Qinside[Shortcuts::rho];
        #if DIMENSIONS == 3
        const auto w = Qinside[Shortcuts::rhow] / Qinside[Shortcuts::rho];
        const auto vSq = u * u + v * v + w * w;
        #else
        const auto vSq = u * u + v * v;
        #endif

        Qoutside[Shortcuts::E] = pressureOutside / (gamma - 1) + 0.5 * densityOutside * vSq;

        if (normal == 1) {
          // Reflective at bottom to keep atmosphere
          // from escaping the domain through the Earth's surface
          Qoutside[Shortcuts::rhou] = u * densityOutside;
          Qoutside[Shortcuts::rhov] = -v * densityOutside;
          #if DIMENSIONS == 3
          Qoutside[Shortcuts::rhow] = w * densityOutside;
          #endif
        } else {
          // Outflow at top
          Qoutside[Shortcuts::rhou] = u * densityOutside;
          Qoutside[Shortcuts::rhov] = v * densityOutside;
          #if DIMENSIONS == 3
          Qoutside[Shortcuts::rhow] = w * densityOutside;
          #endif
        }

        for (int auxiliaryVariable = 0; auxiliaryVariable < NumberOfAuxiliaryVariables; ++auxiliaryVariable) {
          Qoutside[NumberOfUnknowns + auxiliaryVariable] = Qinside[NumberOfUnknowns + auxiliaryVariable];
        }

        Qoutside[Shortcuts::backgroundDensity] = densityOutside;
        Qoutside[Shortcuts::backgroundPressure] = pressureOutside;
      }"""

    atmosphere_unknowns = {
        "rho": 1,
        "rhou": 1,
        "rhov": 1,
        "E": 1,
    }

    if dimensions == 3:
        atmosphere_unknowns["rhow"] = 1

    atmosphere_auxiliary_variables = {
        "T": 1,
        "O": 1,
        "O2": 1,
        "N2": 1,
        "backgroundDensity": 1,
        "backgroundPressure": 1,
    }

    atmosphere_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="FVSolver",
        patch_size=args.degrees_of_freedom,
        unknowns=atmosphere_unknowns,
        auxiliary_variables=atmosphere_auxiliary_variables,
        time_step_relaxation=0.5,
    )

    atmosphere_solver.set_implementation(
        initial_conditions=atmosphere_initial_conditions,
        boundary_conditions=atmosphere_boundary_conditions,
        flux=get_flux(),
        riemann_solver=fWave,
        max_eigenvalue=get_max_eigenvalue(),
    )

    atmosphere_solver.add_user_solver_includes('#include "tarch/reader/CSVReader.h"\n')

    atmosphere_solver.set_implementation(
        source_term="""for(int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
          S[unknown] = 0.0;
      }

      applications::exahype2::GravityWaves::Thunderstorm::ForcingTerms::scenario(Q, x, h, t, S);"""
    )

    atmosphere_solver.add_user_solver_includes('#include "ForcingTerms.h"')
    atmosphere_solver.add_solver_constants(GravityWaves.CSV_CONSTANTS)

    ####################
    # DIFFUSION SOLVER #
    ####################

    diffusion_unknowns = {
        "u": 1,
        "v": 1,
    }

    if dimensions == 3:
        diffusion_unknowns["w"] = 1

    diffusion_unknowns["T"] = 1

    diffusion_auxiliary_variables = {
        "nu": 1,
        "kappa": 1,
        "rho": 1,
        "E": 1,
        "O": 1,
        "O2": 1,
        "N2": 1,
        "backgroundU": 1,
        "backgroundV": 1,
    }

    if dimensions == 3:
        diffusion_auxiliary_variables["backgroundW"] = 1
    diffusion_auxiliary_variables["backgroundT"] = 1

    diffusion_initial_conditions = (
        GravityWaves.get_CSV_initializer(
            csv_file_path=args.csv_file_path,
            initialize_density=True,
            initialize_hydrogen=False,
        )
        + """
      // "Doppler ducting of short-period gravity waves by midlatitude tidal wind structure", Snively et al., 2007
      // exponential function between (110km, 100 m^2/s) and (150km, 5000 m^2/s)
      // 5000 m^2/s implied in Thermosphere Ducted Gravity Waves but fully stated in Doppler Ducting
      auto viscosityFunction = [](const double altitude) {
        if(tarch::la::smaller(altitude, 110'000.0)) {
          return 100.0;
        }

        return 0.00212732 * tarch::la::pow(tarch::la::E, 0.0000978006 * altitude);
      };

      Q[Shortcuts::u]           = HORIZONTAL_WINDSPEED;
      Q[Shortcuts::backgroundU] = Q[Shortcuts::u];

      Q[Shortcuts::v]           = 0.0;
      Q[Shortcuts::backgroundV] = 0.0;

      #if DIMENSIONS == 3
      Q[Shortcuts::w]           = 0.0;
      Q[Shortcuts::backgroundW] = 0.0;
      #endif

      Q[Shortcuts::nu]          = viscosityFunction(x(1));
      Q[Shortcuts::kappa]       = 1.5 * Q[Shortcuts::nu];

      const auto M                  = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
      const auto iM                 = 1.0 / M;
      const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol
      const auto pressure           = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * Q[Shortcuts::T] / meanMolecularMass;
      const auto gamma              = (1.4 * (M - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) * iM;

      Q[Shortcuts::E]           = pressure / (gamma - 1) + 0.5 * Q[Shortcuts::rho] * (HORIZONTAL_WINDSPEED * HORIZONTAL_WINDSPEED);
      Q[Shortcuts::backgroundT] = Q[Shortcuts::T];"""
    )

    diffusion_boundary_conditions = """
      // normal == 0 => left
      // normal == 1 => bottom
      // normal == 2 => right 
      // normal == 3 => top

      for (int unknown = 0; unknown < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++unknown) {
        Qoutside0[unknown] = Qinside[unknown];
      }

      if (normal == 1) {
        // Reflective at bottom to keep atmosphere
        // from escaping the domain through the Earth's surface
        Qoutside0[Shortcuts::u] = Qinside[Shortcuts::u];
        Qoutside0[Shortcuts::v] = -Qinside[Shortcuts::v];
        #if DIMENSIONS == 3
        Qoutside0[Shortcuts::w] = Qinside[Shortcuts::w];
        #endif
      }"""

    diffusion_solver = exahype2.solvers.fd.GlobalAdaptiveTimeStep(
        name="FDSolver",
        patch_size=args.degrees_of_freedom,
        unknowns=diffusion_unknowns,
        auxiliary_variables=diffusion_auxiliary_variables,
        time_step_relaxation=0.5,
        order=1,
        solvers_before_in_coupling=[atmosphere_solver],
    )

    diffusion_solver.set_implementation(
        initial_conditions=diffusion_initial_conditions,
        boundary_conditions=diffusion_boundary_conditions,
        max_eigenvalue="return 1.0;",
        solver=stencil,
    )

    diffusion_solver.add_user_solver_includes('#include "tarch/reader/CSVReader.h"\n')
    diffusion_solver.add_solver_constants(GravityWaves.CSV_CONSTANTS)

    #################
    # PEANO PROJECT #
    #################

    project = GravityWaves.create_peano_project(
        project=project,
        atmospheric_solver=atmosphere_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=None,
    )

    project.output.makefile.add_cpp_file("ForcingTerms.cpph")

    project.output.makefile.add_CXX_flag(f"""-DDENSITY_THRESHOLD=1E-9""")
    project.output.makefile.add_CXX_flag(f"""-DPRESSURE_THRESHOLD=1E-9""")
    project.output.makefile.add_CXX_flag(f"""-DTEMPERATURE_THRESHOLD=1E-9""")
    project.output.makefile.add_CXX_flag(f"""-DVELOCITY_THRESHOLD=1E-9""")
    project.output.makefile.add_CXX_flag(f"""-DHORIZONTAL_WINDSPEED=0.0""")

    project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
