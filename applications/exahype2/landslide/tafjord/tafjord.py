# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import exahype2.solvers.PDETerms

sys.path.insert(0, os.path.abspath(".."))

from landslide import Landslide

if __name__ == "__main__":
    landslide = Landslide()

    my_parser = landslide.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveFV",
        end_time=100.0,
        width=[1900, 1950],
        offset=[414895.5, 6904495.5],
        min_depth=5,
        amr_levels=0,
        patch_size=32,
        rk_order = 2,
        dg_order = 3,
        number_of_snapshots=20,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = landslide.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../Landslide.h"
#include "../ExtrapolateHalo.h"
#include "../ComputePartialDerivatives.h"
#include "../TopologyParser.h"
"""
    )

    my_solver.add_solver_constants(
        """
TopologyParser topologyParser = TopologyParser(
  \"Tafjord_5m_EPSG25832.nc\", \
  \"ini_3.0Mm3_5m_EPSG25832.nc\");
"""
    )

    my_solver.set_implementation(
        initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
        boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
        refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
        flux="applications::exahype2::landslide::flux(Q, faceCentre, volumeH, t, dt, normal, F);",
        ncp="applications::exahype2::landslide::nonconservativeProductWithElevation(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ);",
        eigenvalues="return applications::exahype2::landslide::maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal);",
        source_term="applications::exahype2::landslide::sourceTermWithCoulombFriction(Q, volumeCentre, volumeH, t, dt, S);",
    )

    my_project = landslide.setup_project(my_args, my_solver)

    my_project.output.makefile.add_cpp_file("Landslide.cpp")
    my_project.output.makefile.add_cpp_file("../NetCDFReader.cpp")
    my_project.output.makefile.add_cpp_file("../TopologyParser.cpp")

    landslide.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
