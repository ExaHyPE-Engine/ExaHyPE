# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.datamodel


def create_patch_and_halo(
    current_name,
    previous_name,
    patch_size,
    halo_width,
    number_of_values,
    precision,
):
    """Returns ``(solution, previous_solution, halo_old, halo_new)``. The caller
    configures when each halo is exchanged.
    """
    solution = exahype2.datamodel.Patch(
        (patch_size, patch_size, patch_size),
        number_of_values,
        current_name,
        precision,
    )
    previous_solution = exahype2.datamodel.Patch(
        (patch_size, patch_size, patch_size),
        number_of_values,
        previous_name,
        precision,
    )
    halo_old = exahype2.datamodel.Patch(
        (2 * halo_width, patch_size, patch_size),
        number_of_values,
        previous_name,
        precision,
    )
    halo_new = exahype2.datamodel.Patch(
        (2 * halo_width, patch_size, patch_size),
        number_of_values,
        current_name,
        precision,
    )

    for patch in (solution, previous_solution, halo_old, halo_new):
        patch.generator = exahype2.datamodel.PatchGenerator(patch, precision)

    for interior in (solution, previous_solution):
        interior.generator.send_condition = "false"
        interior.generator.receive_and_merge_condition = "false"
        interior.generator.can_receive = "false"
        interior.generator.can_send = "false"

    for halo in (halo_old, halo_new):
        halo.generator.halo_layout = "overlapping_volumes"

    return solution, previous_solution, halo_old, halo_new
