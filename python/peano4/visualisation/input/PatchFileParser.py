# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np

from .Patch import Patch


class UnknownAttributes:
    """!
      Helper class to capture all of the attributes we need for each
      unknown, but were originally fixed to the PatchFileParser class
      itself.

      For instance, at time of writing the vis scripts cannot handle
      displaying separate vtu files for different data that is produced
      in Peano patch files. An excerpt:

    begin patch
      offset 9.629630e-01 9.629630e-01
      size 3.703704e-02 3.703704e-02
      begin vertex-values "value"
         0 0 0 0
      end vertex-values
      begin vertex-values "rhs"
         0 0 0 0
      end vertex-values
    end patch

      Previously, the only thing that would get parsed is the "value"
      field, before moving on. We aim to get it to do the "rhs" part
      as well.

      However, we cannot assume that the number of dofs per axis or
      unknowns per patch volume will be the same for both. So we
      create this helper class to hold those.

      We fix is_data_associated_to_cell to be the same as the
      PatchFileParser object which, in turn, means they must
      be the same for all of the unknowns

        Attributes:
        ----------

          file_path: String
            String of input file path (including filename and extension)

          cell_data: list of patches
            List of Patches with file data

          dimensions: Integer
            Dimension of domains

          dof: Integer
            Number of degrees of freedom per axis

          unknowns: Integer
            Number of unknowns per patch volume

          mapping: series of d-dimensional tuples
            Distorts the domain

          set_identifier: String
            Can be empty to parse everything.

    """

    def __init__(self, unknown_name, dimensions, is_data_associated_to_cell):
        # this is the name of the unknown we want to capture
        self.unknown_name = unknown_name

        # this is the number of unknowns per patch volume
        self.unknowns = 0
        # number of dofs per axis
        self.dof = 0
        self.description = ""
        self.cell_data = []
        self.mapping = []
        self.is_data_associated_to_cell = is_data_associated_to_cell
        self.dimensions = dimensions

    def set_unknowns(self, unknowns):
        assert self.unknowns == 0, "attribute already set!"
        self.unknowns = unknowns

    def set_dof(self, dof):
        assert self.dof == 0, "attribute already set!"
        self.dof = dof

    def set_description(self, description):
        self.description = description

    def set_celldata(self, celldata):
        assert not self.cell_data, "attribute already set!"
        self.cell_data = celldata

    def set_mapping(self, mapping):
        assert not self.mapping, "attribute already set!"
        self.mapping = mapping

    def _initialise_default_mapping_if_no_mapping_specified(self):
        assert not self.mapping, "attribute already set!"
        vertices_per_axis = self.dof
        if self.is_data_associated_to_cell:
            vertices_per_axis += 1
        if self.mapping == [] and self.dimensions == 2:
            for y in range(vertices_per_axis):
                for x in range(vertices_per_axis):
                    self.mapping.append(x * 1.0 / (vertices_per_axis - 1))
                    self.mapping.append(y * 1.0 / (vertices_per_axis - 1))
        if self.mapping == [] and self.dimensions == 3:
            for z in range(vertices_per_axis):
                for y in range(vertices_per_axis):
                    for x in range(vertices_per_axis):
                        self.mapping.append(x * 1.0 / (vertices_per_axis - 1))
                        self.mapping.append(y * 1.0 / (vertices_per_axis - 1))
                        self.mapping.append(z * 1.0 / (vertices_per_axis - 1))

    def append_patch(self, patch):
        self.cell_data.append(patch)

    def __repr__(self):
        return f"{self.unknown_name}: unknowns:{self.unknowns}, dof:{self.dof}"


