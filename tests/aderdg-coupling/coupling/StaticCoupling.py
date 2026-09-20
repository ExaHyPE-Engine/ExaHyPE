# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import jinja2

import exahype2.actionsets
import exahype2.codegen
import exahype2.datamodel
import exahype2.pde_terms

from .actionsets import SolveRiemannProblem
from .actionsets import VerifyTroubledness
from .kernels import Limiter


class StaticCoupling(object):
    """Couples a regular ADER-DG solver to a second one that takes over a troubled cell and
    projects the face estimates of each solver onto the quadrature points of the other.
    """

    TROUBLED_MARKERS = [
        "REGULAR",
        "REGULAR_TO_LIMITER",
        "LIMITER_TO_REGULAR",
        "TROUBLED",
    ]

    def __init__(
        self,
        name,
        regular_solver,
        limiting_solver,
        limiting_criterion_implementation=True,
    ):
        """``limiting_criterion_implementation`` is True for a criterion the user defines in
        ``<name>.cpp`` or the C++ body of ``isPhysicallyAdmissible``."""
        self._name = name

        self._regular_solver = regular_solver
        self._limiter_solver = limiting_solver

        self._solver_number = 0
        self._dictionary = None

        # The project asks every solver which PDE terms it declares; the coupling declares none.
        self.pde_terms = exahype2.pde_terms.PDETerms({})

        # The templates splice these into the declaration and the definition of the
        # generated class.
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._includes = []

        # The templates splice these into the routines the solver runs at the start and at
        # the end of a time step and into its constructor.
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""

        self._constructor_implementation = ""

        if limiting_criterion_implementation is not True and not isinstance(
            limiting_criterion_implementation, str
        ):
            raise ValueError("The limiting criterion is True or a C++ body.")
        self._physical_admissibility_criterion = limiting_criterion_implementation

        self._couple_solvers()

    def _couple_solvers(self):
        """Extends the guards and the time step size of the two solvers before the project
        builds them."""
        time_step_size = (
            "std::min( repositories::"
            + self._regular_solver.instance_name
            + ".getTimeStepSize(), repositories::"
            + self._limiter_solver.instance_name
            + ".getTimeStepSize())"
        )
        self._regular_solver.use_time_step_size(time_step_size)
        self._limiter_solver.use_time_step_size(time_step_size)

        cell_is = (
            "fineGridCell"
            + self._name
            + "CellLabel.getTroubledMarker()==celldata::"
            + self._name
            + "CellLabel::TroubledMarker::"
        )
        regular_cell = cell_is + "REGULAR"
        troubled_cell = cell_is + "TROUBLED"
        troubled_face = (
            "fineGridFace"
            + self._name
            + "FaceLabel.getTroubledMarker()>=facedata::"
            + self._name
            + "FaceLabel::TroubledMarker::LIMITER_TO_REGULAR"
        )

        self._regular_solver.add_guard("Prediction", regular_cell)
        self._regular_solver.add_guard("Correction", regular_cell, guard="cell_guard")

        self._limiter_solver.add_guard("Prediction", troubled_cell)
        self._limiter_solver.add_guard("Correction", troubled_cell, guard="cell_guard")
        self._limiter_solver.add_guard("Correction", troubled_face, guard="face_guard")

    def create_data_structures(self):
        """Creates the labels that carry the troubled marker of a cell and of a face."""
        self._cell_label = exahype2.actionsets.create_cell_label(self._name)
        self._face_label = exahype2.actionsets.create_face_label(self._name)
        self._cell_label.add_attribute(
            exahype2.datamodel.Enumeration(
                name="troubledMarker",
                variants=self.TROUBLED_MARKERS,
                initval="TroubledMarker::REGULAR",
            )
        )
        self._face_label.add_attribute(
            exahype2.datamodel.Enumeration(
                name="troubledMarker",
                variants=self.TROUBLED_MARKERS,
                initval="TroubledMarker::REGULAR",
            )
        )

        self._face_label.merge_method_definition += "\n  setTroubledMarker( std::max( getTroubledMarker(), neighbour.getTroubledMarker() ) );"

    def create_action_sets(self):
        """Creates the two action sets of the coupling. The troubledness check runs in the
        initialisation, after the regular solver has computed its initial condition."""
        state_is = (
            "repositories::"
            + self.instance_name
            + ".getSolverState()=="
            + self._name
            + "::SolverState::"
        )
        self._action_set_check_troubledness = VerifyTroubledness(
            self,
            use_admissibility_criterion=True,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined() and "
                + state_is
                + "GridInitialisation"
            ),
        )

        self._action_set_copy_face_data = SolveRiemannProblem(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and "
                + state_is
                + "TimeStepping"
                + " and repositories::"
                + self.instance_name
                + ".isFirstGridSweepOfTimeStep()"
            ),
        )

        self._action_set_check_troubledness.descend_invocation_order = 1000
        self._action_set_copy_face_data.descend_invocation_order = 10

    @property
    def name(self):
        return self._name

    @property
    def instance_name(self):
        return "instanceOf" + self._name

    @property
    def regular_solver(self):
        return self._regular_solver

    @property
    def limiter_solver(self):
        return self._limiter_solver

    def statistics_metadata(self):
        return [
            ("Regular Solver", self._regular_solver.name),
            ("Limiter Solver", self._limiter_solver.name),
        ]

    def add_includes(self, *headers):
        """Adds headers to the generated abstract coupling class, and through it to every
        action set of the coupling."""
        self._includes += headers
        return self

    def action_set_includes(self):
        return ["peano4/Loop.h", "repositories/SolverRepository.h", "tarch/la/Vector.h"]

    def __str__(self):
        return "{}(name={}, regular_solver={}, limiter_solver={})".format(
            type(self).__name__,
            self._name,
            self._regular_solver.name,
            self._limiter_solver.name,
        )

    def register_data(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_face(self._face_label)

    def use_data_in_step(self, step):
        step.use_cell(self._cell_label)
        step.use_face(self._face_label)

    def action_sets_per_step(self):
        return {
            "init_grid": [self._action_set_check_troubledness],
            "perform_time_step": [self._action_set_copy_face_data],
        }

    def tracing_action_sets_per_step(self):
        """The coupling projects onto no particles of its own."""
        return {}

    @property
    def dictionary(self):
        """The template entries of the coupling; ``build`` creates them once."""
        if self._dictionary is None:
            raise RuntimeError(
                "[{}] the project has not built the coupling.".format(self._name)
            )
        return self._dictionary

    def build(self, project):
        """Creates the data, the action sets and the dictionary of the coupling, then writes
        its C++ files."""
        namespace = project.namespace
        output = project.output
        self.create_data_structures()
        self.create_action_sets()

        dictionary = {}
        self._fill_dictionary(dictionary)
        self._dictionary = dictionary

        templates = os.path.dirname(os.path.realpath(__file__))

        def template(name):
            return os.path.join(templates, name)

        output.add(
            exahype2.codegen.TemplatedFile(
                template("StaticCouplingAbstract.h.jinja2"),
                template("StaticCouplingAbstract.cpp.jinja2"),
                "Abstract" + self._name,
                namespace,
                ".",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)

        user_defined = dictionary["ADMISSIBILITY_IS_USER_DEFINED"]
        output.add(
            exahype2.codegen.TemplatedFile(
                template("StaticCoupling.h.jinja2"),
                template("StaticCoupling.cpp.jinja2") if user_defined else None,
                self._name,
                namespace,
                ".",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file(self._name + ".h", generated=True)
        if user_defined:
            output.makefile.add_cpp_file(self._name + ".cpp", generated=False)

        # The solver repository includes a PDE term header per solver.
        output.add(
            exahype2.codegen.TemplatedFile(
                template("StaticCouplingPDETerms.h.jinja2"),
                None,
                "PDETerms" + self._name,
                namespace,
                ".",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file("PDETerms" + self._name + ".h", generated=True)

        self._generate_kernels(namespace, output, dictionary)

    def _generate_kernels(self, namespace, output, dictionary):
        Limiter(dictionary).generate_kernels(
            namespace + ["kernels", self._name],
            output,
            os.path.dirname(os.path.realpath(__file__)) + "/kernels/",
            "kernels/" + self._name,
            regular_solver_quadpoints=[
                float(point) for point in self._regular_solver.basis.quadrature_points
            ],
            limiter_solver_quadpoints=[
                float(point) for point in self._limiter_solver.basis.quadrature_points
            ],
        )

    def _fill_dictionary(self, d):
        """Fills the dictionary of the keys that the templates read."""
        d["SOLVER_INSTANCE"] = self.instance_name
        d["REGULAR_SOLVER_INSTANCE"] = self._regular_solver.instance_name
        d["LIMITER_SOLVER_INSTANCE"] = self._limiter_solver.instance_name

        d["SOLVER_NAME"] = self._name
        d["REGULAR_SOLVER_NAME"] = self._regular_solver.name
        d["LIMITER_SOLVER_NAME"] = self._limiter_solver.name

        d["REGULAR_SOLVER_UNKNOWN_IDENTIFIER"] = (
            self._regular_solver._unknown_identifier()
        )
        d["LIMITER_SOLVER_UNKNOWN_IDENTIFIER"] = (
            self._limiter_solver._unknown_identifier()
        )

        d["SOLVER_INCLUDES"] = sorted(set(self._includes))

        d["ORDER"] = self._regular_solver.order
        d["REGULAR_SOLVER_ORDER"] = self._regular_solver.order
        d["LIMITER_SOLVER_ORDER"] = self._limiter_solver.order

        d["REGULAR_SOLVER_STORAGE_PRECISION"] = self._regular_solver.precision.storage

        d["REGULAR_SOLVER_CORRECTION_PRECISION"] = (
            self._regular_solver.precision.compute
        )
        d["LIMITER_SOLVER_CORRECTION_PRECISION"] = (
            self._limiter_solver.precision.compute
        )

        d["SOLVER_USER_DECLARATIONS"] = jinja2.Template(
            self._solver_user_declarations, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["SOLVER_USER_DEFINITIONS"] = jinja2.Template(
            self._solver_user_definitions, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["START_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._start_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._finish_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["ADMISSIBILITY_IS_USER_DEFINED"] = (
            self._physical_admissibility_criterion is True
        )
        d["ADMISSIBILITY_IMPLEMENTATION"] = (
            ""
            if d["ADMISSIBILITY_IS_USER_DEFINED"]
            else self._physical_admissibility_criterion
        )
