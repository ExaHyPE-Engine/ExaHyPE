# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class Boolean(Attribute):
    def __init__(
        self,
        name,
        ifdefs=[],
        is_static=False,
        is_const_static=False,
        initval=None,
    ):
        Attribute.__init__(
            self,
            name,
            ifdefs,
            is_static=is_static,
            is_const_static=is_const_static,
            initval=initval,
        )

        self.compress = True

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        methods = []

        if self.is_static and for_declaration:
            qualifiers = "static "
        if self.is_const_static:
            if for_declaration:
                qualifiers = "static const "
            else:
                qualifiers = "const "

        if self.is_static or self.is_const_static:
            getter = ("get" + accessor_name + "()", qualifiers + "bool")
        else:
            getter = ("get" + accessor_name + "() const", "bool")

        methods.append(getter)

        if not self.is_const_static:
            methods.append(("set" + accessor_name + "(bool value)", "void"))

        return methods

    def get_plain_C_attributes(self):
        typestring = "bool"
        namestring = "_" + self._name
        # add const/static qualifiers and initialization values, if needed
        if self.is_static:
            typestring = "inline static bool"
            if self._initval is not None:
                namestring += " = " + str(self._initval)
        elif self.is_const_static:
            typestring = "inline const static bool"
            namestring += " = " + str(self._initval)

        if self.compress:
            packstring = "[[clang::pack]]"
            return [(namestring, typestring, packstring)]
        else:
            return [(namestring, typestring, "")]

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
        """

        I originally wanted to map the booleans to MPI_CXX_BOOL. But
        neither CXX_BOOL nor C_BOOL work for the Intel archtiectures.
        All the datatypes that I use then seg fault. If I however map
        the bool to an integer, then it seems to work.

        """
        # return [ ("MPI_C_BOOL", 1) ]
        return [("MPI_BYTE", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store:
            return "_dataStore._" + self._name
        else:
            return "_" + self._name
