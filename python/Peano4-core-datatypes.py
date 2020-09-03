"""

 This Python script uses/invokes DaStGen2 to generate all the datatypes
 required by the core.

"""
import dastgen2
import peano4.dastgen2


#
# tarch::mpi::IntegerMessage
#
integer_message = dastgen2.DataModel( "tarch::mpi::IntegerMessage" )
integer_message.add_attribute( dastgen2.attributes.Integer( "value" ) )

integer_message.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
integer_message.add_aspect( dastgen2.aspects.MPI() )

integer_message.write_header_file("../src/tarch/mpi/IntegerMessage.h")
integer_message.write_implementation_file("../src/tarch/mpi/IntegerMessage.cpp")



#
# peano4::parallel::TreeManagementMessage
#
tree_management_message = dastgen2.DataModel( "peano4::parallel::TreeManagementMessage" )
tree_management_message.add_attribute( dastgen2.attributes.Integer( "masterSpacetreeId" ) )
tree_management_message.add_attribute( dastgen2.attributes.Integer( "workerSpacetreeId" ) )
tree_management_message.add_attribute( dastgen2.attributes.Enumeration( "action", [ "RequestNewRemoteTree", "CreateNewRemoteTree", "RemoveChildTreeFromBooksAsChildBecameEmpty", "JoinWithWorker", "Acknowledgement" ]))

tree_management_message.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
tree_management_message.add_aspect( dastgen2.aspects.MPI() )

tree_management_message.write_header_file("../src/peano4/parallel/TreeManagementMessage.h")
tree_management_message.write_implementation_file("../src/peano4/parallel/TreeManagementMessage.cpp")




#
# peano4::parallel::TreeEntry
#
tree_management_message = dastgen2.DataModel( "peano4::parallel::TreeEntry" )
tree_management_message.add_attribute( dastgen2.attributes.Integer( "id" ) )
tree_management_message.add_attribute( dastgen2.attributes.Integer( "master" ) )

tree_management_message.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
tree_management_message.add_aspect( dastgen2.aspects.MPI() )

tree_management_message.write_header_file("../src/peano4/parallel/TreeEntry.h")
tree_management_message.write_implementation_file("../src/peano4/parallel/TreeEntry.cpp")




#
# peano4::parallel::StartTraversalMessage
#
start_traversal_message = dastgen2.DataModel( "peano4::parallel::StartTraversalMessage" )
start_traversal_message.add_attribute( dastgen2.attributes.Integer( "stepIdentifier" ) )

start_traversal_message.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
start_traversal_message.add_aspect( dastgen2.aspects.MPI() )

start_traversal_message.write_header_file("../src/peano4/parallel/StartTraversalMessage.h")
start_traversal_message.write_implementation_file("../src/peano4/parallel/StartTraversalMessage.cpp")





#
# peano4::grid::GridControlEvent
#
grid_control_event = dastgen2.DataModel( "peano4::grid::GridControlEvent" )

grid_control_event.add_attribute( dastgen2.attributes.Enumeration( "refinementControl", [ "Refine", "Erase" ] ))
grid_control_event.add_attribute( peano4.dastgen2.Peano4DoubleArray( "offset", "Dimensions" ))
grid_control_event.add_attribute( peano4.dastgen2.Peano4DoubleArray( "width", "Dimensions" ))
grid_control_event.add_attribute( peano4.dastgen2.Peano4DoubleArray( "h", "Dimensions" ))

grid_control_event.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
grid_control_event.add_aspect( dastgen2.aspects.MPI() )

grid_control_event.write_header_file("../src/peano4/grid/GridControlEvent.h")
grid_control_event.write_implementation_file("../src/peano4/grid/GridControlEvent.cpp")





#
# peano4::grid::GridStatistics
#
grid_statistics = dastgen2.DataModel( "peano4::grid::GridStatistics" )

grid_statistics.add_attribute( dastgen2.attributes.Integer( "numberOfLocalUnrefinedCells" ) )
grid_statistics.add_attribute( dastgen2.attributes.Integer( "numberOfRemoteUnrefinedCells" ) )
grid_statistics.add_attribute( dastgen2.attributes.Integer( "numberOfLocalRefinedCells" ) )
grid_statistics.add_attribute( dastgen2.attributes.Integer( "numberOfRemoteRefinedCells" ) )

grid_statistics.add_attribute( dastgen2.attributes.Integer( "stationarySweeps" ) )

grid_statistics.add_attribute( dastgen2.attributes.Boolean( "coarseningHasBeenVetoed" ) )

grid_statistics.add_attribute( peano4.dastgen2.Peano4DoubleArray( "minH", "Dimensions" ))
  
grid_statistics.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
grid_statistics.add_aspect( dastgen2.aspects.MPI() )

grid_statistics.write_header_file("../src/peano4/grid/GridStatistics.h")
grid_statistics.write_implementation_file("../src/peano4/grid/GridStatistics.cpp")




#
# peano4::grid::AutomatonState
#
automaton_state = dastgen2.DataModel( "peano4::grid::AutomatonState" )

