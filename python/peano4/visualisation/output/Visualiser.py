# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.input.PatchFileParser import PatchFileParser


def validate(
    u_cell_data,
    u_dof,
    u_dimensions,
    u_unknowns,
    u_is_data_associated_to_cell,
    u_description,
    u_mapping,
):
    """
    All the parsers in a data set have to yield the same type of data, i.e.
    with the same number of unknowns and dofs/patch. I return the crucial
    data afterwards, i.e. this routine validates and returns the quantities
    of interest.
    """

    # These are the data to be returned
    dimensions = -1
    unknowns = -1
    dof = -1
    description = ""
    is_data_associated_to_cell = True
    mapping = ""

    # Take copies
    (
        snapshot_cell_data,
        snapshot_dof,
        snapshot_dimensions,
        snapshot_unknowns,
        is_data_associated_to_cell,
        description,
        mapping,
    ) = (
        u_cell_data,
        u_dof,
        u_dimensions,
        u_unknowns,
        u_is_data_associated_to_cell,
        u_description,
        u_mapping,
    )

    if snapshot_dimensions not in [2, 3]:
        print("File parsing unsuccessful. Supported dimensions are d=2 and d=3")
        return
    if snapshot_dof == 0:
        print("File parsing unsuccessful. No dof specified")
        return
    if snapshot_unknowns == 0:
        print("File parsing unsuccessful. No unknowns specified")
        return

    if snapshot_dimensions != dimensions and dimensions > 0:
        print(
            "Dimensions not compatible with dimensions from previous files in the snapshot"
        )
        return
    dimensions = snapshot_dimensions
    if snapshot_dof != dof and dof > 0:
        print("DoF not compatible with dof from previous files in the snapshot")
        return
    dof = snapshot_dof

    if snapshot_unknowns != unknowns and unknowns > 0:
        print(
            "Unknowns not compatible with unknowns from previous files in the snapshot"
        )
        return
    unknowns = snapshot_unknowns
    return (
        snapshot_cell_data,
        dof,
        dimensions,
        unknowns,
        is_data_associated_to_cell,
        description,
        mapping,
    )


class SnapShot:
    """!
    Small class to capture all of the snapshots in a Patch meta file.
    Just need the timestamp this is associated with and all of the
    patch files that this file includes. Add them using the right
    member function.
    """

    def __init__(self):
        self.timestamp = -1
        self.filenames = []

    def add_patch_file(self, patch_file_name):
        """!"""
        # For debugging whilst in testing phase
        assert patch_file_name not in self.filenames, "Already added this patch file!"
        self.filenames.append(patch_file_name)

    @property
    def include_file_counter(self):
        """!
        Property to indicate number of patch files in this snapshot
        """
        return len(self.filenames)

    def set_timestamp(self, timestamp):
        assert self.timestamp == -1, "Should only set this property once!"
        self.timestamp = timestamp

    def __repr__(self):
        return f"Timestamp: {self.timestamp}, {self.include_file_counter} subdomains"


class PatchFileData:
    """!
    Helper class to capture the important data coming from a patch file

    This is what we use to capture the combined patch file data from.
    We could keep everything in the helper class from PatchFileParser,
    but the intention during the rewrite was to keep everything fairly
    close to the way the original vis scripts worked.
    """

    def __init__(self, unknown_name, dimensions):
        self._dimensions = dimensions
        self._unknown_name = unknown_name
        self._cell_data = []
        self._dof = 0
        self._unknowns = 0
        self._is_data_associated_to_cell = None
        self._description = ""
        self._mapping = None

    def copy_data_from_parser_object(self, unknown_attributes):
        """!
        pass in a UnknownAttributes object from PatchFileParser.py,
        steal everything and return
        """
        assert (
            self._unknown_name == unknown_attributes.unknown_name
        ), "Comparing the wrong objects. something must have gone wrong..."
        # Append the cell data to this
        self._cell_data += unknown_attributes.cell_data

        # Make sure all other attributes are the same
        self._dof = unknown_attributes.dof
        # self._dimensions = unknown_attributes.dimensions  # This one was not captured
        self._unknowns = unknown_attributes.unknowns
        self._is_data_associated_to_cell = unknown_attributes.is_data_associated_to_cell
        self._description = unknown_attributes.description
        self._mapping = unknown_attributes.mapping

    def apply_renderer_to_data(self, renderer):
        """!
        Pass in a renderer object which will modify everything
        in the way that the filter.render() function intends

        This is basically the same as render_or_validate_for_each_unknown() from
        the PatchFileParser class. The reason why we don't need
        to do it for each unknown here is that the different unknowns
        have been captured in by the caller of this function

        We could rename this to something more general, since i
        think we modify the same attributes in the same order
        as the validate function
        """
        (
            self._cell_data,
            self._dof,
            self._dimensions,
            self._unknowns,
            self._is_data_associated_to_cell,
            self._description,
            self._mapping,
        ) = renderer(
            self._cell_data,
            self._dof,
            self._dimensions,
            self._unknowns,
            self._is_data_associated_to_cell,
            self._description,
            self._mapping,
        )

    def __repr__(self):
        return f"{self._unknown_name}: {self._unknowns} unknowns"


