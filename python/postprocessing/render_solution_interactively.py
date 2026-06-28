#!/usr/bin/env python3
"""Interactive, headless renderer for ExaHyPE 2 HDF5 output.
"""

from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import asyncio
import math
import os

import matplotlib

# Render head-less (no display) when running as a plain script or in worker
# processes - but leave the backend alone inside an IPython kernel: forcing
# Agg there would silently disable the notebook's own inline plotting (any
# plt.show() in later cells would draw nothing).
try:
    get_ipython()  # type: ignore[name-defined]  # defined inside IPython only
except NameError:
    matplotlib.use("Agg", force=True)

import h5py
import numpy as np


def _new_figure(figsize, dpi):
    """A figure outside pyplot's figure manager, rendered by Agg regardless of
    the active backend: nothing auto-displays at the end of a notebook cell,
    nothing accumulates in pyplot's global state, and no close() is required."""
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.figure import Figure

    fig = Figure(figsize=figsize, dpi=dpi)
    FigureCanvasAgg(fig)
    return fig


# XDMF mixed-topology element type -> number of nodes (as written by ExaHyPE).
_XDMF_NODE_COUNTS = {4: 3, 5: 4, 6: 4, 7: 5, 8: 6, 9: 8}

# Regular Cartesian meshes above this many cells are treated as "large": loaded
# lazily and rendered on demand rather than all pre-rendered up front.
_LARGE_MESH = 250_000

# Fixed figure layout (image axes + colour-bar axes, as left/bottom/width/height
# fractions) shared by the live renderer and the parallel worker. Using explicit
# rectangles instead of tight_layout keeps a frame pixel-identical whichever path
# drew it, so scrubbing never "jumps" when prefetched and on-demand frames mix.
_IMAGE_RECT = (0.04, 0.06, 0.74, 0.88)
_CBAR_RECT = (0.80, 0.06, 0.03, 0.88)


# --------------------------------------------------------------------------- #
# Reading ExaHyPE 2 HDF5 output
# --------------------------------------------------------------------------- #
@dataclass
class SolutionData:
    filename: Path
    geometry: np.ndarray
    cells: object  # (N, k) ndarray for a uniform mesh, else a list of arrays
    point_data: dict
    cell_data: dict

    @property
    def variables(self):
        return list(self.point_data) + list(self.cell_data)

    def field(self, name):
        if name in self.point_data:
            return self.point_data[name], "point"
        if name in self.cell_data:
            return self.cell_data[name], "cell"
        raise KeyError(f"No field named {name!r} in {self.filename}")


def _time_from_name(path):
    """ExaHyPE encodes the snapshot time in the file name, e.g. Solver-0_500000.h5."""
    try:
        return float(path.stem.rsplit("-", 1)[1].replace("_", "."))
    except (IndexError, ValueError):
        return float("nan")


def _time_sort_key(path):
    time = _time_from_name(path)
    # NaN (unparseable) names sort last but keep a deterministic order.
    return (1, 0.0, path.name) if time != time else (0, time, path.name)


def _files_from_timeseries(index_file):
    """The snapshot .h5 files listed in a time-series XDMF index, in index order.

    The index is a temporal collection whose entries xi:include the
    per-snapshot .xmf files; the writer always pairs ``name.xmf`` with
    ``name.h5`` next to it.
    """
    import xml.etree.ElementTree as ET

    files = []
    for include in ET.parse(index_file).getroot().iter(
            "{http://www.w3.org/2001/XInclude}include"):
        href = include.get("href")
        if not href:
            continue
        snapshot = index_file.parent / Path(href).with_suffix(".h5").name
        if snapshot.exists() and snapshot not in files:
            files.append(snapshot)
    return files


def solution_files(solver_name, directory="solutions"):
    """All HDF5 snapshots for ``solver_name``, ordered by simulation time.

    Several runs into the same directory leave snapshots of earlier runs
    behind (a shorter re-run does not delete a longer run's tail), so a plain
    glob would mix independent runs. The solver's time-series index
    ``<solver>.xmf`` is rewritten by each run and lists exactly the snapshots
    of the most recent one - prefer it, and only fall back to the glob when
    there is no (readable) index.
    """
    directory = Path(directory)
    index = directory / f"{solver_name}.xmf"
    if index.exists():
        try:
            files = _files_from_timeseries(index)
            if files:
                return files
        except Exception:
            pass  # unreadable / half-written index: fall back to the glob
    files = sorted(directory.glob(f"{solver_name}-*.h5"), key=_time_sort_key)
    if not files:
        raise FileNotFoundError(
            f"No HDF5 output found for {solver_name!r} in {directory!r}. "
            "Run the notebook cells that build and execute the solver first."
        )
    return files


def _uniform_cells(topology):
    """Fast path: if every cell is the same element type, return an (N, k) array.

    The XDMF mixed topology is a flat ``[type, nodes..., type, nodes..., ...]``
    array. When all cells share one element type it has a constant stride, which
    we can reshape in one go instead of looping over (potentially millions of)
    cells -- the Python loop in _mixed_topology_to_cells would otherwise dominate
    load time for large meshes.
    """
    topology = np.asarray(topology)
    if topology.size == 0:
        return None
    element_type = int(topology[0])
    nodes_per_cell = _XDMF_NODE_COUNTS.get(element_type)
    if nodes_per_cell is None:
        return None
    stride = nodes_per_cell + 1
    if topology.size % stride != 0:
        return None
    block = topology.reshape(-1, stride)
    if np.all(block[:, 0] == element_type):
        return block[:, 1:].astype(np.int64)
    return None


