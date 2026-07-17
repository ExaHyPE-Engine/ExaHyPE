# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel

from peano4.Logger import log


class Project(peano4.Project):
    """Extend a Peano project with ExaHyPE solvers and algorithmic steps.

    Domain, grid, output, and build configuration remain inherited from
    ``peano4.Project``.
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

        self._solvers = []
        self._solver_set_generated = False
        self._min_terminal_time = min_end_time
        self._max_terminal_time = max_end_time
        self._min_grid_levels = min_grid_levels
        self._max_grid_levels = max_grid_levels
        self._additional_includes = []
        self._tracer_particles = []
        self._tracer_particle_sets = []

        self._particle_unknowns = 0
        self._particle_variable_names = []
        self._particle_variable_positions = [0]

    def set_plot_format(self, file_format):
        """Set the default format for solver plotters without an explicit format.

        The command-line plot format overrides this value. ``None`` preserves
        the current default.
        """
        if file_format is not None:
            import peano4.plotter

            peano4.plotter.set_default_format(file_format)

    def _create_steps(self):
        """Register the complete Peano and ExaHyPE step sequence.

        The base call installs ``CreateGrid`` and ``LoadBalancing`` before this
        method adds ``InitGrid``, ``PlotSolution``, and ``TimeStep``.
        """
        super()._create_steps()
        self.init_grid = peano4.solversteps.Step("InitGrid", False)
        self.solversteps.add_step(self.init_grid)
        self.plot_solution = peano4.solversteps.Step("PlotSolution", False)
        self.solversteps.add_step(self.plot_solution)
        self.perform_time_step = peano4.solversteps.Step("TimeStep", False)
        self.solversteps.add_step(self.perform_time_step)

    def add_solver(self, solver):
        """Register a solver and recognise repeated solver declarations.

        Consecutive unique calls build one solver set. A generated project or
        duplicate name starts a new declaration and clears the preceding
        solvers' implementation files, so repeated configurations do not
        accumulate variants.
        """
        redeclaration = self._solver_set_generated or any(
            existing._name == solver._name for existing in self._solvers
        )
        if redeclaration:
            self._solvers = []
            self.output.makefile.clear_user_files()
            self._solver_set_generated = False
        self._solvers.append(solver)

    def remove_all_solvers(self):
        """Remove all registered solvers and implementation files.

        Use this explicit reset when a configuration contains no solver;
        ``add_solver`` handles solver redeclarations automatically.
        """
        self._solvers = []
        self.output.makefile.clear_user_files()
        self._solver_set_generated = False

    def __add_readme_entries(self):
        """Add ExaHyPE package and solver metadata to the generated README."""

        self.output.readme.add_package_description(
            """

### ExaHyPE 2

