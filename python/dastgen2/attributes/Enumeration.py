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

    def __init__(
        self,
        name,
        variants,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        if qualifier != Attribute.Qualifier.NONE:
            raise NotImplementedError(
                f"Attribute {name}: This data type can't work with qualifiers"
            )
        Attribute.__init__(
            self, name, ifdefs=ifdefs, qualifier=qualifier, initval=initval
        )
        self._variants = variants
        self.compress = True

    def _enum_name(self):
        return self._name[:1].title() + self._name[1:]

    def get_public_fields(self):
        result = (
            """
    enum class """
            + self._enum_name()
            + """: int {
      """
        )

        for i in self._variants:
            if self._variants.index(i) != 0:
                result += ", "
            result += i
            result += "="
            result += str(self._variants.index(i))

        result += """
    };"""
        return result

    def get_methods(self, _full_qualified_name, for_declaration=True):
        accessor_name = self.get_accessor_name()

        toStringQualifier = "const"
        if for_declaration:
            toStringQualifier = "const static"

        return [
            (
                "get" + accessor_name + "() const",
                _full_qualified_name + "::" + accessor_name,
            ),
            ("set" + accessor_name + "(" + accessor_name + " value)", "void"),
            # This method provides a way to get enums as strings.
            (
                "toString(" + accessor_name + " value)",
                toStringQualifier + " std::string",
            ),
        ]

    def get_plain_C_attributes(self, for_constructor=False):
        if self.compress:
            return [("_" + self._name, self._enum_name(), "[[clang::pack]]")]
        else:
            return [("_" + self._name, self._enum_name(), "")]

    def get_method_body(self, signature):
        if signature.startswith("get") and self.use_data_store:
            return "  return _dataStore._" + self._name + ";\n"
        elif signature.startswith("set") and self.use_data_store:
            return "  _dataStore._" + self._name + " = value;\n"
        elif signature.startswith("get") and not self.use_data_store:
            return "  return _" + self._name + ";\n"
        elif signature.startswith("set") and not self.use_data_store:
            return "  _" + self._name + " = value;\n"
        elif signature.startswith("toString") and not self.use_data_store:
            return self._get_to_string_method_body()
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_INT", 1)]

    def _get_to_string_method_body(self):
        """
        This function generates the method body for the toString(value)
        method. The idea is to have an easy way to access enum variants as
        strings for meaningful printouts, such as in the yourDataModel.toString()
        method.

        For developing and debugging purposes, it can be useful to have a way
        of accessing these names, so this method gets a standalone definition.
        """
        result = "  std::ostringstream out;\n  out "

        for i in self._variants:
            result += " << "
            result += "(value =="
            result += self._enum_name()
            result += "::"
            result += i
            result += '? "'
            result += i
            result += '" : "") '
        result += ";\n"
        result += "  return out.str();\n"

        return result

    def get_to_string(self):
        """
        This function generates the method body which is called in
        yourDataModel.toString() method.
        """
        accessor_name = self.get_accessor_name()
        varname = "_" + self.name
        if self.use_data_store:
            varname = "_dataStore._" + self._name

        return "toString(" + varname + ")"
