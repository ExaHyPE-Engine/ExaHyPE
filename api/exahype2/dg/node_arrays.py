# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def get_node_array_aliases(prefix, nodes_per_axis, dimensions):
    """Returns the type aliases of the states at the nodes of one DG cell and at the nodes of one
    face. The alias names start with ``prefix``."""
    nodes_per_face = nodes_per_axis ** (dimensions - 1)
    return [
        "using {0}CellStates = {0}State[{1}];".format(
            prefix, nodes_per_axis**dimensions
        ),
        "using {0}FaceStates = {0}State[{1}];".format(prefix, nodes_per_face),
        "using {0}FaceFluxes = {0}Flux[{1}];".format(prefix, nodes_per_face),
    ]