ExaHyPE 2 is the second-generation ExaHyPE engine built on Peano 4. Please cite
the ExaHyPE release paper:

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

        for solver in self._solvers:
            self.output.readme.add_entry(
                solver.create_readme_descriptor(
                    self._domain_offset[0], self._domain_size[0]
                )
            )

    @property
    def domain_size(self):
        return self._domain_size[0 : self._dimensions]

    def add_mainfile_include(self, include):
        self._additional_includes.append(include)

    def __set_solver_repository_dictionary(self):
        project_tracers = []
        for particle in self._tracer_particles:
            if particle.name not in project_tracers:
                project_tracers.append(particle.name)

        tracers = list(project_tracers)
        for solver in self._solvers:
            for particle in getattr(solver, "_tracer_particles", []):
                if particle.name not in tracers:
                    tracers.append(particle.name)

        self._solver_repository_dictionary = {
            "SOLVERS": [],
            "TRACERS": tracers,
            "PROJECT_TRACERS": project_tracers,
            "ADDITIONAL_INCLUDES": self._additional_includes,
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
        """Generate repository code that finalises every tracer traversal."""

        self.__set_solver_repository_dictionary()

        for solver in self._solvers:
            solver_tracers = []
            for particle in getattr(solver, "_tracer_particles", []):
                if particle.name not in solver_tracers:
                    solver_tracers.append(particle.name)
            self._solver_repository_dictionary["SOLVERS"].append(
                (
                    solver._name,
                    solver.get_name_of_global_instance(),
                    solver_tracers,
                    self.__normalise_solver_statistics_metadata(solver),
                )
            )

        template_file_prefix = os.path.splitext(os.path.realpath(__file__))[0]
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                template_file_prefix + "SolverRepository.template.h",
                template_file_prefix + "SolverRepository.template.cpp",
                "SolverRepository",
                self.namespace + ["repositories"],
                "repositories",
                self._solver_repository_dictionary,
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

    def _configure_runner(self, peano_main):
        """Add ExaHyPE hooks and repository callbacks to the generated main.

        Step hooks live in generated step bodies and therefore run on every
        rank. The runner controls when these steps execute.
        """
        project_namespace = "::" + "::".join(self.namespace)
        repository_namespace = project_namespace + "::repositories"

        construction_start = f"{repository_namespace}::startGridConstructionStep();"
        construction_finish = f"{repository_namespace}::finishGridConstructionStep();"
        self.create_grid.runner_start_hook = construction_start
        self.create_grid.runner_finish_hook = construction_finish

        # Grid initialisation may introduce static AMR, so initial splitting
        # finishes before this traversal.
        self.init_grid.runner_start_hook = (
            "::runner::loadBalancer().enable(false);\n"
            f"{repository_namespace}::startGridInitialisationStep();"
        )
        self.init_grid.runner_finish_hook = (
            f"{repository_namespace}::finishGridInitialisationStep();"
        )
        self.plot_solution.runner_start_hook = (
            f"{repository_namespace}::startPlottingStep();"
        )
        self.plot_solution.runner_finish_hook = (
            f"{repository_namespace}::finishPlottingStep();"
        )
        self.perform_time_step.runner_start_hook = (
            f"{repository_namespace}::startTimeStep();"
        )
        self.perform_time_step.runner_finish_hook = (
            f"{repository_namespace}::finishTimeStep();"
        )

        peano_main.additional_includes = (
            '#include "repositories/SolverRepository.h"\n'
            '#include "runner/LoadBalancer.h"'
        )

        peano_main.additional_repository_callbacks = (
            f"  result.startSimulation         = []() {{ "
            f"{repository_namespace}::startSimulation(); }};\n"
            f"  result.finishSimulation        = []() {{ "
            f"{repository_namespace}::finishSimulation(); }};\n"
            f"  result.mayPlot                 = []() {{ return "
            f"{repository_namespace}::mayPlot(); }};\n"
            f"  result.getMinTimeStamp         = []() {{ return "
            f"{repository_namespace}::getMinTimeStamp(); }};\n"
            f"  result.getMaxTimeStamp         = []() {{ return "
            f"{repository_namespace}::getMaxTimeStamp(); }};\n"
            f"  result.getNumberOfDoFs         = []() {{ return "
            f"{repository_namespace}::getNumberOfDoFs(); }};\n"
            f"  result.getSolverStatistics     = []() {{ return "
            f"{repository_namespace}::getSolverStatistics(); }};\n"
            f"  result.getConfigurationSummary = []() {{ return "
            f"{repository_namespace}::getConfigurationSummary(); }};\n"
            "  result.getCopyrightBanner      = []() { return "
            '::std::string("ExaHyPE 2 (C) www.exahype.org"); };'
        )

    def _init_main_template_dictionary(self, peano_main):
        # A non-increasing interval uses the minimum stamp as its sole completion criterion.
        max_terminal_time = self._max_terminal_time
        if (
            max_terminal_time is not None
            and self._min_terminal_time is not None
            and max_terminal_time <= self._min_terminal_time
        ):
            max_terminal_time = None

        peano_main.template_dictionary.update(
            {
                "DEFAULT_MIN_TERMINAL_TIME": self._render_optional_runtime_default(
                    self._min_terminal_time
                ),
                "DEFAULT_MAX_TERMINAL_TIME": self._render_optional_runtime_default(
                    max_terminal_time
                ),
                "DEFAULT_GRID_LEVELS": self._render_optional_integer_runtime_default(
                    self._min_grid_levels
                ),
                "DEFAULT_MAX_GRID_LEVELS": self._render_optional_integer_runtime_default(
                    self._max_grid_levels
                ),
            }
        )

    def __add_solver_implementation_files(self, solver):
        solver.add_implementation_files_to_project(
            self.namespace,
            self.output,
            self._dimensions,
        )

    def add_tracer(
        self,
        name,
        particle_attributes,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        del plot_descend_invocation_order_within_action_sets
        particle = peano4.toolbox.particles.Particle(name)
        if isinstance(particle_attributes, dict):
            self._particle_unknowns = sum(particle_attributes["unknowns"].values())
            self._particle_variable_names += list(
                particle_attributes["unknowns"].keys()
            )

            for number_of_unknowns in particle_attributes["unknowns"].values():
                self._particle_variable_positions.append(
                    number_of_unknowns + self._particle_variable_positions[-1]
                )

            self._generate_particle_shortcuts(name)

        elif isinstance(particle_attributes, int):
            self._particle_unknowns = particle_attributes

        else:
            raise ValueError(
                "Invalid format for 'unknowns'. Must be an integer or a dictionary."
            )

        particle_data = peano4.datamodel.DoubleArray(
            "data", str(self._particle_unknowns)
        )
        particle.add_attribute(particle_data)

        particle.add_attribute(
            peano4.datamodel.Enumeration("moveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.datamodel.IntegerArray("number", "2")
        particle.add_attribute(particle_number)

        self.datamodel.add_global_object(particle)
        particle_set = peano4.toolbox.particles.ParticleSet(particle)
        self.datamodel.add_vertex(particle_set)

        self.plot_solution.use_vertex(particle_set)
        self.init_grid.use_vertex(particle_set)
        self.perform_time_step.use_vertex(particle_set)
        self.create_grid.use_vertex(particle_set)
        self.load_balancing.use_vertex(particle_set)

        self.init_grid.add_action_set(
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(
                particle_set
            )
        )

        update_particle_association_action_set = (
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(
                particle_set
            )
        )
        update_particle_association_action_set.descend_invocation_order = (
            sort_descend_invocation_order_within_action_sets
        )

        update_particle_parallel_state_action_set = (
            peano4.toolbox.particles.api.UpdateParallelState(particle_set)
        )
        update_particle_parallel_state_action_set.descend_invocation_order = (
            sort_descend_invocation_order_within_action_sets
        )

        self.perform_time_step.add_action_set(update_particle_association_action_set)
        self.perform_time_step.add_action_set(update_particle_parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particle_set)

        return particle_set

    def _generate_particle_shortcuts(self, name):
        generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
            os.path.join(
                os.path.dirname(os.path.realpath(__file__)),
                "VariableShortcuts.template.h",
            ),
            "VariableShortcuts" + name,
            self.namespace,
            ".",
            {
                "NUMBER_OF_VARIABLES": len(self._particle_variable_names),
                "VARIABLE_NAMES": self._particle_variable_names,
                "VARIABLE_POSITIONS": self._particle_variable_positions,
                "SOLVER_NAME": name,
            },
            True,
        )
        self.output.add(generated_shortcut_file)
        self.output.makefile.add_h_file(
            "VariableShortcuts" + name + ".h", generated=True
        )

    def add_action_set_to_timestepping(self, action_set):
        """Add an action set to time stepping with its configured priority.

        Use ``perform_time_step.min_priority()`` and ``max_priority()`` to
        position it relative to existing action sets.
        """
        self.perform_time_step.add_action_set(action_set)

    def add_action_set_to_plot_solution(self, action_set):
        """Add an action set to plotting."""
        self.plot_solution.add_action_set(action_set)

    def add_action_set_to_initialisation(self, action_set):
        self.init_grid.add_action_set(action_set)

    def add_action_set_to_create_grid(self, action_set):
        """Add an action set to every refining ``CreateGrid`` traversal."""
        self.create_grid.add_action_set(action_set)

    def __add_solver_label_action_sets_to_load_balancing(self, solver):
        """Add solver labels required by load-balancing traversals."""
        if hasattr(solver, "_action_set_update_face_label"):
            self.load_balancing.add_action_set(solver._action_set_update_face_label)
        if hasattr(solver, "_action_set_update_cell_label"):
            self.load_balancing.add_action_set(solver._action_set_update_cell_label)

    def generate_and_build(self):
        """Generate solver data, actions, repositories, and runner callbacks.

        The configured Peano project is generated and built after these
        ExaHyPE components have been installed.
        """
        # A generated solver set cannot accept incremental additions.
        self._solver_set_generated = True

        # Build settings such as the SIMD width affect generated solver code.
        self.output.makefile.parse_configuration_outcome()
        self._makefile_configured = True

        # Regenerated action sets receive new observer identifiers, so every run
        # starts from the user's project state and restores all injected collections.
        pre_generation_artefacts = list(self.output.artefacts)
        pre_generation_readme_entries = list(self.output.readme._entries)
        pre_generation_generated_files = (
            list(self.output.makefile.generated_hfiles),
            list(self.output.makefile.generated_cppfiles),
            list(self.output.makefile.generated_fortranfiles),
            list(self.output.makefile.generated_gpu_cppfiles),
        )
        pre_generation_datamodel = (
            list(self.datamodel.cell_data),
            list(self.datamodel.face_data),
            list(self.datamodel.vertex_data),
            list(self.datamodel.global_data),
        )
        pre_generation_steps = {
            step: (
                list(step.cell_data),
                list(step.face_data),
                list(step.vertex_data),
                list(step.action_sets),
            )
            for step in self.solversteps._steps
        }

        try:
            self.__add_readme_entries()

            solver_steps = (
                self.create_grid,
                self.load_balancing,
                self.plot_solution,
                self.perform_time_step,
                self.init_grid,
            )
            for solver in self._solvers:
                log.debug("Prepare solver %s.", solver._name)
                log.debug("Solver configuration: %s.", solver)

                solver.add_to_Peano4_datamodel(self.datamodel)

                for step in solver_steps:
                    solver.add_use_data_statements_to_Peano4_solver_step(step)

                solver.add_actions_to_create_grid(self.create_grid)
                self.__add_solver_label_action_sets_to_load_balancing(solver)

                solver.add_actions_to_init_grid(self.init_grid)
                solver.add_actions_to_plot_solution(self.plot_solution)

                solver.add_actions_to_perform_time_step(self.perform_time_step)

                self.__add_solver_implementation_files(solver)

            self.__generate_solver_repository()

            self._configure_runner(self.main)
            self._init_main_template_dictionary(self.main)

            super().generate_and_build()
        finally:
            self.output.artefacts = pre_generation_artefacts
            self.output.readme._entries = pre_generation_readme_entries
            (
                self.output.makefile.generated_hfiles,
                self.output.makefile.generated_cppfiles,
                self.output.makefile.generated_fortranfiles,
                self.output.makefile.generated_gpu_cppfiles,
            ) = pre_generation_generated_files
            (
                self.datamodel.cell_data,
                self.datamodel.face_data,
                self.datamodel.vertex_data,
                self.datamodel.global_data,
            ) = pre_generation_datamodel
            for step, (
                cell_data,
                face_data,
                vertex_data,
                action_sets,
            ) in pre_generation_steps.items():
                step.cell_data = cell_data
                step.face_data = face_data
                step.vertex_data = vertex_data
                step.action_sets = action_sets

    def init_new_user_defined_algorithmic_step(
        self,
        step,
    ):
        """Add solver and tracer data dependencies to a custom step.

        Register every solver and tracer before calling this method. This
        method only modifies ``step``; the caller still registers it with the
        project.
        """
        for solver in self._solvers:
            solver.add_use_data_statements_to_Peano4_solver_step(step)

        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)

    @property
    def additional_includes(self):
        return self._additional_includes
