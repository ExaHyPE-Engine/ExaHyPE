# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import matplotlib.cm as cm
import matplotlib.colors as mcolors
import itertools


def generate_geometries_and_colors(
    parsed_data,
    slice_axis,
    slice_position,
    slice_width,
    verbose=False,
    unknown=None,
    combine_unknowns=None,
):
    """
    Generates cell geometries for plotting.
    - For 3D data, it builds a single, robust PyVista UnstructuredGrid from all patches.
    - For 2D data, it generates vertices and colors for Matplotlib.
    """
    if verbose:
        print("Geometry Processor: Starting geometry and color generation...")

    patches = parsed_data.get("patches", [])
    solvers = parsed_data.get("solvers", {})
    dimensions = parsed_data.get("dimensions", -1)
    patches = parsed_data.get("patches", [])
    solvers = parsed_data.get("solvers", {})

    if not patches or not solvers:
        return {}
    # Use the first patch to extract metadata (e.g., solver name, data type) for the whole dataset. Since all patches share the same solver and type.
    first_patch = patches[0]
    solver_name = first_patch.get("data_name")
    if not solver_name or solver_name not in solvers:
        return {}

    solver_meta = solvers[solver_name]
    num_unknowns = solver_meta.get("unknowns_count", 0)
    dofs_per_axis = solver_meta.get("dofs_per_axis", 0)
    # logger.info(f"DEBUG: solver_meta = {solver_meta}")
    unknown_idx = unknown  # This comes from user input, which is now an integer.

    # Generate a descriptive name for the data being plotted.
    if combine_unknowns:
        unknown_to_visualise = "Combined_Value"
        # When combining, we are creating a new single dataset, so its index is 0.
        unknown_idx = 0
    else:
        # Validate the user-provided index for single unknowns
        if not (0 <= unknown_idx < num_unknowns):
            print(
                f"ERROR: Unknown index {unknown_idx} is out of bounds. Valid indices are 0 to {num_unknowns - 1}. Defaulting to 0."
            )
            unknown_idx = 0
        unknown_to_visualise = f"Component_{unknown_idx}"

    plot_type = first_patch.get("data_type", "none")

    # --- 3D LOGIC ---
    if dimensions == 3:
        try:
            import pyvista as pv
            import vtk
        except ImportError as e:
            print(f"Failed to import PyVista or VTK: {e}")
            return {}

        # we first identify the region containing the most refined patches.
        # This logic finds the leaf patches to calculate a center point,
        # which is then used for slicing if the user doesn't provide a specific position.

        # Step 1: Calculate volume for each patch.
        for p in patches:
            p["volume"] = np.prod(p.get("size", [1] * dimensions))

        # Step 2: Find the minimum volume in the dataset.
        if patches:
            min_volume = min(p["volume"] for p in patches)
            # Identify all patches at the highest refinement level (smallest volume).
            finest_patches = [p for p in patches if np.isclose(p["volume"], min_volume)]

            # Step 3: Calculate the bounding box of ONLY these finest patches.
            min_coord = np.min([p["offset"] for p in finest_patches], axis=0)
            max_coord = np.max(
                [np.array(p["offset"]) + np.array(p["size"]) for p in finest_patches],
                axis=0,
            )
            refined_center = (min_coord + max_coord) / 2.0
        else:
            refined_center = np.array([0.0, 0.0, 0.0])  # Fallback

        if verbose:
            print(f"Building robust UnstructuredGrid for all 3D patches...")

        all_points, all_cells_conn, all_cell_data = [], [], []
        point_offset = 0

        for patch in patches:
            offset, size, patch_vals = (
                np.array(patch["offset"]),
                np.array(patch["size"]),
                np.array(patch.get("values", [])),
            )
            if patch_vals is None:
                continue

            dofs = dofs_per_axis
            patch_points = np.array([])
            patch_cell_values = []

            if plot_type == "cell-values":
                num_cells = dofs
                cell_size_vec = size / num_cells
                points_per_dim = num_cells + 1
                # Vectorized point generation
                grid_indices = np.mgrid[
                    0:points_per_dim, 0:points_per_dim, 0:points_per_dim
                ]
                patch_points = np.vstack(
                    [
                        grid_indices[0].ravel(),
                        grid_indices[1].ravel(),
                        grid_indices[2].ravel(),
                    ]
                ).T.astype(float)
                patch_points = patch_points * cell_size_vec + offset

                patch_cell_values = patch_vals[unknown_idx::num_unknowns].tolist()
                # Vectorized cell connectivity generation
                cell_indices = np.mgrid[0:num_cells, 0:num_cells, 0:num_cells]
                p0 = (
                    cell_indices[2].ravel() * points_per_dim**2
                    + cell_indices[1].ravel() * points_per_dim
                    + cell_indices[0].ravel()
                )

                # Offsets to find the other 7 corners of a standard VTK Hexahedron
                offsets = np.array(
                    [
                        0,
                        1,
                        1 + points_per_dim,
                        points_per_dim,
                        points_per_dim**2,
                        points_per_dim**2 + 1,
                        points_per_dim**2 + 1 + points_per_dim,
                        points_per_dim**2 + points_per_dim,
                    ]
                )

                patch_cell_indices = p0[:, np.newaxis] + offsets
                patch_cell_indices += point_offset

                prefix = np.full((patch_cell_indices.shape[0], 1), 8)
                patch_cells_with_prefix = np.hstack([prefix, patch_cell_indices])

                all_cells_conn.extend(patch_cells_with_prefix.ravel())
                all_points.extend(patch_points)
                all_cell_data.extend(patch_cell_values)
                point_offset += len(patch_points)

            elif plot_type == "vertex-values":
                # This part remains a loop as the geometry is irregular, but it's typically
                # used on smaller, refined regions, so it's not the main bottleneck.
                mapping_grid = solver_meta.get("mapping_grid")
                if mapping_grid is None:
                    continue
                patch_points = [offset + np.array(p) * size for p in mapping_grid]
                num_cells = dofs - 1
                dofs_sq = dofs**2
                patch_cells_list = []
                for k in range(num_cells):
                    for j in range(num_cells):
                        for i in range(num_cells):
                            v_indices = [
                                (k + dk) * dofs_sq + (j + dj) * dofs + (i + di)
                                for dk, dj, di in itertools.product([0, 1], repeat=3)
                            ]
                            vals = [
                                patch_vals[(v_idx * num_unknowns) + unknown_idx]
                                for v_idx in v_indices
                            ]
                            patch_cell_values.append(sum(vals) / 8.0)

                            # Build cell connectivity for unstructured grid
                            p0 = (k * dofs**2) + (j * dofs) + i
                            offsets = np.array(
                                [
                                    0,
                                    1,
                                    1 + dofs,
                                    dofs,
                                    dofs_sq,
                                    dofs_sq + 1,
                                    dofs_sq + 1 + dofs,
                                    dofs_sq + dofs,
                                ]
                            )
                            cell_indices = p0 + offsets
                            patch_cells_list.append(
                                [8] + (cell_indices + point_offset).tolist()
                            )

                all_cells_conn.extend(np.array(patch_cells_list).ravel())
                all_points.extend(patch_points)
                all_cell_data.extend(patch_cell_values)
                point_offset += len(patch_points)

        if not all_points:
            print("No valid points found.")
            return {}

        cell_types = np.full(len(all_cell_data), vtk.VTK_HEXAHEDRON, dtype=np.uint8)
        grid = pv.UnstructuredGrid(all_cells_conn, cell_types, np.array(all_points))
        grid.cell_data[unknown_to_visualise] = np.array(all_cell_data)

        if slice_axis:
            # This block now checks if the user provided a slice position. If not,it uses the automatically calculated center of the refined region.
            if slice_position is not None:
                center = slice_position
                if verbose:
                    print(f"Clipping 3D grid at user-defined position {center}...")
            else:
                axis_map_center = {"x": 0, "y": 1, "z": 2}
                center = refined_center[axis_map_center[slice_axis]]
                if verbose:
                    print(
                        f"Clipping 3D grid at automatically determined center of refined region: {center:.4f}..."
                    )

            # Get the original bounds of the full grid
            original_bounds = list(grid.bounds)

            # Create a new bounds list that preserves all dimensions
            bounds = original_bounds.copy()

            axis_map_bounds = {"x": 0, "y": 2, "z": 4}
            axis_index = axis_map_bounds[slice_axis]
            width = slice_width

            # Only modify the bounds for the slice axis, preserve other dimensions fully
            bounds[axis_index] = center - (width / 2.0)
            bounds[axis_index + 1] = center + (width / 2.0)

            # Ensure the bounds don't exceed the original grid bounds
            bounds[axis_index] = max(bounds[axis_index], original_bounds[axis_index])
            bounds[axis_index + 1] = min(
                bounds[axis_index + 1], original_bounds[axis_index + 1]
            )

            final_grid = grid.clip_box(bounds, invert=False)
        else:
            # No slicing; use the full grid as is.
            final_grid = grid

        return {
            "type": plot_type,
            "pyvista_grid": final_grid,
            "unknown_name": unknown_to_visualise,
            "combine_unknowns": combine_unknowns,
            "slice_axis": slice_axis,
            "solver_meta": solver_meta,
            "unknown_idx": unknown_idx,
        }

    # --- 2D LOGIC ---
    elif dimensions == 2:
        patch_boundaries = []

        for patch in patches:
            offset, size = np.array(patch["offset"]), np.array(patch["size"])

            # --- Calculate and store patch boundaries ---
            ox, oy = offset[0], offset[1]
            sx, sy = size[0], size[1]
            # Create a closed loop of the 4 corners for plotting
            boundary_verts = [
                (ox, oy),
                (ox + sx, oy),
                (ox + sx, oy + sy),
                (ox, oy + sy),
                (ox, oy),
            ]
            patch_boundaries.append(boundary_verts)
        if plot_type == "cell-values":
            if verbose:
                print("Preparing data for 2D cell-values plot...")

                # For the fast imshow path, just pass the raw data through.
                # The plotter will handle the rest.
            return {
                "type": plot_type,
                "patches": patches,
                "solver_meta": solver_meta,
                "unknown_name": unknown_to_visualise,
                "unknown_idx": unknown_idx,
                "patch_boundaries": patch_boundaries,
                "combine_unknowns": combine_unknowns,
            }

        elif plot_type == "vertex-values":
            # For vertex-values, we prepare the vertices as before.
            all_cell_vertices = []
            all_cell_values = []
            patches_processed = 0
            for patch in patches:
                offset, size, patch_vals = (
                    np.array(patch["offset"]),
                    np.array(patch["size"]),
                    np.array(patch.get("values", [])),
                )
                if patch_vals.size == 0:
                    continue
                # DEBUG: Check if we're processing patches
                patches_processed += 1
                if patches_processed <= 3:  # Only log first few patches
                    if verbose:
                        print(
                            f"DEBUG: Processing patch {patches_processed} at offset {offset} with {len(patch_vals)} values"
                        )
                        print(
                            f"DEBUG: First few values: {patch_vals[:12] if len(patch_vals) >= 12 else patch_vals}"
                        )

                mapping_grid = solver_meta.get("mapping_grid")
                if mapping_grid is None:
                    if patches_processed <= 3:
                        print(
                            f"DEBUG: Skipping patch {patches_processed} - no mapping_grid"
                        )
                    continue

                num_cells_per_dim = dofs_per_axis - 1

                for r in range(num_cells_per_dim):
                    for c in range(num_cells_per_dim):
                        v_indices = [
                            r * dofs_per_axis + c,
                            r * dofs_per_axis + (c + 1),
                            (r + 1) * dofs_per_axis + (c + 1),
                            (r + 1) * dofs_per_axis + c,
                        ]
                        local_coords = [mapping_grid[idx] for idx in v_indices]
                        all_cell_vertices.append(
                            [
                                offset[:2] + np.array(lc) * size[:2]
                                for lc in local_coords
                            ]
                        )
                        vals = [
                            patch_vals[(v_idx * num_unknowns) + unknown_idx]
                            for v_idx in v_indices
                        ]
                        all_cell_values.append(sum(vals) / 4.0)
            # DEBUG: Summary after processing all patches
            if verbose:
                print(f"DEBUG: Processed {patches_processed} patches total")
                print(f"DEBUG: Generated {len(all_cell_values)} cells")
                if all_cell_values:
                    print(
                        f"DEBUG: Value range: {min(all_cell_values):.6f} to {max(all_cell_values):.6f}"
                    )

            if not all_cell_values:
                return {}
            values_np = np.array(all_cell_values)
            data_min, data_max = np.min(values_np), np.max(values_np)
            norm = mcolors.Normalize(
                vmin=data_min, vmax=data_max if data_min != data_max else data_min + 1
            )
            colormap = cm.get_cmap("viridis")
            cell_colors = colormap(norm(values_np))

            return {
                "type": plot_type,
                "vertices": all_cell_vertices,
                "cell_colors": cell_colors,
                "patch_boundaries": patch_boundaries,
                "data_range": (data_min, data_max),
                "unknown_name": unknown_to_visualise,
                "combine_unknowns": combine_unknowns,
                "solver_meta": solver_meta,
                "unknown_idx": unknown_idx,
            }

    return {}
