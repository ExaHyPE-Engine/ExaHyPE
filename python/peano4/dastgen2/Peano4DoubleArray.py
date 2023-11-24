# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2.attributes.DoubleArray
from dastgen2.Utils import LLVMSymbol


class Peano4DoubleArray(dastgen2.attributes.DoubleArray):
    """

    Specialisation of dastgen2.attributes.DoubleArray which relies on
    Peano's tarch. Therefore, things alike the vector initialisation
    in the constructor do work.

    """

    def __init__(self, name, cardinality, valid_mantissa_bits=None, ifdefs=[]):
        """
        See superclass' constructor.

        cardinality: String
          This is important: It is not (necessarily) an integer, but can be
          a string which is defined via a pragma or constexpr later.

        """
        super(Peano4DoubleArray, self).__init__(
            name, cardinality, valid_mantissa_bits=valid_mantissa_bits, ifdefs=ifdefs
        )
        self._cardinality = str(cardinality)

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            (
                "get" + accessor_name + "() const",
                "tarch::la::Vector<" + self._cardinality + ",double>",
            ),
            (
                "set"
                + accessor_name
                + "(const tarch::la::Vector<"
                + self._cardinality
                + ",double>& value)",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "double"),
            ("set" + accessor_name + "(int index, double value)", "void"),
        ]

    def use_default_copy_constructor(self):
        return self._valid_mantissa_bits == None

    def get_plain_C_attributes(self, for_constructor=False):
        if self._valid_mantissa_bits != None:
            return [
                (
                    "_" + self._name + "[" + self._cardinality + "]",
                    "double",
                    "[[clang::truncate_mantissa("
                    + str(self._valid_mantissa_bits)
                    + ")]]",
                    ["defined(" + LLVMSymbol + ")"],
                ),
                (
                    "_" + self._name,
                    "tarch::la::Vector<" + self._cardinality + ",double>",
                    "",
                    ["!defined(" + LLVMSymbol + ")"],
                ),
            ]
        else:
            return [
                (
                    "_" + self._name,
                    "tarch::la::Vector<" + self._cardinality + ",double>",
                )
            ]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + ".data()[0]", "double")]

    def get_constructor_arguments(self):
        return [
            ("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",double>")
        ]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
        if (
            signature.startswith("get")
            and "index" in signature
            and self._valid_mantissa_bits == None
        ):
            return "  return " + name + "(index);\n"
        elif (
            signature.startswith("set")
            and "index" in signature
            and self._valid_mantissa_bits == None
        ):
            return name + "(index) = value;\n"
        elif signature.startswith("get") and self._valid_mantissa_bits == None:
            return "  return " + name + ";\n"
        elif signature.startswith("set") and self._valid_mantissa_bits == None:
            return name + " = value;\n"
        elif (
            signature.startswith("get")
            and "index" in signature
            and self._valid_mantissa_bits != None
        ):
            return "  return " + name + "[index];\n"
        elif (
            signature.startswith("set")
            and "index" in signature
            and self._valid_mantissa_bits != None
        ):
            return name + "[index] = value;\n"
        elif signature.startswith("get") and self._valid_mantissa_bits != None:
            return (
                """
  tarch::la::Vector<"""
                + self._cardinality
                + """,double> result;
  for( int i=0; i<"""
                + self._cardinality
                + """; i++) {
    result(i) = """
                + name
                + """[i];
  }
  return result;
      """
            )
        elif signature.startswith("set") and self._valid_mantissa_bits != None:
            return (
                """
  for( int i=0; i<"""
                + self._cardinality
                + """; i++) {
    """
                + name
                + """[i] = value(i);
  }
      """
            )
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_DOUBLE", self._cardinality)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self._valid_mantissa_bits != None:
            return "get" + self._name[0].upper() + self._name[1:] + "()"
        elif self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name
