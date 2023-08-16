# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Integer(Attribute):
    def __init__(
        self,
        name,
        min_value=None,
        max_value=None,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        """

        min_value: None or integer value
          Has to be smaller than max_value. Can also be a string if this string
          is given by the precompiler.

        """
        Attribute.__init__(
            self,
            name,
            ifdefs,
            qualifier=qualifier,
            initval=initval,
        )
        self._min_value = min_value
        self._max_value = max_value

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        methods = []

        if self._is_static:
            if for_declaration:
                getter = ("get" + accessor_name + "()", "static int")
            else:
                getter = ("get" + accessor_name + "()", "int")

        elif self._is_const_static:
            if for_declaration:
                getter = ("get" + accessor_name + "()", "const static int")
            else:
                getter = ("get" + accessor_name + "()", "const int")

        elif self._is_const:
            getter = ("get" + accessor_name + "()", "const int")

        elif self._is_constexpr:
            getter = ("get" + accessor_name + "()", "constexpr int")

        else:
            # default case.
            getter = ("get" + accessor_name + "() const", "int")

        methods.append(getter)

        # Add a setter method, if it makes sense.
        if not (self._is_const_static or self._is_const or self._is_constexpr):
            methods.append(("set" + accessor_name + "(int value)", "void"))

        return methods

    def get_plain_C_attributes(self):
        typestring = "int"
        namestring = "_" + self._name
        # add const/static qualifiers and initialization values, if needed
        if self._is_static:
            typestring = "inline static int"
            if self._initval is not None:
                namestring += " = " + str(self._initval)
        elif self._is_const_static:
            typestring = "inline const static int"
            namestring += " = " + str(self._initval)
        elif self._is_const:
            raise NotImplementedError
        elif self._is_constexpr:
            typestring = "constexpr static int"
            namestring += " = " + str(self._initval)

        if self._min_value is None and self._max_value is None:
            return [(namestring, typestring, "")]
        else:
            packstring = (
                "[[clang::pack_range("
                + str(self._min_value)
                + ","
                + str(self._max_value)
                + ")]]"
            )
            return [(namestring, typestring, packstring)]

    def get_method_body(self, signature):
        if signature.startswith("get") and self.use_data_store:
            return "  return _dataStore._" + self._name + ";\n"
        elif signature.startswith("get") and not self.use_data_store:
            return "  return _" + self._name + ";\n"
        elif signature.startswith("set") and self.use_data_store:
            return "  _dataStore._" + self._name + " = value;\n"
        elif signature.startswith("set") and not self.use_data_store:
            return "  _" + self._name + " = value;\n"
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_INT", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name