def _mixed_topology_to_cells(topology):
    cells = []
    index = 0
    while index < len(topology):
        xdmf_type = int(topology[index])
        index += 1
        if xdmf_type in _XDMF_NODE_COUNTS:
            node_count = _XDMF_NODE_COUNTS[xdmf_type]
        elif xdmf_type in (1, 2, 3):
            node_count = int(topology[index])
            index += 1
        else:
            raise ValueError(f"Unsupported XDMF topology type {xdmf_type}")
        cells.append(np.asarray(topology[index:index + node_count], dtype=np.int64))
        index += node_count
    return cells


def _read_fields(group):
    fields = {}
    for name, dataset in group.items():
        values = np.asarray(dataset)
        if values.ndim == 2 and values.shape[1] == 1:
            values = values[:, 0]
        fields[name] = values
    return fields


def load_solution(filename):
    filename = Path(filename)
    with h5py.File(filename, "r") as data:
        topology = np.asarray(data["topology"])
        cells = _uniform_cells(topology)
        if cells is None:
            cells = _mixed_topology_to_cells(topology)
        return SolutionData(
            filename=filename,
            geometry=np.asarray(data["geometry"]),
            cells=cells,
            point_data=_read_fields(data["data"]) if "data" in data else {},
            cell_data=_read_fields(data["cell_data"]) if "cell_data" in data else {},
        )


# --------------------------------------------------------------------------- #
# Field values
# --------------------------------------------------------------------------- #
def _as_scalar(values):
    """Reduce a possibly vector-valued field to a scalar (its magnitude)."""
    values = np.asarray(values)
    return values if values.ndim == 1 else np.linalg.norm(values, axis=1)


def cell_values(solution, field):
    """One scalar colour value per cell (vectorised where possible)."""
    values, location = solution.field(field)
    values = _as_scalar(values)
    if location == "cell":
        return np.asarray(values, dtype=float)
    cells = solution.cells
    if isinstance(cells, np.ndarray):  # uniform mesh -> fully vectorised average
        return values[cells].mean(axis=1)
    return np.array([float(values[np.asarray(nodes)].mean()) for nodes in cells])


def _cell_polygons_and_values(solution, field):
    """Polygons (one per cell) plus a scalar colour value per cell (slow path)."""
    raw, location = solution.field(field)
    raw = _as_scalar(raw)
    xy = solution.geometry[:, :2]
    polygons, colours = [], []
    for index, nodes in enumerate(solution.cells):
        nodes = np.asarray(nodes)
        if len(nodes) < 3:
            continue
        polygons.append(xy[nodes])
        colours.append(raw[index] if location == "cell" else float(raw[nodes].mean()))
    return polygons, np.asarray(colours)


def _regular_grid(solution):
    """If the cells tile a regular Cartesian lattice, return the imshow mapping."""
    cells = solution.cells
    if not isinstance(cells, np.ndarray):
        return None
    xy = solution.geometry[:, :2]
    centres = xy[cells].mean(axis=1)
    cx = np.round(centres[:, 0], 9)
    cy = np.round(centres[:, 1], 9)
    xs = np.unique(cx)
    ys = np.unique(cy)
    nx, ny = len(xs), len(ys)
    if nx * ny != len(centres):
        return None
    ix = np.searchsorted(xs, cx)
    iy = np.searchsorted(ys, cy)
    if np.unique(iy * nx + ix).size != len(ix):  # must be a bijection
        return None
    extent = [float(xy[:, 0].min()), float(xy[:, 0].max()),
              float(xy[:, 1].min()), float(xy[:, 1].max())]
    return {"xs": xs, "ys": ys, "nx": nx, "ny": ny, "extent": extent}


def _colorbar_mappable(cmap, vmin, vmax):
    import matplotlib.cm as cm
    from matplotlib.colors import Normalize

    mappable = cm.ScalarMappable(norm=Normalize(vmin=vmin, vmax=vmax), cmap=cmap)
    mappable.set_array([])
    return mappable


def _frame_clim(values, user_clim, global_clim, field, robust):
    """Colour range: manual override, fixed (global), or this frame's range.

    With ``robust`` the 2nd-98th percentile is used so a few extreme cells do not
    flatten the rest of the field. Shared by the live renderer and the parallel
    worker so both produce identical colours.
    """
    if user_clim is not None:
        return tuple(user_clim)
    if global_clim is not None:
        return global_clim[field]
    finite = values[np.isfinite(values)]
    if finite.size == 0:
        return (0.0, 1.0)
    if robust:
        lo, hi = (float(value) for value in np.percentile(finite, [2.0, 98.0]))
    else:
        lo, hi = float(finite.min()), float(finite.max())
    return (lo - 1.0e-9, hi + 1.0e-9) if lo == hi else (lo, hi)


def _render_one(file, field, cfg):
    """Render one frame to PNG bytes on a fresh figure (used by parallel workers).

    Self-contained so it can run in a separate process: everything it needs is
    in ``cfg`` (cmap, sizing, the regular-grid mapping or ``None`` for polygons,
    and the colour-range settings).
    """
    from matplotlib.collections import PolyCollection

    solution = load_solution(file)
    fig_w, fig_h = 6.4, 4.8
    dpi = max(80, int(cfg["target_px"] / fig_w))
    fig = _new_figure((fig_w, fig_h), dpi)
    ax = fig.add_axes(_IMAGE_RECT)
    ax.set_axis_off()
    cax = fig.add_axes(_CBAR_RECT)
    grid = cfg["grid"]
    if grid is not None:
        values = cell_values(solution, field)
        vmin, vmax = _frame_clim(values, cfg["user_clim"], cfg["global_clim"], field, cfg["robust"])
        nx, ny = grid["nx"], grid["ny"]
        centres = solution.geometry[:, :2][solution.cells].mean(axis=1)
        ix = np.searchsorted(grid["xs"], np.round(centres[:, 0], 9))
        iy = np.searchsorted(grid["ys"], np.round(centres[:, 1], 9))
        np.clip(ix, 0, nx - 1, out=ix)
        np.clip(iy, 0, ny - 1, out=iy)
        image = np.full((ny, nx), np.nan)
        image[iy, ix] = values
        step = max(1, math.ceil(max(nx, ny) / max(1, cfg["target_px"])))
        if step > 1:
            image = image[::step, ::step]
        artist = ax.imshow(image, origin="lower", extent=grid["extent"], aspect="equal",
                           cmap=cfg["cmap"], interpolation="nearest")
        artist.set_clim(vmin, vmax)
        fig.colorbar(artist, cax=cax).set_label(field)
    else:
        polygons, colours = _cell_polygons_and_values(solution, field)
        vmin, vmax = _frame_clim(colours, cfg["user_clim"], cfg["global_clim"], field, cfg["robust"])
        collection = PolyCollection(
            polygons, array=colours, cmap=cfg["cmap"],
            edgecolors="black" if cfg["show_edges"] else "none",
            linewidths=0.15 if cfg["show_edges"] else 0.0,
        )
        collection.set_clim(vmin, vmax)
        ax.add_collection(collection)
        ax.autoscale_view()
        ax.set_aspect("equal")
        fig.colorbar(collection, cax=cax).set_label(field)
    buffer = BytesIO()
    fig.savefig(buffer, format="png", facecolor="white")
    return buffer.getvalue()


