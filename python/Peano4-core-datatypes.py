"""
 This Python script uses/invokes DaStGen2 to generate all the datatypes
 required by the core.
"""
import dastgen2
import peano4.dastgen2

#
# tarch::mpi::IntegerMessage
#
integer_message = dastgen2.DataModel("tarch::mpi::IntegerMessage")
integer_message.add_attribute(dastgen2.attributes.Integer("value"))

integer_message.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

integer_message.write_header_file("../src/tarch/mpi/IntegerMessage.h")
integer_message.write_implementation_file("../src/tarch/mpi/IntegerMessage.cpp")

#
# tarch::mpi::DoubleMessage
#
double_message = dastgen2.DataModel("tarch::mpi::DoubleMessage")
double_message.add_attribute(dastgen2.attributes.Double("value"))

double_message.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

double_message.write_header_file("../src/tarch/mpi/DoubleMessage.h")
double_message.write_implementation_file("../src/tarch/mpi/DoubleMessage.cpp")

#
# peano4::parallel::TreeManagementMessage
#
tree_management_message = dastgen2.DataModel("peano4::parallel::TreeManagementMessage")
# >= -1
tree_management_message.add_attribute(
    dastgen2.attributes.Integer(
        "masterSpacetreeId", min_value=-1, max_value="std::numeric_limits<int>::max()"
    )
)
# >=0
tree_management_message.add_attribute(
    dastgen2.attributes.Integer(
        "workerSpacetreeId", min_value=0, max_value="std::numeric_limits<int>::max()"
    )
)
tree_management_message.add_attribute(
    dastgen2.attributes.Enumeration(
        "action",
        [
            "RequestNewRemoteTree",
            "CreateNewRemoteTree",
            "RemoveChildTreeFromBooksAsChildBecameEmpty",
            "JoinWithWorker",
            "Acknowledgement",
        ],
    )
)

tree_management_message.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

tree_management_message.write_header_file(
    "../src/peano4/parallel/TreeManagementMessage.h"
)
tree_management_message.write_implementation_file(
    "../src/peano4/parallel/TreeManagementMessage.cpp"
)

#
# peano4::parallel::TreeEntry
#
tree_management_message = dastgen2.DataModel("peano4::parallel::TreeEntry")
# >= 0
tree_management_message.add_attribute(
    dastgen2.attributes.Integer(
        "id", min_value=0, max_value="std::numeric_limits<int>::max()"
    )
)
# >= -1
tree_management_message.add_attribute(
    dastgen2.attributes.Integer(
        "master", min_value=-1, max_value="std::numeric_limits<int>::max()"
    )
)

tree_management_message.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

tree_management_message.write_header_file("../src/peano4/parallel/TreeEntry.h")
tree_management_message.write_implementation_file(
    "../src/peano4/parallel/TreeEntry.cpp"
)

#
# peano4::parallel::StartTraversalMessage
#
start_traversal_message = dastgen2.DataModel("peano4::parallel::StartTraversalMessage")
# positive number
start_traversal_message.add_attribute(
    dastgen2.attributes.Integer(
        "stepIdentifier", min_value=0, max_value="std::numeric_limits<int>::max()"
    )
)

start_traversal_message.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

start_traversal_message.write_header_file(
    "../src/peano4/parallel/StartTraversalMessage.h"
)
start_traversal_message.write_implementation_file(
    "../src/peano4/parallel/StartTraversalMessage.cpp"
)

#
# peano4::grid::GridControlEvent
#
grid_control_event = dastgen2.DataModel("peano4::grid::GridControlEvent")

grid_control_event.add_attribute(
    dastgen2.attributes.Enumeration("refinementControl", ["Refine", "Erase"])
)
# I know that Dimensions usually is 2 or 3 (though I support higher dimensions). Guess this is not relevant
grid_control_event.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("offset", "Dimensions")
)
grid_control_event.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("width", "Dimensions")
)
grid_control_event.add_attribute(peano4.dastgen2.Peano4DoubleArray("h", "Dimensions"))

