# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class UserDefinedType(Attribute):
    """!

    Wrapper around user-defined attribute

    Wrapper around user-defined object, i.e. an instance of a proper class that
    is not built into C++. At the moment, we support this only for static
    attributes. On the one hand, having real object attributes which stem from
    user code is tricky (are there appropriate to string methods, appropriate
    constructors) for a "normal", serial code. It becomes close to impossible
    to map complex nested classes onto MPI. Therefore, this constraint to
    class attributes.

    """

    def __init__(
        self,
        name,
        type,
        include,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
        getter_returns_reference=True,
        user_type_has_toString_method=True,
    ):
        """!

        Create attribute

        @param name Name of the attribute

        @param type Fully qualified type of the attribute as string separated
          with two colons (::)

        @param include String which holds the include of the type

        For all other attributes see superclass' constructor.

        """
        Attribute.__init__(
            self,
            name,
            ifdefs,
            qualifier=qualifier,
            initval=initval,
            expose_in_header_file=qualifier == Attribute.Qualifier.CONSTEXPR,
        )

        self._type = type
        self._include = include
        self._getter_returns_reference = getter_returns_reference
        self._user_type_has_toString_method = user_type_has_toString_method

        assert (
            qualifier == Attribute.Qualifier.STATIC
            or qualifier == Attribute.Qualifier.CONST_STATIC
        )

        pass

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self.get_accessor_name()
        methods = []

        if self._is_static:
            if for_declaration and self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", "static " + self._type + "&  ")
            elif for_declaration and not self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", "static " + self._type)
            elif self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", self._type + "&  ")
            else:
                getter = ("get" + accessor_name + "()", self._type)

        elif self._is_const_static:
            if for_declaration and self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", "static " + self._type + "&  ")
            elif for_declaration and not self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", "static " + self._type)
            elif self._getter_returns_reference:
                getter = ("get" + accessor_name + "()", self._type + "&  ")
            else:
                getter = ("get" + accessor_name + "()", self._type)

        elif self._is_const:
            getter = ("get" + accessor_name + "() const", "const " + self._type)

        elif self._is_constexpr:
            getter = ("get" + accessor_name + "() const", "constexpr " + self._type)

        else:
            # default case.
            getter = ("get" + accessor_name + "() const", self._type)

        methods.append(getter)

        # Add a setter method, if it makes sense.
        if not (
            self._is_const_static
            or self._is_const
            or self._is_constexpr
            or self._is_static
        ):
            methods.append(
                ("set" + accessor_name + "(const " + self._type + "& value)", "void")
            )

        return methods

    def get_plain_C_attributes(self, for_constructor=False):
        typestring = self._type
        namestring = "_" + self._name

        # add const/static qualifiers and initialization values, if needed
        if self._is_static:
            typestring = "inline static " + self._type
            if self._initval is not None:
                namestring += " = " + str(self._initval)

        elif self._is_const_static:
            typestring = "inline const static " + self._type
            namestring += " = " + str(self._initval)

        elif self._is_const:
            typestring = "const " + self._type
            namestring += " = " + str(self._initval)

        elif self._is_constexpr:
            typestring = "constexpr static " + self._type
            namestring += " = " + str(self._initval)

        else:
            # default case
            # If an initial value is provided, add it
            # However, only add it if we need it for a definition,
            # not as optional argument for constructor method
            if self._initval is not None and not for_constructor:
                namestring += " = " + str(self._initval)

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

        I map the type onto a byte field,but it is not clear if this
        actually works. At the moment, we do not use the MPI data
        implicitly, as we support user-defined attributes if and only
        if they are static. That is, we never exchange them as part of
        an object. That is, at the moment this routine is kind of
        deprecated.

        """
        return [("MPI_BYTE", "sizeof(" + self._type + ")")]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        if self.use_data_store and self._user_type_has_toString_method:
            return "_dataStore._" + self._name + ".toString()"
        elif self._user_type_has_toString_method:
            return "_" + self._name + ".toString()"
        else:
            return """ "<no toString() method>" """

    def get_includes(self):
        return self._include
