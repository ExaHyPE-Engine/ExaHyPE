# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.solversteps.UserActionSet

from peano4.solversteps.StepToActionSet import StepToActionSet
from peano4.solversteps.StepToObserver import StepToObserver


class Step:
    """!
    Describes one mesh traversal step.

    Most generic version of a solver step. A solverstep describes what Peano 4
    does when it runs over the grid. As users specify which pieces
    of data from the data model are used by the solver step, it can handle all
    the user data stack flow.

    The class generates the function signatures of the observers and action
    sets. A step corresponds to one observer that accepts traversal events.

    The observer distributes these events as grid actions such as "touch vertex
    first time" among its action sets. One Python action set may belong to
    several observers; each observer owns a corresponding C++ action-set
    instance.

    @see peano4.solversteps.ActionSet

    name: String
      Names the generated observer and its default action set.
    """

    def __init__(self, name, add_user_defined_actions=True):
        """
        Creates a step with one user action set by default. Passing False as
        add_user_defined_actions or calling remove_all_actions() creates an
        empty action-set list.
        """
        self.name = name
        self.cell_data = []
        self.face_data = []
        self.vertex_data = []
        if add_user_defined_actions:
            self.action_sets = [peano4.solversteps.UserActionSet(name)]
        else:
            self.action_sets = []
        self.action_set_generator = StepToActionSet(self)
        self.observer_generator = StepToObserver(self)

    def remove_all_actions(self):
        """
        Removes every action set from the step.
        """
        self.action_sets = []

    def __sort(self):
        """Sorts action sets by their descend_invocation_order.

        The exchange sort can swap action sets of equal order, so their
        generated call order follows this sort and not their insertion order.
        @see peano4.solversteps.ActionSet.descend_invocation_order
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
        """!

        Adds an action set to this mesh traversal step.

        First-touch operations run in ascending invocation order and last-touch
        operations in reverse order. Registering the same object twice has no
        effect because project generation may register cached action sets more
        than once.
        """
        if any(existing is action_set for existing in self.action_sets):
            return
        self.action_sets.append(action_set)
        self.__sort()

    def set_project(self, project):
        """!

        Ties the step to a project.

        A project calls this method when it adds the step.
        """
        self.project = project
        self.namespace = project.namespace

    # Project generation may register cached data objects more than once, while
    # traversal signatures require each object exactly once.
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

    def highest_descend_invocation_order(self):
        """!

        Returns the maximum descend_invocation_order of the step's action sets.

        @see peano4.solversteps.ActionSet.descend_invocation_order

        """
        self.__sort()
        if len(self.action_sets) == 0:
            return 0
        return self.action_sets[-1].descend_invocation_order

    def lowest_descend_invocation_order(self):
        """!

        Returns the minimum descend_invocation_order of the step's action sets.

        @see peano4.solversteps.ActionSet.descend_invocation_order

        """
        self.__sort()
        if len(self.action_sets) == 0:
            return 0
        return self.action_sets[0].descend_invocation_order

    def get_vertex_operations_signature(self):
        result = ["marker", "[[maybe_unused]] const ::peano4::dataview::VertexMarker&"]
        for data in self.vertex_data:
            result += ["fineGridVertex" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        for data in self.vertex_data:
            result += ["coarseGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["coarseGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["coarseGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        return result

    def get_face_operations_signature(self):
        result = ["marker", "[[maybe_unused]] const ::peano4::dataview::FaceMarker&"]
        for data in self.vertex_data:
            result += ["fineGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["fineGridFace" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        for data in self.vertex_data:
            result += ["coarseGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["coarseGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["coarseGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        return result

    def get_cell_operations_signature(self):
        result = ["marker", "[[maybe_unused]] const ::peano4::dataview::CellMarker&"]
        for data in self.vertex_data:
            result += ["fineGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["fineGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["fineGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        for data in self.vertex_data:
            result += ["coarseGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["coarseGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["coarseGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]
        return result

    def get_touch_cell_signature(self):
        result = ["marker", "[[maybe_unused]] const ::peano4::dataview::CellMarker&"]
        for data in self.vertex_data:
            result += ["fineGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["fineGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["fineGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]

        for data in self.vertex_data:
            result += ["coarseGridVertices" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.face_data:
            result += ["coarseGridFaces" + data.name, "[[maybe_unused]] " + data.get_enumeration_type()]
        for data in self.cell_data:
            result += ["coarseGridCell" + data.name, "[[maybe_unused]] " + data.get_full_qualified_type() + "&"]

        return result

    def construct_output(self, output):
        """
        Generates the observer and action-set classes of this step.
        """

        included_action_sets = []
        for action_set in self.action_sets:
            full_qualified_action_set_name = self.action_set_generator.construct_output(
                output, action_set
            )
            included_action_sets.append(full_qualified_action_set_name)

        self.observer_generator.construct_output(output, included_action_sets)

    def copy_action_sets_from_other_step(self, other_step):
        self.action_sets += other_step.action_sets
