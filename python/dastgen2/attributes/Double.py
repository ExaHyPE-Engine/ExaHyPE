# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Double(Attribute):
  """
  
    cardinality is a string (you can thus use symbols as well as long as 
    they will be defined at compile time). Pass None if you are working 
    with a scalar.
    
  """
  def __init__(self, name, valid_mantissa_bits=None, ifdefs=[]):
    """
    
    @Pawel Can you add some docu?
    
    """
    Attribute.__init__(self, name, ifdefs)
    self._valid_mantissa_bits = valid_mantissa_bits
    
    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "double"),
      ("set" + accessor_name + "(double value)", "void")
    ]


  def get_plain_C_attributes(self):
    if self._valid_mantissa_bits==None:
      return [ ("_" + self._name, "double", "" ) ]
    else:
      return [ ("_" + self._name, "double", "[[clang::truncate_mantissa(" + str(self._valid_mantissa_bits) + ")]]" ) ]


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
    return [ ("MPI_DOUBLE", 1) ]
      
  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name
    else:
      return "_" + self._name