_RENDER_CFG = None


def _init_render_worker(cfg):
    global _RENDER_CFG
    _RENDER_CFG = cfg


def _render_worker(task):
    file, field, index = task
    return index, _render_one(file, field, _RENDER_CFG)


def _make_render_pool(cfg=None, max_workers=None):
    """A process pool of render workers, or None if one cannot be started.

    Fork is used deliberately: the workers inherit the already-imported matplotlib
    and do not re-import the caller's ``__main__`` — which is exactly what breaks
    spawn/forkserver inside a Jupyter kernel (and forkserver is Python 3.14's Linux
    default). On a platform without fork we fall back to the default context.
    ``cfg`` (the 2D frame-render configuration) is pickled to each worker, so
    every value in it must be pickle-safe; pass None for workers whose tasks are
    self-contained (the 3D slice prefetch).
    """
    try:
        import multiprocessing as mp
        from concurrent.futures import ProcessPoolExecutor

        try:
            context = mp.get_context("fork")
        except ValueError:
            context = mp.get_context()
        init = {} if cfg is None else dict(initializer=_init_render_worker, initargs=(cfg,))
        return ProcessPoolExecutor(max_workers=max_workers, mp_context=context, **init)
    except Exception:
        return None


# --------------------------------------------------------------------------- #
# Frame renderer (imshow vs polygons, lazy-loads, reuses one figure)
# --------------------------------------------------------------------------- #
class _FrameRenderer:
    def __init__(self, files, *, cmap="coolwarm", show_edges=False, width=640,
                 normalise="frame", clim=None, robust=False, progress=None):
        def report(message):
            if progress is not None:
                progress(message)

        self.files = list(files)
        self.n = len(self.files)
        self.cmap = cmap
        self.show_edges = show_edges
        self.normalise = normalise          # "frame" (autoscale) or "global"
        self._user_clim = tuple(clim) if clim is not None else None  # manual override
        self.robust = robust                # ignore outliers (use 2-98th percentile)
        self._last_index = None             # one-frame load cache -> bounded memory
        self._last_solution = None
        self._cache = {}                    # (field, index) -> PNG bytes

        report("Loading output …")
        first = self._solution(0)
        self.variables = first.variables
        if not self.variables:
            raise ValueError("No plottable fields in the solver output.")
        self.large = len(first.cells) > _LARGE_MESH
        default = self.variables[0]

        report("Detecting grid layout …")
        grid = _regular_grid(first)
        if grid is not None and self.n > 1:
            if len(self._solution(self.n - 1).cells) != len(first.cells):
                grid = None
        self._grid = grid
        if grid is not None:
            self._xs, self._ys = grid["xs"], grid["ys"]
            self._nx, self._ny = grid["nx"], grid["ny"]
            self._extent = grid["extent"]
        elif self.large:
            raise ValueError(
                f"This mesh has {len(first.cells):,} cells and is not a regular "
                "Cartesian grid, so it cannot be drawn efficiently. Re-run with a "
                "coarser mesh (fewer grid levels) or fewer snapshots."
            )

        # Fixed global colour range only if explicitly requested (loads every
        # frame once). The default per-frame autoscaling needs no up-front pass.
        self._global_clim = None
        if normalise == "global":
            accumulator = {var: [math.inf, -math.inf] for var in self.variables}
            for count in range(self.n):
                report(f"Computing colour range … ({count + 1}/{self.n})")
                solution = self._solution(count)
                for var in self.variables:
                    values = cell_values(solution, var)
                    values = values[np.isfinite(values)]
                    if values.size:
                        accumulator[var][0] = min(accumulator[var][0], float(values.min()))
                        accumulator[var][1] = max(accumulator[var][1], float(values.max()))
            self._global_clim = {}
            for var in self.variables:
                lo, hi = accumulator[var]
                if not math.isfinite(lo):
                    lo, hi = 0.0, 1.0
                self._global_clim[var] = (lo - 1.0e-9, hi + 1.0e-9) if lo == hi else (lo, hi)

        # Render larger than the display so the down-scaled image stays crisp.
        fig_w, fig_h = 6.4, 4.8
        oversample = 1.6 if self._grid is not None else 1.4
        self._target_px = int(oversample * width)
        dpi = max(80, int(self._target_px / fig_w))
        self.fig = _new_figure((fig_w, fig_h), dpi)
        self.ax = self.fig.add_axes(_IMAGE_RECT)
        self.ax.set_axis_off()
        self._cax = self.fig.add_axes(_CBAR_RECT)
        init_lo, init_hi = self._clim(default, cell_values(first, default))

        if self._grid is not None:
            self._image = self.ax.imshow(
                np.zeros((1, 1)), origin="lower", extent=self._extent,
                aspect="equal", cmap=cmap, interpolation="nearest",
            )
            self._image.set_clim(init_lo, init_hi)
            self.colorbar = self.fig.colorbar(self._image, cax=self._cax)
        else:
            self._mappable = _colorbar_mappable(cmap, init_lo, init_hi)
            self.colorbar = self.fig.colorbar(self._mappable, cax=self._cax)
        self.colorbar.set_label(default)
        # No tight_layout: the fixed rectangles above keep this identical to the
        # parallel worker (_render_one), so a frame looks the same either way.

    def _solution(self, index):
        """Load a frame on demand, keeping only the most recent one in memory."""
        if self._last_index != index:
            self._last_solution = load_solution(self.files[index])
            self._last_index = index
        return self._last_solution

    def _clim(self, field, values):
        return _frame_clim(values, self._user_clim, self._global_clim, field, self.robust)

    def _grid_image(self, solution, values):
        """2D field image for a regular grid, downsampled to ~display resolution."""
        centres = solution.geometry[:, :2][solution.cells].mean(axis=1)
        ix = np.searchsorted(self._xs, np.round(centres[:, 0], 9))
        iy = np.searchsorted(self._ys, np.round(centres[:, 1], 9))
        np.clip(ix, 0, self._nx - 1, out=ix)
        np.clip(iy, 0, self._ny - 1, out=iy)
        image = np.full((self._ny, self._nx), np.nan)
        image[iy, ix] = values
        step = max(1, math.ceil(max(self._nx, self._ny) / max(1, self._target_px)))
        return image[::step, ::step] if step > 1 else image

    def render(self, field, index):
        key = (field, index)
        if key in self._cache:
            return self._cache[key]
        solution = self._solution(index)
        if self._grid is not None:
            values = cell_values(solution, field)
            vmin, vmax = self._clim(field, values)
            self._image.set_data(self._grid_image(solution, values))
            self._image.set_extent(self._extent)
            self._image.set_cmap(self.cmap)
            self._image.set_clim(vmin, vmax)
        else:
            from matplotlib.collections import PolyCollection

            polygons, colours = _cell_polygons_and_values(solution, field)
            vmin, vmax = self._clim(field, colours)
            self.ax.clear()
            self.ax.set_axis_off()
            self.ax.set_aspect("equal")
            collection = PolyCollection(
                polygons, array=colours, cmap=self.cmap,
                edgecolors="black" if self.show_edges else "none",
                linewidths=0.15 if self.show_edges else 0.0,
            )
            collection.set_clim(vmin, vmax)
            self.ax.add_collection(collection)
            self.ax.autoscale_view()
            self._mappable.set_cmap(self.cmap)
            self._mappable.set_clim(vmin, vmax)
            self._mappable.changed()
            try:
                self.colorbar.update_normal(self._mappable)
            except Exception:
                pass
        self.colorbar.set_label(field)
        buffer = BytesIO()
        self.fig.savefig(buffer, format="png", facecolor="white")
        self._cache[key] = buffer.getvalue()
        return self._cache[key]

    def _render_config(self):
        """Picklable settings so a worker process can reproduce our frames."""
        grid = None
        if self._grid is not None:
            grid = {"xs": self._xs, "ys": self._ys, "nx": self._nx, "ny": self._ny,
                    "extent": self._extent}
        return {"cmap": self.cmap, "show_edges": self.show_edges,
                "target_px": self._target_px, "grid": grid,
                "user_clim": self._user_clim, "global_clim": self._global_clim,
                "robust": self.robust}

    def prerender(self, field, *, progress=None, max_workers=None):
        """Render every frame of ``field`` into the cache, using all cores.

        Frames are independent, so each one is loaded and drawn to PNG in its own
        process and the bytes are pickled back. Falls back to a sequential loop
        when only one worker is available or a pool cannot be started.
        """
        pending = [index for index in range(self.n) if (field, index) not in self._cache]
        if not pending:
            return
        cfg = self._render_config()
        if max_workers is None:
            cores = os.cpu_count() or 1
            # Cap workers on big meshes: each holds a whole frame in memory.
            max_workers = min(cores, 4 if self.large else 8, len(pending))

        def sequential():
            for done, index in enumerate(pending, start=1):
                self._cache[(field, index)] = _render_one(str(self.files[index]), field, cfg)
                if progress:
                    progress(done, len(pending))

        if max_workers <= 1:
            sequential()
            return
        tasks = [(str(self.files[index]), field, index) for index in pending]
        pool = _make_render_pool(cfg, max_workers)
        if pool is None:
            sequential()
            return
        try:
            with pool:
                for done, (index, png) in enumerate(pool.map(_render_worker, tasks), start=1):
                    self._cache[(field, index)] = png
                    if progress:
                        progress(done, len(tasks))
        except Exception:
            sequential()

    def close(self):
        self.fig = None  # not pyplot-managed, so dropping the reference frees it


