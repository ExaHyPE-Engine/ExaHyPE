# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import re
from pathlib import Path

PNG_PATTERN = "*.png"
MACRO_BLOCK_SIZE = 16


def _natural_key(p: Path):
    parts = re.findall(r"\d+|\D+", p.name)
    return [int(s) if s.isdigit() else s.lower() for s in parts]


def _build_argparser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="Create an MP4 animation from PNG frames")
    ap.add_argument(
        "--input-dir", default=".", help="Directory containing PNG frames (default: .)"
    )
    ap.add_argument(
        "--output",
        default="animation.mp4",
        help="Output MP4 filename (default: animation.mp4)",
    )
    ap.add_argument(
        "--fps", type=float, default=24.0, help="Frames per second (default: 24)"
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="List the frames and the computed target size, then exit",
    )
    return ap


def _collect_frames(input_dir: Path):
    frames = sorted(input_dir.glob(PNG_PATTERN), key=_natural_key)
    return [p for p in frames if p.suffix.lower() == ".png" and p.is_file()]


def _round_up(n: int, base: int) -> int:
    return n if n % base == 0 else n + (base - n % base)


def _ensure_rgb_uint8(arr):
    import numpy as np

    if arr.ndim == 2:
        arr = np.stack([arr, arr, arr], axis=2)
    elif arr.ndim == 3 and arr.shape[2] == 4:
        arr = arr[..., :3]  # drop alpha
    # make sure dtype is uint8
    if arr.dtype != np.uint8:
        if np.issubdtype(arr.dtype, np.floating):
            arr = (arr.clip(0, 1) * 255.0).astype(np.uint8)
        else:
            arr = arr.astype(np.uint8)
    return arr


def _pad_to(img, target_h, target_w):
    import numpy as np

    h, w = img.shape[:2]
    top = (target_h - h) // 2
    bottom = target_h - h - top
    left = (target_w - w) // 2
    right = target_w - w - left
    if top < 0 or left < 0:
        raise SystemExit("Internal error: negative padding requested.")
    return np.pad(
        img, ((top, bottom), (left, right), (0, 0)), mode="constant", constant_values=0
    )


def _compute_target_size(frames):
    import imageio.v2 as imageio

    max_h = 0
    max_w = 0
    for f in frames:
        img = imageio.imread(f)
        img = _ensure_rgb_uint8(img)
        h, w = img.shape[:2]
        if h > max_h:
            max_h = h
        if w > max_w:
            max_w = w
    # round up to next multiple of MACRO_BLOCK_SIZE to avoid ffmpeg resizing warnings
    return _round_up(max_h, MACRO_BLOCK_SIZE), _round_up(max_w, MACRO_BLOCK_SIZE)


def _write_video(frames, output_path: Path, fps: float, target_h: int, target_w: int):
    import imageio.v2 as imageio

    writer = imageio.get_writer(
        output_path.as_posix(),
        fps=fps,
        format="FFMPEG",
        codec="libx264",
        pixelformat="yuv420p",
    )

    try:
        for i, f in enumerate(frames, start=1):
            img = imageio.imread(f)
            img = _ensure_rgb_uint8(img)
            h, w = img.shape[:2]

            # pad if needed to the common (macro-aligned) canvas
            if (h, w) != (target_h, target_w):
                img = _pad_to(img, target_h, target_w)

            writer.append_data(img)
            if i % 25 == 0 or i == len(frames):
                print(f"Write {i}/{len(frames)} frames", flush=True)
    finally:
        writer.close()


def main():
    # import imageio_ffmpeg  # noqa: F401
    args = _build_argparser().parse_args()

    input_dir = Path(args.input_dir)
    if not input_dir.exists():
        raise SystemExit(f"ERROR: Input directory not found: {input_dir}")

    frames = _collect_frames(input_dir)
    if not frames:
        raise SystemExit("ERROR: No PNG frames found to assemble.")

    target_h, target_w = _compute_target_size(frames)

    if args.dry_run:
        print(f"Frames in '{input_dir}' matching '{PNG_PATTERN}':")
        for p in frames:
            print(" -", p.name)
        print(f"Total: {len(frames)}")
        print(
            f"Target canvas (HxW): {target_h} x {target_w} (rounded to multiple of {MACRO_BLOCK_SIZE})"
        )
        return

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    print(
        f"Assembling {len(frames)} frames from '{input_dir}' into '{output_path}' at {args.fps} fps"
    )
    print(f"Target size (HxW): {target_h}x{target_w} (multiple of {MACRO_BLOCK_SIZE})")
    _write_video(frames, output_path, args.fps, target_h, target_w)
    print("Wrote:", output_path.resolve())


if __name__ == "__main__":
    main()
