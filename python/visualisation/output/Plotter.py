# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import matplotlib as mpl
from matplotlib.collections import PatchCollection
from matplotlib.patches import Polygon
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import numpy as np
import os
import time

# PlotOptiX singleton instance (reused across all renders)
_plotoptix_instance = None


def plot_with_plotoptix(grid, scalar_name, output_filepath=None):
    """
    Render data using PlotOptiX GPU-accelerated raytracing.
    Alternative to PyVista rasterisation rendering.

    Args:
        grid: PyVista grid object (already validated by caller)
        scalar_name: Name of the scalar field to visualise
        output_filepath: Where to save the rendered image (optional)

    Returns:
        True if successful, None if PlotOptiX unavailable or error occurred
    """

    try:
        print("INFO: Using PlotOptiX GPU-accelerated raytracing...")
        from plotoptix import NpOptiX
        import numpy as np
        import pyvista as pv  # For type checking PolyData

        # Extract surface geometry (PlotOptiX works with meshes)
        if not isinstance(grid, pv.PolyData):
            surface_grid = grid.extract_surface()
        else:
            surface_grid = grid

        if surface_grid.n_cells == 0:
            print("WARNING: No surface cells available for PlotOptiX")
            return None

        # Extract vertices, faces, and scalar values
        # Ensure vertex positions are float32 and C-contiguous for native bindings
        vertices = np.ascontiguousarray(surface_grid.points.astype(np.float32))

        # Get scalar values - ensure we have point data (per-vertex)
        # If data is per-cell, convert to per-point for color mapping
        if scalar_name in surface_grid.point_data:
            scalars = surface_grid.point_data[scalar_name]
        elif scalar_name in surface_grid.cell_data:
            # Convert cell data to point data by averaging neighboring cells
            surface_grid = surface_grid.cell_data_to_point_data()
            scalars = surface_grid.point_data[scalar_name]
        else:
            print(f"ERROR: Scalar field '{scalar_name}' not found in grid")
            return None

        # Convert PyVista faces format to simple triangle/quad indices
        faces_raw = surface_grid.faces
        faces_list = []
        i = 0
        while i < len(faces_raw):
            n_points = faces_raw[i]
            face_indices = faces_raw[i + 1 : i + 1 + n_points].tolist()

            # PlotOptiX needs triangles - triangulate quads if necessary
            if n_points == 3:
                faces_list.append(face_indices)
            elif n_points == 4:
                # Split quad into 2 triangles
                faces_list.append([face_indices[0], face_indices[1], face_indices[2]])
                faces_list.append([face_indices[0], face_indices[2], face_indices[3]])
            else:
                # Unsupported polygon: log warning and skip
                print(
                    f"WARNING: Skipping face with {n_points} vertices (only triangles and quads supported)"
                )
            i += n_points + 1

        # Faces as uint32 and C-contiguous (n_faces, 3)
        faces = np.ascontiguousarray(np.array(faces_list, dtype=np.uint32))

        # STEP 1: Initialise PlotOptiX singleton (once per process)
        global _plotoptix_instance

        if _plotoptix_instance is None:
            from plotoptix.materials import m_diffuse

            # Create raytracer instance - start_now=False lets us configure before rendering
            _plotoptix_instance = NpOptiX(width=1920, height=1080, start_now=False)

            # Set high ambient light to make vertex colors bright and visible
            _plotoptix_instance.set_ambient([0.7, 0.7, 0.7])

            # Setup diffuse material - respects per-vertex colors by default
            _plotoptix_instance.setup_material("data_material", m_diffuse)

        plot = _plotoptix_instance

        # STEP 2: Compute per-vertex colors from scalar data
        try:
            from matplotlib import cm

            # Normalize scalars to [0, 1] range
            vmin, vmax = np.min(scalars), np.max(scalars)
            if abs(vmax - vmin) < 1e-6:
                vmax = vmin + 1e-3

            n_vertices = vertices.shape[0]
            n_faces = len(faces_list)

            # Map scalars to per-vertex colors using viridis colormap
            if len(scalars) == n_vertices:
                # Per-vertex scalars: direct mapping
                values = np.asarray(scalars, dtype=float)
                normalized = (values - vmin) / (vmax - vmin)
                cmap = cm.get_cmap("viridis")
                vertex_colors = cmap(normalized)[:, :3]  # RGB only (drop alpha)

            elif len(scalars) == n_faces:
                # Per-face scalars: distribute to vertices by averaging
                face_values = np.asarray(scalars, dtype=float)
                vertex_accum = np.zeros(n_vertices, dtype=float)
                vertex_counts = np.zeros(n_vertices, dtype=int)

                for fi, face in enumerate(faces_list):
                    for vidx in face:
                        vertex_accum[vidx] += face_values[fi]
                        vertex_counts[vidx] += 1

                vertex_counts[vertex_counts == 0] = 1  # Avoid division by zero
                vertex_values = vertex_accum / vertex_counts
                normalized = (vertex_values - vmin) / (vmax - vmin)
                cmap = cm.get_cmap("viridis")
                vertex_colors = cmap(normalized)[:, :3]

            else:
                # Unexpected scalar count: use mean color for all vertices
                print(
                    f"WARNING: Scalar count {len(scalars)} doesn't match vertices ({n_vertices}) or faces ({n_faces})"
                )
                mean_val = np.mean(np.asarray(scalars, dtype=float))
                normalized = np.clip((mean_val - vmin) / (vmax - vmin), 0.0, 1.0)
                cmap = cm.get_cmap("viridis")
                color = cmap(normalized)[:3]
                vertex_colors = np.tile(color, (n_vertices, 1))

            # Ensure float32, C-contiguous for GPU upload
            vertex_colors = np.ascontiguousarray(vertex_colors, dtype=np.float32)

            # Validate array shapes before GPU upload
            if vertices.ndim != 2 or vertices.shape[1] != 3:
                raise ValueError(
                    f"Invalid vertices shape: {vertices.shape}, expected (n, 3)"
                )
            if faces.ndim != 2 or faces.shape[1] != 3:
                raise ValueError(f"Invalid faces shape: {faces.shape}, expected (m, 3)")
            if vertex_colors.shape != (n_vertices, 3):
                raise ValueError(
                    f"Color shape {vertex_colors.shape} doesn't match vertices ({n_vertices}, 3)"
                )
            if faces.max() >= n_vertices or faces.min() < 0:
                raise ValueError(
                    f"Face indices out of bounds: [{faces.min()}, {faces.max()}] for {n_vertices} vertices"
                )

            # Upload mesh to GPU with per-vertex colors
            plot.set_mesh(
                "data_mesh", vertices, faces, c=vertex_colors, mat="data_material"
            )

        except Exception as e:
            print(
                f"WARNING: PlotOptiX mesh setup failed ({e}), falling back to PyVista"
            )
            return None

        # STEP 3: Setup camera and lighting (first render only)
        try:
            # Compute scene bounds and center
            bounds = surface_grid.bounds
            center = np.array(
                [
                    (bounds[0] + bounds[1]) / 2,
                    (bounds[2] + bounds[3]) / 2,
                    (bounds[4] + bounds[5]) / 2,
                ]
            )
            diagonal = np.sqrt(
                (bounds[1] - bounds[0]) ** 2
                + (bounds[3] - bounds[2]) ** 2
                + (bounds[5] - bounds[4]) ** 2
            )

            # Isometric camera position (diagonal view)
            camera_distance = diagonal * 1.6  # Match PyVista's default zoom level
            cam_eye = center + camera_distance * np.array([1, 1, 1]) / np.sqrt(3)
            cam_up = [0, 0, 1]  # Z-up convention

            is_first_render = not plot.is_started()

            if is_first_render:
                # Three-point lighting scaled to scene size
                light_distance = diagonal * 2.0
                plot.setup_light(
                    "key_light",
                    pos=[
                        center[0] + light_distance,
                        center[1] + light_distance,
                        center[2] + light_distance,
                    ],
                    color=[8.0, 8.0, 8.0],
                )
                plot.setup_light(
                    "fill_light",
                    pos=[
                        center[0] - light_distance,
                        center[1] + light_distance / 2,
                        center[2] + light_distance,
                    ],
                    color=[3.0, 3.0, 3.0],
                )
                plot.setup_light(
                    "back_light",
                    pos=[
                        center[0],
                        center[1] - light_distance,
                        center[2] - light_distance,
                    ],
                    color=[2.0, 2.0, 2.0],
                )

                # Setup camera with 30° FOV (matches PyVista default)
                plot.setup_camera(
                    "cam1",
                    eye=cam_eye,
                    target=center,
                    up=cam_up,
                    fov=30,
                    make_current=True,
                )
            else:
                # Update camera position for new data bounds
                plot.update_camera("cam1", eye=cam_eye, target=center, up=cam_up)

        except Exception as e:
            print(
                f"WARNING: PlotOptiX camera/lighting setup failed ({e}), falling back to PyVista"
            )
            return None

        # STEP 4: Start raytracer and render (first time only)
        if is_first_render:
            plot.set_background(1)  # White background (1.0, 1.0, 1.0)
            plot.set_param(min_accumulation_step=4, max_accumulation_frames=16)
            plot.start()  # Compile shaders and start raytracing thread

        # Brief pause to ensure frame completion
        time.sleep(0.3)

        # STEP 5: Save rendered image
        if output_filepath:
            plot.save_image(output_filepath)
            print(f"INFO: PlotOptiX saved raytraced image to {output_filepath}")

        # Return data needed for annotation overlay
        return {"success": True, "vmin": vmin, "vmax": vmax, "scalar_name": scalar_name}

    except ImportError as e:
        print(
            f"INFO: PlotOptiX not available ({e}), will use standard PyVista rendering"
        )
        return None
    except Exception as e:
        print(f"WARNING: PlotOptiX rendering failed ({e}), falling back to PyVista")
        return None