class Visualiser:
    """!
    The visualiser is first and foremost a persistency layer around
    datasets. It serves as abstract base class for real output formats,
    i.e. it can parse files and hold them, but it does not do anything
    with them.

    The flow should work like this:
      - Pass the file name of the patch file meta data to this class.
        We then use read_metadata() to read this file, and gather, for
        each timestamp, the list of patch files that we need to read.
        For each timestamp, we create an instance of the Snapshot
        helper class, which captures the timestamp, plus a list of
        patch files to parse
      - We then pass back to display(). Then, for each snapshot, we
        do the following:

        - Create a PatchFileParser object for each patch file within
          this timestamp
        - Read all the patch file data and combine it into the
          PatchFileData class in this file.
        - Return this data to child class, so that it can be rendered
          by display()

    Currently, the only working child class is the VTU renderer.
    """

    def __init__(self, file_name, start, end, verbose=False):
        """!
        Abstract superclass of a visualiser

        file_name: String
           Name of a Peano patch file. This does not
           need to be a meta file.

           If we run render.py with the argument -s (to denote we are using single
           file), then we will use the file_name to create a member of the
           snapshot class (which captures the patch file to read, with default
           timestamp of 0). This is then parsed by parse_and_render, and handed
           back to child class.

           If this is a meta file, then display() will be used.

        verbose: Boolean
           Controls how much the class writes out.

        ## Attributes

        self.identifier: String
           Pick one subidentifier from the input file. If this one is empty, we take
           every input data set. However, some output formats (such as ParaView)
           don't support multiple output data sets, so we might end up with either
           the first or last one from the file - depending on the implementations.
        """
        self._file_name = file_name  # Either for meta file or single file we pass in
        self._start = start
        self._end = end
        self._filter = []

        # This is used for writing patch files
        self.identifier = (
            ""  # This identifier is also used in helper class. pass it along
        )
        self.verbose = verbose

        self._dimensions = 0
        self._description = ""  #
        self._is_data_associated_to_cell = False

        # Holds all of the timestamps and file names of
        # all the snapshots that we wanna visualise
        self.snapshots = []

        # for debugging
        self.meta_data_has_been_read = False

    def display(self):
        """
        This is the one routine the visualisers typically do overwrite.
        """
        # First, read metadata file. Won't be doing this for single file
        self.read_metadata()
        return

    def display_single_file(self):
        """!
        If we are here we've supplied a single file which is
        itself a snapshot. Turn it into an instance of the
        snapshot helper class, set the timestamp to 0 and
        pass it back. We can just call parse_and_render_snapshot()
        on the result
        """
        snapshot = SnapShot()
        snapshot.add_patch_file(self._file_name)
        snapshot.set_timestamp(0)
        return snapshot

    def read_metadata(self):
        """!
        Read the patch file metadata. In this routine we should:
         - Count the number of timesteps this patch file records
         - Get each of the file paths for the timestep
        """
        found_snapshot = False
        found_snapshots = []

        print(f"Reading metadata for {self._file_name}")
        with open(self._file_name, "r") as f:
            line = f.readline()
            while line:  # While there is another line to read, increment at bottom
                if "begin dataset" in line:
                    # Encountered a dataset. Get the snapshot and all the filenames associated

                    # Create a new snapshot instance
                    s = SnapShot()
                    while (
                        "end dataset" not in line
                    ):  # Read until the end of the dataset
                        if "timestamp" in line:
                            s.set_timestamp(float(list(line.split("  "))[2][:-1]))

                        if "include" in line:
                            file_name = line.split('"')[1]
                            # print(f"Adding {file_name} to snapshot")
                            s.add_patch_file(file_name)
                        line = f.readline()

                    # We have left the while loop for dataset. add snapshot to list
                    found_snapshots.append(s)
                    found_snapshot = True
                line = f.readline()

        assert (
            found_snapshot
        ), "No snapshots found. Did you mean to run just for a single file? If so, try using the argument -s"

        # Remove when working
        self.meta_data_has_been_read = True
        self.snapshots = found_snapshots[self._start : self._end + 1]
        for snapshot in self.snapshots:
            print(f"Got snapshot: {snapshot}")

    def append_filter(self, filter):
        """ """
        self._filter.append(filter)

    def remove_filters(self):
        """ """
        self._filter = []

    def parse_and_render_snapshot(self, snapshot):
        """!
        We pass in a snapshot object here, which should contain
        time stamp and filenames for everything we want to render here.

        Behaviour is:
         - parse all the files listed in snapshot (use the method we wrote)
         - return a dict, where the keys are unknown names, and the values are cell data to be rendered
         - child class should handle the rest.
        """

        data_to_be_rendered = self.parse_snapshot(snapshot)

        # Hand back to child class
        return data_to_be_rendered

    def parse_snapshot(self, snapshot):
        """!
        We rewrite "reload" from previous version of this file.

        We take in a argument of "snapshot", which is a helper class
        with two attributes: timestamp and a list of patch files to parse.

        We need to ensure that self.identifier is set correctly so that
        the correct unknown is picked by the PatchFileParser. Remove this
        line of comment when this is done.

        Also need to handle removal of relative file paths here

        The reason we (possibly) apply filters on individual pieces of data,
        or once the patches have been concatenated is because we may have
        more than one file to parse per snapshot. The choice is is about
        whether we want to apply filters before combining all the patches
        into one list, or afterwards?

        What does this function do?
          - Get filename for each patch file in this timestamp
          - Produce a PatchFileParser object for each
          - Parse each file in parallel
          - Apply all the filters specified in render.py
          - Amalgamate data as in original reload() method
          - Returns a dict, where the keys are unknown names, and the values are patches that can be placed into individual patch files
        """
        parsers = []

        for file_counter, patch_filename in enumerate(snapshot.filenames):
            if self.verbose:
                print(f"Parsing file {patch_filename}")
            parser = PatchFileParser(patch_filename, self.identifier, file_counter)
            parsers.append(parser)

        for parser in parsers:
            parser.parse_file()

        if self.verbose:
            print("All individual files are parsed")
            print("Apply filter(s) to individual files (where appropriate)")

        for parser in parsers:
            for fil in self._filter:
                if fil.run_on_individual_pieces_of_data:
                    if self.verbose:
                        print(f"Apply filter {fil} to snapshot")
                    # We modify the attributes of parser here, and amalgamate after.

                    # Let's hide what it does.
                    # Pass in the function to be evaluated. Handle each of the unknowns in patch file parser class

                    # By default, we don't go here.
                    parser.render_or_validate_for_each_unknown(fil.render)

        # Let's create a dict of pairs unknown_name, patch_file_data
        patch_file_data_to_render = {}

        # Let's get all of the unknown names seen by the parsers
        # This is obviously redundant, but let's not assume (at this stage)
        # that all of the unknown names have been seen in all of the patch files
        # for this snapshot.
        for parser in parsers:
            for unknown_name, unknown_data in parser.unknown_attributes.items():
                # ie we create empty helper class with the unknown name
                if unknown_name not in patch_file_data_to_render.keys():
                    # Not seen it yet, add for first time.
                    patch_file_data_to_render[unknown_name] = PatchFileData(
                        unknown_name, unknown_data.dimensions
                    )
                # Steal data
                # This will amalgamate cell data on a like-for-like basis
                patch_file_data_to_render[unknown_name].copy_data_from_parser_object(
                    parser.unknown_attributes[unknown_name]
                )

        # Now we have amalgamated all of the patch data, let's apply the filters
        for unknown_name, patch_file_data in patch_file_data_to_render.items():
            for fil in self._filter:
                if fil.run_on_concatenated_data:
                    # Pass this function in, rest is handled inside
                    patch_file_data.apply_renderer_to_data(fil.render)
            if self.verbose:
                print(
                    f"Parsing {unknown_name} complete. Found {len(patch_file_data._cell_data)} cells, {patch_file_data._dof} dofs/cell with {patch_file_data._unknowns} unknowns per dof"
                )

        for parser in parsers:
            # Pass the validate function along to validate that the
            # attributes have been correctly set.
            parser.render_or_validate_for_each_unknown(validate)
        return patch_file_data_to_render