# --------------------------------------------------------------------------- #
# Single-frame helpers
# --------------------------------------------------------------------------- #
def render_solution_image(filename, field=None, *, show_edges=False, cmap="coolwarm", width=720, clim=None):
    """Render one HDF5 snapshot to PNG bytes."""
    renderer = _FrameRenderer([filename], cmap=cmap, show_edges=show_edges, width=width, clim=clim)
    scalar = field if field in renderer.variables else renderer.variables[0]
    try:
        return renderer.render(scalar, 0)
    finally:
        renderer.close()


def plot_solution(filename, field=None, *, show_edges=False, cmap="coolwarm", width=720):
    """Show a single snapshot inline in a notebook."""
    png = render_solution_image(filename, field, show_edges=show_edges, cmap=cmap, width=width)
    try:
        from IPython.display import Image
    except ImportError:
        return png
    return Image(data=png)


# --------------------------------------------------------------------------- #
# Interactive viewer
# --------------------------------------------------------------------------- #
def show_solution(
    solver_name,
    field=None,
    directory="solutions",
    *,
    cmap="coolwarm",
    show_edges=False,
    play_interval=200,
    width=900,
    normalise="frame",
    clim=None,
    robust=False,
    prerender="auto",
):
    """Interactive viewer: a time slider, a frame selector, ▶ Play / ■ Stop and a
    field selector, with a status line reporting what is being loaded/rendered.

    The colour scale autoscales per frame by default (``normalise="frame"``);
    pass ``normalise="global"`` for a fixed scale comparable across time. When a
    field has extreme outliers (e.g. the dense airfoil blob hides the surrounding
    flow), pass ``robust=True`` (use the 2nd-98th percentile) or a manual
    ``clim=(lo, hi)`` to bring the flow out. ``width`` is the on-screen image
    width in pixels. ▶ Play renders frames one after another (never skips) and
    can be interrupted with ■ Stop.

    ``prerender`` controls how frames are rendered ahead of time (always in
    parallel across CPU cores):

    - by default the viewer shows frame 0 immediately, then *blocks* with a
      live "Prefetching N/M" progress line until every frame of the active
      field is rendered - after the short wait, scrubbing and playback are
      perfectly smooth. Switching fields prefetches the new field the same
      way. Runs with more than 400 snapshots skip the warm-up;
    - ``False`` disables all pre-rendering (every frame is rendered lazily on
      demand).
    """
    files = solution_files(solver_name, directory)
    times = [_time_from_name(path) for path in files]
    last = len(files) - 1

    try:
        import ipywidgets as widgets
        from IPython.display import display
    except ImportError:  # Not in a notebook: render the last frame and return it.
        renderer = _FrameRenderer(files, cmap=cmap, show_edges=show_edges, width=width,
                                  normalise=normalise, clim=clim, robust=robust)
        default_field = field if field in renderer.variables else renderer.variables[0]
        png = renderer.render(default_field, last)
        renderer.close()
        return png

    status = widgets.HTML(value="<i>Loading …</i>")
    container = widgets.VBox([status])
    display(container)

    renderer = _FrameRenderer(
        files, cmap=cmap, show_edges=show_edges, width=width, normalise=normalise,
        clim=clim, robust=robust,
        progress=lambda message: setattr(status, "value", f"<i>{message}</i>"),
    )
    variables = renderer.variables
    default_field = field if field in variables else variables[0]
    # Prefetch every frame of the active field up front, in parallel across
    # cores, with a visible progress line - then scrubbing and playback are
    # instant. Rendering lazily while the user interacts feels sluggish for
    # anything but trivial meshes, so the only opt-outs are prerender=False
    # (render on demand) and absurd frame counts, where the warm-up would
    # take longer than anyone is willing to wait.
    do_prerender = prerender is not False and len(files) <= 400

    slider = widgets.IntSlider(
        value=0, min=0, max=last, step=1, description="step",
        continuous_update=do_prerender, readout=False,
        layout=widgets.Layout(width=f"{max(200, width - 120)}px"),
    )
    play_button = widgets.Button(description="▶ Play", layout=widgets.Layout(width="80px"))
    stop_button = widgets.Button(description="■ Stop", disabled=True, layout=widgets.Layout(width="80px"))
    frame_dropdown = widgets.Dropdown(
        options=[(f"frame {i}  (t = {times[i]:g})", i) for i in range(len(files))],
        value=0, description="frame",
    )
    housekeeping = ("Subdomain", "RefinementLevel")
    field_options = [v for v in variables if v not in housekeeping] + [
        v for v in variables if v in housekeeping
    ]
    field_dropdown = widgets.Dropdown(options=field_options, value=default_field, description="field")
    image = widgets.Image(format="png", layout=widgets.Layout(width=f"{width}px", height="auto"))

    state = {"syncing": False, "playing": False}

    def set_busy(busy):
        for control in (slider, frame_dropdown, field_dropdown, play_button):
            control.disabled = busy
        image.layout.opacity = "0.5" if busy else "1"

    def status_showing(index, field):
        status.value = (
            f"frame <b>{index}</b> / {last} &nbsp;&bull;&nbsp; t = {times[index]:g} "
            f"&nbsp;&bull;&nbsp; field = <b>{field}</b>"
        )

    def show_index(index, field):
        """Render and display one frame, reporting progress while it renders."""
        if (field, index) not in renderer._cache:
            status.value = f"<i>Rendering frame {index} (t = {times[index]:g}) …</i>"
            set_busy(True)
            try:
                image.value = renderer.render(field, index)
            finally:
                set_busy(False)
        else:
            image.value = renderer.render(field, index)
        status_showing(index, field)

    def goto(index):
        if state["syncing"]:
            return
        state["syncing"] = True
        try:
            slider.value = index
            frame_dropdown.value = index
        finally:
            state["syncing"] = False
        show_index(index, field_dropdown.value)

    precached_fields = set()

    def precache(field):
        """Render every frame once, in parallel across cores, so scrubbing is instant."""
        if not do_prerender or field in precached_fields:
            return
        pending = sum((field, i) not in renderer._cache for i in range(len(files)))
        if pending == 0:
            precached_fields.add(field)
            return
        status.value = f"<i>Prefetching {pending} frames of <b>{field}</b> … (please wait)</i>"
        set_busy(True)
        try:
            renderer.prerender(
                field,
                progress=lambda done, total: setattr(
                    status, "value",
                    f"<i>Prefetching <b>{field}</b>: {done}/{total} frames (parallel) …</i>",
                ),
            )
            precached_fields.add(field)
        finally:
            set_busy(False)
            status_showing(slider.value, field)

    def _set_playing(playing):
        # During playback keep ■ Stop live but lock the other controls.
        play_button.disabled = playing
        stop_button.disabled = not playing
        slider.disabled = playing
        frame_dropdown.disabled = playing
        field_dropdown.disabled = playing

    async def _play_loop():
        """Render frames one after another. Runs on the event loop so that a
        ■ Stop click (which sets state['playing']=False) is handled between
        frames -- a plain blocking loop could not be interrupted."""
        field = field_dropdown.value
        index = slider.value if slider.value < last else 0
        while index < len(files) and state["playing"]:
            status.value = f"<i>Playing — frame {index} / {last} (t = {times[index]:g}) …</i>"
            image.value = renderer.render(field, index)
            state["syncing"] = True
            slider.value = index
            frame_dropdown.value = index
            state["syncing"] = False
            await asyncio.sleep(play_interval / 1000.0)
            index += 1
        state["playing"] = False
        _set_playing(False)
        status_showing(slider.value, field)

    def on_play(_):
        if state["playing"]:
            return
        state["playing"] = True
        _set_playing(True)
        # Schedule on the (running) Jupyter event loop so ■ Stop stays responsive.
        try:
            asyncio.get_running_loop().create_task(_play_loop())
        except RuntimeError:
            asyncio.ensure_future(_play_loop())

    def on_stop(_):
        state["playing"] = False  # the loop notices this between frames and stops

    def on_slider(change):
        goto(change["new"])

    def on_frame_dropdown(change):
        goto(change["new"])

    def on_field(change):
        precache(change["new"])
        goto(0)  # a new field starts from the first frame

    slider.observe(on_slider, names="value")
    frame_dropdown.observe(on_frame_dropdown, names="value")
    field_dropdown.observe(on_field, names="value")
    play_button.on_click(on_play)
    stop_button.on_click(on_stop)

    controls = widgets.HBox([play_button, stop_button, slider, frame_dropdown])
    container.children = [field_dropdown, controls, status, image]

    # First frame immediately for instant feedback, then the blocking parallel
    # prefetch of the remaining frames.
    show_index(0, default_field)
    precache(default_field)
    return None


