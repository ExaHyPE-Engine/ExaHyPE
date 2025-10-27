# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Visualiser import Visualiser

import vtk
import time
import traceback
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np
from vtk.util.numpy_support import numpy_to_vtk, numpy_to_vtkIdTypeArray

from functools import lru_cache


@lru_cache(maxsize=8)
def _local_cells_2d(d):
    stride = d + 1
    ncp = d * d
    lc = np.empty((ncp, 5), dtype=np.intp)
    idx = 0
    for k in range(d):
        row = k * stride
        nxt = (k + 1) * stride
        for l in range(d):
            a = row + l
            lc[idx] = (4, a, a + 1, nxt + l, nxt + l + 1)
            idx += 1
    return lc


@lru_cache(maxsize=8)
def _split_mapping_2d(mapping_tuple):
    a = np.asarray(mapping_tuple, dtype=np.float32)
    return a[0::2].copy(), a[1::2].copy()


def prepare_2D_patches_unstructured_grid(
    cell_data,
    dof,
    unknowns,
    depth_scaling,
    description,
    is_data_associated_to_cell,
    mapping,
):
    """
    Prepare 2D patches in a unstructured grid for rendering

    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data

    dof: Integer
      Number of degrees of freedom per axis

    unknowns: Integer
      Number of unknowns per patch volume

    Returns:
    ----------
    grid: vtk.vtkUnstructuredGrid
      Grid cells
    """

    n_patches = len(cell_data)
    dofs_in_patch_per_axis = dof if is_data_associated_to_cell else dof - 1
    stride = dofs_in_patch_per_axis + 1
    points_per_patch = stride * stride
    cells_per_patch = dofs_in_patch_per_axis * dofs_in_patch_per_axis

    # Mapping is [xi0, eta0, xi1, eta1, ...]; split once and reuse
    mx, my = _split_mapping_2d(tuple(mapping))

    # Points
    pts = np.empty((n_patches * points_per_patch, 3), dtype=np.float32)
    base = 0
    for p in range(n_patches):
        patch = cell_data[p]
        x0, y0 = patch.offset
        w, h = patch.size
        zval = w * depth_scaling

        sl = slice(base, base + points_per_patch)
        pts[sl, 0] = x0 + mx * w
        pts[sl, 1] = y0 + my * h
        pts[sl, 2] = zval
        base += points_per_patch

    vtk_points = vtk.vtkPoints()
    vtk_points.SetData(numpy_to_vtk(pts, deep=False))

    # Build local connectivity once for a single patch, then offset and tile
    local_cells = _local_cells_2d(dofs_in_patch_per_axis)

    # Tile for all patches with appropriate point-id offsets
    all_cells = np.empty((n_patches * cells_per_patch, 5), dtype=np.intp)
    base_pt = 0
    out = 0
    for _ in range(n_patches):
        all_cells[out : out + cells_per_patch, 0] = 4
        all_cells[out : out + cells_per_patch, 1:] = local_cells[:, 1:] + base_pt
        base_pt += points_per_patch
        out += cells_per_patch

    cells_flat = np.ascontiguousarray(all_cells.reshape(-1))
    vtk_cells = vtk.vtkCellArray()
    vtk_cells.SetCells(
        n_patches * cells_per_patch, numpy_to_vtkIdTypeArray(cells_flat, deep=False)
    )

    # Data arrays
    total_items = (
        cells_per_patch if is_data_associated_to_cell else points_per_patch
    ) * n_patches

    unknowns_np = np.empty((total_items, unknowns), dtype=np.float32)
    subdomain_np = np.empty((total_items,), dtype=np.intp)

    write_pos = 0
    for p in range(n_patches):
        patch = cell_data[p]
        vals = np.asarray(patch.values, dtype=np.float32)

        if is_data_associated_to_cell:
            # Values are [cell0_u0..uM, cell1_u0..uM, ...] in k*l order (k*dof + l)
            patch_block = vals.reshape(cells_per_patch, unknowns)
        else:
            # Values are on vertices: (stride*stride) * unknowns
            patch_block = vals.reshape(points_per_patch, unknowns)

        n_rows = patch_block.shape[0]
        sl = slice(write_pos, write_pos + n_rows)
        unknowns_np[sl, :] = patch_block
        subdomain_np[sl] = int(cell_data[p].subdomain_number)
        write_pos += n_rows

    vtk_unknowns = numpy_to_vtk(np.ascontiguousarray(unknowns_np), deep=False)
    vtk_unknowns.SetNumberOfComponents(unknowns)
    vtk_unknowns.SetName(description or "Unknowns")

    vtk_sub = numpy_to_vtk(np.ascontiguousarray(subdomain_np), deep=False)
    vtk_sub.SetNumberOfComponents(1)
    vtk_sub.SetName("Subdomain")

    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(vtk_points)
    grid.SetCells(vtk.VTK_PIXEL, vtk_cells)

    if is_data_associated_to_cell:
        cd = grid.GetCellData()
        cd.SetScalars(vtk_unknowns)
        cd.AddArray(vtk_sub)
    else:
        pd = grid.GetPointData()
        pd.SetScalars(vtk_unknowns)
        pd.AddArray(vtk_sub)

    grid._np_keepalive = (pts, cells_flat, unknowns_np, subdomain_np)

    return grid


