# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Integer import Integer


class IntegerArray(Integer):
  def __init__(self, name, cardinality, min_value=None, max_value=None):
    Integer.__init__(self, name)
    self._cardinality = cardinality
    self._min_value   = min_value
    self._max_value   = max_value
    self.compress     = min_value!=None or max_value!=None


  def use_default_copy_constructor(self):
    """
    
    Cannot use the default copy constructor, as it is an array, 
    i.e. we need some manual deep copying.
    
    """
    return False;


  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "const int*"),
      ("set" + accessor_name + "(int value[" + self._cardinality + "])", "void"),
      ("get" + accessor_name + "(int index) const", "int"),
      ("set" + accessor_name + "(int index, int)", "void"),
    ]


  def get_plain_C_attributes(self):
    if self._min_value!=None or self._max_value!=None:
      return [ ("_" + self._name + "[" + self._cardinality + "]", "double", "[[clang::pack_range(" + str(self._min_value) + "," + str(self._max_value) + ")]]" ) ]
    else:
      return [ ("_" + self._name + "[" + self._cardinality + "]", "double", "" ) ]


  def get_first_plain_C_attribute(self):
    return [ ("_" + self._name + "[0]", "int" ) ]


  def get_method_body(self,signature):
    if self.use_data_store:
      name = "  _dataStore._" + self._name
    else:
      name = "  _" + self._name
    if signature.startswith( "get" ) and  "index" in signature:
      return "  return  " + name + "[index];\n"
    elif signature.startswith( "set" ) and  "index" in signature:
      return name + "[index] = value;\n"
    elif signature.startswith( "get" ):
      return "  return  " + name + ";\n"
    elif signature.startswith( "set" ):
      return "  std::copy_n(value, " + self._cardinality + ", " + name + " );\n"
    else:
      assert(False)
    return ""
  
  def get_native_MPI_type(self):
    return [ ("MPI_INT", self._cardinality ) ]
      
  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name + "[0] << \",...\""
    else:
      return "_" + self._name + "[0] << \",...\""
