# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute
from .Boolean import Boolean
from dastgen2.Utils import LLVMSymbol


class BooleanArray(Boolean):
    """

    Represent an array of boolean flags

    Consult the constructor on details onto which C++ data these flags
    are mapped.

    """

    def __init__(
        self,
        name,
        cardinality,
        compress=False,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        """

        C++ offers the bitset to accommodate an array of boolean flags. While
        a bitset provides a memory-efficient way to encode a set of booleans,
        it is not efficient if we use it in combination with other booleans or
        other bitsets, as the C++ compiler will insert empty bits to align
        each attribute at least with a byte.

        Our LLVM modification can eliminate these fill-in bits, but it does
        not natively support bitsets (as we, in general, do not cover stuff
        from the standard library). If you want to use the extension, you have
        to model the bitset as an array of booleans natively.



        ## Attributes

        compress: Boolean
          Without compression (default), map array onto bitet. Otherwise,
          use native array of boolean.

        """
        if qualifier != Attribute.Qualifier.NONE:
            raise NotImplementedError(
                f"Attribute {name}: This data type can't work with qualifiers (yet)"
            )
        if initval is not None:
            raise NotImplementedError(
                f"Attribute {name}: Can't generate arrays with initial values (yet)."
            )
        Boolean.__init__(
            self, name, ifdefs=ifdefs, qualifier=qualifier, initval=initval
        )
        self._cardinality = str(cardinality)
        self.compress = compress

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            (
                "get" + accessor_name + "() const",
                "std::bitset<" + self._cardinality + ">",
            ),
            (
                "set"
                + accessor_name
                + "(const std::bitset<"
                + self._cardinality
                + ">&  value)",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "bool"),
            ("set" + accessor_name + "(int index, bool value)", "void"),
            ("flip" + accessor_name + "(int index)", "void"),
        ]

    def get_plain_C_attributes(self, for_constructor=False):
        if self.compress:
            return [
                (
                    "_" + self._name + "[" + self._cardinality + "]",
                    "bool",
                    "[[clang::pack]]",
                    ["defined(" + LLVMSymbol + ")"],
                ),
                (
                    "_" + self._name,
                    "std::bitset<" + self._cardinality + ">",
                    "",
                    ["!defined(" + LLVMSymbol + ")"],
                ),
            ]
        else:
            return [("_" + self._name, "std::bitset<" + self._cardinality + ">")]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name, "bool")]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
        if signature.startswith("flip") and "index" in signature and self.compress:
            return name + "[index] = not " + name + "[index];\n"
        if signature.startswith("flip") and "index" in signature:
            return name + ".flip(index);\n"
        elif signature.startswith("get") and "index" in signature:
            return "  return " + name + "[index];\n"
        elif signature.startswith("set") and "index" in signature:
            return name + "[index] = value;\n"
        elif signature.startswith("get") and self.compress:
            return """
  std::bitset<{}> result;
  for (int i=0; i<{}; i++) result[i] = {}[i];
  return result;
""".format(
                self._cardinality, self._cardinality, name
            )
        elif signature.startswith("get"):
            return "  return " + name + ";\n"
        elif signature.startswith("set") and self.compress:
            return (
                """
  for (int i=0; i<"""
                + self._cardinality
                + """; i++) """
                + name
                + """[i]=value[i];
"""
            )
        elif signature.startswith("set"):
            return name + " = value;\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_UNSIGNED_LONG", 1)]

    def get_constructor_arguments(self):
        return [("_" + self._name, "std::bitset<" + self._cardinality + ">")]

    def use_default_copy_constructor(self):
        return not self.compress

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
