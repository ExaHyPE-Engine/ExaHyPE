# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import matplotlib as mpl

# This Matplotlib setup remains global as it configures the backend
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
):
    """
    Plots cell geometries. Handles 2D data with Matplotlib and 3D data with PyVista.
    """
    # Value here acts as a placeholder for plotting and labeling in case unknown_name is not found.
    raw_unknown_name = processed_data.get("unknown_name", "Value")

    # --- 3D PLOTTING PATH WITH PYVISTA ---
    if dimensions == 3:
        import pyvista as pv
        import vtk  # Needed for cell type constants

        grid = processed_data.get("pyvista_grid")
        if grid is None or grid.n_points == 0:
            print(
                f"WARNING: Plotter: No 3D geometry data provided for '{title_suffix}'. Skipping plot generation."
            )
            return
        # Format the label for the scalar bar using LaTeX if needed
        if "_" in raw_unknown_name:
            parts = raw_unknown_name.split("_", 1)
            scalar_bar_title = f"${parts[0]}_{{{parts[1]}}}$"
        else:
            scalar_bar_title = f"${raw_unknown_name}$"

        # This block decides which rendering engine to use for 3D plots.
        # For raster formats (PNG, JPG), it uses PyVista's fast screenshotter.
        # For vector formats (PDF, SVG), it uses the new hybrid Matplotlib method.

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
                line_width=0.5,
            )
            # Add a single, explicitly defined scalar bar
            plotter.add_scalar_bar(
                title=scalar_bar_title,
                n_labels=5,
                fmt="%.3f",
                title_font_size=23,
                label_font_size=18,
            )

            # Add the main title as a text actor
            plotter.add_title(title_suffix, font_size=25)
            # sets the camera's position to show a standard isometric view of the 3D object
            plotter.view_isometric()
            # changes the camera's projection from the default "perspective" to "parallel."
            plotter.enable_parallel_projection()
            plotter.show_axes()
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
        # add pgf
        elif output_format in ["pdf", "svg", "pgf"]:
            import matplotlib.pyplot as plt

            from mpl_toolkits.mplot3d.art3d import Poly3DCollection

            if verbose:
                print(
                    f"INFO: Plotter: Generating 3D vector plot for '{title_suffix}' with Matplotlib..."
                )
            # the clipped grid contains 3D cells (like VTK_HEXAHEDRON, type 12), not 2D polygons. To render this in Matplotlib,
            # we must first extract its external surface. This converts the faces of the 3D cells into 2D polygons (triangles and quads) that can be plotted.
            # This method "peels" the outer surface off the 3D model so Matplotlib can draw it.
            surface_grid = grid.extract_surface()

            if surface_grid.n_cells == 0:
                print(
                    f"WARNING: Plotter: The 3D grid for '{title_suffix}' contains no surface cells after slicing. Skipping plot generation."
                )
                return

            fig = plt.figure(figsize=fig_size_inches)
            ax = fig.add_subplot(111, projection="3d")
            ax.set_title(title_suffix)
            ax.set_xlabel("X")
            ax.set_ylabel("Y")
            ax.set_zlabel("Z")

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
                rasterized=False,  # Disable rasterization for better quality
            )
            ax.add_collection3d(poly_collection)

            # Set plot limits

            # Matplotlib's 3D axes were not respecting the exact slice width.
            # This new logic takes complete control by not only
            # setting the axis limits but also explicitly defining the tick locations.
            # This forces the plot to be scientifically precise.
            xmin, xmax, ymin, ymax, zmin, zmax = surface_grid.bounds
            ax.set_xlim(xmin, xmax)
            ax.set_ylim(ymin, ymax)
            ax.set_zlim(zmin, zmax)

            # Explicitly set 5 tick marks for each axis to enforce the limits
            ax.set_xticks(np.linspace(xmin, xmax, 5))
            ax.set_yticks(np.linspace(ymin, ymax, 5))
            ax.set_zticks(np.linspace(zmin, zmax, 5))

            # Add color bar
            sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
            sm.set_array([])
            cbar = fig.colorbar(
                sm, ax=ax, shrink=0.8, pad=0.1, orientation="horizontal"
            )
            cbar.set_label(scalar_bar_title, fontsize=14)

            try:
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
        # We have two rendering paths. The `imshow` path is
        # orders of magnitude faster for `cell-values` data, which was the source
        # of the major performance bottleneck identified by the profiler.
        patch_boundaries = processed_data.get("patch_boundaries", [])
        if plot_type == "cell-values":
            # --- PATH 1: Ultra-fast imshow for cell-values data ---
            if verbose:
                print("INFO: Using fast imshow rendering for cell-values data...")
            patches_data = processed_data.get("patches", [])
            solver_meta = processed_data.get("solver_meta", {})
            num_unknowns = solver_meta.get("unknowns_count", 1)
            dofs = solver_meta.get("dofs_per_axis", 1)
            if processed_data.get(
                "is_combined", False
            ):  # failsafe for the --combine-unknowns filter
                unknown_idx = 0
            else:
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
        if "_" in raw_unknown_name:
            parts = raw_unknown_name.split("_", 1)
            label_text = f"Value of ${parts[0]}_{{{parts[1]}}}$"
        else:
            label_text = f"Value of ${raw_unknown_name}$"
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
