#!/usr/bin/env python3

"""Compare two solution files bitwise and report the first mismatch."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
import xml.etree.ElementTree as ET


NETCDF_SUFFIXES = {".nc", ".nc4", ".netcdf"}
CHUNK_SIZE = 1024 * 1024


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare two solution files and report the first mismatch"
    )
    parser.add_argument("left", help="First solution file")
    parser.add_argument("right", help="Second solution file")
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    left = Path(args.left)
    right = Path(args.right)

    left_kind = detect_input_kind(left)
    right_kind = detect_input_kind(right)

    if left_kind != right_kind:
        mismatch = (
            f'Mismatch: Input types differ: "{left}" is {left_kind}, '
            f'but "{right}" is {right_kind}.'
        )
    elif left_kind == "xmf":
        mismatch = compare_xmf_graph(left, right)
    else:
        mismatch = compare_by_kind(left, right, left_kind)

    if mismatch is None:
        print(f'Files match: "{left}" and "{right}".')
        raise SystemExit(0)

    print(mismatch, file=sys.stderr)
    raise SystemExit(1)


def detect_input_kind(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix == ".xmf":
        return "xmf"
    if suffix == ".h5":
        return "hdf5"
    if suffix == ".vtu":
        return "vtu"
    if suffix in NETCDF_SUFFIXES:
        return "netcdf"
    return suffix.lstrip(".") or "unknown"


def compare_xmf_graph(left: Path, right: Path) -> str | None:
    mismatch = compare_file_pair(left, right)
    if mismatch is not None:
        return mismatch

    left_refs = collect_xmf_references(left)
    right_refs = collect_xmf_references(right)

    if len(left_refs) != len(right_refs):
        return (
            f'Mismatch: Referenced file count differs for "{left}" and "{right}": '
            f"{len(left_refs)} != {len(right_refs)}."
        )

    for left_ref, right_ref in zip(left_refs, right_refs, strict=True):
        left_rel = left_ref.relative_to(left.parent)
        right_rel = right_ref.relative_to(right.parent)
        if left_rel.as_posix() != right_rel.as_posix():
            return (
                f'Mismatch: Referenced file names differ: "{left_rel}" '
                f'vs "{right_rel}".'
            )
        mismatch = compare_by_kind(left_ref, right_ref, detect_input_kind(left_ref))
        if mismatch is not None:
            return mismatch

    return None


def collect_xmf_references(path: Path) -> list[Path]:
    references: list[Path] = []
    seen: set[Path] = set()
    _collect_xmf_references(path.resolve(), references, seen)
    return references


def _collect_xmf_references(
    path: Path, references: list[Path], seen: set[Path]
) -> None:
    if path in seen:
        return
    seen.add(path)

    root = ET.parse(path).getroot()

    for include in root.findall(".//{http://www.w3.org/2001/XInclude}include"):
        href = include.attrib.get("href")
        if not href:
            continue
        target = (path.parent / href).resolve()
        references.append(target)
        if target.suffix.lower() == ".xmf":
            _collect_xmf_references(target, references, seen)

    for data_item in root.findall(".//DataItem"):
        text = data_item.text.strip() if data_item.text else ""
        if ":/" not in text:
            continue
        file_name, _ = text.split(":/", 1)
        target = (path.parent / file_name).resolve()
        if target not in references:
            references.append(target)


def compare_file_pair(left: Path, right: Path) -> str | None:
    if not left.exists():
        return f'Mismatch: File "{left}" does not exist.'
    if not right.exists():
        return f'Mismatch: File "{right}" does not exist.'

    left_size = left.stat().st_size
    right_size = right.stat().st_size
    if left_size != right_size:
        return (
            f'Mismatch: File sizes differ for "{left}" and "{right}": '
            f"{left_size} != {right_size} bytes."
        )

    with left.open("rb") as left_handle, right.open("rb") as right_handle:
        offset = 0
        while True:
            left_chunk = left_handle.read(CHUNK_SIZE)
            right_chunk = right_handle.read(CHUNK_SIZE)
            if not left_chunk and not right_chunk:
                return None
            if left_chunk == right_chunk:
                offset += len(left_chunk)
                continue

            mismatch_index = first_mismatch_index(left_chunk, right_chunk)
            absolute_offset = offset + mismatch_index
            left_byte = left_chunk[mismatch_index]
            right_byte = right_chunk[mismatch_index]
            return (
                f"Mismatch: First differing byte at offset {absolute_offset} "
                f'for "{left}" and "{right}": '
                f"{left_byte:#04x} != {right_byte:#04x}."
            )


def compare_by_kind(left: Path, right: Path, kind: str) -> str | None:
    if kind == "hdf5":
        return compare_hdf5_files(left, right)
    return compare_file_pair(left, right)


def compare_hdf5_files(left: Path, right: Path) -> str | None:
    import h5py

    if not left.exists():
        return f'Mismatch: File "{left}" does not exist.'
    if not right.exists():
        return f'Mismatch: File "{right}" does not exist.'

    with h5py.File(left, "r") as left_handle, h5py.File(right, "r") as right_handle:
        return compare_hdf5_groups(left_handle, right_handle, left, right, "")


def compare_hdf5_groups(
    left_group,
    right_group,
    left_path: Path,
    right_path: Path,
    prefix: str,
) -> str | None:
    left_keys = sorted(left_group.keys())
    right_keys = sorted(right_group.keys())
    if left_keys != right_keys:
        return (
            f'Mismatch: HDF5 object names differ under "{prefix or "/"}" '
            f'for "{left_path}" and "{right_path}": {left_keys} != {right_keys}.'
        )
    if dict(left_group.attrs) != dict(right_group.attrs):
        return (
            f'Mismatch: HDF5 attributes differ for "{prefix or "/"}" '
            f'in "{left_path}" and "{right_path}".'
        )

    for key in left_keys:
        child_prefix = f"{prefix}/{key}" if prefix else key
        left_obj = left_group[key]
        right_obj = right_group[key]
        mismatch = compare_hdf5_objects(
            left_obj, right_obj, left_path, right_path, child_prefix
        )
        if mismatch is not None:
            return mismatch
    return None


def compare_hdf5_objects(
    left_obj,
    right_obj,
    left_path: Path,
    right_path: Path,
    object_name: str,
) -> str | None:
    import h5py
    import numpy as np

    if isinstance(left_obj, h5py.Dataset) != isinstance(right_obj, h5py.Dataset):
        return (
            f'Mismatch: HDF5 object type differs for "{object_name}" '
            f'in "{left_path}" and "{right_path}".'
        )

    if isinstance(left_obj, h5py.Dataset):
        if left_obj.shape != right_obj.shape or left_obj.dtype != right_obj.dtype:
            return (
                f'Mismatch: HDF5 dataset metadata differs for "{object_name}" '
                f'in "{left_path}" and "{right_path}": '
                f"shape {left_obj.shape} != {right_obj.shape} or "
                f"dtype {left_obj.dtype} != {right_obj.dtype}."
            )
        if dict(left_obj.attrs) != dict(right_obj.attrs):
            return (
                f'Mismatch: HDF5 dataset attributes differ for "{object_name}" '
                f'in "{left_path}" and "{right_path}".'
            )
        left_data = np.asarray(left_obj)
        right_data = np.asarray(right_obj)
        equal = np.array_equal(left_data, right_data, equal_nan=True)
        if not equal:
            mismatch_index = first_array_mismatch_index(left_data, right_data)
            return (
                f'Mismatch: HDF5 dataset values differ for "{object_name}" '
                f'in "{left_path}" and "{right_path}" at index {mismatch_index}: '
                f"{left_data[mismatch_index]!r} != {right_data[mismatch_index]!r}."
            )
        return None

    return compare_hdf5_groups(left_obj, right_obj, left_path, right_path, object_name)


def first_array_mismatch_index(left_array, right_array):
    import numpy as np

    left_flat = np.ravel(left_array)
    right_flat = np.ravel(right_array)
    mismatch_positions = np.flatnonzero(left_flat != right_flat)
    if mismatch_positions.size == 0:
        mismatch_positions = np.flatnonzero(~(left_flat == right_flat))
    flat_index = int(mismatch_positions[0])
    return np.unravel_index(flat_index, left_array.shape)


def first_mismatch_index(left_chunk: bytes, right_chunk: bytes) -> int:
    for index, (left_byte, right_byte) in enumerate(
        zip(left_chunk, right_chunk, strict=True)
    ):
        if left_byte != right_byte:
            return index
    if len(left_chunk) != len(right_chunk):
        return min(len(left_chunk), len(right_chunk))
    raise ValueError("Chunks are identical")


if __name__ == "__main__":
    main()
