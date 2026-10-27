# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math

from pathlib import Path

import exahype2

END_TIME = 2 * math.sqrt(2)

project = exahype2.Project(
    namespace=["examples", "coupling"],
    project_name="PlanarWaves",
    dimensions=2,
    offset=[-1, -1, -1],
    size=[2, 2, 2],
    periodic_bc=[True, True, True],
    min_end_time=END_TIME,
    max_end_time=END_TIME,
    min_grid_levels=3,
    max_grid_levels=3,
    directory=str(Path(__file__).resolve().parent),
)
project.output.makefile.add_include_directory(str(Path(__file__).resolve().parents[2]))
project.add_runtime_parameter("density", "double", 1.0)
project.add_runtime_parameter("waveSpeed", "double", 1.0)

write_planar_wave = "examples::acoustic::writePlanarWave<Shortcuts>(x, {}, parameters.density, parameters.waveSpeed, {});"

interface = (
    "DomainOffset(0) + DomainSize(0) / peano4::sfc::ActiveCurve::SubdivisionFactor"
)
solvers = []
for name, precision, cells in (
    ("LeftPlanarWaves", "double", "x(0) < " + interface),
    ("RightPlanarWaves", "float", "x(0) > " + interface),
):
    solver = exahype2.dg.ader.Solver(
        name=name,
        order=3,
        unknowns={"p": 1, "u": project.dimensions},
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        linear=True,
        precision=precision,
    )
    solver.restrict_to_cells(cells)
    solver.set_pde_terms(
        max_eigenvalue="return parameters.waveSpeed;",
        flux="examples::acoustic::computeFlux<Shortcuts>(Q, normal, parameters.density, parameters.waveSpeed, F);",
        initial_condition=write_planar_wave.format("0.0", "Q"),
        analytical_solution=write_planar_wave.format("t", "solution"),
    )
    solver.add_includes("examples/acoustic/Acoustic.h", "peano4/sfc/ActiveCurve.h")
    solver.measure_error(file="Error" + name, every=END_TIME / 100)
    project.add_solver(solver)
    solvers.append(solver)
project.couple(solvers[0], solvers[1])
project.generate_and_build()
