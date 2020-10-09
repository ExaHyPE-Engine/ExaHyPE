# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Enumeration(Attribute):
  """
  
  Wrapper around C++ enumerations which is not a datatype supported
  natively by MPI.
  
  The attribute has two-fold meaning. It defines a enum class subtype
  within the generated code and it creates a new attribute of this 
  type.
  
  :Arguments:
  
  name: String
     Something that can become a C++ identifier
    
  variants: [String]
     Sequence of strings. The strings have to be something C++ accepts
     as enumeration identifiers.
  
  """
  def __init__(self, name, variants):
    Attribute.__init__(self, name)
    self._variants = variants
    
  def _enum_name(self):
    return self._name[:1].title() + self._name[1:]

  def get_public_fields(self):
    result = "    enum class " + self._enum_name() + """: int {
      """
    
    for i in self._variants:
      if self._variants.index(i)!=0:
        result += ", "
      result += i
      result += "="
      result += str(self._variants.index(i))
    
    result += """    
    };"""
    return result
    
  def get_methods(self,_full_qualified_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", _full_qualified_name + "::" + accessor_name),
      ("set" + accessor_name + "(" + accessor_name + " value)", "void")
    ]

  def get_plain_C_attributes(self):
    return [ ("_" + self._name, self._enum_name() ) ]

  def get_method_body(self,signature):
    if signature.startswith( "get" ):
      return "  return _" + self._name + ";\n"
    elif signature.startswith( "set" ):
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
    result  = ""
    for i in self._variants:
      if self._variants.index(i) != 0:
        result += " << "
      result += "(_" + self._name + "=="
      result += self._enum_name()
      result += "::"
      result += i
      result += "? \""
      result += i
      result += "\" : \"\") "
    return result

