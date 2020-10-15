# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from abc import abstractmethod
 

class ActionSet(object):
  """
  
    Actions are triggered by the observer and represent the highest
    grid traversal view that I have implemented within Peano. They
    listen to the tree traversal through notifications alike "this 
    is the first time I see (touch) a vertex". A list of messages 
    that we can listen to is specified by this class through class
    attributes starting with OPERATION_.
    
    Action sets are non-persistent, i.e. if you generate C++ code, 
    remind yourself that the class implementing a particular action
    set will be generated once per grid sweep per tree. As a logical
    consequence, different action sets are in principle totally 
    independent. If they exchange data, you have to realise this.
    However, there's a creational routine and a merge command, i.e.
    you can implement forks and joins (reductions) "natively". There
    are also functions to inject static data into an action set.
    
  """
  def __init__(self):
    pass


  @abstractmethod
  def get_constructor_body(self):
    return "// @todo Should be overwritten\n"


  def get_static_initialisations(self,full_qualified_classname):
    return ""


  @abstractmethod
  def get_destructor_body(self):
    return "// @todo Should be overwritten\n"


  def get_body_of_getGridControlEvents(self):
    return "return std::vector< peano4::grid::GridControlEvent >();\n" 


  OPERATION_BEGIN_TRAVERSAL           = "beginTraversal"
  OPERATION_END_TRAVERSAL             = "endTraversal"
  
  OPERATION_CREATE_PERSISTENT_VERTEX  = "createPersistentVertex"
  OPERATION_DESTROY_PERSISTENT_VERTEX = "destroyPersistentVertex"
  OPERATION_CREATE_HANGING_VERTEX     = "createHangingVertex"
  OPERATION_DESTROY_HANGING_VERTEX    = "destroyHangingVertex"
  OPERATION_CREATE_PERSISTENT_FACE    = "createPersistentFace"
  OPERATION_DESTROY_PERSISTENT_FACE   = "destroyPersistentFace"
  OPERATION_CREATE_HANGING_FACE       = "createHangingFace"
  OPERATION_DESTROY_HANGING_FACE      = "destroyHangingFace"
  OPERATION_CREATE_CELL               = "createCell"
  OPERATION_DESTROY_CELL              = "destroyCell"
  
  OPERATION_TOUCH_VERTEX_FIRST_TIME   = "touchVertexFirstTime"
  OPERATION_TOUCH_VERTEX_LAST_TIME    = "touchVertexLastTime"
  OPERATION_TOUCH_FACE_FIRST_TIME     = "touchFaceFirstTime"
  OPERATION_TOUCH_FACE_LAST_TIME      = "touchFaceLastTime"
  OPERATION_TOUCH_CELL_FIRST_TIME     = "touchCellFirstTime"
  OPERATION_TOUCH_CELL_LAST_TIME      = "touchCellLastTime"


  @abstractmethod
  def get_body_of_operation(self,operation_name):
    """
     See class' string constants starting with OPERATION_ for possible values
     of operation_name
    """
    return "// @todo Should be overwritten by mapping\n"


  @abstractmethod
  def get_action_set_name(self):
    """
     Returns a description (word) for the mapping which is also used as class name
     for the generated type. As a consequence, the result should be one word (if 
     possible) and uppercase. Also, every subclass should overwrite this routine.
    """
    return "Dummy"


  @abstractmethod
  def user_should_modify_template(self):
    """
     Return whether you expect the user to modify the generated code. If this 
     is the case, then the API places the generated output in the directory
     actions. Otherwise, it goes into the observer directory.
    """
    return True


  @abstractmethod
  def get_attributes(self):
    """

     Return attributes as copied and pasted into the generated class.
     
     Please note that action sets are not persistent, i.e. there is one
     object creation per grid sweep per tree.
     
    """
    return ""


  @abstractmethod
  def get_includes(self):
    """
     Return include statements that you need.
    """
    return ""

