# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute
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

    def __init__(
        self,
        name,
        cardinality,
        valid_mantissa_bits=None,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        if qualifier != Attribute.Qualifier.NONE:
            raise NotImplementedError(
                f"Attribute {name}: This data type can't work with qualifiers (yet)"
            )
        if initval is not None:
            raise NotImplementedError(
                f"Attribute {name}: Can't generate arrays with initial values (yet)."
            )
        Double.__init__(self, name, ifdefs=ifdefs, qualifier=qualifier, initval=initval)

        self._cardinality = str(cardinality)
        self._valid_mantissa_bits = valid_mantissa_bits

    def use_default_copy_constructor(self):
        """

        Cannot use the default copy constructor, as it is an array,
        i.e. we need some manual deep copying.

        """
        return False

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            ("get" + accessor_name + "() const", "const double*"),
            (
                "set"
                + accessor_name
                + "(const double value["
                + self._cardinality
                + "])",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "double"),
            ("set" + accessor_name + "(int index, double value)", "void"),
        ]

    def get_plain_C_attributes(self, for_constructor=False):
        if self._valid_mantissa_bits != None:
            return [
                (
                    "_" + self._name + "[" + self._cardinality + "]",
                    "double",
                    "[[clang::truncate_mantissa("
                    + str(self._valid_mantissa_bits)
                    + ")]]",
                )
            ]
        else:
            return [("_" + self._name + "[" + self._cardinality + "]", "double", "")]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + "[0]", "double")]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
        if signature.startswith("get") and "index" in signature:
            return "  return " + name + "[index];\n"
        elif signature.startswith("set") and "index" in signature:
            return name + "[index] = value;\n"
        elif signature.startswith("get"):
            return "  return " + name + ";\n"
        elif signature.startswith("set"):
            return "  std::copy_n(value, " + self._cardinality + ", " + name + " );\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_DOUBLE", self._cardinality)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name + '[0] << ",..."'
        else:
            return "_" + self._name + '[0] << ",..."'

    @property
    def valid_mantissa_bits(self):
        return self._valid_mantissa_bits

    @valid_mantissa_bits.setter
    def valid_mantissa_bits(self, value):
        """!

        Set mantissa used

        Pass in None, if you want to use the default accuracy.

        """
        assert value == None or value > 0
        self._valid_mantissa_bits = value
