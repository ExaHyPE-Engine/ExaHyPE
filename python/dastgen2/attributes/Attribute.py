# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import abc
from dastgen2.Utils import construct_ifdef_string


class Attribute(object):
  """

  Represents one attribute

  is_static: bool
    if True, make attribute static.

  is_const_static: bool
    if True make attribute const and static.

  initval: str or None
    if is_static or is_const_static are True, then provide initialisation
    value via this parameter. This is optional is is_static = True, but
    mandatory if is_const_static = True.
  """
  def __init__(self, name, ifdefs=[], is_static=False, is_const_static=False, initval=None):
    self._name          = name
    self.use_data_store = False
    self.ifdefs         = ifdefs
    self.is_static      = is_static
    self.is_const_static= is_const_static
    self._initval       = initval

    if self.is_static and self.is_const_static:
        raise ValueError("Attribute", self._name, ": Can't be static and const static. Pick one.")

    if self.is_const_static and self._initval is None:
        raise ValueError("Attribute", self._name, ": You need to provide an initialisation value for a static attribute")


  def get_public_fields(self):
    """

      Return string that is to be embedded into the public part of the class
      definition. Most attributes don't add anything here, but some (alike
      enumerations) need to.

    """
    return ""

  @abc.abstractmethod
  def get_methods(self,_full_qualified_class_name):
    """

      Return sequence of methods that are defined for this attribute. Each
      entry is a tuple. Its first entry is the signature of the function
      (not including the semicolon), the second entry is the return type.

    """
    return


  def use_default_copy_constructor(self):
    """

    If this routine returns False, the generator will create a copy
    constructor copying each attribute over via a setter/getter
    combination.

    """
    return True


  def get_constructor_arguments(self):
    """

    Return list of tuple of arguments for the constructor. The first
    tuple entry is the name, the second one the type. By default,
    I take them from the plain C attributes, but you can alter
    this behaviour.

    """
    return self.get_plain_C_attributes()


  @abc.abstractmethod
  def get_plain_C_attributes(self):
    """

      Return list of n-tuples. The tuple can either host two, three or
      four entries.


      ### Two entries

      The first triple entry always is the name, the second one the type.
      Type has to be plain C.

      ### Three entries

      The first triple entry always is the name, the second one the type.
      Type has to be plain C. The third entry is a C++ attribute, i.e.
      a string embedded into [[...]].

      ### Four entries

      The first triple entry always is the name, the second one the type.
      Type has to be plain C. The third entry is a C++ attribute, i.e.
      a string embedded into [[...]].
      The fourth attribute is a list of ifdef symbols that have to be
      defined to use this attribute. The list of
      symbols is concatenated with an and. You can return an empty list.
      All symbol definitions can also be expressions such sa DEBUG>0
      or defined(DEBUG).

      Please note that these latter ifdefs are something completely
      different than the base class ifdefs. The base class ifdefs are
      to be used if you want to switch attributes on and off. The
      attributes here are used to switch between realisation variants.


      ## Arrays

      Arrays can be modelled by adding a cardinality ("[15]" for example)
      to the first triple entry, i.e. the name.

    """
    assert False, "abstract function should not be called"
    return


  def get_first_plain_C_attribute(self):
    """

     For MPI for example, I need to know the first attribute. If you map
     your attribute onto multiple data types, it is one type that represents the whole thing.

    """
    return self.get_plain_C_attributes()


  def get_attribute_declaration_string(self):
    """
    docu: TODO MLADEN
    """

    decString = ""
    decString += construct_ifdef_string(self.ifdefs)

    for subattribute in self.get_plain_C_attributes():   # one attribute logically can map onto multiple technical attributes
      if len(subattribute)==4:
        decString  += construct_ifdef_string(subattribute[3])
        decString  += "    " + subattribute[2] + "  " + subattribute[1] + "   " + subattribute[0] + ";\n"
        decString  += "#endif\n"
      elif len(subattribute)==3:
        decString  += "    " + subattribute[2] + "  " + subattribute[1] + "   " + subattribute[0] + ";\n"
      elif len(subattribute)==2:
        decString  += "    " + subattribute[1] + "   " + subattribute[0] + ";\n"
      else:
        raise IndexError("wrong number of attribute tuples")

    if self.ifdefs!=[]:
      decString  += "#endif \n"

    return decString


  @abc.abstractmethod
  def get_method_body(self,signature):
    """

      I hand in the method signature (see get_methods()) and wanna get
      the whole implementation.

    """
    assert False, "not implemented"
    return


  @property
  def name(self):
    """

    I expect that there's at least one setter/getter pair

    """
    return self._name


  @abc.abstractmethod
  def get_native_MPI_type(self):
    """

      Return native (built-in) MPI datatype. Return None if there's no
      direct mapping. The actual result is not a string only, but a list
      of tuples from native type to cardinality.

    """
    return


  @abc.abstractmethod
  def get_to_string(self):
    """

      Return string representation of attribute. Should be something
      that can be streamed via << into a sstream. So the result has
      to be of type string.

    """
    return
