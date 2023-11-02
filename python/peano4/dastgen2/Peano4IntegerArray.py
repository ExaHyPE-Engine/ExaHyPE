# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2.attributes.IntegerArray
from dastgen2.Utils import LLVMSymbol


class Peano4IntegerArray(dastgen2.attributes.IntegerArray):
    def __init__(self, name, cardinality, min_value=None, max_value=None, ifdefs=[]):
        dastgen2.attributes.IntegerArray.__init__(
            self, name, cardinality, min_value, max_value, ifdefs=ifdefs
        )
        self._cardinality = str(cardinality)

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            (
                "get" + accessor_name + "() const",
                "tarch::la::Vector<" + self._cardinality + ",int>",
            ),
            (
                "set"
                + accessor_name
                + "(const tarch::la::Vector<"
                + self._cardinality
                + ",int>& value)",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "int"),
            ("set" + accessor_name + "(int index, int value)", "void"),
        ]

    def get_plain_C_attributes(self, for_constructor=False):
        if self.compress:
            return [
                (
                    "_" + self._name + "[" + self._cardinality + "]",
                    "int",
                    "[[clang::pack_range("
                    + str(self._min_value)
                    + ","
                    + str(self._max_value)
                    + ")]]",
                    ["defined(" + LLVMSymbol + ")"],
                ),
                (
                    "_" + self._name,
                    "tarch::la::Vector<" + self._cardinality + ",int>",
                    "",
                    ["!defined(" + LLVMSymbol + ")"],
                ),
            ]
        else:
            return [
                ("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",int>")
            ]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + ".data()[0]", "int")]

    def get_constructor_arguments(self):
        return [("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",int>")]

    def use_default_copy_constructor(self):
        return not self.compress

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name

        if signature.startswith("get") and "index" in signature and not self.compress:
            return "  return " + name + "(index);\n"
        elif signature.startswith("set") and "index" in signature and not self.compress:
            return name + "(index) = value;\n"
        elif signature.startswith("get") and "index" in signature and not self.compress:
            return "  return " + name + "(index);\n"
        elif signature.startswith("set") and "index" in signature and not self.compress:
            return name + "(index) = value;\n"
        elif signature.startswith("get") and not self.compress:
            return "  return  " + name + ";\n"
        elif signature.startswith("set") and not self.compress:
            return name + " = value;\n"

        elif signature.startswith("get") and "index" in signature:
            return "  return " + name + "[index];\n"
        elif signature.startswith("set") and "index" in signature:
            return name + "[index] = value;\n"
        elif signature.startswith("get"):
            return (
                """
  tarch::la::Vector<"""
                + self._cardinality
                + """,int> result;
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
        elif signature.startswith("set"):
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
            assert False, "signature=" + signature

        return ""

    def get_native_MPI_type(self):
        return [("MPI_INT", self._cardinality)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.compress:
            return "get" + self._name[0].upper() + self._name[1:] + "()"
        elif self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name
