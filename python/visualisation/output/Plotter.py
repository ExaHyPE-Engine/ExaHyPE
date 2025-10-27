# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import matplotlib as mpl
from matplotlib.collections import PatchCollection
from matplotlib.patches import Polygon
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import numpy as np
import os


def plot_data(
    processed_data,
    dimensions,
    output_filepath,
    title_suffix="Data Visualisation",
    dpi=300,
    fig_size_inches=(10, 8),
    output_format="png",
    verbose=False,
    flip_view=None,
):
    """
    Plots cell geometries. Handles 2D data with Matplotlib and 3D data with PyVista.
    """
    combine_unknowns = processed_data.get("combine_unknowns")

    raw_unknown_name = processed_data.get("unknown_name", "Value")

    # Generate the title for the color bar based on whether we are plotting
    # a sum of unknowns or a single component.
    if raw_unknown_name == "Combined_Value" and combine_unknowns:
        indices_str = ", ".join(map(str, combine_unknowns))
        scalar_bar_title = f"Sum of unknowns ({indices_str})"
    elif "Component" in raw_unknown_name:
        # Assumes name is "Component_X"
        parts = raw_unknown_name.split("_", 1)
        scalar_bar_title = f"${parts[0]}_{{{parts[1]}}}$"
    else:
        # Fallback for any other case
        scalar_bar_title = raw_unknown_name

    label_text = f"Value of {scalar_bar_title}"
    # --- 3D PLOTTING PATH WITH PYVISTA ---
    if dimensions == 3:
        try:
            import pyvista as pv
            import vtk  # Needed for cell type constants
        except ImportError as e:
            print(f"ERROR: Failed to import PyVista or VTK in plotter: {e}")
            return
        # renderer = vtk.OSPRAY
        grid = processed_data.get("pyvista_grid")
        if grid is None or grid.n_points == 0:
            print(
                f"WARNING: Plotter: No 3D geometry data provided for '{title_suffix}'. Skipping plot generation."
            )
            return

        # This block decides which rendering engine to use for 3D plots.
        # For raster formats (PNG, JPG), it uses PyVista's fast screenshotter.
        # For vector formats (PDF, SVG), it uses the new hybrid Matplotlib method.
        slice_axis = processed_data.get("slice_axis")  # Get from processed_data
        # --- PATH 1: RASTER OUTPUT (PNG, JPG) VIA PYVISTA ---
        if output_format in ["png", "jpg", "jpeg", "bmp", "tif", "tiff"]:
            if verbose:
                print(
                    f"INFO: Plotter: Generating 3D plot for '{title_suffix}' with PyVista..."
                )
            plotter = pv.Plotter(off_screen=True, window_size=[1920, 1080])
            data = grid.get_array(raw_unknown_name)
            vmin, vmax = np.min(data), np.max(data)
            if abs(vmax - vmin) < 1e-6:
                vmax = vmin + 1e-3
            plotter.add_mesh(
                grid,
                scalars=raw_unknown_name,
                cmap="viridis",
                clim=[vmin, vmax],
                show_scalar_bar=False,
                show_edges=True,
                edge_color="gray",
                edge_opacity=0.4,
                line_width=0.5,
            )
            # Add a single, explicitly defined scalar bar on the left side
            plotter.add_scalar_bar(
                title=scalar_bar_title,
                n_labels=5,
                fmt="%g",
                title_font_size=20,
                label_font_size=20,
                vertical=True,
                height=0.8,
                position_x=0.85,
            )

            # Add the main title as a text actor
            plotter.add_title(title_suffix, font_size=40)
            # sets the camera's position to show a standard isometric view of the 3D object
            plotter.view_isometric()
            # changes the camera's projection from the default "perspective" to "parallel."
            plotter.enable_parallel_projection()

            # Default: show all labels
            show_x_labels = True
            show_y_labels = True
            show_z_labels = True

            # Hide labels for the axis being sliced (since it's thin)
            if slice_axis == "x":
                show_x_labels = False
            elif slice_axis == "y":
                show_y_labels = False
            elif slice_axis == "z":
                show_z_labels = False
            plotter.show_bounds(
                grid="back",
                location="all",
                all_edges=True,
                show_xaxis=True,
                show_yaxis=True,
                show_zaxis=True,
                show_xlabels=show_x_labels,
                show_ylabels=show_y_labels,
                show_zlabels=show_z_labels,
                n_xlabels=5,
                n_ylabels=5,
                n_zlabels=5,
                xtitle="X Axis",
                ytitle="Y Axis",
                ztitle="Z Axis",
                font_size=20,
                padding=0.01
                # use_3d_text=True
            )

            # Apply camera flip by inverting position and up-vector along specified axis
            if flip_view in {"x", "y", "z"}:
                axis_index = {"x": 0, "y": 1, "z": 2}[flip_view]
                camera = plotter.camera
                focal = list(camera.focal_point)
                offset = [camera.position[i] - focal[i] for i in range(3)]

                # Invert camera offset and up-vector component along flip axis
                offset[axis_index] *= -1.0
                camera.position = [focal[i] + offset[i] for i in range(3)]

                up = list(camera.up)
                up[axis_index] *= -1.0
                camera.up = up

            if verbose:
                print(
                    f"INFO: Plotter: Attempting to save PyVista figure to {output_filepath}..."
                )
            try:
                plotter.screenshot(output_filepath, scale=dpi / 100)
                print(
                    f"INFO: -------- Plotter: Successfully saved PyVista plot to {output_filepath} ---------"
                )
            except Exception as e:
                print(
                    f"ERROR: Plotter: Error saving PyVista figure {output_filepath}: {e}"
                )
            finally:
                plotter.close()
            return
        # PATH 2: Vector graphics via MATPLOTLIB
        elif output_format in ["pdf", "svg", "pgf"]:
            import matplotlib.pyplot as plt

            from mpl_toolkits.mplot3d.art3d import Poly3DCollection

            if verbose:
                print(
                    f"INFO: Plotter: Generating 3D vector plot for '{title_suffix}' with Matplotlib..."
                )
            # the clipped grid contains 3D cells (like VTK_HEXAHEDRON, type 12), not 2D polygons. To render this in Matplotlib,
            # we must first extract its external surface. This converts the faces of the 3D cells into 2D polygons (triangles and quads) that can be plotted.

            if not isinstance(grid, pv.PolyData):
                surface_grid = grid.extract_surface()
            else:
                surface_grid = grid

            if surface_grid.n_cells == 0:
                print(
                    f"WARNING: Plotter: The 3D grid for '{title_suffix}' contains no surface cells after slicing. Skipping plot generation."
                )
                return

            # Increase figure size slightly for vector formats to accommodate labels
            vector_fig_size = (fig_size_inches[0] * 1.15, fig_size_inches[1] * 1.15)
            fig = plt.figure(figsize=vector_fig_size)
            ax = fig.add_subplot(111, projection="3d")
            ax.set_title(title_suffix, fontsize=16, pad=20)
            ax.set_xlabel("X Axis", fontsize=11, labelpad=15)
            ax.set_ylabel("Y Axis", fontsize=11, labelpad=15)
            ax.set_zlabel("Z Axis", fontsize=11, labelpad=15)

            # Enable orthographic projection to match PyVista's parallel projection
            ax.set_proj_type("ortho")

            # Extract geometry and data from the PyVista grid in other words
            # it translates the geometry from PyVista's format to Matplotlib's format
            points = surface_grid.points
            values = surface_grid.get_array(raw_unknown_name)
            faces = surface_grid.faces
            polygons_indices = []
            i = 0
            while i < faces.size:
                # The first number is the count of points in this face
                num_points_in_face = faces[i]
                # The next 'num_points_in_face' numbers are the indices
                face_indices = faces[i + 1 : i + 1 + num_points_in_face]
                polygons_indices.append(face_indices)
                # Move the pointer to the start of the next face
                i += num_points_in_face + 1

            # Use a list comprehension to build the final list of polygons for Matplotlib.
            polygons = [points[p] for p in polygons_indices]

            # Apply geometric flip transformation if requested
            # Mirrors all polygon vertices along the specified axis to achieve camera flip effect
            if flip_view in {"x", "y", "z"}:
                axis_map = {"x": 0, "y": 1, "z": 2}
                flip_axis = axis_map[flip_view]
                flipped_polygons = []
                for poly in polygons:
                    poly_array = np.array(poly, copy=True)
                    poly_array[:, flip_axis] *= -1  # Mirror coordinates along flip axis
                    flipped_polygons.append(poly_array)
                polygons = flipped_polygons

            if len(values) != len(polygons):
                print(
                    f"WARNING: Data array length ({len(values)}) does not match polygon count ({len(polygons)}). Colors may be incorrect."
                )
                # We can still try to plot by resizing the color array
                colors_array = np.resize(values, len(polygons))
            else:
                colors_array = values
            # Create a Matplotlib color map
            data_min, data_max = np.min(values), np.max(values)
            if abs(data_max - data_min) < 1e-6:
                data_max = data_min + 1e-3  # or another small value
            norm = plt.Normalize(vmin=data_min, vmax=data_max)
            cmap = plt.get_cmap("viridis")
            colors = cmap(norm(colors_array))

            # Create a 3D polygon collection for Matplotlib
            poly_collection = Poly3DCollection(
                polygons,
                facecolors=colors,
                edgecolor="none",
                linewidth=0,
            )
            ax.add_collection3d(poly_collection)

            # Set plot limits

            # Set axis limits with explicit tick locations for scientific precision
            xmin, xmax, ymin, ymax, zmin, zmax = surface_grid.bounds

            # Transform axis limits to match the flipped coordinate space
            if flip_view == "x":
                xmin, xmax = -xmax, -xmin
            elif flip_view == "y":
                ymin, ymax = -ymax, -ymin
            elif flip_view == "z":
                zmin, zmax = -zmax, -zmin

            ax.set_xlim(xmin, xmax)
            ax.set_ylim(ymin, ymax)
            ax.set_zlim(zmin, zmax)

            # Reduce to 4 ticks per axis to prevent crowding in corners
            ax.set_xticks(np.linspace(xmin, xmax, 5))
            ax.set_yticks(np.linspace(ymin, ymax, 5))
            ax.set_zticks(np.linspace(zmin, zmax, 5))
            # Adjust tick label size and padding to prevent overlapping
            ax.tick_params(axis="x", which="major", labelsize=9, pad=4)
            ax.tick_params(axis="y", which="major", labelsize=9, pad=4)
            ax.tick_params(axis="z", which="major", labelsize=9, pad=4)

            # Format tick labels to use fewer decimal places to reduce width
            ax.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, p: f"{x:.2f}"))
            ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda x, p: f"{x:.2f}"))
            ax.zaxis.set_major_formatter(plt.FuncFormatter(lambda x, p: f"{x:.2f}"))

            # Increase axis label padding for the sliced axis
            if slice_axis == "x":
                ax.set_xticklabels([])
            elif slice_axis == "y":
                ax.set_yticklabels([])
            elif slice_axis == "z":
                ax.set_zticklabels([])

            # Add grid lines to match PyVista's bounds display
            ax.grid(True, linestyle="--", linewidth=0.5, alpha=0.7)

            # Make the panes transparent to match PyVista's clean look
            ax.xaxis.pane.fill = False
            ax.yaxis.pane.fill = False
            ax.zaxis.pane.fill = False
            ax.xaxis.pane.set_edgecolor("gray")
            ax.yaxis.pane.set_edgecolor("gray")
            ax.zaxis.pane.set_edgecolor("gray")
            ax.xaxis.pane.set_alpha(0.3)
            ax.yaxis.pane.set_alpha(0.3)
            ax.zaxis.pane.set_alpha(0.3)
            ax.set_box_aspect([xmax - xmin, ymax - ymin, zmax - zmin])
            # Add color bar
            sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
            sm.set_array([])
            cbar = fig.colorbar(sm, ax=ax, shrink=0.8, pad=0.15, orientation="vertical")
            cbar.set_label(scalar_bar_title, fontsize=14)
            cbar.ax.tick_params(labelsize=12)

            # Set isometric view angles (elev=35.264°, azim=45° matches PyVista default)
            # When geometry is flipped, adjust view angles to maintain equivalent perspective
            elev = 35.264
            azim = 45
            if flip_view == "x":
                azim = 225  # View from opposite X side
            elif flip_view == "y":
                azim = 315  # View from opposite Y side
            elif flip_view == "z":
                elev = -35.264  # View from below

            ax.view_init(elev=elev, azim=azim)

            try:
                plt.subplots_adjust(left=0.05, right=0.85, top=0.95, bottom=0.10)
                plt.savefig(output_filepath, dpi=dpi, bbox_inches="tight")
                print(
                    f"INFO: -------- Plotter: Successfully saved Matplotlib plot to {output_filepath} ---------"
                )
            except Exception as e:
                print(
                    f"ERROR: Plotter: Error saving Matplotlib figure {output_filepath}: {e}"
                )
            finally:
                plt.close(fig)
            return

    # --- 2D PLOTTING PATH WITH MATPLOTLIB ---
    elif dimensions == 2:
        if output_format == "pgf":
            if verbose:
                print("INFO: Generating PGF file for LaTeX inclusion...")
            mpl.use("pgf")
            mpl.rcParams.update(
                {
                    "font.family": "serif",
                    "text.usetex": True,
                    "pgf.rcfonts": False,
                    "pgf.texsystem": "pdflatex",
                    "pgf.preamble": "\n".join(
                        [
                            r"\usepackage[T1]{fontenc}",
                            r"\usepackage[utf8]{inputenc}",
                            r"\usepackage{amsmath}",
                            r"\usepackage{siunitx}",  # For proper scientific notation
                            r"\usepackage{xcolor}",  # For colors
                        ]
                    ),
                }
            )
        elif output_format in ["pdf", "svg"]:
            if verbose:
                print("INFO: Using PGF backend for high-quality vector PDF/SVG...")
            mpl.use("pgf")  # PGF can compile to PDF/SVG
            mpl.rcParams.update(
                {
                    "font.family": "serif",
                    "text.usetex": True,
                    "pgf.rcfonts": False,
                    "pgf.texsystem": "pdflatex",
                    "pgf.preamble": "\n".join(
                        [
                            r"\usepackage[T1]{fontenc}",
                            r"\usepackage[utf8]{inputenc}",
                            r"\usepackage{amsmath}",
                        ]
                    ),
                }
            )
        else:
            if verbose:
                print("INFO: Using Agg backend for fast raster output.")
            mpl.use("Agg")

        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(figsize=fig_size_inches)
        ax.set_aspect("equal", adjustable="box")
        ax.set_xlabel("x")
        ax.set_ylabel("y")
        ax.set_title(title_suffix)
        ax.grid(False)

        plot_type = processed_data.get("type")

        # --- Use the fastest rendering method based on the data type ---
        # We have two rendering paths. The `imshow` path is faster for cell-values data
        patch_boundaries = processed_data.get("patch_boundaries", [])
        if plot_type == "cell-values":
            # --- PATH 1: Ultra-fast imshow for cell-values data ---
            if verbose:
                print("INFO: Using fast imshow rendering for cell-values data...")
            patches_data = processed_data.get("patches", [])
            solver_meta = processed_data.get("solver_meta", {})
            num_unknowns = solver_meta.get("unknowns_count", 1)
            dofs = solver_meta.get("dofs_per_axis", 1)
            # The index of the unknown to plot is passed from the visualiser.
            # This is 0 for combined unknowns, or the user-specified index otherwise.
            unknown_idx = processed_data.get("unknown_idx", 0)
            if not patches_data:
                print(f"WARNING: Plotter: No 2D patches to plot for '{title_suffix}'.")
                plt.close(fig)
                return
            # Calculate a single color normalization range for all patches
            all_values = np.concatenate(
                [
                    p["values"][unknown_idx::num_unknowns]
                    for p in patches_data
                    if p.get("values") is not None
                ]
            )
            if all_values.size == 0:
                print(
                    f"WARNING: Plotter: No data values found to plot for '{title_suffix}'."
                )
                plt.close(fig)
                return
            data_min, data_max = np.min(all_values), np.max(all_values)
            norm = mpl.colors.Normalize(
                vmin=data_min, vmax=data_max if data_min != data_max else data_min + 1
            )

            # Loop through each patch and render it as a small, fast image
            for p in patches_data:
                offset, size, values = (
                    np.array(p["offset"]),
                    np.array(p["size"]),
                    p.get("values"),
                )
                if values is None or dofs <= 0:
                    continue
                # (convert to NumPy first)
                values_array = (
                    np.array(values) if not isinstance(values, np.ndarray) else values
                )
                grid_data = values_array[unknown_idx::num_unknowns].reshape(
                    (dofs, dofs)
                )
                extent = [
                    offset[0],
                    offset[0] + size[0],
                    offset[1],
                    offset[1] + size[1],
                ]
                ax.imshow(
                    grid_data,
                    extent=extent,
                    cmap="viridis",
                    origin="lower",
                    norm=norm,
                    interpolation="none",
                )
            # Tell the PGF backend to treat the contents of the axes (all the imshow calls)
            # as a single rasterized image at the save DPI.
            ax.set_rasterized(True)

            # Set plot limits and create color bar for the imshow plot
            all_x_coords = [p["offset"][0] for p in patches_data] + [
                p["offset"][0] + p["size"][0] for p in patches_data
            ]
            all_y_coords = [p["offset"][1] for p in patches_data] + [
                p["offset"][1] + p["size"][1] for p in patches_data
            ]
            ax.set_xlim(min(all_x_coords), max(all_x_coords))
            ax.set_ylim(min(all_y_coords), max(all_y_coords))
            sm = plt.cm.ScalarMappable(cmap="viridis", norm=norm)
            sm.set_array([])
            cbar = fig.colorbar(sm, ax=ax, shrink=0.8, pad=0.1)

        else:
            # --- PATH 2: Fallback to original PatchCollection method for other data types ---
            if verbose:
                print(f"INFO: Using PatchCollection rendering for '{plot_type}' data.")
            cell_vertices = processed_data.get("vertices", [])
            if not cell_vertices:
                print(
                    f"WARNING: Plotter: No 2D cell vertices to plot for '{title_suffix}'. Skipping plot generation."
                )
                return

            cell_colors = processed_data.get("cell_colors", [])
            data_range = processed_data.get("data_range", (0, 1))

            patches = [Polygon(verts, closed=True) for verts in cell_vertices]
            p = PatchCollection(patches, alpha=1.0)
            # necessary to avoid a memory overflow
            if output_format in ["pdf", "svg"]:
                p.set_rasterized(True)
            else:
                p.set_rasterized(False)

            p.set_color(cell_colors)
            ax.add_collection(p)

            for boundary_verts in patch_boundaries:
                if not boundary_verts:
                    continue
                xs = [v[0] for v in boundary_verts]
                ys = [v[1] for v in boundary_verts]
                ax.plot(xs, ys, linewidth=0.5, color="white", linestyle="--")

            all_x_coords = [v[0] for cell in cell_vertices for v in cell] + [
                v[0] for boundary in patch_boundaries for v in boundary
            ]
            all_y_coords = [v[1] for cell in cell_vertices for v in cell] + [
                v[1] for boundary in patch_boundaries for v in boundary
            ]
            if all_x_coords and all_y_coords:
                min_x, max_x = np.min(all_x_coords), np.max(all_x_coords)
                min_y, max_y = np.min(all_y_coords), np.max(all_y_coords)
                ax.set_xlim(min_x, max_x)
                ax.set_ylim(min_y, max_y)

            norm = plt.Normalize(vmin=data_range[0], vmax=data_range[1])
            sm = plt.cm.ScalarMappable(cmap="viridis", norm=norm)
            sm.set_array([])
            cbar = fig.colorbar(sm, ax=ax, shrink=0.8, pad=0.1)
        for boundary_verts in patch_boundaries:
            if not boundary_verts:
                continue
            xs = [v[0] for v in boundary_verts]
            ys = [v[1] for v in boundary_verts]
            # Using a thin, gray, solid line for the grid
            ax.plot(xs, ys, linewidth=0.1, color="white", linestyle="-")
        cbar.set_label(label_text)

        output_dir = os.path.dirname(output_filepath)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)

        try:
            plt.savefig(output_filepath, dpi=dpi, bbox_inches="tight")
            print(
                f"INFO: -------- Plotter: Successfully saved plot to {output_filepath} ---------"
            )
        except Exception as e:
            print(f"ERROR: Plotter: Error saving figure {output_filepath}: {e}")
        finally:
            plt.close(fig)
