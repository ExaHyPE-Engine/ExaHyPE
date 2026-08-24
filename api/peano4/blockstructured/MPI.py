# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def get_face_merge_implementation(patch_overlap):
    """Returns a plain-copy merge for block-structured halo layers.

    The first patch dimension is the even, face-normal overlap width. Every
    other dimension matches the adjacent cell patch.
    """
    dictionary = {
        "OVERLAP": int(patch_overlap.dim[0] / 2),
        "DOFS_PER_AXIS": int(patch_overlap.dim[1]),
        "UNKNOWNS": int(patch_overlap.no_of_unknowns),
    }

    template = """
  const auto serialisePatchIndex = [](
    const ::tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    const int faceNormal
  ) {{
    int base = 1;
    int result = 0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {{
      result += overlapCell(dimension) * base;
      if (dimension == faceNormal) {{
        base *= {OVERLAP} * 2;
      }} else {{
        base *= {DOFS_PER_AXIS};
      }}
    }}
    return result;
  }};

  switch (context) {{
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange: {{
        auto* const destination = this->data();
        const auto* const source = neighbour.data();
        assertion(destination);
        assertion(source);

        const int faceNormal = marker.getSelectedFaceNumber() % DIMENSIONS;
        ::peano4::dForEachExclusive(
          {DOFS_PER_AXIS},
          faceNormal,
          0,
          [&](const ::tarch::la::Vector<DIMENSIONS, int>& tangentialCell) {{
            for (int overlapIndex = 0; overlapIndex < {OVERLAP}; overlapIndex++) {{
              auto volume = tangentialCell;
              volume(faceNormal) += marker.outerNormal()(faceNormal) > 0
                ? overlapIndex + {OVERLAP}
                : overlapIndex;
              const int serialisedVolume = serialisePatchIndex(volume, faceNormal);
              for (int unknown = 0; unknown < {UNKNOWNS}; unknown++) {{
                const int valueIndex = serialisedVolume * {UNKNOWNS} + unknown;
                assertion(source[valueIndex] == source[valueIndex], source[valueIndex]);
                destination[valueIndex] = source[valueIndex];
              }}
            }}
          }}
        );
        break;
      }}
    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertion(false, "Multiscale exchange is not implemented.");
      break;
    default:
      assertion(false, "This exchange context is not implemented.");
      break;
  }}
"""

    return template.format(**dictionary)


def get_cell_merge_implementation(patch_overlap):
    template = """
  switch (context) {{
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      assertion(false, "Cells on the same level cannot overlap in Peano.");
      break;
    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertion(false, "Multiscale exchange is not implemented.");
      break;
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertion(false, "Cell data has no periodic boundary swap.");
      break;
    default:
      assertion(false, "This exchange context is not implemented.");
      break;
  }}
"""

    return template.format()
