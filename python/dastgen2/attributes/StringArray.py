# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute
from .Integer import Integer


class StringArray(String):
    def __init__(
        self,
        name,
        cardinality,
        max_length=80,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        if qualifier != Attribute.Qualifier.NONE:
            raise NotImplementedError(
                f"Attribute {name}: This data type can't work with qualifiers (yet)"
            )
        String.__init__(
            self, name, max_length=max_length, ifdefs=ifdefs, qualifier=qualifier, initval=initval
        )
        self._cardinality = str(cardinality)  # ensure this is a string.

    def use_default_copy_constructor(self):
        """

        Cannot use the default copy constructor, as it is an array,
        i.e. we need some manual deep copying.

        """
        return False

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
                ("get" + accessor_name + "() const", "std::string*"),
                ("set" + accessor_name + "(const std::string value[" + self._cardinality + "])", "void"),
                ("get" + accessor_name + "(int index) const", "std::string"),
                ("set" + accessor_name + "(int index, std::string& value)", "void"),
        ]

    def get_plain_C_attributes(self):
        return [("_" + self._name + "[" + self._cardinality + "]", "std::string", "")]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + "[0]", "int")]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
        if signature.startswith("get") and "index" in signature:
            return "  return  " + name + "[index];\n"
        elif signature.startswith("set") and "index" in signature:
            return name + "[index] = value;\n"
        elif signature.startswith("get"):
            return "  return  " + name + ";\n"
        elif signature.startswith("set"):
            return "  std::copy_n(value, " + self._cardinality + ", " + name + " );\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        # TODO: this needs checking.
        return [("MPI_CHAR", self._max_length * self._cardinality), ("MPI_INT", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name + '[0] << ",..."'
        else:
            return "_" + self._name + '[0] << ",..."'