# --------------------------------------------------------------------------- #
# 3D slicing (interactive cube slicer + headless slice render)
# --------------------------------------------------------------------------- #
_AXIS_NAMES = {0: "x", 1: "y", 2: "z"}
_AXIS_INDEX = {"x": 0, "y": 1, "z": 2}


def _axis_index(axis):
    return _AXIS_INDEX[axis] if isinstance(axis, str) else int(axis)


def _cell_centres_3d(solution):
    cells = solution.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError("3D slicing needs a uniform (hexahedral) mesh.")
    if solution.geometry.shape[1] < 3:
        raise ValueError("This snapshot is not three-dimensional.")
    return solution.geometry[cells].mean(axis=1)  # (Ncells, 3)


def slice_layers(solution, axis="z"):
    """Sorted slice positions available along ``axis`` ("x"/"y"/"z" or 0/1/2)."""
    return np.unique(np.round(_cell_centres_3d(solution)[:, _axis_index(axis)], 9))


def _slice_image(solution, field, axis, layer_value):
    """One axis-aligned slice as a 2D image, its extent and the in-plane axis labels."""
    a = _axis_index(axis)
    b0, b1 = [k for k in (0, 1, 2) if k != a]
    centres = _cell_centres_3d(solution)
    mask = np.round(centres[:, a], 9) == layer_value
    values = cell_values(solution, field)[mask]
    pb0 = np.round(centres[mask, b0], 9)
    pb1 = np.round(centres[mask, b1], 9)
    xs, ys = np.unique(pb0), np.unique(pb1)
    image = np.full((len(ys), len(xs)), np.nan)
    image[np.searchsorted(ys, pb1), np.searchsorted(xs, pb0)] = values
    extent = [float(xs.min()), float(xs.max()), float(ys.min()), float(ys.max())]
    return image, extent, (_AXIS_NAMES[b0], _AXIS_NAMES[b1])


