# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Double import Double


class DoubleArray(Double):
  """
   An array of doubles, i.e. a vector in the mathematical sense
   
   
   :Arguments:
   
   name: String
     Name of this vector
     
   cardinality: String
     Cardinality of data. This has to be a string. Therefore it can be a 
     symbol, i.e. a name defined via ifdef somewhere in your code.
          
  """
  def __init__(self, name, cardinality, relative_accuracy_in_bits=None):
    Double.__init__(self, name)
    self._cardinality = cardinality
    self._relative_accuracy_in_bits = relative_accuracy_in_bits

    
  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [ 
      ("get" + accessor_name + "() const", "const double*"),
      ("set" + accessor_name + "(double value[" + self._cardinality + "])", "void"),
      ("get" + accessor_name + "(int index) const", "double"),
      ("set" + accessor_name + "(int index, double value)", "void"),
    ]


  def get_plain_C_attributes(self):
    if self._relative_accuracy_in_bits != None:
      return [ ("_" + self._name + "[" + self._cardinality + "]", "double",
        "[[clang::compress_truncate_mantissa(" + str(self._relative_accuracy_in_bits) + ")]]" ) ]
    else:
      return [ ("_" + self._name + "[" + self._cardinality + "]", "double", "" ) ]


  def get_first_plain_C_attribute(self):
    return [ ("_" + self._name + "[0]", "double" ) ]

  def get_method_body(self,signature):
    if self.use_data_store:
      name = "  _dataStore._" + self._name
    else:
      name = "  _" + self._name
    if signature.startswith( "get" ) and  "index" in signature:
      return "  return " + name + "[index];\n"
    elif signature.startswith( "set" ) and  "index" in signature:
      return name + "[index] = value;\n"
    elif signature.startswith( "get" ):
      return "  return " + name + ";\n"
    elif signature.startswith( "set" ):
      return "  std::copy_n(value, " + self._cardinality + ", " + name + " );\n"
    else:
      assert(False)
    return ""
  
  def get_native_MPI_type(self):
    return [ ("MPI_DOUBLE", self._cardinality ) ]
      
  
  def get_to_string(self):
    """
    
      Return string representation of attribute.

    """
    if self.use_data_store:
      return "_dataStore._" + self._name + "[0] << \",...\""
    else:
      return "_" + self._name + "[0] << \",...\""