automaton_state.add_attribute( dastgen2.attributes.Integer( "level" ) )
automaton_state.add_attribute( peano4.dastgen2.Peano4DoubleArray( "x", "Dimensions" ))
automaton_state.add_attribute( peano4.dastgen2.Peano4DoubleArray( "h", "Dimensions" ))

automaton_state.add_attribute( dastgen2.attributes.Boolean( "inverted" ) )
automaton_state.add_attribute( dastgen2.attributes.BooleanArray( "evenFlags", "Dimensions" ) )

# @todo Should be char array likely 
automaton_state.add_attribute( peano4.dastgen2.Peano4IntegerArray( "accessNumber", "DimensionsTimesTwo" ) )

automaton_state.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic )  )
automaton_state.add_aspect( dastgen2.aspects.MPI() )

automaton_state.write_header_file("../src/peano4/grid/AutomatonState.h")
automaton_state.write_implementation_file("../src/peano4/grid/AutomatonState.cpp")






#
# peano4::grid::GridTraversalEvent
#
grid_traversal_event = dastgen2.DataModel( "peano4::grid::GridTraversalEvent" )

grid_traversal_event.add_attribute( peano4.dastgen2.Peano4DoubleArray( "x", "Dimensions" ))
grid_traversal_event.add_attribute( peano4.dastgen2.Peano4DoubleArray( "h", "Dimensions" ))
grid_traversal_event.add_attribute( dastgen2.attributes.BooleanArray( "isRefined", "TwoPowerD" ) )
grid_traversal_event.add_attribute( dastgen2.attributes.BooleanArray( "isVertexLocal", "TwoPowerD" ) )
grid_traversal_event.add_attribute( dastgen2.attributes.BooleanArray( "isFaceLocal",   "TwoTimesD" ) )
grid_traversal_event.add_attribute( dastgen2.attributes.Boolean(      "isCellLocal"                ) )

grid_traversal_event.add_attribute( dastgen2.attributes.BooleanArray( "isVertexInsideDomain", "TwoPowerD" ) )

# @todo Should be char array likely with -4 to 9
grid_traversal_event.add_attribute( peano4.dastgen2.Peano4IntegerArray( "vertexDataFrom", "TwoPowerD" ) )
grid_traversal_event.add_attribute( peano4.dastgen2.Peano4IntegerArray( "vertexDataTo", "TwoPowerD" ) )
grid_traversal_event.add_attribute( peano4.dastgen2.Peano4IntegerArray( "faceDataFrom", "TwoTimesD" ) )
grid_traversal_event.add_attribute( peano4.dastgen2.Peano4IntegerArray( "faceDataTo", "TwoTimesD" ) )

grid_traversal_event.add_attribute( dastgen2.attributes.Integer( "cellData" ) )

grid_traversal_event.add_attribute( peano4.dastgen2.Peano4IntegerArray( "relativePositionToFather", "Dimensions" ) )

grid_traversal_event.add_attribute( dastgen2.attributes.Integer( "invokingSpacetree" ) )
grid_traversal_event.add_attribute( dastgen2.attributes.Integer( "invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing" ) )

grid_traversal_event.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
grid_traversal_event.add_aspect( dastgen2.aspects.MPI() )

grid_traversal_event.write_header_file("../src/peano4/grid/GridTraversalEvent.h")
grid_traversal_event.write_implementation_file("../src/peano4/grid/GridTraversalEvent.cpp")



#
# peano4::grid::GridVertex
#
grid_vertex = dastgen2.DataModel( "peano4::grid::GridVertex" )

grid_vertex.add_attribute( dastgen2.attributes.Enumeration( "state", [ "HangingVertex", "New", "Unrefined", "Refined", "RefinementTriggered", "Refining", "EraseTriggered", "Erasing", "Delete" ] ) )

grid_vertex.add_attribute( peano4.dastgen2.Peano4IntegerArray( "adjacentRanks", "TwoPowerD" ) )
grid_vertex.add_attribute( peano4.dastgen2.Peano4IntegerArray( "backupOfAdjacentRanks", "TwoPowerD" ) )

grid_vertex.add_attribute( dastgen2.attributes.Boolean( "hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep" ) )
grid_vertex.add_attribute( dastgen2.attributes.Boolean( "isAntecessorOfRefinedVertexInCurrentTreeSweep" ) )

grid_vertex.add_attribute( dastgen2.attributes.Integer( "numberOfAdjacentRefinedLocalCells" ) )

# Brauch ich nur im Debug mode. Das wird bisher net unterstuetzt
grid_vertex.add_attribute( peano4.dastgen2.Peano4DoubleArray( "x", "Dimensions" ))
grid_vertex.add_attribute( dastgen2.attributes.Integer( "level" ) )

grid_vertex.add_aspect( peano4.dastgen2.MPI( peano4.datamodel.DoFAssociation.Generic ) )
grid_vertex.add_aspect( dastgen2.aspects.MPI() )

grid_vertex.write_header_file("../src/peano4/grid/GridVertex.h")
grid_vertex.write_implementation_file("../src/peano4/grid/GridVertex.cpp")


