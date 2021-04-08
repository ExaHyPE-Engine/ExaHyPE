import argparse
import os
import sys

from peano4.visualisation.OutputFileParser import OutputFileParser
from peano4.visualisation.Patch import Patch


def peano_patch_to_legacy_vtk_2d(patch_file, vtk_file_name):
    ofparser = OutputFileParser(patch_file, '', 0)
    ofparser.parse_file()
    patch_size = ofparser.get_smallest_patch_size()
    patch_boundaries_x, patch_boundaries_y, patch_boundaries_z = ofparser.get_patch_boundaries(
        patch_size)
    num_cells_on_axis = len(patch_boundaries_x) * \
        ofparser.dof   # assumes x==y==z
    spacing = ((patch_boundaries_x[-1] + patch_size) -
               patch_boundaries_x[0]) / num_cells_on_axis

    with open(os.path.join("vtk_files", vtk_file_name), "w") as vtk:
        vtk.write(
            "# vtk DataFile Version 3.0\n"
            "insert some description of the data here\n"
            "ASCII\n"
            "DATASET STRUCTURED_POINTS\n"
            f"DIMENSIONS {num_cells_on_axis} {num_cells_on_axis}\n"
            f"ORIGIN {patch_boundaries_x[0]} {patch_boundaries_x[0]}\n"
            f"SPACING {spacing}\n"
        )

        # To reduce time complexity I create a structured
        # matrix of patches from the unstructured set of
        # patches that make up the Peano patch file
        patch_matrix = [[Patch for i in range(len(patch_boundaries_x))] for j in range(
            len(patch_boundaries_x))]  # assumes x==y==z
        for patch in ofparser.cell_data:
            if patch.size[0] == patch_size:
                x_pos_matrix = patch_boundaries_x.index(patch.offset[0])
                y_pos_matrix = patch_boundaries_y.index(patch.offset[1])
                patch_matrix[x_pos_matrix][y_pos_matrix] = patch

        '''
        # use this to check matrix
        for i in range(len(patch_boundaries_x)):
            for j in range(len(patch_boundaries_x)):
                print(patch_matrix[j][i].offset)
            print()
        '''

        # Again to reduce compute time I store all unkowns in list
        # It is organised as follows: unknown 1 ... unknown n
        # For each coordinate x axis before shifting up y axis
        structured_values = []

        for i in range(len(patch_boundaries_x)):
            value_sets = []
            for j in range(len(patch_boundaries_x)):   # assumes x==y==z
                value_sets.append(patch_matrix[j][i].values)

            for j in range(ofparser.dof):
                for values in value_sets:
                    for k in range(ofparser.dof):
                        p_first_value = (
                            j * ofparser.dof * ofparser.unknowns) + k * ofparser.unknowns
                        for unknown in values[p_first_value:
                                              p_first_value + ofparser.unknowns]:
                            structured_values.append(unknown)

        numComp = 1
        for unknown in range(ofparser.unknowns):
            vtk.write(
                f"SCALARS unknown_{unknown+1} float {numComp}\n"
                f"LOOKUP_TABLE unknown_{unknown+1}_table\n"
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
    args = parser.parse_args()

    if args.filepath:
        if not os.path.exists(args.filepath):
            print(
                "Error, specified input file '{}' does not exist, exiting...".format(
                    args.filepath))
            sys.exit(1)

        output_file = 'vtk_file_' + os.path.basename(args.filepath)
        peano_patch_to_legacy_vtk_2d(args.filepath, output_file)

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
            output_file = 'vtk_file_' + file
            peano_patch_to_legacy_vtk_2d(
                os.path.join(
                    os.path.dirname(
                        args.metafile),
                    file),
                output_file)
