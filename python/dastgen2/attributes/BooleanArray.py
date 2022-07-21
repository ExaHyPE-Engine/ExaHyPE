# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Boolean import Boolean


class BooleanArray(Boolean):
  def __init__(self, name, cardinality):
    Boolean.__init__(self, name)
    self._cardinality = cardinality
    self.compress     = True
   
    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "std::bitset<" + self._cardinality + ">"),
      ("set" + accessor_name + "(const std::bitset<" + self._cardinality + ">&  value)", "void"),
      ("get" + accessor_name + "(int index) const", "bool"),
      ("set" + accessor_name + "(int index, bool value)", "void"),
      ("flip" + accessor_name + "(int index)", "void"),
    ]

  def get_plain_C_attributes(self):
    if self.compress:
      return [ ("_" + self._name + "[" + self._cardinality + "]", "bool", "[[clang::pack]]" ) ]
    else:
      return [ ("_" + self._name, "std::bitset<" + self._cardinality + ">", "" ) ]


  def get_first_plain_C_attribute(self):
    return [ ("_" + self._name , "bool" ) ]


  def get_method_body(self,signature):
    if self.use_data_store:
      name = "  _dataStore._" + self._name
    else:
      name = "  _" + self._name
    if signature.startswith( "flip" ) and  "index" in signature and self.compress:
      return name + "[index] = not " + name + "[index];\n"
    if signature.startswith( "flip" ) and  "index" in signature:
      return name + ".flip(index);\n"
    elif signature.startswith( "get" ) and  "index" in signature:
      return "  return " + name + "[index];\n"
    elif signature.startswith( "set" ) and  "index" in signature:
      return name + "[index] = value;\n"
    elif signature.startswith( "get" ) and self.compress:
      return """
  std::bitset<""" + self._cardinality + """> result;
  for (int i=0; i<""" + self._cardinality + """; i++) result[i] = """ + name + """[i];
  return result;
"""
    elif signature.startswith( "get" ):
      return "  return " + name + ";\n"
    elif signature.startswith( "set" ) and self.compress:
      return """
  for (int i=0; i<""" + self._cardinality + """; i++) """ + name + """[i]=value[i];
"""      
    elif signature.startswith( "set" ):
      return name + " = value;\n"
    else:
      assert(False)
    return ""
 
  
  def get_native_MPI_type(self):
    return [ ("MPI_UNSIGNED_LONG", 1 ) ]
 
 
  def get_constructor_arguments(self):
    return [( "_"+self._name, "std::bitset<" + self._cardinality + ">" )]
  
  
  def use_setter_to_initialise_arguments(self):
    return self.compress

  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name
    else:
      return "_" + self._name

