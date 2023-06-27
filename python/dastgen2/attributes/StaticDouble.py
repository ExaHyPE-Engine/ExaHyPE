# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class StaticDouble(Attribute):
  """

    cardinality is a string (you can thus use symbols as well as long as
    they will be defined at compile time). Pass None if you are working
    with a scalar.

  """
  def __init__(self, name, valid_mantissa_bits=None, ifdefs=[], is_static=False, is_const_static=False, initval=None):
    """

    @Pawel Can you add some docu?

    const: bool
        if True, will make a const static double. Then it also needs an initialisation
        value, and will not have a setter method.

    """
    Attribute.__init__(self, name, ifdefs, is_static=is_static, is_const_static=is_const_static, initval=initval)
    self._valid_mantissa_bits = valid_mantissa_bits


  def get_methods(self,_full_qualified_class_name):
    accessor_name = self._name[:1].title() + self._name[1:]
    return [
      ("get" + accessor_name + "() const", "double"),
      #  ("set" + accessor_name + "(double value)", "void") # TODO clean this
    ]


  def get_plain_C_attributes(self):
    #  if self.is_static or self

    typestring = "double"
    namestring = "_" + self._name
    if self.is_static:
        typestring = "inline static double"
    if self.is_const_static:
        typestring = "inline const static double"
        namestring += " = " + str(self._initval)
    if self._valid_mantissa_bits is None:
      return [ (namestring, typestring, "" ) ]
    else:
      return [ (namestring, typestring, "[[clang::truncate_mantissa(" + str(self._valid_mantissa_bits) + ")]]" ) ]


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


  def get_public_fields(self):

    return ""
    # TODO: DO I STILL NEED THIS????
    if self.is_static:
      return self.get_plain_C_attributes() + ";\n"

    elif self.is_const_static:
      return self.get_plain_C_attributes() + " = " + self._initval  + ";\n"

    else:
      return super(self).get_public_fields()





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

