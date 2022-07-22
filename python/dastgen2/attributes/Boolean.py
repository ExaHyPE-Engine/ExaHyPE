# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Boolean(Attribute):
  def __init__(self, name):
    Attribute.__init__(self, name)
    self.compress       = True
    
    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "bool"),
      ("set" + accessor_name + "(bool value)", "void")
    ]


  def get_plain_C_attributes(self):
    if self.compress:
      return [ ("_" + self._name, "bool", "[[clang::pack]]" ) ]
    else:
      return [ ("_" + self._name, "bool", "" ) ]


  def get_method_body(self,signature):
    if signature.startswith( "get" ) and self.use_data_store:
      return "  return _dataStore._" + self._name + ";\n"
    elif signature.startswith( "get" ) and not self.use_data_store:
      return "  return _" + self._name + ";\n"
    elif signature.startswith( "set" ) and self.use_data_store:
      return "  _dataStore._" + self._name + " = value;\n"
    elif signature.startswith( "set" ) and not self.use_data_store:
      return "  _" + self._name + " = value;\n"
    else:
      assert(False)
    return ""

  
  def get_native_MPI_type(self):
    """
    
      I originally wanted to map the booleans to MPI_CXX_BOOL. But 
      neither CXX_BOOL nor C_BOOL work for the Intel archtiectures. 
      All the datatypes that I use then seg fault. If I however map
      the bool to an integer, then it seems to work. 
      
    """
    #return [ ("MPI_C_BOOL", 1) ]
    return [ ("MPI_BYTE", 1) ]

  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name
    else:
      return "_" + self._name