def _slice_png(solution, field, axis, layer_index, *, cmap, width, clim, title):
    layers = slice_layers(solution, axis)
    layer_index = int(np.clip(layer_index, 0, len(layers) - 1))
    image, extent, (la, lb) = _slice_image(solution, field, axis, layers[layer_index])
    if clim is not None:
        vmin, vmax = clim
    else:
        finite = image[np.isfinite(image)]
        if finite.size:
            vmin, vmax = float(finite.min()), float(finite.max())
            if vmin == vmax:
                vmin, vmax = vmin - 1.0e-9, vmax + 1.0e-9
        else:
            vmin, vmax = 0.0, 1.0
    fig_w = 6.4
    fig = _new_figure((fig_w, 4.8), max(80, int(width / fig_w)))
    ax = fig.add_axes([0.10, 0.10, 0.74, 0.82])
    # The lattice subsamples a smooth high-order field, so smooth interpolation
    # is the more faithful rendering than flat-shaded lattice cells - except
    # over gaps (NaN), which interpolation would smear into blobs.
    interpolation = "nearest" if np.isnan(image).any() else "bicubic"
    im = ax.imshow(image, origin="lower", extent=extent, cmap=cmap,
                   vmin=vmin, vmax=vmax, aspect="equal", interpolation=interpolation)
    ax.set_xlabel(la)
    ax.set_ylabel(lb)
    if title:
        ax.set_title(title)
    fig.colorbar(im, cax=fig.add_axes([0.86, 0.10, 0.03, 0.82]), label=field)
    buffer = BytesIO()
    fig.savefig(buffer, format="png", facecolor="white")
    return buffer.getvalue()


def _slice_worker(task):
    """Pool worker for the 3D slice prefetch: one (file, frame) -> PNG bytes."""
    path, field, axis, layer_index, cmap, width, clim, title, frame = task
    return frame, _slice_png(load_solution(path), field, axis, layer_index,
                             cmap=cmap, width=width, clim=clim, title=title)