grid_control_event.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

grid_control_event.write_header_file("../src/peano4/grid/GridControlEvent.h")
grid_control_event.write_implementation_file("../src/peano4/grid/GridControlEvent.cpp")

#
# peano4::grid::GridStatistics
#
grid_statistics = dastgen2.DataModel("peano4::grid::GridStatistics")

# all the five integer values are >=0
grid_statistics.add_attribute(
    dastgen2.attributes.Integer(
        "numberOfLocalUnrefinedCells",
        min_value=0,
        max_value="std::numeric_limits<int>::max()",
    )
)
grid_statistics.add_attribute(
    dastgen2.attributes.Integer(
        "numberOfRemoteUnrefinedCells",
        min_value=0,
        max_value="std::numeric_limits<int>::max()",
    )
)
grid_statistics.add_attribute(
    dastgen2.attributes.Integer(
        "numberOfLocalRefinedCells",
        min_value=0,
        max_value="std::numeric_limits<int>::max()",
    )
)
grid_statistics.add_attribute(
    dastgen2.attributes.Integer(
        "numberOfRemoteRefinedCells",
        min_value=0,
        max_value="std::numeric_limits<int>::max()",
    )
)

grid_statistics.add_attribute(
    dastgen2.attributes.Integer(
        "stationarySweeps", min_value=0, max_value="std::numeric_limits<int>::max()"
    )
)

grid_statistics.add_attribute(dastgen2.attributes.Boolean("coarseningHasBeenVetoed"))
grid_statistics.add_attribute(dastgen2.attributes.Boolean("removedEmptySubtree"))

grid_statistics.add_attribute(peano4.dastgen2.Peano4DoubleArray("minH", "Dimensions"))

grid_statistics.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

grid_statistics.write_header_file("../src/peano4/grid/GridStatistics.h")
grid_statistics.write_implementation_file("../src/peano4/grid/GridStatistics.cpp")

#
# peano4::grid::AutomatonState
#
automaton_state = dastgen2.DataModel("peano4::grid::AutomatonState")

automaton_state.add_attribute(
    dastgen2.attributes.Integer("level", min_value=0, max_value=63)
)
automaton_state.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("x", "Dimensions", valid_mantissa_bits=23)
)
automaton_state.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("h", "Dimensions", valid_mantissa_bits=23)
)

automaton_state.add_attribute(dastgen2.attributes.Boolean("inverted"))
automaton_state.add_attribute(
    dastgen2.attributes.BooleanArray("evenFlags", "Dimensions", compress=True)
)

automaton_state.add_attribute(
    peano4.dastgen2.Peano4IntegerArray(
        "accessNumber",
        cardinality="DimensionsTimesTwo",
        min_value="-TwoTimesD",
        max_value="TwoTimesD",
    )
)

automaton_state.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

automaton_state.write_header_file("../src/peano4/grid/AutomatonState.h")
automaton_state.write_implementation_file("../src/peano4/grid/AutomatonState.cpp")

#
# peano4::grid::GridTraversalEvent
#
grid_traversal_event = dastgen2.DataModel("peano4::grid::GridTraversalEvent")

grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("x", "Dimensions", valid_mantissa_bits=23)
)
grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4DoubleArray("h", "Dimensions", valid_mantissa_bits=23)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("hasBeenRefined", "TwoPowerD", compress=True)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("willBeRefined", "TwoPowerD", compress=True)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("isVertexLocal", "TwoPowerD", compress=True)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("isParentVertexLocal", "TwoPowerD", compress=True)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("isVertexParentOfSubtree", "TwoPowerD", compress=True)
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray("isFaceLocal", "TwoTimesD", compress=True)
)
grid_traversal_event.add_attribute(dastgen2.attributes.Boolean("isCellLocal"))
grid_traversal_event.add_attribute(dastgen2.attributes.Boolean("isParentCellLocal"))


