# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Double(Attribute):
    """!

    cardinality is a string (you can thus use symbols as well as long as
    they will be defined at compile time). Pass None if you are working
    with a scalar.

    """

    def __init__(
        self,
        name,
        valid_mantissa_bits=None,
        ifdefs=[],
        is_static=False,
        is_const_static=False,
        initval=None,
    ):
        """

        @Pawel Can you add some docu?

        """
        Attribute.__init__(
            self,
            name,
            ifdefs,
            is_static=is_static,
            is_const_static=is_const_static,
            initval=initval,
        )
        self._valid_mantissa_bits = valid_mantissa_bits

    def get_methods(self, _full_qualified_class_name):
        accessor_name = self._name[:1].title() + self._name[1:]
        methods = []
        methods.append(
            ("get" + accessor_name + "() const", "double"),
        )
        if not self.is_const_static:
            methods.append(("set" + accessor_name + "(double value)", "void"))

        return methods

    def get_plain_C_attributes(self):
        typestring = "double"
        namestring = "_" + self._name
        # add const/static qualifiers and initialization values, if needed
        if self.is_static:
            typestring = "inline static double"
            if self._initval is not None:
                namestring += " = " + str(self._initval)
        elif self.is_const_static:
            typestring = "inline const static double"
            namestring += " = " + str(self._initval)

        if self._valid_mantissa_bits is None:
            return [(namestring, typestring, "")]
        else:
            mantissastring = (
                "[[clang::truncate_mantissa(" + str(self._valid_mantissa_bits) + ")]]"
            )
            return [(namestring, typestring, mantissastring)]

    def get_method_body(self, signature):
        if signature.startswith("get") and self.use_data_store:
            return "  return _dataStore._" + self._name + ";\n"
        elif signature.startswith("set") and self.use_data_store:
            return "  _dataStore._" + self._name + " = value;\n"
        elif signature.startswith("get") and not self.use_data_store:
            return "  return _" + self._name + ";\n"
        elif signature.startswith("set") and not self.use_data_store:
            return "  _" + self._name + " = value;\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_DOUBLE", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name

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
