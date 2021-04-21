import argparse
import os
import sys

from peano4.visualisation.OutputFileParser import OutputFileParser
from peano4.visualisation.Patch import Patch


def write_preamble(vtk_file, num_cells_on_axis, origin, spacing, dimensions):
    if dimensions == 2:
        z_axis_depth = 1
    elif dimensions == 3:
        z_axis_depth = num_cells_on_axis + 1 # Add one described here:
                                             # https://discourse.vtk.org/t/dimensions-field-of-the-vtk-legacy-format/1810
    else:
        print("Error, specified dimensions '{}' not supported, exiting...".format(
            dimensions))
        sys.exit(1)

    vtk_file.write(
        "# vtk DataFile Version 3.0\n"
        "insert some description of the data here\n"
        "ASCII\n"
        "DATASET STRUCTURED_POINTS\n"
        f"DIMENSIONS {num_cells_on_axis+1} {num_cells_on_axis+1} {z_axis_depth}\n" 
                      # Add one described here:
                      # https://discourse.vtk.org/t/dimensions-field-of-the-vtk-legacy-format/1810
                      # incrementation of z_axis_depth covered above
        f"ORIGIN {origin} {origin} {origin}\n"
        f"SPACING {spacing} {spacing} {spacing}\n"
    )


def get_structured_values_2d(patch_boundaries, ofparser, patch_size):
    # To reduce time complexity I create a structured
    # matrix of patches from the unstructured set of
    # patches that make up the Peano patch file
    patch_matrix = [[Patch for i in range(len(patch_boundaries))] for j in range(
        len(patch_boundaries))]  # assumes x==y==z
    for patch in ofparser.cell_data:
        if patch.size[0] == patch_size:
            x_pos_matrix = patch_boundaries.index(patch.offset[0])
            y_pos_matrix = patch_boundaries.index(patch.offset[1])
            patch_matrix[x_pos_matrix][y_pos_matrix] = patch

    # Again to reduce compute time I store all unkowns in list
    # It is organised as follows: unknown 1 ... unknown n
    # For each coordinate x axis before shifting up y axis
    structured_values = []

    for i in range(len(patch_boundaries)):
        value_sets = []
        for j in range(len(patch_boundaries)):   # assumes x==y==z
            value_sets.append(patch_matrix[j][i].values)

        for j in range(ofparser.dof):
            for values in value_sets:
                for k in range(ofparser.dof):
                    p_first_value = (
                        j * ofparser.dof * ofparser.unknowns) + k * ofparser.unknowns
                    for unknown in values[p_first_value:
                                          p_first_value + ofparser.unknowns]:
                        structured_values.append(unknown)
    return structured_values


def get_structured_values_3d(
        patch_boundaries_x,
        patch_boundaries_y,
        patch_boundaries_z,
        ofparser,
        patch_size):
    # To reduce time complexity I create a structured
    # matrix of patches from the unstructured set of
    # patches that make up the Peano patch file
    patch_matrix = [[[Patch for i in range(len(patch_boundaries_x))] for j in range(
        len(patch_boundaries_x))] for k in range(len(patch_boundaries_x))]  # assumes x==y==z
    for patch in ofparser.cell_data:
        if patch.size[0] == patch_size:
            x_pos_matrix = patch_boundaries_x.index(patch.offset[0])
            y_pos_matrix = patch_boundaries_y.index(patch.offset[1])
            z_pos_matrix = patch_boundaries_z.index(patch.offset[2])
            patch_matrix[z_pos_matrix][y_pos_matrix][x_pos_matrix] = patch

    structured_values = []
    for z_axis_of_patches in range(len(patch_boundaries_x)):
        for z_axis_of_patch in range(ofparser.dof):
            for y_axis_position in range(len(patch_boundaries_x)):

                for j in range(ofparser.dof):
                    for k in range(len(patch_boundaries_x)):
                        try:
                            values = patch_matrix[z_axis_of_patches][y_axis_position][k].values
                            p_first_val = (
                                z_axis_of_patch * ofparser.dof * ofparser.dof * ofparser.unknowns) + (
                                j * ofparser.dof * ofparser.unknowns)
                            for val in values[p_first_val: p_first_val +
                                              (ofparser.dof * ofparser.unknowns)]:
                                structured_values.append(val)
                        except AttributeError:  # Thrown when patch file has no data
                                                # for this position in the mesh
                            for val in range(ofparser.dof * ofparser.unknowns):
                                structured_values.append(0.0)

    return structured_values


