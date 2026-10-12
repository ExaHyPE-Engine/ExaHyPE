# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
from pathlib import Path

import exahype2

HERE = Path(__file__).resolve().parent


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=10.0)
    parser.add_argument("--cfl", type=float, default=0.5, help="CFL factor of the adaptive time step.")
    parser.add_argument(
        "--dry-threshold", type=float, default=1e-5,
        help="Water height below which a state counts as dry (hThreshold).",
    )
    parser.add_argument(
        "--dg-wet-height", type=float, default=1e-3,
        help="Water height above which every node of a DG cell must lie for the cell to skip the limiter.",
    )
    parser.add_argument("--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=5)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=5)
    parser.add_argument(
        "--solver",
        choices=["aderdg", "ader-weno"],
        default="aderdg",
        help="ADER-DG with a subcell limiter, or a finite-volume ADER-WENO solver.",
    )
    parser.add_argument(
        "--limiter",
        choices=["godunov", "muscl-hancock", "weno"],
        default="godunov",
        help="Subcell limiter of the ADER-DG solver.",
    )
    parser.add_argument(
        "--weno-order", type=int, choices=[2, 3, 4, 5], default=3,
        help="Polynomial degree of ADER-WENO, as limiter or as solver.",
    )
    parser.add_argument(
        "--patch-size", type=int, default=None,
        help="Volumes per patch axis of the ADER-WENO solver (default: the DG nodes per axis).",
    )
    parser.add_argument(
        "--polynomial-scaling", action="store_true",
        help="Scale every ADER-WENO polynomial towards its average until it is admissible.",
    )
    parser.add_argument(
        "--no-fallback", action="store_true",
        help="ADER-WENO without the a-posteriori fallback to the Godunov scheme.",
    )
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
dg_order = max(1, (args.degrees_of_freedom - 1) // 2)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="OscillatingLake",
    dimensions=2,
    offset=[-2.0, -2.0],
    size=[4.0, 4.0],
    periodic_bc=[False, False],
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The oscillating-lake application is two-dimensional.")

weno = exahype2.fv.Weno(
    order=args.weno_order,
    reconstruction_limiter=(
        exahype2.fv.PolynomialLimiter.admissibility_scaling
        if args.polynomial_scaling
        else exahype2.fv.PolynomialLimiter.none
    ),
    fallback=None if args.no_fallback else exahype2.fv.Fallback(),
)

if args.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="ADERDGSolver",
        order=dg_order,
        unknowns={"h": 1, "hu": project.dimensions, "b": 1},
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(args.cfl),
        basis=exahype2.dg.GaussLobatto,
        linear=False,
    )
    solver.set_pde_terms(
        initial_condition=True,
        boundary_conditions=True,
        refinement_criterion=True,
        flux=True,
        max_eigenvalue=True,
        ncp=True,
        riemann_solver=True,
    )
    solver.set_pde_terms(
        subcell_riemann_solver=True,
        physical_admissibility=True,
    )
    solver.add_subcell_limiter(
        {
            "godunov": exahype2.fv.Godunov(),
            "muscl-hancock": exahype2.fv.MusclHancock(),
            "weno": weno,
        }[args.limiter]
    )
    implementation = "ADERDGSolver.cpph"
else:
    solver = exahype2.fv.ader.Solver(
        name="FVSolver",
        patch_size=args.patch_size if args.patch_size is not None else dg_order + 1,
        unknowns={"h": 1, "hu": project.dimensions, "b": 1},
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(args.cfl),
        scheme=weno,
    )
    solver.set_pde_terms(
        initial_condition=True,
        boundary_conditions=True,
        refinement_criterion=True,
        flux=True,
        max_eigenvalue=True,
        ncp=True,
        riemann_solver=True,
        physical_admissibility=True,
    )
    implementation = "FVSolver.cpph"

solver.set_pde_terms(analytical_solution=True)
solver.measure_error(every=args.end_time / 100.0)

project.add_solver(solver)

project.add_constant("g", 9.81)
project.add_constant("hThreshold", args.dry_threshold)
project.add_constant("DGWetHeight", args.dg_wet_height)
project.add_constant("LakeRadius", 1.0)
project.add_constant("ReferenceDepth", 0.1)
project.add_constant("OrbitDisplacement", 0.5)

project.output.makefile.add_include_directory(str(HERE.parents[3]))
project.output.makefile.add_h_file(implementation)
project.generate_and_build()
