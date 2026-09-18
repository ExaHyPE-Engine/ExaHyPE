# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel

from exahype2.solvers.PDETerms import PDETerms
from peano4.Logger import log


class Project(peano4.Project):
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
        """Sets the format of every solver plotter that carries no format of its own.

        ``None`` keeps the current default.
        """
        if file_format is not None:
            import peano4.plotter

            peano4.plotter.set_default_format(file_format)

    def _create_steps(self):
        super()._create_steps()
        self.init_grid = peano4.solversteps.Step("InitGrid", False)
        self.solversteps.add_step(self.init_grid)
        self.plot_solution = peano4.solversteps.Step("PlotSolution", False)
        self.solversteps.add_step(self.plot_solution)
        self.perform_time_step = peano4.solversteps.Step("TimeStep", False)
        self.solversteps.add_step(self.perform_time_step)
        self.dynamic_amr = peano4.solversteps.Step("DynamicAMR", False)
        self.solversteps.add_step(self.dynamic_amr)

    def add_solver(self, solver):
        """Appends a solver to the solver set.

        A repeated solver name and a project that has already generated its code both start a
        new solver set and delete the implementation files of the previous one.
        """
        redeclaration = self._solver_set_generated or any(
            existing._name == solver._name for existing in self._solvers
        )
        if redeclaration:
            self._solvers = []
            self.output.makefile.clear_user_files()
            self._solver_set_generated = False
        solver._solver_number = len(self._solvers)
        self._solvers.append(solver)

    def remove_all_solvers(self):
        self._solvers = []
        self.output.makefile.clear_user_files()
        self._solver_set_generated = False

    def __add_readme_entries(self):
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
         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X}
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

        # A solver without a refinement criterion never submits an erase request, so only a
        # solver that carries one counts towards the coarsening consensus.
        amr_participants = sum(
            1
            for solver in self._solvers
            if getattr(
                solver,
                "_refinement_criterion_implementation",
                PDETerms.Empty_Implementation,
            )
            not in (PDETerms.Empty_Implementation, PDETerms.None_Implementation)
        )

        self._solver_repository_dictionary = {
            "SOLVERS": [],
            "TRACERS": tracers,
            "PROJECT_TRACERS": project_tracers,
            "NUMBER_OF_AMR_PARTICIPANTS": max(amr_participants, 1),
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
        """Gives every algorithmic step the start and finish hook of its repository phase."""
        project_namespace = "::" + "::".join(self.namespace)
        repository_namespace = project_namespace + "::repositories"

        construction_start = f"{repository_namespace}::startGridConstructionStep();"
        construction_finish = f"{repository_namespace}::finishGridConstructionStep();"
        self.create_grid.runner_start_hook = construction_start
        self.create_grid.runner_finish_hook = construction_finish

        self.init_grid.runner_start_hook = (
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
        self.dynamic_amr.runner_start_hook = (
            f"{repository_namespace}::startDynamicMeshRefinementStep();"
        )
        self.dynamic_amr.runner_finish_hook = (
            f"{repository_namespace}::finishDynamicMeshRefinementStep();"
        )

        peano_main.additional_includes = (
            '#include "repositories/SolverRepository.h"\n'
            '#include "exahype2/RefinementControlService.h"'
        )

        peano_main.additional_repository_callbacks = (
            f"  result.startSimulation         = []() {{ "
            f"{repository_namespace}::startSimulation(); }};\n"
            f"  result.finishSimulation        = []() {{ "
            f"{repository_namespace}::finishSimulation(); }};\n"
            f"  result.mayPlot                 = []() {{ return "
            f"{repository_namespace}::mayPlot(); }};\n"
            "  result.hasPendingMeshUpdates   = []() { return "
            "::exahype2::RefinementControlService::getInstance().hasGridControlEvents(); };\n"
            "  result.hasUndecidedRefinementCriterion = []() { return "
            "::exahype2::RefinementControlService::getInstance().lastCriterionStepWasSuppressed(); };\n"
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
            'std::string("ExaHyPE 2 (C) www.exahype.org"); };'
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
        particle = peano4.particles.Particle(name)
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
        particle_set = peano4.particles.ParticleSet(particle)
        self.datamodel.add_vertex(particle_set)

        self.plot_solution.use_vertex(particle_set)
        self.init_grid.use_vertex(particle_set)
        self.perform_time_step.use_vertex(particle_set)
        self.create_grid.use_vertex(particle_set)

        self.init_grid.add_action_set(
            peano4.particles.UpdateParticleGridAssociation_LiftDrop(particle_set)
        )

        update_particle_association_action_set = (
            peano4.particles.UpdateParticleGridAssociation_LiftDrop(particle_set)
        )
        update_particle_association_action_set.descend_invocation_order = (
            sort_descend_invocation_order_within_action_sets
        )

        update_particle_parallel_state_action_set = (
            peano4.particles.UpdateParallelState(particle_set)
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
        self.perform_time_step.add_action_set(action_set)

    def add_action_set_to_plot_solution(self, action_set):
        self.plot_solution.add_action_set(action_set)

    def add_action_set_to_initialisation(self, action_set):
        self.init_grid.add_action_set(action_set)

    def add_action_set_to_create_grid(self, action_set):
        self.create_grid.add_action_set(action_set)

    def generate_and_build(self):
        for solver in self._solvers:
            configuration_validator = getattr(
                solver, "validate_project_configuration", None
            )
            if configuration_validator is not None:
                configuration_validator(
                    self._min_grid_levels,
                    self._max_grid_levels,
                )

        # Generating freezes the declared solver set: The next add_solver call starts a new one.
        self._solver_set_generated = True

        # Build settings such as the SIMD width affect generated solver code.
        self.output.makefile.parse_configuration_outcome()
        self._makefile_configured = True

        # Regenerated action sets receive new observer identifiers, so the project state is
        # copied here and put back in the finally block below.
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
                self.plot_solution,
                self.perform_time_step,
                self.init_grid,
                self.dynamic_amr,
            )
            for solver in self._solvers:
                log.debug("Prepare solver %s.", solver._name)
                log.debug("Solver configuration: %s.", solver)

                solver.add_to_Peano4_datamodel(self.datamodel)

                for step in solver_steps:
                    solver.add_use_data_statements_to_Peano4_solver_step(step)

                solver.add_actions_to_create_grid(self.create_grid)

                solver.add_actions_to_init_grid(self.init_grid)
                solver.add_actions_to_plot_solution(self.plot_solution)

                solver.add_actions_to_perform_time_step(self.perform_time_step)
                if hasattr(solver, "add_actions_to_dynamic_mesh_refinement"):
                    solver.add_actions_to_dynamic_mesh_refinement(self.dynamic_amr)

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
        """Adds the data of every solver and tracer to ``step``.

        The caller registers every solver and tracer before this call, and registers ``step``
        with the project afterwards.
        """
        for solver in self._solvers:
            solver.add_use_data_statements_to_Peano4_solver_step(step)

        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)

    @property
    def additional_includes(self):
        return self._additional_includes
