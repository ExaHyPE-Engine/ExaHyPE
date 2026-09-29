import argparse
import exahype2
import Elastic

def compile_HHS1(
        order=5,
        precision="fp32",
        dimensions=2
    ):

    use_tracers         = True

    size    = [12.0, 12.0, 12.0]  # thousand meters
    offset  = [-2.0, 0.,  -2.0]  # thousand meters

    unknowns            = {"v": 3, "sigma": 6}
    auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}

    project = exahype2.Project(
        ["applications", "exahype2", "exaseis"],
        "Cartesian",
        dimensions=dimensions,
        offset=offset,
        size=size,
        periodic_bc=[False, False, False],
        executable="ExaSeis",
        min_end_time=2.0,
        max_end_time=2.0,
        min_grid_levels=3,
        max_grid_levels=3,
    )

    theSolver = exahype2.dg.ader.Solver(
        name="elastic",  order=order,
        unknowns=unknowns, auxiliary_variables=auxiliary_variables,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        linear=True,
        precision=precision,
        point_sources=1
    )

    theSolver.set_pde_terms(
        initial_condition           = Elastic.initial(),
        boundary_conditions         = Elastic.boundary(),
        max_eigenvalue              = Elastic.eigenvalue(),
        flux                        = Elastic.flux(),
        refinement_criterion        = Elastic.refinement_criterion(),
        riemann_solver              = Elastic.riemann_solver(),
        point_source                = Elastic.point_source(),
        point_source_locations      = True
    )

    theSolver.add_includes(
        "../../exaseis/riemannSolvers/cartesianRoutines.h",
        "../../exaseis/riemannSolvers/riemannsolverIsotropic.h"
    )

    project.add_solver(theSolver)

    if use_tracers:
        tracer_particles = project.add_tracer(name="Tracer", particle_attributes=12)

        project.add_action_set_to_initialisation(
            exahype2.particles.InsertTracersByCoordinates(
                particle_set=tracer_particles,
                coordinates=[
                    [0.000, 0., 0.693],
                    # [0.000, 0., 5.543], [0.000, 0., 10.392],
                    [0.490, 0., 0.490], [3.919, 0., 3.919],
                    # [7.348, 0., 7.3480],
                    [0.577, 0., 0.384],
                    # [4.612, 0., 3.075], [8.647, 0., 5.7640]
                ]
            )
        )

    theSolver.trace(
        tracer_particles,
        kernel="::exahype2::dg::projectAllValuesOntoParticle",
        initialisation=False,
    )

    project.add_action_set_to_timestepping(
        exahype2.particles.DumpTracerIntoDatabase(
            particle_set=tracer_particles,
            solver=theSolver,
            filename="elastic",
            time_delta_between_two_snapshots=0.01,
            output_precision=10,
        )
    )

    project.add_runtime_parameter("pointSourcePosition", ("double", dimensions), [0., 2.0, 0.][0:dimensions])
    project.output.makefile.add_cpp_file("elastic.cpp")
    project.generate_and_build()


compile_HHS1(
    order=5,
    precision="fp32",
    dimensions=2
)