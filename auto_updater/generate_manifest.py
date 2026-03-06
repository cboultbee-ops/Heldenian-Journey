#!/usr/bin/env python3
"""Generate a SHA-256 manifest of all files in the client distribution folder.

Usage:
    python generate_manifest.py --version 0.1.0 --source "../Client_Dist"
    python generate_manifest.py --version 0.2.0 --source "../Client_Dist" --output manifest.json
"""

import argparse
import hashlib
import json
import os
import sys
from datetime import datetime, timezone


# Files/dirs to exclude from the manifest (case-insensitive basenames)
EXCLUDE_NAMES = {
    "thumbs.db", "desktop.ini", ".ds_store",
    "manifest.json", "updater_config.json",
    "launcher.exe", "launcher_update.exe",
    # Debug / dev artifacts
    "client_d.exe", "play_me.exe.bak",
    "color_debug.txt", "mapchange_debug.txt",
    "client.map",
}

# Directory basenames to skip entirely
EXCLUDE_DIRS = {
    ".git", "__pycache__", "sprites(original)", "save",
}

# File extensions to skip
EXCLUDE_EXTS = {
    ".pdb", ".ilk", ".obj", ".log", ".bak",
}


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(1 << 16)  # 64 KB
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def should_exclude(rel_path: str, basename: str) -> bool:
    if basename.lower() in EXCLUDE_NAMES:
        return True
    _, ext = os.path.splitext(basename)
    if ext.lower() in EXCLUDE_EXTS:
        return True
    return False


def scan_directory(source_dir: str):
    files = []
    source_dir = os.path.normpath(source_dir)

    for dirpath, dirnames, filenames in os.walk(source_dir):
        # Prune excluded directories (modifying dirnames in-place)
        dirnames[:] = [
            d for d in dirnames
            if d.lower() not in EXCLUDE_DIRS
        ]

        for fname in sorted(filenames):
            if should_exclude(
                os.path.relpath(os.path.join(dirpath, fname), source_dir),
                fname,
            ):
                continue

            full_path = os.path.join(dirpath, fname)
            rel_path = os.path.relpath(full_path, source_dir)
            # Normalize to forward slashes for cross-platform consistency
            rel_path = rel_path.replace("\\", "/")

            file_size = os.path.getsize(full_path)
            file_hash = sha256_file(full_path)

            files.append({
                "path": rel_path,
                "sha256": file_hash,
                "size": file_size,
            })

    return files


def main():
    parser = argparse.ArgumentParser(
        description="Generate a SHA-256 file manifest for the Helbreath client distribution."
    )
    parser.add_argument(
        "--version", required=True,
        help="Version string (e.g. 0.1.0)"
    )
    parser.add_argument(
        "--source", required=True,
        help="Path to the client distribution folder"
    )
    parser.add_argument(
        "--output", default=None,
        help="Output manifest path (default: <source>/manifest.json)"
    )
    parser.add_argument(
        "--launcher-version", default=None,
        help="Launcher version string for self-update (e.g. 1.0.0)"
    )
    args = parser.parse_args()

    source = os.path.abspath(args.source)
    if not os.path.isdir(source):
        print(f"Error: source directory not found: {source}", file=sys.stderr)
        sys.exit(1)

    output = args.output or os.path.join(source, "manifest.json")

    print(f"Scanning: {source}")
    print(f"Version:  {args.version}")

    files = scan_directory(source)
    total_size = sum(f["size"] for f in files)

    manifest = {
        "version": args.version,
        "build_date": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "file_count": len(files),
        "total_size": total_size,
        "files": files,
    }

    if args.launcher_version:
        manifest["launcher_version"] = args.launcher_version

    with open(output, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"Manifest written: {output}")
    print(f"  Files:      {len(files)}")
    print(f"  Total size: {total_size / (1024*1024):.1f} MB")


if __name__ == "__main__":
    main()
