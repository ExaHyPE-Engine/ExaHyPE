# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from abc import abstractmethod


class ActionSet(object):
    """!

    Action set (reactions to events)

    Peano runs through the mesh tied to an observer. The observer 
    "sees" events and issues actions tied to these events. That is
    the observer listens to the tree traversal for events alike "this
    is the first time I see (touch) a vertex". A list of events
    that we can listen to is specified by this class through class
    attributes starting with OPERATION_. Peano's documentation holds
    a @ref peano_action_sets "discussion of events and their temporal and spatial ordering".

    For each grid traversal, Peano expects one C++ observer. The
    observer is represented within Python through
    peano4.solversteps.Step, i.e. this class creates eventually one
    peano4.output.Observer which in return writes out the C++ code.
    Each observer can have an arbitrary number of action set classes.
    Each step consequently can hold an action set. The observer's job
    is it to take information like "I've just entered a spacetree
    cell" and to break it down into actions like

      - this vertex is read for the first time
      - this hanging vertex has to be created
      - this face is read for the first time
      - this cell now is entered and these are the associated faces
        and vertices

    For each of these "actions", the generated C++ code will call the
    corresponding functions of all associated action sets one by one.

    @image html ActionSet.png

    The cartoon above illustrates these three layers: Users or Peano
    extensions build up logical program steps (left top). They are
    basically containers with a unique name. Each step holds an arbitrary
    number of ActionSets. They are, first of all, mappings where each
    logical action is potentially tied to a C++ code snippet. When
    the Peano project is generated, each action set is translated into
    its counterpart in the output namespace which is now a complete
    description of this class, i.e. holds all functions required, all
    parameters, ... It copies over the C++ code snippet from its
    specifiation. This Python representation can then be dumped into
    a C++ class.


    ## Lifecycle and parallelisation

    Action sets are non-persistent, i.e. if you generate C++ code,
    remind yourself that the class implementing a particular action
    set will be generated once per grid sweep per tree. As a logical
    consequence, different action sets are in principle totally
    independent. If they exchange data, you have to realise this.
    However, there's a creational routine and a merge command, i.e.
    you can implement forks and joins (reductions) "natively". There
    are also functions to inject static data into an action set.

    There will be one class per action set per context (observer)
    in which it is used. One object will be created per grid sweep, and
    it will have the tree identifier -1 by default. The actual
    objects that then are used are constructed from this prototype
    object via the augmented copy constructor. The other important
    role of the prototype object, i.e. the one tied to spacetree id
    -1, is to manage the grid traversal events. It is this object
    that is asked to hand out grid traversals. Consult the documentation
    of get_constructor_body() for details on the tree id.

    If you want to realise BSP-like programming, then you typically
    realise the data sharing via the copy constructor. The
    attributes of the action set then are all thread-local, i.e.
    there's no need to be afraid of memory races. The join in turn
    should be done within OPERATION_END_TRAVERSAL. Here, you'll need a
    semaphore, as the fusion of objects is not per se thread-safe.


    ## The injected C++ snippets

    To inject C++ code, i.e. what has to be done, you make get_body_of_operation()
    return this code if the corresponding argument is passed in. This result
    should be plain C++ code which the generator really can take and copy
    n paste into the generated implementation files. If you want these
    injected snippets to do something solver-specific, a lot of action
    sets read out there environment and apply jinja2 templates to tailor
    the snippet to their needs. For example: If you want to do something
    with the adjacent vertices of a cell, you have to know what these will
    be called eventually. In this case, it makes sense to study the naming
    conventions that peano4.output.ActionSet will put in place.

    The easiest way to find these out is to generate some code, to look into
    the generated output, and then to reconstruct the naming conventions.
    To be able to do so, it makes sense to let get_action_set_name() return
    a unique name, so it is easier for you to find what C++ file corresponds
    to which action set.
    
    A discussion some event functions and their semantics as well as further
    info on the arguments can be found in the documentation of
    @ref peano_action_sets.


    ## Order of invocation and concurrency level
    
    A discussion of the order of events can be found in the documentation of
    @ref peano_action_sets.
    


    
    @param parallel: Boolean
      See section on "Order of invocation and concurrency level" in class
      documentation.

    @param descend_invocation_order: Integer
      See section on "Order of invocation and concurrency level" in class
      documentation.

    """

    def __init__(self, descend_invocation_order=0, parallel=False):
        self.descend_invocation_order = descend_invocation_order
        self.parallel = parallel
        pass

    @abstractmethod
    def get_constructor_body(self):
        """! Define a tailored constructor body

        By default, the constructor of an action set is empty. If you you assign
        attributes to your action set, you however might want to initialise them
        here. We do not support initialisation lists, to all has to be done via
        setters unless you create attributes on the heap.

        The constructor's signature will look similar to


               EnumerateAndInitSolution2petsc_actionsets_InitVertexDoFs0(int treeNumber);


        where the treeNumber is -1 if this is the global instance of the
        action set owned by a rank, or a number greater or equal 0 if this action
        set is a clone of the glocal action set that's used by one tree traversal.

        @see get_attributes() to add attributes to your action set

        """
        return "// @todo Should be overwritten\n"

    def get_static_initialisations(self, full_qualified_classname):
        return ""

    @abstractmethod
    def get_destructor_body(self):
        return "// @todo Should be overwritten\n"

    def get_body_of_getGridControlEvents(self):
        return "return std::vector< peano4::grid::GridControlEvent >();\n"

    @abstractmethod
    def get_body_of_prepareTraversal(self):
        return "\n"

    @abstractmethod
    def get_body_of_unprepareTraversal(self):
        return "\n"

    OPERATION_BEGIN_TRAVERSAL = "beginTraversal"
    OPERATION_END_TRAVERSAL = "endTraversal"

    OPERATION_CREATE_PERSISTENT_VERTEX = "createPersistentVertex"
    OPERATION_DESTROY_PERSISTENT_VERTEX = "destroyPersistentVertex"
    OPERATION_CREATE_HANGING_VERTEX = "createHangingVertex"
    OPERATION_DESTROY_HANGING_VERTEX = "destroyHangingVertex"
    OPERATION_CREATE_PERSISTENT_FACE = "createPersistentFace"
    OPERATION_DESTROY_PERSISTENT_FACE = "destroyPersistentFace"
    OPERATION_CREATE_HANGING_FACE = "createHangingFace"
    OPERATION_DESTROY_HANGING_FACE = "destroyHangingFace"
    OPERATION_CREATE_CELL = "createCell"
    OPERATION_DESTROY_CELL = "destroyCell"

    OPERATION_TOUCH_VERTEX_FIRST_TIME = "touchVertexFirstTime"
    OPERATION_TOUCH_VERTEX_LAST_TIME = "touchVertexLastTime"
    OPERATION_TOUCH_FACE_FIRST_TIME = "touchFaceFirstTime"
    OPERATION_TOUCH_FACE_LAST_TIME = "touchFaceLastTime"
    OPERATION_TOUCH_CELL_FIRST_TIME = "touchCellFirstTime"
    OPERATION_TOUCH_CELL_LAST_TIME = "touchCellLastTime"

    @abstractmethod
    def get_body_of_operation(self, operation_name):
        """!

        Return actual C++ code snippets to be inserted into C++ code

        See class' string constants starting with OPERATION_ for possible values
        of operation_name.

        """
        return "// @todo Should be overwritten by mapping\n"

    @abstractmethod
    def get_action_set_name(self):
        """!

        Return unique action set name

        Returns a description (word) for the mapping which is also used as class name
        for the generated type. As a consequence, the result should be one word (if
        possible) and uppercase. Also, every subclass should overwrite this routine.

        The generator will take the result and construct eventually classes similar to
        MyStep2Dummy.h and MyStep2Dummy.cpp or similar for the example below, where we
        return Dummy.

        """
        return "Dummy"

    @abstractmethod
    def user_should_modify_template(self):
        """!

        Is the user allowed to modify the output

        Return whether you expect the user to modify the generated code. If this
        is the case, then the API places the generated output in the directory
        actions. Otherwise, it goes into the observer directory and will be
        overwritten in each and every Python run.

        """
        return True

    @abstractmethod
    def get_attributes(self):
        """!

        Return attributes as copied and pasted into the generated class.

        Please note that action sets are not persistent, i.e. there is one
        object creation per grid sweep per tree.

        """
        return ""

    @abstractmethod
    def get_includes(self):
        """!

        Return include statements that you need.

        All of these includes will eventually end up in the header of the generated
        C++ code.

        """
        return ""