def plot_with_volume_rendering(
    grid, scalar_name, output_filepath, title="Volume Rendering", scalar_bar_title=None
):
    """
    GPU-accelerated volume rendering using PyVista.

    This performs volume rendering (alpha-blended ray marching) which composites
    semi-transparent samples through the volume to reveal interior structures.

    Note: The opacity transfer function emphasizes high-value features. This makes
    volume rendering effective for visualising dynamic phenomena (e.g., propagating
    waves, shock fronts) but means that timesteps with uniform low-amplitude data
    will appear mostly transparent. For initial conditions or low-amplitude states,
    consider using slice-based visualisation instead.

    Args:
        grid: PyVista UnstructuredGrid with cell data
        scalar_name: Name of the scalar field to render
        output_filepath: Where to save the rendered PNG
        title: Title to display on the plot
        scalar_bar_title: Title for the colorbar (if None, uses scalar_name)

    Returns:
        dict with 'success', 'vmin', 'vmax', 'scalar_name' (matches PlotOptiX interface)
    """
    try:
        import pyvista as pv
        import numpy as np

        print(f"INFO: Starting volume rendering for '{scalar_name}'...")
        plotter = pv.Plotter(off_screen=True, window_size=[1920, 1080])

        # Get scalar range
        data = grid.cell_data[scalar_name]
        vmin, vmax = np.min(data), np.max(data)

        # Avoid degenerate range
        if abs(vmax - vmin) < 1e-6:
            vmax = vmin + 1e-3
        # Opacity transfer function for volume rendering
        # Initial conditions with uniform low values may require slice-based visualisation.
        opacity = [0.03, 0.05, 0.08, 0.1, 0.3, 0.6, 1.0]

        # Calculate opacity_unit_distance based on grid spacing
        # This controls how opacity accumulates along ray paths through the volume
        grid_bounds = grid.bounds
        grid_diagonal = np.sqrt(
            (grid_bounds[1] - grid_bounds[0]) ** 2
            + (grid_bounds[3] - grid_bounds[2]) ** 2
            + (grid_bounds[5] - grid_bounds[4]) ** 2
        )
        opacity_unit_dist = grid_diagonal / 50.0

        print(
            f"INFO: Using custom opacity transfer function with opacity_unit_distance={opacity_unit_dist:.2f}"
        )

        try:
            actor = plotter.add_volume(
                grid,
                scalars=scalar_name,
                cmap="viridis",
                clim=[vmin, vmax],
                opacity=opacity,
                opacity_unit_distance=opacity_unit_dist,
                shade=True,
                ambient=0.4,
                diffuse=0.6,
                specular=0.3,
                show_scalar_bar=False,
                mapper="smart_volume",
            )

            # Set interpolation to 'linear' for smoother rendering
            if hasattr(actor, "prop"):
                actor.prop.interpolation_type = "linear"
            print(f"INFO: Volume added successfully with linear interpolation")
        except Exception as e:
            print(f"ERROR: Failed to add volume: {e}")
            print(f"ERROR: Falling back to surface rendering...")
            surface = grid.extract_surface()
            plotter.add_mesh(
                surface,
                scalars=scalar_name,
                cmap="viridis",
                clim=[vmin, vmax],
                show_scalar_bar=False,
                show_edges=True,
                opacity=0.8,
            )

        # Add colorbar - match slice rendering style exactly
        plotter.add_scalar_bar(
            title=scalar_bar_title if scalar_bar_title is not None else scalar_name,
            n_labels=5,
            fmt="%g",
            title_font_size=20,
            label_font_size=20,
            vertical=True,
            height=0.8,
            position_x=0.85,
        )

        # Camera setup - match slice rendering
        plotter.view_isometric()
        plotter.enable_parallel_projection()

        # Add axis bounds to match slice rendering
        plotter.show_bounds(
            grid="back",
            location="all",
            all_edges=True,
            show_xaxis=True,
            show_yaxis=True,
            show_zaxis=True,
            show_xlabels=True,
            show_ylabels=True,
            show_zlabels=True,
            n_xlabels=5,
            n_ylabels=5,
            n_zlabels=5,
            xtitle="X Axis",
            ytitle="Y Axis",
            ztitle="Z Axis",
            font_size=20,
            padding=0.01,
        )
        # Add title AFTER bounds so it's not covered
        plotter.add_title(title, font_size=25)

        # Volume rendering needs more space than surface rendering
        # Reset camera and zoom out significantly to show colorbar on right side
        plotter.reset_camera()
        plotter.camera.zoom(0.7)  # Zoom out 30% to ensure colorbar is fully visible
        # Render and save
        print(f"INFO: Rendering volume to {output_filepath}...")
        plotter.screenshot(output_filepath, transparent_background=False)
        plotter.close()

        print(
            f"INFO: -------- Successfully saved volume rendering to {output_filepath} ---------"
        )

        return {"success": True, "vmin": vmin, "vmax": vmax, "scalar_name": scalar_name}

    except ImportError as e:
        print(f"ERROR: PyVista not available for volume rendering ({e})")
        return {"success": False}
    except Exception as e:
        print(f"ERROR: Volume rendering failed: {e}")
        import traceback

        traceback.print_exc()
        return {"success": False}