# One pool per kernel, shared by every slice viewer: repeated viewer cells would
# otherwise each fork their own set of workers and leak them until kernel exit.
_SLICE_POOL = None


def _slice_pool():
    global _SLICE_POOL
    if _SLICE_POOL is None:
        _SLICE_POOL = _make_render_pool(max_workers=max(1, (os.cpu_count() or 2) - 1))
    return _SLICE_POOL


def _reset_slice_pool():
    """Drop a broken pool so the next prefetch can start a fresh one."""
    global _SLICE_POOL
    if _SLICE_POOL is not None:
        _SLICE_POOL.shutdown(wait=False, cancel_futures=True)
        _SLICE_POOL = None


def render_slice_image(filename, field=None, *, axis="z", index=None, cmap="coolwarm", width=720):
    """Render one axis-aligned slice of a 3D snapshot to PNG bytes (headless)."""
    solution = load_solution(filename)
    field = field if field in solution.variables else solution.variables[0]
    layers = slice_layers(solution, axis)
    index = len(layers) // 2 if index is None else int(np.clip(index, 0, len(layers) - 1))
    title = f"{_AXIS_NAMES[_axis_index(axis)]} = {layers[index]:.3g}"
    return _slice_png(solution, field, axis, index, cmap=cmap, width=width, clim=None, title=title)


def _global_slice_clim(files, field):
    lo, hi = math.inf, -math.inf
    for path in files:
        values = _as_scalar(load_solution(path).field(field)[0])
        finite = values[np.isfinite(values)]
        if finite.size:
            lo, hi = min(lo, float(finite.min())), max(hi, float(finite.max()))
    if not math.isfinite(lo):
        return None
    return (lo, hi) if lo != hi else (lo - 1.0e-9, hi + 1.0e-9)


