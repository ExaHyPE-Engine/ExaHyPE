"""
 This Python script uses/invokes DaStGen2 to generate all the datatypes
 required by the unit tests for the stack component. The file is part of 
 the Peano 4 code base. For details (copyright) consult the project's
 webpage. For further information on how to use this file, please study
 the documentation of the class STDVectorStackOverObjectsWithSmartPointersTest.
"""
import dastgen2
import peano4.dastgen2


test_object = dastgen2.DataModel("peano4::stacks::tests::GeneratedTestObject")

test_object.add_attribute(dastgen2.attributes.Integer("id"))
test_object.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "data", "256" ))

test_object.add_aspect( dastgen2.aspects.MPIForSmartPointers(test_object) )
test_object.add_aspect( peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic) )

test_object.write_header_file("GeneratedTestObject.h")
test_object.write_implementation_file("GeneratedTestObject.cpp")


flattened_test_object = dastgen2.DataModel("peano4::stacks::tests::GeneratedTestObject_Flattened")

flattened_test_object.add_attribute(dastgen2.attributes.Integer("id"))
flattened_test_object.add_attribute(peano4.dastgen2.Peano4DoubleArray( "data", "256" ))

flattened_test_object.add_aspect( dastgen2.aspects.MPI() )
flattened_test_object.add_aspect( peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic) )

flattened_test_object.write_header_file("GeneratedTestObject_Flattened.h")
flattened_test_object.write_implementation_file("GeneratedTestObject_Flattened.cpp")


