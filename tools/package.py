#!/usr/bin/env python3
"""Create a reproducible SBAModLoader pack from the native library and optional DEX."""

import argparse
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

STAMP = (2026, 1, 1, 0, 0, 0)
LIBRARY_NAME = "libsba_physics.so"


def write_entry(archive: ZipFile, name: str, data: bytes) -> None:
    info = ZipInfo(name, STAMP)
    info.compress_type = ZIP_DEFLATED
    info.external_attr = 0o644 << 16
    archive.writestr(info, data, compresslevel=9)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--library", type=Path, default=Path("build") / LIBRARY_NAME)
    parser.add_argument("--dex", type=Path)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("dist") / "SBA-Costume-Map-Maker.sbapack",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    manifest_path = root / "sbapack.json"
    library_path = args.library if args.library.is_absolute() else root / args.library
    output_path = args.output if args.output.is_absolute() else root / args.output

    manifest = manifest_path.read_bytes()
    json.loads(manifest.decode("utf-8"))
    if not library_path.is_file():
        raise SystemExit(f"Native library not found: {library_path}")

    dex_path = None
    if args.dex is not None:
        dex_path = args.dex if args.dex.is_absolute() else root / args.dex
        if not dex_path.is_file():
            raise SystemExit(f"DEX file not found: {dex_path}")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output_path, "w") as archive:
        write_entry(archive, "sbapack.json", manifest)
        write_entry(archive, LIBRARY_NAME, library_path.read_bytes())
        if dex_path is not None:
            write_entry(archive, "overlay/classes.dex", dex_path.read_bytes())


if __name__ == "__main__":
    main()