def show_solution_3d(solver_name, field=None, directory="solutions", *, axis="z",
                     cmap="coolwarm", width=720, play_interval=200, normalise="frame"):
    """Interactive slice viewer for a 3D ExaHyPE 2 solution.

    A 3D field is explored as axis-aligned 2D slices through the cube: pick the
    slice axis (x/y/z), move the *slice* slider through the volume, pick a
    field, and animate over time with the *time* slider / ▶ Play. Rendering is
    plain matplotlib, so the viewer works in any JupyterLab - no GPU, OpenGL
    context or browser 3D support required. Pass ``normalise="global"`` for a
    colour scale held fixed across time (computed over all frames once per
    field). Outside a notebook it returns the mid-plane slice of the last
    frame as PNG bytes.

    Like the 2D viewer, the active slice is prefetched: the first frame shows
    immediately, then every time frame of the current field/axis/slice renders
    in parallel across CPU cores behind a "Prefetching N/M" progress line, so
    time-scrubbing and ▶ Play are perfectly smooth. Moving to another slice or
    field prefetches that one the same way.
    """
    files = solution_files(solver_name, directory)
    times = [_time_from_name(p) for p in files]
    last = len(files) - 1

    try:
        import ipywidgets as widgets
        from IPython.display import display
    except ImportError:  # Not in a notebook: render one slice and return it.
        return render_slice_image(files[last], field, axis=axis, cmap=cmap, width=width)

    solution_cache = {}  # one frame only - bounded memory, like the 2D renderer

    def solution(i):
        if i not in solution_cache:
            solution_cache.clear()
            solution_cache[i] = load_solution(files[i])
        return solution_cache[i]

    base = solution(0)
    housekeeping = ("Subdomain", "RefinementLevel")
    variables = [v for v in base.variables if v not in housekeeping] + \
                [v for v in base.variables if v in housekeeping]
    default_field = field if field in variables else variables[0]

    field_dd = widgets.Dropdown(options=variables, value=default_field, description="field")
    axis_dd = widgets.Dropdown(options=[("x", "x"), ("y", "y"), ("z", "z")], value=axis,
                               description="slice ⊥")
    nlayers = len(slice_layers(base, axis))
    # The slice slider updates on release: every new position triggers a
    # prefetch of all its time frames, which would thrash while dragging.
    slice_sl = widgets.IntSlider(value=nlayers // 2, min=0, max=nlayers - 1, step=1,
                                 description="slice", continuous_update=False)
    time_sl = widgets.IntSlider(value=0, min=0, max=last, step=1, description="time",
                                readout=False,
                                layout=widgets.Layout(width=f"{max(200, width - 120)}px"))
    play_button = widgets.Button(description="▶ Play", layout=widgets.Layout(width="80px"))
    stop_button = widgets.Button(description="■ Stop", disabled=True,
                                 layout=widgets.Layout(width="80px"))
    status = widgets.HTML()
    image = widgets.Image(format="png", layout=widgets.Layout(width=f"{width}px", height="auto"))
    state = {"playing": False, "clim": {}}
    # (field, axis, layer, frame) -> PNG bytes; slices re-show instantly. Bounded:
    # at ~50 kB per slice PNG the cap keeps the cache in the tens of megabytes.
    png_cache = {}
    png_cache_max = 1500

    def cache_put(key, png):
        if len(png_cache) >= png_cache_max:
            png_cache.pop(next(iter(png_cache)))  # FIFO: drop the oldest slice
        png_cache[key] = png

    def clim_for(name):
        if normalise != "global":
            return None
        if name not in state["clim"]:
            status.value = f"<i>Computing global colour range of <b>{name}</b> …</i>"
            state["clim"][name] = _global_slice_clim(files, name)
        return state["clim"][name]

    def set_busy(busy):
        for control in (field_dd, axis_dd, slice_sl, time_sl, play_button):
            control.disabled = busy
        image.layout.opacity = "0.5" if busy else "1"

    def prefetch():
        """Render every time frame of the current field/axis/slice in parallel."""
        if len(files) > 400:  # a warm-up this long is worse than lazy rendering
            return
        fld, a = field_dd.value, axis_dd.value
        layers = slice_layers(base, a)
        idx = int(np.clip(slice_sl.value, 0, len(layers) - 1))
        pending = [i for i in range(len(files)) if (fld, a, idx, i) not in png_cache]
        if not pending:
            return
        pool = _slice_pool()
        if pool is None:  # no multiprocessing available: render lazily on demand
            return
        clim = clim_for(fld)
        tasks = [(files[i], fld, a, idx, cmap, width, clim,
                  f"{a} = {layers[idx]:.3g}   (t = {times[i]:g})", i) for i in pending]
        set_busy(True)
        try:
            from concurrent.futures import as_completed

            done = 0
            for future in as_completed([pool.submit(_slice_worker, t) for t in tasks]):
                frame, png = future.result()
                cache_put((fld, a, idx, frame), png)
                done += 1
                status.value = (f"<i>Prefetching <b>{fld}</b> ({a} = {layers[idx]:.3g}): "
                                f"{done}/{len(tasks)} frames (parallel) …</i>")
        except Exception:
            # Fall back to lazy rendering; draw() fills the cache on demand. A
            # dead executor would silently fail every future prefetch, so drop
            # it - the next prefetch starts a fresh pool.
            _reset_slice_pool()
        finally:
            set_busy(False)

    def draw():
        i = time_sl.value
        fld = field_dd.value
        a = axis_dd.value
        layers = slice_layers(base, a)
        idx = int(np.clip(slice_sl.value, 0, len(layers) - 1))
        key = (fld, a, idx, i)
        if key not in png_cache:
            cache_put(key, _slice_png(
                solution(i), fld, a, idx, cmap=cmap, width=width, clim=clim_for(fld),
                title=f"{a} = {layers[idx]:.3g}   (t = {times[i]:g})"))
        image.value = png_cache[key]
        status.value = (f"frame <b>{i}</b> / {last} &nbsp;&bull;&nbsp; t = {times[i]:g} "
                        f"&nbsp;&bull;&nbsp; field = <b>{fld}</b> "
                        f"&nbsp;&bull;&nbsp; slice {a} = {layers[idx]:.3g}")

    def on_time(_):
        if not state["playing"]:
            draw()

    def on_slice(_):
        if state["playing"]:
            return
        draw()       # the new slice position shows immediately ...
        prefetch()   # ... then its remaining time frames warm up in parallel
        draw()       # restore the status line after the progress messages

    def on_axis(change):
        layers = slice_layers(base, change["new"])
        old_value = slice_sl.value
        slice_sl.max = len(layers) - 1  # auto-clamps the value, firing on_slice
        if slice_sl.value == old_value and not state["playing"]:
            on_slice(None)  # value unchanged -> no observer fired -> update here

    def on_field(_):
        if state["playing"]:
            return
        if time_sl.value != 0:
            time_sl.value = 0  # a new field starts from the first frame (draws via on_time)
        on_slice(None)

    async def play_loop():
        try:
            i = time_sl.value if time_sl.value < last else 0
            while i <= last and state["playing"]:
                time_sl.value = i
                draw()
                await asyncio.sleep(play_interval / 1000.0)
                i += 1
        finally:
            # Always release the controls - a draw() failure mid-playback must
            # not leave the viewer locked in the "playing" state.
            state["playing"] = False
            play_button.disabled = False
            stop_button.disabled = True

    def on_play(_):
        if state["playing"]:
            return
        state["playing"] = True
        play_button.disabled = True
        stop_button.disabled = False
        try:
            asyncio.get_running_loop().create_task(play_loop())
        except RuntimeError:
            asyncio.ensure_future(play_loop())

    def on_stop(_):
        state["playing"] = False

    time_sl.observe(on_time, names="value")
    slice_sl.observe(on_slice, names="value")
    field_dd.observe(on_field, names="value")
    axis_dd.observe(on_axis, names="value")
    play_button.on_click(on_play)
    stop_button.on_click(on_stop)

    display(widgets.VBox([
        widgets.HBox([field_dd, axis_dd, slice_sl]),
        widgets.HBox([play_button, stop_button, time_sl]),
        status, image,
    ]))
    draw()       # first frame appears immediately ...
    prefetch()   # ... then the remaining frames warm up in parallel
    draw()
    return None


def _main():
    import argparse

    parser = argparse.ArgumentParser(description="Render one ExaHyPE 2 HDF5 snapshot to a PNG.")
    parser.add_argument("--render", metavar="FILE", required=True, help="input .h5 snapshot")
    parser.add_argument("--field", required=True)
    parser.add_argument("--output", required=True, help="output .png path")
    parser.add_argument("--cmap", default="coolwarm")
    parser.add_argument("--width", type=int, default=720)
    parser.add_argument("--show-edges", action="store_true")
    parser.add_argument("--slice", metavar="AXIS", choices=["x", "y", "z"], default=None,
                        help="render an axis-aligned slice of a 3D snapshot (x/y/z)")
    parser.add_argument("--slice-index", type=int, default=None,
                        help="slice layer index along --slice (default: mid-plane)")
    parser.add_argument("--clim", default=None, help="fixed colour range 'lo,hi'")
    args = parser.parse_args()

    clim = tuple(float(v) for v in args.clim.split(",")) if args.clim else None
    if args.slice is not None:
        png = render_slice_image(args.render, args.field, axis=args.slice,
                                 index=args.slice_index, cmap=args.cmap, width=args.width)
    else:
        png = render_solution_image(args.render, args.field, show_edges=args.show_edges, clim=clim,
                                    cmap=args.cmap, width=args.width)
    Path(args.output).write_bytes(png)


if __name__ == "__main__":
    _main()
