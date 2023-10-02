# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Attribute import Attribute


class String(Attribute):
    """

    Wrapper around C++ string which is not a dataype supported by
    MPI natively.

    max_length  Maximum length of the strings that can be handled.
                The shorter you keep this value, the smaller the
                MPI message size, as we don't use dynamic data
                structures. We always map the C++ string onto an
                array with max_length entries.

    """

    def __init__(
        self,
        name,
        max_length=80,
        ifdefs=[],
        qualifier=Attribute.Qualifier.NONE,
        initval=None,
    ):
        #  if qualifier != Attribute.Qualifier.NONE:
        #      raise NotImplementedError(
        #          f"Attribute {name}: This data type can't work with qualifiers (yet)"
        #      )

        # sanitize initval
        # Make sure it starts with a `"`
        if not initval.startswith('"'):
            initval = '"' + initval
        # and that it ends with a `\0"`
        if not initval.endswith('\\0"'):
            if initval.endswith('"'):
                # only cut off trailing '"'
                # will be added back one line below, after the null character
                initval = initval[:-1]
            initval += '\\0"'


        Attribute.__init__(
            self, name, ifdefs=ifdefs, qualifier=qualifier, initval=initval
        )
        self._max_length = max_length

    def get_methods(self, _full_qualified_name, for_declaration=True):
        accessor_name = self._name.capitalize()
        methods = []

        if self._is_static:
            if for_declaration:
                getter = ("get" + accessor_name + "()", "static std::string")
            else:
                getter = ("get" + accessor_name + "()", "std::string")

        elif self._is_const_static:
            if for_declaration:
                # TODO: Why is this not const static std::string?
                getter = ("get" + accessor_name + "()", "static std::string")
            else:
                getter = ("get" + accessor_name + "()", "std::string")

        elif self._is_const:
            # TODO: Why is this not const std::string?
            getter = ("get" + accessor_name + "()", "std::string")

        elif self._is_constexpr:
            getter = ("get" + accessor_name + "()", "constexpr std::string")

        else:
            # default case.
            getter = ("get" + accessor_name + "() const", "std::string")

        methods.append(getter)


        # Add a setter method, if it makes sense.
        if not (self._is_const_static or self._is_const or self._is_constexpr):
            methods.append(("set" + accessor_name + "(const std::string& value)", "void"))


        #  return [
        #      ("get" + accessor_name + "() const", "std::string"),
        #      ("set" + accessor_name + "(const std::string& value)", "void"),
        #  ]

    def get_plain_C_attributes(self):
        typestring = "std::string"
        namestring = "_" + self._name #+ "[" + str(self._max_length) + "]"
        packstring = "" # for compression

        if self._initval is not None:
            namestring += " = " + str(self._initval)

        return [(namestring, typestring, packstring)]

        return [
                ("_" + self._name + "[" + str(self._max_length) + "]", "char"),
                ("_" + self._name + "Length", "int"),
        ]

    def get_setter_getter_name(self):
        return self._name

    def get_method_body(self, signature):
        if signature.startswith("get"):
            return (
                """
  std::ostringstream result;
  for (int i=0; i<_"""
                + self._name
                + """Length; i++) {
    result << static_cast<char>(_"""
                + self._name
                + """[i]);
  }
  return result.str();
"""
            )
        elif signature.startswith("set"):
            return (
                """
  _"""
                + self._name
                + """Length = value.length();
  for (int i=0; i<value.length(); i++) {
    _"""
                + self._name
                + """[i] = value.data()[i];
  }

"""
            )
        else:
            assert False
        return ""

    def get_native_MPI_type(self):
        return [("MPI_CHAR", self._max_length), ("MPI_INT", 1)]

    def get_to_string(self):
        """

        Return string representation of attribute.

        """
        return "get" + self._name.capitalize() + "()"
