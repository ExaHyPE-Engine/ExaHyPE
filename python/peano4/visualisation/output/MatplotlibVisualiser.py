# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from unittest import result
from .Visualiser import Visualiser
from .Plotter import plot_data
from .GeometryProcessor import generate_geometries_and_colors


def patch_file_data_to_dict(patch_file_data):
    """
    Convert PatchFileData object to plotting dictionary for visualisation.
    """
    patches_list = []
    for idx, patch in enumerate(patch_file_data._cell_data):
        patch_dict = {
            "id": idx + 1,
            "offset": list(patch.offset),
            "size": list(patch.size),
            "values": patch.values.tolist()
            if hasattr(patch.values, "tolist")
            else patch.values,
            "data_type": "cell-values"
            if patch_file_data._is_data_associated_to_cell
            else "vertex-values",
            "data_name": patch_file_data._unknown_name,
            "subdomain_number": getattr(patch, "subdomain_number", None),
        }
        patches_list.append(patch_dict)

    mapping = patch_file_data._mapping
    if patch_file_data._dimensions == 3:
        mapping_grid = [
            (mapping[i], mapping[i + 1], mapping[i + 2])
            for i in range(0, len(mapping), 3)
        ]
    elif patch_file_data._dimensions == 2:
        mapping_grid = [(mapping[i], mapping[i + 1]) for i in range(0, len(mapping), 2)]
    else:
        mapping_grid = []
    # we set the description to sum to handle --combine-unknowns flag
    if patch_file_data._unknowns == 1:
        descriptions = ["Combined Value"]
    else:
        descriptions = (
            [desc.strip() for desc in patch_file_data._description.split(",")]
            if patch_file_data._description
            else []
        )
    solver_meta = {
        "name": patch_file_data._unknown_name,
        "descriptions": descriptions,
        "unknowns_count": patch_file_data._unknowns,
        "mapping_grid": mapping_grid,
        "dofs_per_axis": patch_file_data._dof,
        "patches": patches_list,
    }

    result = {
        "patches": patches_list,
        "solvers": {patch_file_data._unknown_name: solver_meta},
        "dimensions": patch_file_data._dimensions,
    }
    return result


# Top-level worker function for parallel processing
def process_snapshot_worker(snapshot, i, config, filters):
    """
    Top-level worker for parallel processing of snapshots.
    Accepts only picklable arguments.
    """
    from .GeometryProcessor import generate_geometries_and_colors
    from .Plotter import plot_data

    # Unpack config
    output_directory = config["output_directory"]
    output_format = config["format"]
    slice_axis = config["slice_axis"]
    slice_position = config["slice_position"]
    slice_width = config["slice_width"]
    verbose = config["verbose"]
    unknown = config["unknown"]
    dpi = config["dpi"]
    file_name = config["file_name"]

    # Parse and render snapshot
    from .Visualiser import Visualiser  # Import here for multiprocessing safety

    vis = Visualiser(
        file_name, 0, 0, verbose
    )  # minimal instance of visualiser to call parse_and_render_snapshot
    for f in filters:
        vis.append_filter(f)
    data_to_be_rendered = vis.parse_and_render_snapshot(snapshot)

    for unknown_name, unknown_data in data_to_be_rendered.items():
        plot_dict = patch_file_data_to_dict(unknown_data)
        processed_geometries = generate_geometries_and_colors(
            plot_dict,
            slice_axis,
            slice_position,
            slice_width,
            verbose=verbose,
            unknown=unknown,
        )
        clean_file_name = file_name.replace(".peano-patch-file", "")
        output_filepath = (
            f"{output_directory}/{clean_file_name}.{unknown_name}-{i}.{output_format}"
        )
        plot_data(
            processed_data=processed_geometries,
            dimensions=unknown_data._dimensions,
            output_filepath=output_filepath,
            title_suffix=f"{clean_file_name}.{unknown_name}-{i}",
            dpi=dpi,
            output_format=output_format,
            verbose=verbose,
        )


class MatplotlibVisualiser(Visualiser):
    def __init__(
        self,
        file_name,
        dpi,
        output_directory=".",
        start=0,
        end=0,
        verbose=False,
        format="png",
        unknown=None,
        slice_axis="z",
        slice_position=None,
        slice_width=0.01,
    ):
        super().__init__(file_name, start, end, verbose)
        self.dpi = dpi
        self.output_directory = output_directory
        self.format = format
        self.unknown = unknown
        self.slice_axis = slice_axis
        self.slice_position = slice_position
        self.slice_width = slice_width
        print(f"Initialized MatplotlibVisualiser with format: {self.format}")

    def display(self, max_workers=None):
        super().display()
        from concurrent.futures import ProcessPoolExecutor

        # Prepare a config dict with all needed parameters
        config = {
            "output_directory": self.output_directory,
            "format": self.format,
            "slice_axis": self.slice_axis,
            "slice_position": self.slice_position,
            "slice_width": self.slice_width,
            "verbose": self.verbose,
            "unknown": self.unknown,
            "dpi": self.dpi,
            "file_name": self._file_name,
        }

        with ProcessPoolExecutor(max_workers=max_workers) as executor:
            futures = [
                executor.submit(
                    process_snapshot_worker, snapshot, i, config, self._filter
                )
                for i, snapshot in enumerate(self.snapshots)
            ]
            for future in futures:
                future.result()  # Wait for all to finish

    def display_single_file(self):
        """
        Processes and renders a single snapshot file.
        """
        super().display()
        print(f"Processing single snapshot file: {self._file_name}")
        # For a single file, we process it with index 0.
        self.process_snapshot_serial(self._file_name, 0)

    # serial version for debugging:
    def process_snapshot_serial(self, snapshot, i):
        data_to_be_rendered = super().parse_and_render_snapshot(snapshot)
        for unknown_name, unknown_data in data_to_be_rendered.items():
            plot_dict = patch_file_data_to_dict(unknown_data)
            processed_geometries = generate_geometries_and_colors(
                plot_dict,
                self.slice_axis,
                self.slice_position,
                self.slice_width,
                verbose=self.verbose,
                unknown=self.unknown,
            )
            clean_file_name = self._file_name.replace(".peano-patch-file", "")
            output_filepath = f"{self.output_directory}/{clean_file_name}.{unknown_name}-{i}.{self.format}"
            plot_data(
                processed_data=processed_geometries,
                dimensions=unknown_data._dimensions,
                output_filepath=output_filepath,
                title_suffix=f"{clean_file_name}.{unknown_name}-{i}",
                dpi=self.dpi,
                output_format=self.format,
                verbose=self.verbose,
            )
