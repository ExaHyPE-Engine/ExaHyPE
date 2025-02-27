# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2.attributes.DoubleArray


class Peano4SmartPointerDoubleArray(dastgen2.attributes.DoubleArray):
    """!
        
    Specialisation of array using Peano's tarch

    Specialisation of dastgen2.attributes.DoubleArray which relies on
    Peano's tarch. Therefore, things alike the vector initialisation
    in the constructor do work. We can also directly use Peano's linear
    algebra operators.
    
    This routine should include Vector and SmartPointerVector. I'm however
    lazy here and assume that someone else will inject these includes, and
    indeed Peano uses this class always in combination with MPIAndStorageAspect
    which in turn includes the right headers.

    ## Smart pointer variant
    
    If arrays associated with the datatype are very big, the 
    """

    def __init__(self, 
                 name, 
                 cardinality, 
                 ifdefs=[], 
                 ):
        """
            
        See superclass' constructor.

        @param cardinality: String
          This is important: It is not (necessarily) an integer, but can be
          a string which is defined via a pragma or constexpr later.
          
        """
        super(Peano4SmartPointerDoubleArray, self).__init__(
            name, cardinality, valid_mantissa_bits=None, ifdefs=ifdefs
        )
        self._cardinality       = str(cardinality)

    def get_methods(self, _full_qualified_class_name, for_declaration=True):
        accessor_name = self._name[:1].title() + self._name[1:]
        return [
            (
                "get" + accessor_name + "() const",
                "tarch::la::Vector<" + self._cardinality + ",double>",
            ),
            (
                "set"
                + accessor_name
                + "(const tarch::la::Vector<"
                + self._cardinality
                + ",double>& value)",
                "void",
            ),
            ("get" + accessor_name + "(int index) const", "double"),
            ("set" + accessor_name + "(int index, double value)", "void"),
        ]

    def use_default_copy_constructor(self):
        return True

    def get_plain_C_attributes(self, for_constructor=False):
        return [
                (
                    "_" + self._name,
                    "tarch::la::SmartPointerVector<" + self._cardinality + ",double>",
                )
        ]

    def get_first_plain_C_attribute(self):
        return [("_" + self._name + ".data()[0]", "void")]

    def get_constructor_arguments(self):
        return [
            ("_" + self._name, "tarch::la::Vector<" + self._cardinality + ",double>")
        ]

    def get_method_body(self, signature):
        if self.use_data_store:
            name = "  _dataStore._" + self._name
        else:
            name = "  _" + self._name
            
        if (
            signature.startswith("get")
            and "index" in signature
        ):
            return "  return " + name + "(index);\n"
        elif (
            signature.startswith("set")
            and "index" in signature
        ):
            return name + "(index) = value;\n"
        elif signature.startswith("get"):
            return "  return " + name + ".toVector();\n"
        elif signature.startswith("set"):
            return name + " = value;\n"
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
             return "_dataStore._" + self._name + ".toVector()"
        else:
            return "_" + self._name + ".toVector()"