class PatchFileParser:
    """!
    Parser for Peano block file output.

    This class should be instantiated from within the Visualiser class.
    This class should be called once per patch file that contains
    patch data that we want to render. The main method that is called
    is parse_file().

    This method doesn't return anything. Rather, it sets unknown_attributes
    to capture all the patch data for each of the unknowns that we find.

    Then, the Visualiser class will steal this data, and combine it into
    its own helper class.

    Attributes:
    ----------

      file_path: String
        String of input file path (including filename and extension)

      dimensions: Integer
        Dimension of domains

      mapping: series of d-dimensional tuples
        Distorts the domain

      set_identifier: String
        Can be empty to parse everything.

      unknown_attributes: Dict{ String, UnknownAttributes }
        Dictionary that has the names of each unknown we wanna capture
        as its keys, and the values are the helper class which captures
        dimensions, cell data etc. Each of the unknowns should be
        listed during the metadata part of the patch file, ie between
        "begin cell-metadata" and "end cell-metadata".
    """

    def __init__(self, file_path, set_identifier, subdomain_number):
        """ """
        self.file_path = file_path
        self.is_data_associated_to_cell = None
        self.dimensions = -1

        self.set_identifier = set_identifier
        self.subdomain_number = subdomain_number
        self.parsed = False
        self.file_contains_patches = False

        """
    Here we expected the keys to be a list of unknown names that we
    have associated with each cell/vertex, and the values to be 
    instances of the helper class UnknownAttributes
    """
        self.unknown_attributes = {}

    def __repr__(self):
        return f"Parser for {self.file_path}, subdomain {self.subdomain_number}, identifier {self.set_identifier}"

    def __parse_meta_data_region(self, current_line, file_object, end_condition):
        """!
        Pass in file object, move it along and return it
        """
        # unknown_name is contained at end of "begin x-metadata" line
        unknown_name = current_line.strip().split()[-1][1:-1]

        # create new helper class for this
        # set the bool for data associated with cell to be the same as self.
        # we set self.is_data_associated_to_cell already, during parse_file(),
        # just before we came into this function.
        patch_unknown = UnknownAttributes(
            unknown_name, self.dimensions, self.is_data_associated_to_cell
        )

        current_line = file_object.readline().strip()

        use_default_mapping = True

        # do everything that we did in old line reading function
        while not end_condition in current_line:
            if "number-of-unknowns" in current_line:
                patch_unknown.set_unknowns(int(current_line.strip().split()[1]))
            if "number-of-dofs-per-axis" in current_line:
                patch_unknown.set_dof(int(current_line.strip().split()[1]))
            if "description" in current_line:
                patch_unknown.set_description(
                    current_line.strip().split("description")[1]
                )
                if '"' in patch_unknown.description:
                    print(
                        "Warning: data description field "
                        + patch_unknown.description
                        + " holds \" which might lead to issues with VTK's XML export. Remove them"
                    )
                    patch_unknown.description = patch_unknown.description.replace(
                        '"', ""
                    )
            if "mapping" in current_line:
                use_default_mapping = False
                values = current_line.strip().split("mapping")[1]
                patch_unknown.set_mapping(np.fromstring(values, dtype=float, sep=" "))

            current_line = file_object.readline().strip()

        if use_default_mapping:
            patch_unknown._initialise_default_mapping_if_no_mapping_specified()

        # finally, add this unknown to our dictionary
        self.unknown_attributes[unknown_name] = patch_unknown

        return file_object, current_line

    def __parse_patch_region(
        self, current_line, file_object, end_condition="end patch"
    ):
        """!
        Pass in file object, run it until we encounter the end condition,
        and then return it

        We assume that current_line == "begin patch file" at this stage

        This is where we read the actual data
        """
        self.file_contains_patches = True
        # Get patch offset
        current_line = file_object.readline().strip()
        line = current_line.strip().split()
        if self.dimensions == 2:
            offset = (float(line[1]), float(line[2]))
        elif self.dimensions == 3:
            offset = (float(line[1]), float(line[2]), float(line[3]))

        # Get patch size
        current_line = file_object.readline().strip()
        line = current_line.strip().split()
        if self.dimensions == 2:
            size = (float(line[1]), float(line[2]))
        elif self.dimensions == 3:
            size = (float(line[1]), float(line[2]), float(line[3]))

        # run until we have found all of the fields in this
        # end condition is "end patch" by default
        while end_condition not in current_line:
            current_line = file_object.readline().strip()
            values = None  # prevents us adding empty patch to list at end of loop
            unknown_name = ""

            # get cell data
            if current_line.startswith("begin cell-values") and (
                self.set_identifier == ""
                or current_line.endswith('"' + self.set_identifier + '"')
            ):
                assert (
                    self.is_data_associated_to_cell
                ), "is_data_associated_to_cell flag set incorrectly"
                unknown_name = current_line.strip().split()[-1][1:-1]
                current_line = file_object.readline()
                values = np.fromstring(current_line, dtype=float, sep=" ")

            # do same for vertex data
            if current_line.startswith("begin vertex-values") and (
                self.set_identifier == ""
                or current_line.endswith('"' + self.set_identifier + '"')
            ):
                assert (
                    not self.is_data_associated_to_cell
                ), "is_data_associated_to_cell flag set incorrectly"
                unknown_name = current_line.strip().split()[-1][1:-1]
                current_line = file_object.readline()
                values = np.fromstring(current_line, dtype=float, sep=" ")

            # if we have read some data, let's add it to our list
            if values is not None:
                patch = Patch(offset, size, values, unknown_name, self.subdomain_number)

                # by this point, all the unknown_names should be present
                self.unknown_attributes[unknown_name].append_patch(patch)

        return file_object, current_line

    def parse_file(self):
        """!
        Read file and return cell data, dimensions, number of degrees of freedom and number of unknowns

        Parameters:
        ----------
        set_identifier: String
          Name of the set of unknowns we want to extract. If this is the empty string
          then I parse all content.

        @todo: clean up the loop below. we can separate into getting metadata, and then reading patches
        """
        # print("Reading " + self.file_path + " as subdomain " + str(self.subdomain_number))
        current_line = ""
        try:
            with open(self.file_path, "r") as data_file:
                current_line = data_file.readline()
                while current_line:
                    # when we reach EOF, this_line will eval to False
                    """
                    the reason we keep track of 2 variables current_line and
                    line  is because we want current_line to eval to False when we reach
                    EOF. However, we need to strip the whitespaces and newline
                    characters off the line for the data processing, without
                    exiting the loop if we have empty lines.
                    """
                    current_line = data_file.readline()
                    stripped_line = current_line.strip()
                    if stripped_line.startswith("dimensions"):
                        self.dimensions = int(stripped_line.strip().split()[1])
                        assert self.dimensions in [
                            2,
                            3,
                        ], "Only 2d and 3d patches are supported"

                    # Read out meta data
                    if stripped_line.startswith("begin cell-metadata") and (
                        self.set_identifier == ""
                        or stripped_line.endswith('"' + self.set_identifier + '"')
                    ):
                        self.is_data_associated_to_cell = True
                        # this function advances us along the data file
                        data_file, stripped_line = self.__parse_meta_data_region(
                            stripped_line, data_file, "end cell-metadata"
                        )

                    if stripped_line.startswith("begin vertex-metadata") and (
                        self.set_identifier == ""
                        or stripped_line.endswith('"' + self.set_identifier + '"')
                    ):
                        self.is_data_associated_to_cell = False
                        # this function advances us along the data file
                        data_file, stripped_line = self.__parse_meta_data_region(
                            stripped_line, data_file, "end vertex-metadata"
                        )

                    elif stripped_line.startswith("begin patch"):
                        # pass data_file into self.__parse_patch_region, which will modify and return it
                        # ie we move further along the file inside this function
                        # appending new patches happens in this function
                        data_file, stripped_line = self.__parse_patch_region(
                            stripped_line, data_file
                        )

                # self._initialise_default_mapping_if_no_mapping_specified() # handled in meta data reader
                self.parsed = True
        except Exception as e:
            print(
                "Error: was not able to read "
                + self.file_path
                + ": "
                + str(e)
                + " in line "
                + current_line
            )
            self.parsed = False

        # COMMENTING OUT DEBUG STATEMENTS
        # print("finished parsing the file.")
        # print(f"we now have {len(self.unknown_attributes.keys())} unknown names, and we have seen a total of {sum([len(v.cell_data) for _,v in self.unknown_attributes.items()])} patches...")
        # print("we pipe the data into UnknownAttributes objects")

    def render_or_validate_for_each_unknown(self, render_or_validate):
        """!
        We need to call the render function for each filter
        on a variety of attributes, and then return them if they
        are modified.

        We do this in a functional style, ie the Visualiser class
        contains all of the filters we wish to apply, and we pass
        the render function that we intend to use back to this
        layer. Ugly, but we need to do some looping over the
        different unknowns we see in the patch files, and I thought
        it best to hide that from the parse_snapshot region of
        the vis code.

        The same apply for the validate function from the Visualiser
        file as well.
        """

        for unknown_name, unknown_data in self.unknown_attributes.items():
            # this is horrible to read. but we need to pass all these
            # attributes in, and capture their return values from the
            # renderer function
            (
                unknown_data.cell_data,
                unknown_data.dof,
                unknown_data.dimensions,
                unknown_data.unknowns,
                unknown_data.is_data_associated_to_cell,
                unknown_data.description,
                unknown_data.mapping,
            ) = render_or_validate(
                unknown_data.cell_data,
                unknown_data.dof,
                unknown_data.dimensions,
                unknown_data.unknowns,
                unknown_data.is_data_associated_to_cell,
                unknown_data.description,
                unknown_data.mapping,
            )