def peano_patch_to_legacy_vtk(patch_file, vtk_file, dimensions):
    ofparser = OutputFileParser(patch_file, '', 0)
    ofparser.parse_file()
    patch_size = ofparser.get_smallest_patch_size()
    patch_boundaries_x, patch_boundaries_y, patch_boundaries_z = ofparser.get_patch_boundaries(
        patch_size)
    num_cells_on_axis = len(patch_boundaries_x) * \
        ofparser.dof   # assumes x==y==z
    spacing = ((patch_boundaries_x[-1] + patch_size) -
               patch_boundaries_x[0]) / num_cells_on_axis

    with open(vtk_file, "w") as vtk:
        write_preamble(
            vtk,
            num_cells_on_axis,
            patch_boundaries_x[0],
            spacing,
            dimensions)

        numPoints = 0
        structured_values = []
        if dimensions == 2:
            structured_values = get_structured_values_2d(
                patch_boundaries_x, ofparser, patch_size)
            numPoints = num_cells_on_axis * num_cells_on_axis  # assumes x==y
        else:  # for 3D
            structured_values = get_structured_values_3d(
                patch_boundaries_x,
                patch_boundaries_y,
                patch_boundaries_z,
                ofparser,
                patch_size)
            numPoints = num_cells_on_axis * num_cells_on_axis * \
                num_cells_on_axis  # assumes x==y==z

        numComp = 1
        for unknown in range(ofparser.unknowns):
            vtk.write(
                f"CELL_DATA {numPoints}\n"
                f"SCALARS unknown_{unknown} float {numComp}\n"
                f"LOOKUP_TABLE unknown_{unknown}_table\n"
            )
            for val in structured_values[unknown::ofparser.unknowns]:
                vtk.write(str(float(val)))
                vtk.write(' ')
            vtk.write('\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Peano 4 - generate a legacy VTK file from '
        'from a peano patch file')
    parser.add_argument(
        '--input-file',
        dest='filepath',
        help='File path to a Peano patch file')
    parser.add_argument(
        '--meta-file',
        dest='metafile',
        help='The main file that links to a set of Peano patch files - these must be in the same directory')
    parser.add_argument(
        '--output-dir',
        dest='output_dir',
        help='Output directory name for generated legacy VTK files',
        default='vtk_files')
    parser.add_argument(
        '--n-dims',
        dest='n_dims',
        help='Number of dimensions recorded in patch file (either 2D or 3D)',
        required=True,
        type=int)
    args = parser.parse_args()

    if args.filepath:
        if not os.path.exists(args.filepath):
            print(
                "Error, specified input file '{}' does not exist, exiting...".format(
                    args.filepath))
            sys.exit(1)

        output_file = os.path.join(
            args.output_dir,
            'vtk_file_' +
            os.path.basename(
                args.filepath).replace(
                '.peano-patch-file',
                '.vtk'))
        peano_patch_to_legacy_vtk(args.filepath, output_file, args.n_dims)

    if args.metafile:
        if not os.path.exists(args.metafile):
            print(
                "Error, specified input directory '{}' does not exist, exiting...".format(
                    args.directory_path))
            sys.exit(1)

        patch_file_names = []
        with open(args.metafile, 'r') as metafile:
            for line in metafile.readlines():
                words = line.split()
                if 'include' in words:
                    patch_file_names.append(words[1].strip('"'))

        for file in patch_file_names:
            output_file = os.path.join(
                args.output_dir,
                'vtk_file_' +
                file.replace(
                    '.peano-patch-file',
                    '.vtk'))
            peano_patch_to_legacy_vtk(
                os.path.join(
                    os.path.dirname(
                        args.metafile),
                    file),
                output_file,
                args.n_dims)