@lru_cache(maxsize=8)
def _local_cells_3d(d):
    stride = d + 1
    s2 = stride * stride
    ncp = d * d * d
    lc = np.empty((ncp, 9), dtype=np.intp)
    idx = 0
    for k in range(d):
        k0 = k * s2
        k1 = (k + 1) * s2
        for l in range(d):
            l0 = l * stride
            l1 = (l + 1) * stride
            for m in range(d):
                m0 = m
                m1 = m + 1
                lc[idx] = (
                    8,
                    k0 + l0 + m0,
                    k0 + l0 + m1,
                    k0 + l1 + m0,
                    k0 + l1 + m1,
                    k1 + l0 + m0,
                    k1 + l0 + m1,
                    k1 + l1 + m0,
                    k1 + l1 + m1,
                )
                idx += 1
    return lc


@lru_cache(maxsize=8)
def _split_mapping_3d(mapping_tuple):
    a = np.asarray(mapping_tuple, dtype=np.float32)
    return a[0::3].copy(), a[1::3].copy(), a[2::3].copy()


def prepare_3D_patches_unstructured_grid(
    cell_data, dof, unknowns, description, is_data_associated_to_cell, mapping
):
    """
    Prepare 3D patches in a unstructured grid for rendering

    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data

    dof: Integer
      Number of degrees of freedom per axis

    unknowns: Integer
      Number of unknowns per patch volume

    Returns:
    ----------
    grid: vtk.vtkUnstructuredGrid
      Grid cells
    """

    n_patches = len(cell_data)
    dofs_in_patch_per_axis = dof if is_data_associated_to_cell else dof - 1
    stride = dofs_in_patch_per_axis + 1
    points_per_patch = stride * stride * stride
    cells_per_patch = dofs_in_patch_per_axis**3

    # Mapping is [x0,y0,z0, x1,y1,z1, ...]; split once
    mx, my, mz = _split_mapping_3d(tuple(mapping))

    # Points
    pts = np.empty((n_patches * points_per_patch, 3), dtype=np.float32)
    base = 0
    for p in range(n_patches):
        offx, offy, offz = cell_data[p].offset
        sx, sy, sz = cell_data[p].size
        sl = slice(base, base + points_per_patch)
        pts[sl, 0] = offx + mx * sx
        pts[sl, 1] = offy + my * sy
        pts[sl, 2] = offz + mz * sz
        base += points_per_patch

    vtk_points = vtk.vtkPoints()
    vtk_points.SetData(numpy_to_vtk(pts, deep=False))

    # Connectivity: (k,l,m): (0,0,0), (0,0,1), (0,1,0), (0,1,1), (1,0,0), (1,0,1), (1,1,0), (1,1,1)
    local_cells = _local_cells_3d(dofs_in_patch_per_axis)

    # Tile for all patches with appropriate point-id offsets
    all_cells = np.empty((n_patches * cells_per_patch, 9), dtype=np.intp)
    out = 0
    base_pt = 0
    for _ in range(n_patches):
        all_cells[out : out + cells_per_patch, 0] = 8
        all_cells[out : out + cells_per_patch, 1:] = local_cells[:, 1:] + base_pt
        base_pt += points_per_patch
        out += cells_per_patch

    cells_flat = np.ascontiguousarray(all_cells.reshape(-1))
    vtk_cells = vtk.vtkCellArray()
    vtk_cells.SetCells(
        n_patches * cells_per_patch, numpy_to_vtkIdTypeArray(cells_flat, deep=False)
    )

    # Data arrays
    total_rows_per_patch = (
        cells_per_patch if is_data_associated_to_cell else points_per_patch
    )
    total_rows = n_patches * total_rows_per_patch

    unknowns_np = np.empty((total_rows, unknowns), dtype=np.float32)
    subdomain_np = np.empty((total_rows,), dtype=np.intp)

    write_pos = 0
    for p in range(n_patches):
        vals = np.asarray(cell_data[p].values, dtype=np.float32)
        if is_data_associated_to_cell:
            block = vals.reshape(cells_per_patch, unknowns)
        else:
            block = vals.reshape(points_per_patch, unknowns)

        rows = block.shape[0]
        sl = slice(write_pos, write_pos + rows)
        unknowns_np[sl, :] = block
        subdomain_np[sl] = int(cell_data[p].subdomain_number)
        write_pos += rows

    vtk_unknowns = numpy_to_vtk(np.ascontiguousarray(unknowns_np), deep=False)
    vtk_unknowns.SetNumberOfComponents(unknowns)
    vtk_unknowns.SetName(description or "Unknowns")

    vtk_sub = numpy_to_vtk(np.ascontiguousarray(subdomain_np), deep=False)
    vtk_sub.SetNumberOfComponents(1)
    vtk_sub.SetName("Subdomain")

    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(vtk_points)
    grid.SetCells(vtk.VTK_VOXEL, vtk_cells)

    if is_data_associated_to_cell:
        cd = grid.GetCellData()
        cd.SetScalars(vtk_unknowns)
        cd.AddArray(vtk_sub)
    else:
        pd = grid.GetPointData()
        pd.SetScalars(vtk_unknowns)
        pd.AddArray(vtk_sub)

    grid._np_keepalive = (pts, cells_flat, unknowns_np, subdomain_np)

    return grid


