# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2.attributes.DoubleArray


class Peano4DoubleArray(dastgen2.attributes.DoubleArray):
  """
  
    Specialisation of dastgen2.attributes.DoubleArray which relies on
    Peano's tarch. Therefore, things alike the vector initialisation 
    in the constructor do work.
     
  """
  
  def __init__(self, name, cardinality):
    """
      See superclass' constructor.
    """
    dastgen2.attributes.DoubleArray.__init__(self, name, cardinality)
    self._cardinality = cardinality
    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "tarch::la::Vector<" + self._cardinality + ",double>"),
      ("set" + accessor_name + "(const tarch::la::Vector<" + self._cardinality + ",double>& value)", "void"),
      ("get" + accessor_name + "(int index) const", "double"),
      ("set" + accessor_name + "(int index, double value)", "void"),
    ]

  def get_plain_C_attributes(self):
    return [ ("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",double>" ) ]

  def get_first_plain_C_attribute(self):
    return [ ("_" + self._name + ".data()[0]", "double" ) ]

  def get_method_body(self,signature):
    if signature.startswith( "get" ) and  "index" in signature:
      return "  return _" + self._name + "(index);\n"
    elif signature.startswith( "set" ) and  "index" in signature:
      return "  _" + self._name + "(index) = value;\n"
    elif signature.startswith( "get" ):
      return "  return _" + self._name + ";\n"
    elif signature.startswith( "set" ):
      return "  _" + self._name + " = value;\n"
    else:
      assert(False)
    return ""
  
  def get_native_MPI_type(self):
    return [ ("MPI_DOUBLE", self._cardinality ) ]
      
  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    return "_" + self._name

