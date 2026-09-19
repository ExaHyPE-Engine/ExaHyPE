# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
class ActionSet(object):
    """!

    Action set (reactions to events)

    The class attributes starting with OPERATION_ name the events an action set listens to.
    Peano's documentation holds a @ref peano_action_sets "discussion of events and their
    temporal and spatial ordering".
    """

    def __init__(self, descend_invocation_order=0):
        self.descend_invocation_order = descend_invocation_order

    def get_constructor_body(self):
        return ""

    def get_static_initialisations(self, full_qualified_classname):
        return ""

    def get_destructor_body(self):
        return ""

    def grid_control_events_body(self):
        return "  return std::vector<peano4::grid::GridControlEvent>();\n"

    def prepare_traversal_body(self):
        return "\n"

    def unprepare_traversal_body(self):
        return "\n"

    OPERATION_BEGIN_TRAVERSAL = "beginTraversal"
    OPERATION_END_TRAVERSAL = "endTraversal"

    OPERATION_CREATE_VERTEX = "createVertex"
    OPERATION_DESTROY_VERTEX = "destroyVertex"
    OPERATION_CREATE_FACE = "createFace"
    OPERATION_DESTROY_FACE = "destroyFace"
    OPERATION_CREATE_CELL = "createCell"
    OPERATION_DESTROY_CELL = "destroyCell"

    OPERATION_TOUCH_VERTEX_FIRST_TIME = "touchVertexFirstTime"
    OPERATION_TOUCH_VERTEX_LAST_TIME = "touchVertexLastTime"
    OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME = "touchHangingVertexFirstTime"
    OPERATION_TOUCH_HANGING_VERTEX_LAST_TIME = "touchHangingVertexLastTime"
    OPERATION_TOUCH_FACE_FIRST_TIME = "touchFaceFirstTime"
    OPERATION_TOUCH_FACE_LAST_TIME = "touchFaceLastTime"
    OPERATION_TOUCH_HANGING_FACE_FIRST_TIME = "touchHangingFaceFirstTime"
    OPERATION_TOUCH_HANGING_FACE_LAST_TIME = "touchHangingFaceLastTime"
    OPERATION_TOUCH_CELL_FIRST_TIME = "touchCellFirstTime"
    OPERATION_TOUCH_CELL_LAST_TIME = "touchCellLastTime"

    def operation_body(self, operation_name):
        """!

        Returns the C++ snippet of the event operation_name, one of the OPERATION_ constants.
        """
        return ""

    @property
    def name(self):
        """!

        Returns the unique name of the action set, which the name of the generated class
        contains. It is the defining module of the concrete class, so every action set is
        distinct without restating it.
        """
        return type(self).__module__.replace(".", "_")

    def is_user_template(self):
        """!

        Returns whether the user modifies the generated code, which then goes into the
        directory actions.
        """
        return False

    def attributes(self):
        """!

        Returns the attributes of the generated class.

        An action set is not persistent: The traversal creates one object per
        grid sweep per tree.
        """
        return ""

    def includes(self):
        """Returns the headers the generated class includes, as paths below ``src`` or below
        the generated directory; the writer merges, sorts and deduplicates them."""
        return []