class VTUUnstructuredGrid(Visualiser):
    """!
    The visualiser is first and foremost a persistency layer around
    datasets. It does not work on the command line.

    Use this class by feeding the patch meta file you want to render.

    Everything happens in either display() or display_single_file().
    Please see documentation of these functions for more info.
    """

    def __init__(
        self,
        file_name,
        output_directory=".",
        strip_relative_paths=True,
        start=0,
        end=0,
        verbose=False,
    ):
        super(VTUUnstructuredGrid, self).__init__(file_name, start, end, verbose)
        self._tp = None
        self._grid = None
        self.output_directory = output_directory
        self.display_as_tree = False
        self._strip_relative_paths = strip_relative_paths

        # This is the standard header that we can put at the top of all pvd files. Don't overwrite it
        self.pvd_file = """<?xml version="1.0"?>
<VTKFile type="Collection" version="0.1"
         byte_order="LittleEndian">
  <Collection>
"""

    def process_snapshot(self, snapshot, i):
        """
        Helper function to process a single snapshot.
        Calls parse_and_render_snapshot and returns the snapshot timestamp and filenames.
        """
        if self.verbose:
            print("==================================")
            print(f"Process snapshot {i}")
            print("==================================")
        written_filenames = self.parse_and_render_snapshot(snapshot, i)
        return snapshot.timestamp, written_filenames

    def display(self, max_workers=None):
        """
        Should be called only once.

        How does this work?

        - First, hand over to the superclass display() function.
          This reads the metadata file, which should be available
          as self._file_name. This will populate self.snapshots
          with a list of the Snapshots helper classes. This helper
          class just contains timestamp plus a list of all patch
          files to parse for this timestamp

        - Once the list of snapshots has been populated, we loop
          over each snapshot, calling parse_and_render_snapshot()

        For each unknown that we see on each cell/vertex, we
        produce an ordered list of VTUUnstructuredGrid files, and a PVD file
        to glue them together.
        """

        super(VTUUnstructuredGrid, self).display()

        # This will be the file name that we output
        self._file_name = self._file_name.replace(".peano-patch-file", "")

        start_time = time.time()
        used_workers = None
        try:
            filenames = {}
            total_snapshots = len(self.snapshots)
            completed_snapshots = 0

            """
            Here's how this loop works:
              - call parse_and_render for each dataset
              - this should write the VTUUnstructuredGrid file and return the filename it wrote
              - we collect these filenames and then write the PVD files at the end
            """
            with ProcessPoolExecutor(max_workers=max_workers) as executor:
                print(
                    f"Processing snapshots in parallel using {executor._max_workers} workers"
                )
                used_workers = executor._max_workers
                # Loop over all the snapshots we detected whilst reading metadata
                # Submit all snapshots to be processed in parallel
                futures = [
                    executor.submit(self.process_snapshot, snapshot, i)
                    for i, snapshot in enumerate(self.snapshots)
                ]

                for future in as_completed(futures):
                    timestamp, written_filenames = future.result()

                    # Update filenames dictionary with the results from each completed future
                    for unknown_name, file_name in written_filenames.items():
                        if unknown_name not in filenames.keys():
                            # Haven't seen the name of this unknown before
                            filenames[unknown_name] = []
                        # Add the vtu filename we've written to the list for this unknown
                        filenames[unknown_name].append(
                            (timestamp, file_name)
                        )  # Assuming file_name is not a list

                    completed_snapshots += 1
                    progress = (completed_snapshots / total_snapshots) * 100
                    print(
                        f"Progress: {completed_snapshots}/{total_snapshots} snapshots processed ({progress:.2f}%)"
                    )
        except Exception as e:
            print("Got an exception: " + str(e))
            traceback.print_exc()
            pass
        end_time = time.time()

        print(
            f"Completed in {end_time - start_time:.2f} seconds with {used_workers} workers"
        )
        print(f"Dumped {sum([len(v) for _,v in filenames.items()])} datasets")

        # Next, we need to write a pvd "glue" file for each of these data series
        for unknown_name, file_data in filenames.items():
            file_data.sort(key=lambda x: x[0])  # Sort by timestamp
            # Make a copy of self.pvd file
            pvd_file = self.pvd_file

            # Now we need to loop over all of the pairs of (timestamp, file_name)
            for timestamp, file_name in file_data:
                if self._strip_relative_paths:
                    file_name = file_name.split("/")[-1]

                # Add the vtu filename to the pvd file.
                pvd_file += f"""
    <DataSet timestep=\"{timestamp}\" group=\"\" part=\"0\" file=\"{file_name}\" />
"""
            pvd_file += """
  </Collection>
</VTKFile>
"""
            # Finally, let's write this pvd file.
            with open(
                f"{self.output_directory}/{self._file_name}.{unknown_name}.pvd", "w"
            ) as meta_file:
                meta_file.write(pvd_file)

        if "/" in file_name and not self._strip_relative_paths:
            print(
                """
WARNING: The conversion tool has processed a dataset stored in a different
directory. If the resulting .pvd file ("""
                + file_name
                + """.pvd)
contains relative paths, you will have to copy it into the present working
directory before you load the outcome into ParaView. Otherwise, ParaView
will not be able to locate your actual data files.

Alternatively, you can rerun the postprocessing and eliminate relative
paths (see documentation/help of your conversion script).
"""
            )

    def display_single_file(self):
        snapshot = super(VTUUnstructuredGrid, self).display_single_file()
        return self.parse_and_render_snapshot(snapshot)

    def write_vtu(self, unknown_name, grid_data, index=None):
        """!
        unknown_name should describe which unknown we are
        writing here.

        Add an index to be placed in the filename
        """
        if index is None:
            # We splice this number into the filename.
            # So as to avoid another if/else statement,
            # we make this the empty string if not
            # supplied any integer.
            index = ""
        else:
            index = f"-{index}"  # Add a dash in
        if self._file_name.endswith(".vtu"):
            # Strip the vtu off
            self._file_name = self._file_name[:-4]
        file_name = (
            f"{self.output_directory}/{self._file_name}.{unknown_name}{index}.vtu"
        )

        writer = vtk.vtkXMLUnstructuredGridWriter()

        try:
            writer.SetCompressorTypeToLZ4()
        except AttributeError:
            # Fall back to zlib or set an LZ4 compressor object if available
            try:
                writer.SetCompressorTypeToZLib()
            except AttributeError:
                pass

        try:
            writer.SetDataModeToAppended()
            writer.EncodeAppendedDataOff()
        except AttributeError:
            writer.SetDataModeToBinary()

        writer.SetFileName(file_name)
        writer.SetInputData(grid_data)
        writer.Write()
        del writer  # Explicitly destroy object to speed up things

        if self.verbose:
            print("Wrote file " + file_name)

        return file_name

    def parse_and_render_snapshot(self, snapshot, index=0):
        """!
        We want to call the method in superclass,
        and then process this data into VTUUnstructuredGrid.

        Returns the filenames that we wrote, for each
        unknown. We combine these into vtk at the end.
        """
        data_to_be_rendered = super(
            VTUUnstructuredGrid, self
        ).parse_and_render_snapshot(snapshot)

        written_filenames = {}
        grid = None

        for unknown_name, unknown_data in data_to_be_rendered.items():
            """
            Need to convert all of this data into 2d/3d patches
            """
            if unknown_data._cell_data != []:
                if unknown_data._dimensions == 2:
                    grid = prepare_2D_patches_unstructured_grid(
                        unknown_data._cell_data,
                        unknown_data._dof,
                        unknown_data._unknowns,
                        1.0 if self.display_as_tree else 0.0,
                        unknown_data._description,
                        unknown_data._is_data_associated_to_cell,
                        unknown_data._mapping,
                    )
                else:
                    grid = prepare_3D_patches_unstructured_grid(
                        unknown_data._cell_data,
                        unknown_data._dof,
                        unknown_data._unknowns,
                        unknown_data._description,
                        unknown_data._is_data_associated_to_cell,
                        unknown_data._mapping,
                    )
            else:
                assert False, "Found empty cell data"
                # Write this data to vtu, return the filename for later processing
            if grid is not None:
                written_filenames[unknown_name] = self.write_vtu(
                    unknown_name, grid, index
                )

        return written_filenames
