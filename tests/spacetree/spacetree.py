# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4

STATIC_REFINEMENT = False


class StaticRefinement(peano4.solversteps.ActionSet):
    def __init__(self, domain_size, refine_offset, refine_width):
        super().__init__()
        self.domain_size = domain_size
        self.refine_offset = refine_offset
        self.refine_width = refine_width

    def get_action_set_name(self):
        return "StaticRefinement"

    def get_body_of_getGridControlEvents(self):
        # targetH is the finest mesh size, derived from the full domain so it matches the
        # requested grid level regardless of how large the refined region is.
        domain = "{" + ", ".join(str(value) for value in self.domain_size) + "}"
        offset = "{" + ", ".join(str(value) for value in self.refine_offset) + "}"
        width = "{" + ", ".join(str(value) for value in self.refine_width) + "}"
        target_h = "{" + ", ".join("targetH" for _ in self.refine_width) + "}"
        return f"""
  const tarch::la::Vector<DIMENSIONS, double> domainSize = {domain};
  const double targetH = ::runner::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(domainSize);

  ::peano4::grid::GridControlEvent newEvent;
  newEvent.setRefinementControl(::peano4::grid::GridControlEvent::RefinementControl::Refine);
  newEvent.offset  = {offset};
  newEvent.width   = {width};
  newEvent.targetH = {target_h};
  return {{newEvent}};
"""

    def get_includes(self):
        return '#include "runner/RuntimeParameters.h"'


project = peano4.Project(
    ["tests", "peano4"],
    "Spacetree",
    executable="Spacetree",
    dimensions=3,
    offset=[0.0, 0.0, 0.0],
    size=[1.0, 1.0, 1.0],
    grid_levels=5,
    build_mode=peano4.output.CompileMode.Release,
)

d = project.dimensions
domain_size = [1.0] * d
refine_offset = [0.3] * d if STATIC_REFINEMENT else [0.0] * d
refine_width = [0.4] * d if STATIC_REFINEMENT else [1.0] * d

project.create_grid.add_action_set(
    StaticRefinement(domain_size, refine_offset, refine_width)
)
project.generate_and_build()
