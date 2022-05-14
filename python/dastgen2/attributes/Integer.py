# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Integer(Attribute):
  def __init__(self, name, min_value=None, max_value=None):
    """
    
    min_value: None or integer value
      Has to be smaller than max_value. Can also be a string if this string
      is given by the precompiler.
    
    """
    Attribute.__init__(self, name)
    self._min_value = min_value
    self._max_value = max_value
    
    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "int"),
      ("set" + accessor_name + "(int value)", "void")
    ]
    

  def get_plain_C_attributes(self):
    if self._min_value==None and self._max_value==None:
      return [ ("_" + self._name, "int", "" ) ]
    else:
      return [ ("_" + self._name, "int", "[[clang::compress::" + str(self._min_value) + "-" + str(self._max_value) + "]]" ) ]


  def get_method_body(self,signature):
    if signature.startswith( "get" ) and self.use_data_store:
      return "  return _dataStore._" + self._name + ";\n"
    elif signature.startswith( "set" ) and self.use_data_store:
      return "  _dataStore._" + self._name + " = value;\n"
    elif signature.startswith( "get" ) and not self.use_data_store:
      return "  return _" + self._name + ";\n"
    elif signature.startswith( "set" ) and not self.use_data_store:
      return "  _" + self._name + " = value;\n"
    else:
      assert(False)
    return ""

  
  def get_native_MPI_type(self):
    return [ ("MPI_INT", 1) ]

  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name
    else:
      return "_" + self._name

