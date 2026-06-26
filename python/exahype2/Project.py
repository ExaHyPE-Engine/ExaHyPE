# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import json

import peano4
import exahype2
import peano4.datamodel

from peano4.Logger import log

class Project(peano4.Project):
    """!
    ExaHyPE 2 project

    An ExaHyPE 2 project is a Peano 4 project (it extends peano4.Project). It adds
    the ExaHyPE solver types and the time-stepping/plotting algorithmic steps on top
    of Peano's grid machinery; everything else (domain, grid, output, build) is
    inherited.
    """

    def __init__(
        self,
        namespace,
        project_name,
        dimensions,
        offset,
        size,
        periodic_bc,
        executable="ExaHyPE",
        min_end_time=None,
        max_end_time=None,
        min_grid_levels=None,
        max_grid_levels=None,
    ):
        # The base peano4.Project applies the standard command-line arguments (-d/-m/-v)
        # and slices the domain to the resulting dimensions; we just hand over the full
        # configuration and the default build mode.
        super().__init__(
            namespace,
            project_name,
            executable,
            dimensions=dimensions,
            offset=offset,
            size=size,
            periodic_bc=periodic_bc,
            build_mode=peano4.output.CompileMode.Release,
        )

        # ExaHyPE-specific state on top of the inherited Peano project.
        self._solvers = []
        self._min_terminal_time = min_end_time
        self._max_terminal_time = max_end_time
        self._min_grid_levels = min_grid_levels
        self._max_grid_levels = max_grid_levels
        self._additional_includes = []
        self._executable_name = executable
        self._tracer_particles = []
        self._tracer_particle_sets = []

        self._particle_unknowns = 0
        self._particle_variable_names = []
        self._particle_variable_pos = [0]
        self._use_particle_var_shortcut = False

    def _create_steps(self):
        """Register ExaHyPE's steps on top of Peano's via the single uniform path.

        super() registers the generic Peano steps (CreateGrid, LoadBalancing); we add
        the ExaHyPE-specific ones the same way, in the same place. Called from the base
        constructor and from cleanup(), so a regenerate keeps the full step set.
        """
        super()._create_steps()
        self.init_grid = peano4.solversteps.Step("InitGrid", False)
        self.solversteps.add_step(self.init_grid)
        self.plot_solution = peano4.solversteps.Step("PlotSolution", False)
        self.solversteps.add_step(self.plot_solution)
        self.perform_time_step = peano4.solversteps.Step("TimeStep", False)
        self.solversteps.add_step(self.perform_time_step)

    def add_solver(self, solver):
        self._solvers.append(solver)

    def remove_all_solvers(self):
        self._solvers = []
        self.cleanup()

    def __export_constants(self):
        """
        We export ExaHyPE's constants. Besides the constants from ExaHyPE,
        we also export some parameters from Peano onto the ExaHyPE constants
        file. Therefore, it is important that you parse the configure output
        before we export the constants.
        """

        self.output.readme.add_package_description(
            """

### ExaHyPE 2

This code uses ExaHyPE in its second generation. The first generation of the
code was developed through an EU FET HPC project called ExaHyPE. This first
generation was built on top of Peano in its third generation. The present
code uses ExaHyPE 2 which is a complete rewrite built on top of Peano 4. We
do not yet have a release paper for this second generation of ExaHyPE, and
thus appreciate any citation of the original release paper

       @article{Reinarz:2020:ExaHyPE,
         title = {ExaHyPE: An engine for parallel dynamically adaptive simulations of wave problems},
         journal = {Computer Physics Communications},
         volume = {254},
         pages = {107251},
         year = {2020},
         issn = {0010-4655},
         doi = {https://doi.org/10.1016/j.cpc.2020.107251},
         url = {https://www.sciencedirect.com/science/article/pii/S001046552030076X},
         author = {Anne Reinarz and Dominic E. Charrier and Michael Bader and Luke Bovard and Michael Dumbser and Kenneth Duru and Francesco Fambri and Alice-Agnes Gabriel and Jean-Matthieu Gallard and Sven K\"oppel and Lukas Krenz and Leonhard Rannabauer and Luciano Rezzolla and Philipp Samfass and Maurizio Tavelli and Tobias Weinzierl},
         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X},
         abstract = {ExaHyPE (An Exascale Hyperbolic PDE Engine) is a software engine for solving systems of first-order hyperbolic partial differential equations (PDEs). Hyperbolic PDEs are typically derived from the conservation laws of physics and are useful in a wide range of application areas. Applications powered by ExaHyPE can be run on a student's laptop, but are also able to exploit thousands of processor cores on state-of-the-art supercomputers. The engine is able to dynamically increase the accuracy of the simulation using adaptive mesh refinement where required. Due to the robustness and shock capturing abilities of ExaHyPE's numerical methods, users of the engine can simulate linear and non-linear hyperbolic PDEs with very high accuracy. Users can tailor the engine to their particular PDE by specifying evolved quantities, fluxes, and source terms. A complete simulation code for a new hyperbolic PDE can often be realised within a few hours - a task that, traditionally, can take weeks, months, often years for researchers starting from scratch. In this paper, we showcase ExaHyPE's workflow and capabilities through real-world scenarios from our two main application areas: seismology and astrophysics.
           Program summary
           Program title: ExaHyPE-Engine Program Files doi: http://dx.doi.org/10.17632/6sz8h6hnpz.1 Licensing provisions: BSD 3-clause Programming languages: C++, Python, Fortran Nature of Problem: The ExaHyPE PDE engine offers robust algorithms to solve linear and non-linear hyperbolic systems of PDEs written in first order form. The systems may contain both conservative and non-conservative terms. Solution method: ExaHyPE employs the discontinuous Galerkin (DG) method combined with explicit one-step ADER (arbitrary high-order derivative) time-stepping. An a-posteriori limiting approach is applied to the ADER-DG solution, whereby spurious solutions are discarded and recomputed with a robust, patch-based finite volume scheme. ExaHyPE uses dynamical adaptive mesh refinement to enhance the accuracy of the solution around shock waves, complex geometries, and interesting features.
         }
       }
"""
        )

        for i in self._solvers:
            self.output.readme.add_entry(
                i.create_readme_descriptor(self._domain_offset[0], self._domain_size[0])
            )
    @property
    def domain_size(self):
        return self._domain_size[0 : self._dimensions]

    def add_mainfile_include(self, value):
        self._additional_includes.append(value)

    def __set_solver_repository_dict(self):
        project_tracers = []
        for particle in self._tracer_particles:
            if particle.name not in project_tracers:
                project_tracers.append(particle.name)

        tracers = list(project_tracers)
        for solver in self._solvers:
            for particle in getattr(solver, "_tracer_particles", []):
                if particle.name not in tracers:
                    tracers.append(particle.name)

        self.solverRepositoryDictionary = {
            "SOLVERS":                  [],
            "TRACERS":                  tracers,
            "PROJECT_TRACERS":          project_tracers,
            "ADDITIONAL_INCLUDES":      self._additional_includes,
        }

    @staticmethod
    def __normalise_solver_statistics_metadata(solver):
        result = []
        if hasattr(solver, "get_solver_statistics_metadata"):
            for label, value in solver.get_solver_statistics_metadata():
                label = str(label)
                value = str(value)
                result.append((label, value))
        return result

    def __generate_solver_repository(self):
        """
        I have to call finishedTraversal() for each tracer set. As tracers
        are a subclass of DoF, they have a name property.
        """

        self.__set_solver_repository_dict()

        for solver in self._solvers:
            solver_tracers = []
            for particle in getattr(solver, "_tracer_particles", []):
                if particle.name not in solver_tracers:
                    solver_tracers.append(particle.name)
            self.solverRepositoryDictionary["SOLVERS"].append(
                (
                    solver._name,
                    solver.get_name_of_global_instance(),
                    solver_tracers,
                    self.__normalise_solver_statistics_metadata(solver),
                )
            )

        templatefile_prefix = os.path.splitext(os.path.realpath(__file__))[0]
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + "SolverRepository.template.h",
                templatefile_prefix + "SolverRepository.template.cpp",
                "SolverRepository",
                self.namespace + ["repositories"],
                "repositories",
                self.solverRepositoryDictionary,
            )
        )

        self.output.add(generated_solver_files)
        self.output.makefile.add_cpp_file(
            "repositories/SolverRepository.cpp", generated=True
        )

    @staticmethod
    def _render_optional_runtime_default(value):
        return "-1.0" if value is None else str(value)

    @staticmethod
    def _render_optional_integer_runtime_default(value):
        return "-1" if value is None else str(value)

    def _configure_runner(self, main):
        """ExaHyPE contributes its specifics into the otherwise generic Peano main:
        the per-step solver hooks (baked into the Step bodies so they run on every
        rank), the solver-repository callbacks and the extra includes. The schedule
        itself is the C++ runner::Orchestrator, which finds these steps by name."""
        construction_start = "repositories::startGridConstructionStep();"
        construction_finish = "repositories::finishGridConstructionStep();"
        self.create_grid.runner_start_hook = construction_start
        self.create_grid.runner_finish_hook = construction_finish

        # Freeze load balancing once the grid has been initialised (static AMR).
        self.init_grid.runner_start_hook = (
            "::runner::loadBalancer().enable(false); repositories::startGridInitialisationStep();"
        )
        self.init_grid.runner_finish_hook = "repositories::finishGridInitialisationStep();"
        self.plot_solution.runner_start_hook = "repositories::startPlottingStep();"
        self.plot_solution.runner_finish_hook = "repositories::finishPlottingStep();"
        self.perform_time_step.runner_start_hook = "repositories::startTimeStep();"
        self.perform_time_step.runner_finish_hook = "repositories::finishTimeStep();"

        main.extra_includes = (
            '#include "repositories/SolverRepository.h"\n'
            '#include "runner/LoadBalancer.h"'
        )

        main.extra_repository_callbacks = (
            "    result.startSimulation     = []() { repositories::startSimulation(); };\n"
            "    result.finishSimulation    = []() { repositories::finishSimulation(); };\n"
            "    result.mayPlot             = []() { return repositories::mayPlot(); };\n"
            "    result.getMinTimeStamp     = []() { return repositories::getMinTimeStamp(); };\n"
            "    result.getMaxTimeStamp     = []() { return repositories::getMaxTimeStamp(); };\n"
            "    result.getNumberOfDoFs     = []() { return repositories::getNumberOfDoFs(); };\n"
            "    result.getSolverStatistics = []() { return repositories::getSolverStatistics(); };\n"
            "    result.getConfigurationSummary = []() { return repositories::getConfigurationSummary(); };\n"
            '    result.getCopyrightBanner = []() { return std::string("ExaHyPE 2 (C) www.exahype.org"); };'
        )

    def _init_main_template_dictionary(self, main):
        main.d.update(
            {
                "DEFAULT_MIN_TERMINAL_TIME": self._render_optional_runtime_default(
                    self._min_terminal_time
                ),
                "DEFAULT_MAX_TERMINAL_TIME": self._render_optional_runtime_default(
                    self._max_terminal_time
                ),
                "DEFAULT_GRID_LEVELS": self._render_optional_integer_runtime_default(
                    self._min_grid_levels
                ),
                "DEFAULT_MAX_GRID_LEVELS": self._render_optional_integer_runtime_default(
                    self._max_grid_levels
                ),
                "EXECUTABLE_NAME": json.dumps(self._executable_name),
            }
        )


    def __add_solver_implementation_files(self, solver):
        solver.add_implementation_files_to_project(
            self.namespace,
            self.output,
            self._dimensions,
        )

    # TODO: Move to solvers
    def add_tracer(
        self,
        name,
        particle_attributes,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        particle = peano4.toolbox.particles.Particle(name)
        if isinstance(particle_attributes, dict):
            self._particle_unknowns = sum(particle_attributes["unknowns"].values())
            self._particle_variable_names += list(particle_attributes["unknowns"].keys())

            for var in particle_attributes["unknowns"].values():
                self._particle_variable_pos.append(var + self._particle_variable_pos[-1])

            self._use_particle_var_shortcut = True
            self._generate_particle_shortcuts(name)

        elif isinstance(particle_attributes, int):
            self._particle_unknowns = particle_attributes

        else:
            raise Exception(
                "Invalid format for 'unknowns'. Must be an integer or a dictionary."
            )

        particle_attr = peano4.datamodel.DoubleArray("data", str(self._particle_unknowns))
        particle.add_attribute(particle_attr)

        particle.add_attribute(
            peano4.datamodel.Enumeration("moveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.datamodel.IntegerArray("number", "2")
        particle.add_attribute(particle_number)

        self.datamodel.add_global_object(particle)
        particles = peano4.toolbox.particles.ParticleSet(particle)
        self.datamodel.add_vertex(particles)

        self.plot_solution.use_vertex(particles)
        self.init_grid.use_vertex(particles)
        self.perform_time_step.use_vertex(particles)
        self.create_grid.use_vertex(particles)
        self.load_balancing.use_vertex(particles)

        #
        # Initialisation
        #
        self.init_grid.add_action_set(
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(particles)
        )

        update_particle_association_action_set = (
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(particles)
        )
        update_particle_association_action_set.descend_invocation_order = sort_descend_invocation_order_within_action_sets

        update_particle_parallel_state_action_set = (
            peano4.toolbox.particles.api.UpdateParallelState(particles)
        )
        update_particle_parallel_state_action_set.descend_invocation_order = (
            sort_descend_invocation_order_within_action_sets
        )

        #
        # Move particles
        #
        self.perform_time_step.add_action_set(update_particle_association_action_set)
        self.perform_time_step.add_action_set(update_particle_parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def _generate_particle_shortcuts(self, name):
        generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
            os.path.join(os.path.dirname(os.path.realpath(__file__)), "VariableShortcuts.template.h"),
            "VariableShortcuts" + name,
            self.namespace,
            ".",
            {
                "NUMBER_OF_VARIABLES": len(self._particle_variable_names),  # Standardised naming
                "VARIABLE_NAMES": self._particle_variable_names,
                "VARIABLE_POSITIONS": self._particle_variable_pos,
                "SOLVER_NAME": name,
            },
            True,
        )
        self.output.add(generated_shortcut_file)
        self.output.makefile.add_h_file("VariableShortcuts"+ name + ".h", generated=True)

    def add_action_set_to_timestepping(self, action_set):
        """!

        Add a new action set to the time stepping

        You have to be careful with the priorities, i.e. you might want to
        use myproject.perform_time_step.max_priority() or myproject.perform_time_step.min_priority()
        to find out which
        priority you should assign to action_set.

        """
        self.perform_time_step.add_action_set(action_set)

    def add_action_set_to_plot_solution(self, action_set):
        """!

        Add a new action set to the plotting

        """
        self.plot_solution.add_action_set(action_set)

    def add_action_set_to_initialisation(self, action_set):
        self.init_grid.add_action_set(action_set)

    def add_action_set_to_create_grid(self, action_set):
        """!

        Add an action set to create grid

        This routine actually adds an action set to each and every grid
        creation routine. After all, we distinguish three flavours of it

        - with refinement;
        - without refinement as we've just refined or split up domain and to let load balancing converge.

        """
        self.create_grid.add_action_set(action_set)

    def __add_solver_label_action_sets_to_load_balancing(self, solver):
        """Load balancing still traverses the mesh and can create labels on new entities."""
         # TODO: Remove
        if hasattr(solver, "_action_set_update_face_label"):
            self.load_balancing.add_action_set(solver._action_set_update_face_label)
        if hasattr(solver, "_action_set_update_cell_label"):
            self.load_balancing.add_action_set(solver._action_set_update_cell_label)

    def generate_and_build(self):
        """! Generate the ExaHyPE-specific code, then generate and build the Peano project.

        ExaHyPE extends Peano, so this adds the solver data model, the solver
        action sets and the time-stepping/plotting steps, the solver repository and
        the runtime parameters, configures the runner and the main, and then defers
        to peano4.Project.generate_and_build() for the actual file generation + build.
        """
        # Solver code generation reads the build configuration (e.g. SIMD width),
        # so parse it now. The base generate_and_build() will not parse it again.
        self.output.makefile.parse_configuration_outcome()
        self._makefile_configured = True
        self.__export_constants()

        # Every step is already registered uniformly in _create_steps(); here we only
        # inject each solver's data model and action sets into the relevant steps.
        solver_steps = (
            self.create_grid,
            self.load_balancing,
            self.plot_solution,
            self.perform_time_step,
            self.init_grid,
        )
        for solver in self._solvers:
            log.debug("Prepare solver %s", solver._name)
            log.debug("%s", solver)

            solver.add_to_Peano4_datamodel(self.datamodel)

            for step in solver_steps:
                solver.add_use_data_statements_to_Peano4_solver_step(step)

            solver.add_actions_to_create_grid(self.create_grid)
            self.__add_solver_label_action_sets_to_load_balancing(solver)

            solver.add_actions_to_init_grid(self.init_grid)
            solver.add_actions_to_plot_solution(self.plot_solution, "./")

            solver.add_actions_to_perform_time_step(self.perform_time_step)

            self.__add_solver_implementation_files(solver)

        self.__generate_solver_repository()

        self._configure_runner(self.main)
        self._init_main_template_dictionary(self.main)

        super().generate_and_build()


    def init_new_user_defined_algorithmic_step(self,
                                               step,
                                               ):
        """! Inform ExaHyPE 2 that there is an additional solver step not resulting from vanilla solver versions

        Use this routine whenever you have created your own
        @ref page_architecture "algorithmic step". Please note that the routine
        should be called after you have created all of your solvers. Otherwise,
        we'll miss out a few of your solvers and the data access patterns
        will/might be corrupted. Also all tracers have to be added at this
        point already.

        The routine has no side-effects, i.e. does not alter the underlying
        project. It only manipulates the step. You still have to add the new step
        manually to the project: this routine merely initialises the new step, but
        it does not yet add it to the sweeps known by Peano.

        @param step: peano4.solversteps.Step

        """
        for solver in self._solvers:
            solver.add_use_data_statements_to_Peano4_solver_step(step)

        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)

    @property
    def additional_includes(self):
        return self._additional_includes
