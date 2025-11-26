# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def get_face_merge_implementation(patch_overlap):
    """

    Generic exchange of block-structured halo layers

    This routine realises a halo exchange via plain copies. It assumes that the
    overhead itself is modelled as a patch, where the first dimension of the
    patch_overlap is actually the "short" dimension, i.e. the direction which
    indeed overlaps with the two adjacent patches. It has to be a multiple of 2.
    The other remaining dimensions are plain Cartesian overlaps and match the
    size of the adjacent blocks.

    """
    d = {
        "OVERLAP": int(patch_overlap.dim[0] / 2),
        "DOFS_PER_AXIS": int(patch_overlap.dim[1]),
        "UNKNOWNS": int(patch_overlap.no_of_unknowns),
    }

    template = """
  auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS,int>& overlapCell, int normal) {{
    int base   = 1;
    int result = 0;
    for (int d=0; d<DIMENSIONS; d++) {{
      result += overlapCell(d) * base;
      if (d==normal) {{
        base *= {OVERLAP}*2;
      }}
      else {{
        base *= {DOFS_PER_AXIS};
      }}
    }}
    return result;
  }};

  switch (context) {{
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      {{
        auto* dst = this->data();
        const auto* const src = neighbour.data();
        assertion(dst);
        assertion(src);

        const int faceNormal = marker.getSelectedFaceNumber() % DIMENSIONS;
        dfore(i, {DOFS_PER_AXIS}, faceNormal, 0) {{
          for (int j = 0; j < {OVERLAP}; j++) {{
            tarch::la::Vector<DIMENSIONS, int> volume = i;
            volume(faceNormal) += marker.outerNormal()(faceNormal) > 0 ? j + {OVERLAP} : j;
            const int volumeSerialised = serialisePatchIndex(volume, faceNormal);
            for (int k = 0; k < {UNKNOWNS}; k++) {{
              assertion1(src[volumeSerialised * {UNKNOWNS} + k] == src[volumeSerialised * {UNKNOWNS} + k], src[volumeSerialised * {UNKNOWNS} + k]);
              dst[volumeSerialised * {UNKNOWNS} + k] = src[volumeSerialised * {UNKNOWNS} + k];
            }}
          }}
        }}
      }}
      break;
    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;
    default:
      assertionMsg( false, "not implemented yet" );
      break;
  }}
"""

    return template.format(**d)


def get_cell_merge_implementation(patch_overlap):
    d = {
        "DOFS_PER_AXIS": int(patch_overlap.dim[1]),
        "UNKNOWNS": int(patch_overlap.no_of_unknowns),
    }

    template = """
  switch (context) {{
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      assertionMsg( false, "should not be called, as cells (on the same level) cannot overlap in Peano" );
      break;
      
    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;

    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertionMsg( false, "should never be called" );
      break;
      
    default:
      assertionMsg( false, "not implemented yet" );
      break;
  }}
"""

    return template.format(**d)