#
# Also holds if a vertex is adjacent to a periodic boundary.
#
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray(
        "isVertexAdjacentToParallelDomainBoundary", "TwoPowerD", compress=True
    )
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray(
        "isFaceAdjacentToParallelDomainBoundary", "TwoTimesD", compress=True
    )
)

grid_traversal_event.add_attribute(
    dastgen2.attributes.BooleanArray(
        "isAdjacentCellLocal", "ThreePowerD", compress=True
    )
)

# @todo Should be char array likely with -4 to 9 per entry
grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4IntegerArray("vertexDataFrom", "TwoPowerD")
)
grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4IntegerArray("vertexDataTo", "TwoPowerD")
)
grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4IntegerArray("faceDataFrom", "TwoTimesD")
)
grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4IntegerArray("faceDataTo", "TwoTimesD")
)

grid_traversal_event.add_attribute(dastgen2.attributes.Integer("cellData"))

grid_traversal_event.add_attribute(
    peano4.dastgen2.Peano4IntegerArray(
        "relativePositionToFather", cardinality="Dimensions", min_value=0, max_value=3
    )
)

# >= -1 and smaller than mpi ranks. Not sure if this is of any help
grid_traversal_event.add_attribute(
    dastgen2.attributes.Integer(
        "invokingSpacetree", min_value=-1, max_value="std::numeric_limits<int>::max()"
    )
)
grid_traversal_event.add_attribute(
    dastgen2.attributes.Boolean(
        "invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing"
    )
)

grid_traversal_event.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

grid_traversal_event.write_header_file("../src/peano4/grid/GridTraversalEvent.h")
grid_traversal_event.write_implementation_file(
    "../src/peano4/grid/GridTraversalEvent.cpp"
)

#
# peano4::grid::GridVertex
#
grid_vertex = dastgen2.DataModel("peano4::grid::GridVertex")

grid_vertex.add_attribute(
    dastgen2.attributes.Enumeration(
        "state",
        [
            "HangingVertex",
            "New",
            "Unrefined",
            "Refined",
            "RefinementTriggered",
            "Refining",
            "EraseTriggered",
            "Erasing",
            "Delete",
        ],
    )
)

# >= -1 and smaller than ranks
grid_vertex.add_attribute(
    peano4.dastgen2.Peano4IntegerArray(
        "adjacentRanks",
        "TwoPowerD",
        min_value=-1,
        max_value="std::numeric_limits<int>::max()",
    )
)
grid_vertex.add_attribute(
    peano4.dastgen2.Peano4IntegerArray("backupOfAdjacentRanks", "TwoPowerD")
)

grid_vertex.add_attribute(
    dastgen2.attributes.Boolean("hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep")
)
grid_vertex.add_attribute(
    dastgen2.attributes.Boolean("isAntecessorOfRefinedVertexInCurrentTreeSweep")
)

grid_vertex.add_attribute(
    dastgen2.attributes.Boolean("hasBeenParentOfSubtreeVertexInPreviousTreeSweep")
)
grid_vertex.add_attribute(
    dastgen2.attributes.Boolean("isParentOfSubtreeVertexInCurrentTreeSweep")
)

# 0 <= value <= 2^d
grid_vertex.add_attribute(
    dastgen2.attributes.Integer(
        "numberOfAdjacentRefinedLocalCells", min_value=0, max_value="TwoPowerD"
    )
)

# Brauch ich nur im Debug mode. Das wird bisher net unterstuetzt
grid_vertex.add_attribute(
    peano4.dastgen2.Peano4DoubleArray(
        "x", "Dimensions", valid_mantissa_bits=23, ifdefs=["PeanoDebug>0"]
    )
)
grid_vertex.add_attribute(dastgen2.attributes.Integer("level"))

grid_vertex.add_aspect(
    peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Generic)
)

grid_vertex.write_header_file("../src/peano4/grid/GridVertex.h")
grid_vertex.write_implementation_file("../src/peano4/grid/GridVertex.cpp")
