# Peano Visualisation Module

This module converts Peano patch outputs (single files or meta files containing multiple **snapshots**) into publication‑quality figures or ParaView datasets. It supports 2D and 3D data, slicing, selecting components (“unknowns”), optional filtering, and parallel processing of snapshots.

## TL;DR

If you use Peano's native patch file output, choose one the following functions to convert to your desired output format:

```bash
python convert-to-vtu DATA.peano-patch-file
python convert-to-png DATA.peano-patch-file
python convert-to-pdf DATA.peano-patch-file
python convert-to-svg DATA.peano-patch-file
python convert-to-pgf DATA.peano-patch-file
```

## Overview

At a high level:

- **Parsing**: `PatchFileParser` reads Peano patch files, capturing offsets, sizes, degrees of freedom (DoFs), unknowns and an optional geometric mapping per unknown. It builds per‑unknown data structures used later during rendering.
- **Orchestration**: `Visualiser` loads a meta file, groups included patch files into snapshots, and drives parsing for each snapshot.
- **Backends**:
  - `MatplotlibVisualiser` renders to **PNG / PDF / PGF / SVG** via Matplotlib (with PyVista as a helper for 3D).
  - `VTUUnstructuredGrid` writes **.vtu** per snapshot and a **.pvd** “glue” file (timeline) per unknown for ParaView.

The command‑line entry points are thin wrappers that select the backend/format and share a common CLI.

## What gets rendered (2D vs 3D)

- **2D**: Matplotlib draws either a fast `imshow` path for cell‑values or a polygon collection; patch boundaries may be overlaid.
- **3D**:
  - **Raster outputs (PNG/JPG)**: PyVista renders the unstructured grid off‑screen and screenshots it.
  - **Vector outputs (PDF/SVG/PGF)**: the outer surface is extracted and converted to Matplotlib 3D polygons for a true vector figure.

Optional 3D features include axis‑aligned **slicing** and **camera flip** to mirror views along an axis.

## CLI

All converters share the same arguments (see `ArgumentParser`). Key flags:

```
positional:
  filename                    Path to a Peano patch meta file OR a single patch file

optional:
  -s, --single-file           Treat input as a single patch file (no snapshots)
  --dir DIR                   Output directory (default: .)
  --dpi DPI                   DPI for rasterization / PGF raster layers (default: 300)
  -u, --unknown N             Index of the unknown/component to plot (default: 0)
  --slice-axis {x,y,z}        3D: axis to slice along
  --slice-position FLOAT      3D: slice center (if omitted, an auto center of the refined region is used)
  --slice-width FLOAT         3D: width of the slice (default: 0.01)
  --flip-view {x,y,z}         Mirror camera around axis
  -j, --max-workers [N]       Process snapshots in parallel (default: 1)
  -start N -end M             Restrict snapshot index range (end=-1 means “until last”)

filters:
  --filter-fine-grid          Show only finest grid
  --filter-average            Average cell data (reduces resolution/memory)
  --norm-calculator           Replace vector components by their norm
  --shrink-cells FACTOR       Shrink/expand cells for visual separation
```

This list may not be complete. Use `--help` to see all flags.

## Commands & examples

> Replace `DATA.peano-patch-file` by your meta file. Use `-s` if you provide a single patch file instead.

### PNG (2D or 3D, raster)

```bash
python convert-to-png.py DATA.peano-patch-file --dir out --dpi 200 -u 0
# 3D with slicing (example: Z-slice of width 0.05)
python convert-to-png.py DATA.peano-patch-file --slice-axis z --slice-width 0.05 --dir out
```

### PDF (vector)

```bash
python convert-to-pdf.py DATA.peano-patch-file --dir figs -u 1
```

For 2D, Matplotlib’s **PGF backend** is used (LaTeX typesetting). For 3D, the grid surface is converted to Matplotlib polygons for a vector PDF.

### PGF (for LaTeX inclusion)

```bash
python convert-to-pgf.py DATA.peano-patch-file --dir figs --dpi 300
```

The file `*.pgf` can be included in LaTeX with `\input{...}`. The backend sets `text.usetex=True` and `pgf.texsystem=pdflatex`.

### SVG (vector)

```bash
python convert-to-svg.py DATA.peano-patch-file --dir figs
```

Uses the same PGF/LaTeX path for 2D to keep fonts/labels consistent.

### VTU/PVD (ParaView)

```bash
python convert-to-vtu.py DATA.peano-patch-file --dir paraview
```

Writes one `.vtu` per snapshot and one `.pvd` per unknown aggregating the time series. Be aware of relative paths when the input lives in a different directory; the writer warns about this scenario.

## Requirements

### Python

- **Core**: `numpy`, `matplotlib`
- **3D raster output**: `pyvista` and `vtk` (PyVista uses VTK for off‑screen rendering).
- **VTU conversion**: `vtk` (to emit `.vtu`/`.pvd`).

Install with pip, for example:

```bash
pip install numpy matplotlib pyvista vtk
```

### LaTeX (for PGF/PDF)

For `--format pgf` and for 2D **PDF/SVG** paths the code switches Matplotlib to the **PGF backend** and asks LaTeX to typeset labels (`text.usetex=True`, `pgf.texsystem=pdflatex`). Your TeX installation must provide at least:

- `pdflatex` executable
- Packages: `fontenc`, `inputenc`, `amsmath`, and for PGF output also `siunitx` and `xcolor`

```bash
sudo apt install texlive-latex-base texlive-latex-recommended texlive-latex-extra texlive-fonts-recommended texlive-science texlive-pictures texlive-fonts-extra dvipng
```

> Tip: If you get LaTeX errors, test a minimal `pdflatex` run and ensure the listed packages are installed in your TeX distro.

### File types & inputs

- **Meta file**: contains multiple datasets with timestamps and `include` lines to the actual patch files. The module parses snapshots from this structure.
- **Single patch file**: pass `-s/--single-file` to force single‑snapshot mode.

## Animation helper (optional)

With `create-animation-from-png.py`, you can turn a folder of `*.png` frames into an MP4:

```bash
python create-animation-from-png.py --input-dir out --output anim.mp4 --fps 24
```

This script requires `imageio` + `imageio-ffmpeg`.
