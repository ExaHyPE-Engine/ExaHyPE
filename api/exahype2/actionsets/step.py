# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import textwrap

import exahype2.codegen

from exahype2.log import log

from .action_set import ActionSet
from .observer import Observer
from .user_action_set import UserActionSet


class Step:
    """One mesh traversal: The data it reads and writes and the action sets it runs.

    A step becomes one C++ observer that receives the traversal events and forwards them to
    one C++ class per action set; a Python action set may belong to several steps.
    """

    def __init__(self, name, add_user_defined_actions=True):
        self.name = name
        self.cell_data = []
        self.face_data = []
        self.vertex_data = []
        self.action_sets = [UserActionSet(name)] if add_user_defined_actions else []
        self.project = None
        self.namespace = []
        self.runner_start_hook = ""
        self.runner_finish_hook = ""

    def __sort(self):
        """Sorts the action sets by descend_invocation_order with an exchange sort, so action
        sets of equal order keep the order this sort produces, not their insertion order.
        """
        for i in range(0, len(self.action_sets)):
            for j in range(i + 1, len(self.action_sets)):
                if (
                    self.action_sets[i].descend_invocation_order
                    > self.action_sets[j].descend_invocation_order
                ):
                    self.action_sets[i], self.action_sets[j] = (
                        self.action_sets[j],
                        self.action_sets[i],
                    )

    def add_action_set(self, action_set):
        """Adds an action set; first-touch operations run in ascending invocation order and
        last-touch operations in reverse order. A second registration of the same object has
        no effect."""
        if any(existing is action_set for existing in self.action_sets):
            return
        self.action_sets.append(action_set)
        self.__sort()

    def set_project(self, project):
        self.project = project
        self.namespace = project.namespace

    def use_cell(self, submodel):
        if any(existing is submodel for existing in self.cell_data):
            return
        self.cell_data.append(submodel)

    def use_face(self, submodel):
        if any(existing is submodel for existing in self.face_data):
            return
        self.face_data.append(submodel)

    def use_vertex(self, submodel):
        if any(existing is submodel for existing in self.vertex_data):
            return
        self.vertex_data.append(submodel)

    @staticmethod
    def _enumerated(prefix, data):
        return [
            (prefix + entry.name, "[[maybe_unused]] " + entry.get_enumeration_type())
            for entry in data
        ]

    @staticmethod
    def _referenced(prefix, data):
        return [
            (
                prefix + entry.name,
                "[[maybe_unused]] " + entry.get_full_qualified_type() + "&",
            )
            for entry in data
        ]

    def _coarse_signature(self):
        return (
            self._enumerated("coarseGridVertices", self.vertex_data)
            + self._enumerated("coarseGridFaces", self.face_data)
            + self._referenced("coarseGridCell", self.cell_data)
        )

    def vertex_operations_signature(self):
        return (
            [("marker", "[[maybe_unused]] const ::peano4::dataview::VertexMarker&")]
            + self._referenced("fineGridVertex", self.vertex_data)
            + self._coarse_signature()
        )

    def face_operations_signature(self):
        return (
            [("marker", "[[maybe_unused]] const ::peano4::dataview::FaceMarker&")]
            + self._enumerated("fineGridVertices", self.vertex_data)
            + self._referenced("fineGridFace", self.face_data)
            + self._coarse_signature()
        )

    def cell_operations_signature(self):
        return (
            [("marker", "[[maybe_unused]] const ::peano4::dataview::CellMarker&")]
            + self._enumerated("fineGridVertices", self.vertex_data)
            + self._enumerated("fineGridFaces", self.face_data)
            + self._referenced("fineGridCell", self.cell_data)
            + self._coarse_signature()
        )

    def _operations(self):
        """Returns the traversal events of the step as (name, parameters) pairs."""
        result = [
            (ActionSet.OPERATION_BEGIN_TRAVERSAL, []),
            (ActionSet.OPERATION_END_TRAVERSAL, []),
        ]
        if self.vertex_data:
            signature = self.vertex_operations_signature()
            result += [
                (operation, signature)
                for operation in (
                    ActionSet.OPERATION_CREATE_VERTEX,
                    ActionSet.OPERATION_DESTROY_VERTEX,
                    ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME,
                    ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME,
                    ActionSet.OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME,
                    ActionSet.OPERATION_TOUCH_HANGING_VERTEX_LAST_TIME,
                )
            ]
        if self.face_data:
            signature = self.face_operations_signature()
            result += [
                (operation, signature)
                for operation in (
                    ActionSet.OPERATION_CREATE_FACE,
                    ActionSet.OPERATION_DESTROY_FACE,
                    ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME,
                    ActionSet.OPERATION_TOUCH_FACE_LAST_TIME,
                    ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME,
                    ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME,
                )
            ]
        if self.cell_data:
            signature = self.cell_operations_signature()
            result += [
                (ActionSet.OPERATION_CREATE_CELL, signature),
                (ActionSet.OPERATION_DESTROY_CELL, signature),
            ]
        signature = self.cell_operations_signature()
        result += [
            (ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME, signature),
            (ActionSet.OPERATION_TOUCH_CELL_LAST_TIME, signature),
        ]
        return result

    @staticmethod
    def _body(code):
        """Returns ``code`` without its common indentation, so a template indents it."""
        return textwrap.dedent(code).strip()

    def _write_action_set(self, output, action_set):
        """Writes the C++ class of one action set and returns its qualified name."""
        if action_set.is_user_template():
            subdirectory = "actions"
            class_name = action_set.name
            log.info(
                "User has to modify class %s in actions directory manually", class_name
            )
        else:
            subdirectory = "observers"
            class_name = "{}_{}_{}".format(
                self.name, self.action_sets.index(action_set), action_set.name
            )
        namespace = self.project.namespace + [subdirectory]
        includes = [
            "peano4/Constants.h",
            "peano4/dataview/CellMarker.h",
            "peano4/dataview/FaceEnumerator.h",
            "peano4/dataview/FaceMarker.h",
            "peano4/dataview/VertexEnumerator.h",
            "peano4/dataview/VertexMarker.h",
            "peano4/grid/GridControlEvent.h",
            "tarch/la/Vector.h",
        ]
        includes += [
            "{}/{}.h".format(kind, entry.name)
            for kind, data in (
                ("vertexdata", self.vertex_data),
                ("facedata", self.face_data),
                ("celldata", self.cell_data),
            )
            for entry in data
        ]
        includes += action_set.includes()
        dictionary = {
            "INCLUDES": sorted(set(includes)),
            "ATTRIBUTES": self._body(action_set.attributes()),
            "STATIC_INITIALISATIONS": action_set.get_static_initialisations(
                "::".join(namespace + [class_name])
            ).strip("\n"),
            "CONSTRUCTOR_BODY": self._body(action_set.get_constructor_body()),
            "DESTRUCTOR_BODY": self._body(action_set.get_destructor_body()),
            "GRID_CONTROL_EVENTS_BODY": self._body(
                action_set.grid_control_events_body()
            ),
            "PREPARE_TRAVERSAL_BODY": self._body(action_set.prepare_traversal_body()),
            "UNPREPARE_TRAVERSAL_BODY": self._body(
                action_set.unprepare_traversal_body()
            ),
            "OPERATIONS": [
                {
                    "name": name,
                    "returns": "void",
                    "parameters": [
                        {"type": cpp_type, "name": parameter}
                        for parameter, cpp_type in signature
                    ],
                    "body": self._body(action_set.operation_body(name)),
                }
                for name, signature in self._operations()
            ],
        }
        template_directory = os.path.dirname(os.path.realpath(__file__))
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "ActionSet.h.jinja2"),
                os.path.join(template_directory, "ActionSet.cpp.jinja2"),
                class_name,
                namespace,
                subdirectory,
                dictionary,
            )
        )
        output.makefile.add_cpp_file(
            subdirectory + "/" + class_name + ".cpp", generated=True
        )
        return subdirectory + "::" + class_name

    def construct_output(self, output):
        included_action_sets = [
            self._write_action_set(output, action_set)
            for action_set in self.action_sets
        ]
        output.artefacts.append(
            Observer(
                self.name,
                self.project.namespace + ["observers"],
                "observers",
                included_action_sets,
                self.vertex_data,
                self.face_data,
                self.cell_data,
            )
        )
        output.makefile.add_h_file("observers/" + self.name + ".h", generated=True)
        output.makefile.add_cpp_file("observers/" + self.name + ".cpp", generated=True)