def add_annotations_to_raytraced_image(
    image_path, title, scalar_bar_title, vmin, vmax, output_path=None
):
    """
    Add matplotlib-based annotations (title, colorbar) to a raytraced image.
    Uses matplotlib instead of PyVista to avoid GPU/EGL context issues.
    """
    try:
        import matplotlib.pyplot as plt
        import matplotlib.cm as cm
        from matplotlib.colorbar import ColorbarBase
        from matplotlib.colors import Normalize
        from PIL import Image
        import numpy as np

        if output_path is None:
            output_path = image_path

        # Load the raytraced image
        base_image = Image.open(image_path)
        width, height = base_image.size

        # Create figure with exact pixel dimensions
        dpi = 100
        fig = plt.figure(figsize=(width / dpi, height / dpi), dpi=dpi)

        # Main axes for the image (takes most of the space)
        ax_img = fig.add_axes([0, 0, 0.85, 1])  # left, bottom, width, height
        ax_img.imshow(base_image)
        ax_img.axis("off")

        # Add title at the top
        ax_img.set_title(title, fontsize=40, pad=20)

        # Colorbar axes on the right
        ax_cbar = fig.add_axes([0.87, 0.1, 0.03, 0.8])  # narrow vertical strip

        # Create colorbar
        norm = Normalize(vmin=vmin, vmax=vmax)
        cbar = ColorbarBase(ax_cbar, cmap=cm.viridis, norm=norm, orientation="vertical")
        cbar.set_label(scalar_bar_title, fontsize=20, rotation=90, labelpad=15)
        cbar.ax.tick_params(labelsize=20)

        # Format to match PyVista style (5 labels, scientific notation)
        import matplotlib.ticker as ticker

        cbar.locator = ticker.MaxNLocator(nbins=5)
        cbar.formatter = ticker.FormatStrFormatter("%g")
        cbar.update_ticks()

        # Save with tight layout
        plt.savefig(output_path, dpi=dpi, bbox_inches="tight", pad_inches=0.1)
        plt.close(fig)

        print(f"INFO: Added annotations to raytraced image: {output_path}")
        return True

    except Exception as e:
        print(f"WARNING: Failed to add annotations ({e}), keeping plain render")
        return False


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
    enable_raytracing=False,
):
    """
    Plots cell geometries. Handles 2D data with Matplotlib and 3D data with PyVista.
    """
    # Boost DPI for PGF output since the rasterised portion will be embedded at this resolution
    # and then potentially resized in LaTeX, requiring higher initial resolution
    if output_format == "pgf" and dpi < 600:
        if verbose:
            print(
                f"INFO: Boosting DPI from {dpi} to 600 for high-quality PGF rasterisation"
            )
        dpi = 600

    combine_unknowns = processed_data.get("combine_unknowns")

    raw_unknown_name = processed_data.get("unknown_name", "Value")

    # Get the descriptions list and unknown_idx from processed_data
    solver_meta = processed_data.get("solver_meta", {})
    descriptions = solver_meta.get("descriptions", [])
    unknown_idx = processed_data.get("unknown_idx", 0)
    num_unknowns = solver_meta.get("unknowns_count", 1)

    # Check if descriptions list covers all unknowns (vector names vs scalar components)
    # Example: 4 vector quantities (Rho, v_x, v_y, v_z) but 12 scalar unknowns (3 components each)
    # In this case, len(descriptions) = 4 but num_unknowns = 12, so we need Component_{idx}
    descriptions_incomplete = len(descriptions) < num_unknowns if descriptions else True

    # Try to get the description for the selected unknown
    if descriptions and unknown_idx < len(descriptions) and not descriptions_incomplete:
        description = descriptions[unknown_idx].strip()
    else:
        description = None

    # Generate both scalar bar title and plot title based on data type
    if raw_unknown_name == "Combined_Value" and combine_unknowns:
        # Combined unknowns: show explicit sum notation
        indices_str = ", ".join(map(str, combine_unknowns))
        scalar_bar_title = f"Sum of unknowns ({indices_str})"
        improved_title = f"{title_suffix} - Sum({indices_str})"
    elif description:
        # Use description from patch file (only when descriptions cover all unknowns)
        scalar_bar_title = description
        improved_title = f"{title_suffix} - {description}"
    elif "Component" in raw_unknown_name or descriptions_incomplete:
        # Fallback: Format as Component_{idx} when:
        # 1. Name already contains "Component" (standard naming)
        # 2. Description list has vector names but unknowns are scalar components
        if "Component" in raw_unknown_name:
            parts = raw_unknown_name.split("_", 1)
            scalar_bar_title = f"${parts[0]}_{{{parts[1]}}}$"
            improved_title = f"${title_suffix} - {raw_unknown_name}$"
        else:
            # Descriptions are vector names, generate scalar Component_{idx} format
            scalar_bar_title = f"$Component_{{{unknown_idx}}}$"
            improved_title = f"${title_suffix} - Component_{unknown_idx}$"
    else:
        # Fallback for any other case
        scalar_bar_title = raw_unknown_name
        improved_title = title_suffix

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
            # Use PlotOptiX for raytracing if requested
            if slice_axis:
                if enable_raytracing:
                    result = plot_with_plotoptix(
                        grid, raw_unknown_name, output_filepath
                    )
                    if result is not None and result.get("success"):
                        # PlotOptiX rendered successfully, now add annotations
                        if verbose:
                            print(
                                f"INFO: Plotter: PlotOptiX rendering complete, adding annotations..."
                            )

                        try:
                            add_annotations_to_raytraced_image(
                                image_path=output_filepath,
                                title=improved_title,
                                scalar_bar_title=label_text,
                                vmin=result["vmin"],
                                vmax=result["vmax"],
                                output_path=output_filepath,
                            )
                        except Exception as e:
                            # If annotation fails (e.g., in multiprocessing), keep the plain render
                            print(
                                f"WARNING: Failed to add annotations ({e}), keeping plain render"
                            )
                        return

                else:
                    if verbose:
                        print(
                            f"INFO: Plotter: Generating 3D raster plot for '{title_suffix}' with PyVista..."
                        )
                    # Standard PyVista rendering
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
                        title=label_text,
                        n_labels=5,
                        fmt="%g",
                        title_font_size=20,
                        label_font_size=20,
                        vertical=True,
                        height=0.8,
                        position_x=0.85,
                    )

                    # Add the main title as a text actor
                    plotter.add_title(improved_title, font_size=40)
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
            else:
                # No slice specified - do volume rendering
                result = plot_with_volume_rendering(
                    grid,
                    raw_unknown_name,
                    output_filepath,
                    title=improved_title,
                    scalar_bar_title=label_text,
                )
                if result and result.get("success"):
                    return
        # PATH 2: Vector graphics via MATPLOTLIB
        elif output_format in ["pdf", "svg", "pgf"]:
            # Configure matplotlib backend for vector output (must be done before importing pyplot)
            if output_format == "pgf":
                # CRITICAL: Set pgf.texsystem BEFORE calling mpl.use("pgf")
                mpl.rcParams["pgf.texsystem"] = "pdflatex"
                mpl.use("pgf")
                # Configure PGF to match PNG/PyVista appearance
                mpl.rcParams.update(
                    {
                        "font.family": "sans-serif",  # Match default matplotlib font
                        "font.sans-serif": ["DejaVu Sans", "Arial"],
                        "font.size": 10,
                        "axes.labelsize": 11,
                        "axes.titlesize": 16,
                        "xtick.labelsize": 9,
                        "ytick.labelsize": 9,
                        "legend.fontsize": 10,
                        "figure.titlesize": 16,
                        "text.usetex": True,  # Use LaTeX for text rendering
                        "pgf.rcfonts": False,  # Don't use default rc font settings
                    }
                )
                if verbose:
                    print("INFO: Using PGF backend for 3D vector graphics...")
            elif output_format == "svg":
                mpl.use("svg")
            elif output_format == "pdf":
                mpl.use("pdf")
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
            ax.set_title(improved_title, fontsize=16, pad=20)
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
            # Rasterise the mesh data for vector formats to keep file size manageable
            # Text, axes, and colorbar remain as vectors for editability
            poly_collection.set_rasterized(True)
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
            x_range = xmax - xmin
            y_range = ymax - ymin
            z_range = zmax - zmin
            x_margin = x_range * 0.03
            y_margin = y_range * 0.03
            z_margin = z_range * 0.03

            # Calculate new limits with margin
            xmin_with_margin = xmin - x_margin
            xmax_with_margin = xmax + x_margin
            ymin_with_margin = ymin - y_margin
            ymax_with_margin = ymax + y_margin
            zmin_with_margin = zmin - z_margin
            zmax_with_margin = zmax + z_margin

            ax.set_xlim(xmin_with_margin, xmax_with_margin)
            ax.set_ylim(ymin_with_margin, ymax_with_margin)
            ax.set_zlim(zmin_with_margin, zmax_with_margin)

            # Reduce to 4 ticks per axis to prevent crowding in corners
            # Use the NEW limits (with margin) for tick calculation to avoid gaps
            ax.set_xticks(np.linspace(xmin_with_margin, xmax_with_margin, 5))
            ax.set_yticks(np.linspace(ymin_with_margin, ymax_with_margin, 5))
            ax.set_zticks(np.linspace(zmin_with_margin, zmax_with_margin, 5))
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
            cbar.set_label(label_text, fontsize=20)
            cbar.ax.tick_params(labelsize=20)

            # Match PNG number formatting: use ScalarFormatter with %g-style output
            from matplotlib.ticker import ScalarFormatter

            formatter = ScalarFormatter(useOffset=False)
            formatter.set_scientific(False)
            formatter.set_powerlimits((-3, 4))
            cbar.ax.yaxis.set_major_formatter(formatter)

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
                    "text.latex.preamble": r"\usepackage{xcolor}",
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
        ax.set_title(improved_title)
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
            # as a single rasterised image at the save DPI.
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
            if output_format in ["pdf", "svg", "pgf"]:
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
