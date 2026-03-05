#!/usr/bin/env python3
"""Publish a Helbreath client release to GitHub.

One-command workflow:
  1. (Optional) Compile the client
  2. Assemble the Client_Dist folder from Client/
  3. Generate manifest.json
  4. Create a GitHub Release and upload assets via `gh` CLI

Usage:
    python publish_release.py --version 0.1.0
    python publish_release.py --version 0.2.0 --compile
    python publish_release.py --version 0.1.0 --skip-upload   # local test only
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
LAUNCHER_VERSION = "1.0.0"
PROJECT_ROOT = Path(__file__).resolve().parent.parent
CLIENT_DIR = PROJECT_ROOT / "Client"
DIST_DIR = PROJECT_ROOT / "Client_Dist"
MANIFEST_GENERATOR = Path(__file__).resolve().parent / "generate_manifest.py"
TESTER_GM_CFG = Path(__file__).resolve().parent / "GM.cfg.tester"
REPO = "cboultbee-ops/Heldenian-Journey"

# What to copy from Client/ into Client_Dist/
COPY_FILES = [
    "Play_Me.exe",
    "glew32.dll",
    "cximage.dll",
    "search.dll",
    "msvcp100d.dll",
    "msvcr100d.dll",
    "classicconfig.ini",
    "config.ini",
    "equipsets.cfg",
    "menu_bg.png",
]

COPY_DIRS = [
    "CONTENTS",
    "FONTS",
    "MAPDATA",
    "MUSIC",
    "SOUNDS",
    "SPRITES",
    "shaders",
]

# Files to always skip inside copied dirs
SKIP_PATTERNS = {".bak", ".pdb", ".ilk", ".obj", ".log"}


def run(cmd, **kwargs):
    print(f"  > {cmd if isinstance(cmd, str) else ' '.join(cmd)}")
    return subprocess.run(cmd, **kwargs)


def compile_client():
    """Compile the client via MSBuild."""
    print("\n=== Compiling Client (Release) ===")
    msbuild = (
        r"C:\Program Files\Microsoft Visual Studio\2022\Community"
        r"\MSBuild\Current\Bin\MSBuild.exe"
    )
    sln = str(PROJECT_ROOT / "Sources" / "Client" / "Client.sln")
    result = run(
        ["powershell.exe", "-Command",
         f"& '{msbuild}' '{sln}' /p:Configuration=Release /p:Platform=Win32 /m /v:minimal /t:Rebuild"],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print("BUILD FAILED:")
        print(result.stdout)
        print(result.stderr)
        sys.exit(1)
    print("  Build succeeded.")


def assemble_dist():
    """Copy distribution files from Client/ to Client_Dist/."""
    print("\n=== Assembling Client_Dist ===")

    if DIST_DIR.exists():
        print(f"  Removing old {DIST_DIR}")
        shutil.rmtree(DIST_DIR)
    DIST_DIR.mkdir()

    # Copy individual files
    for fname in COPY_FILES:
        src = CLIENT_DIR / fname
        dst = DIST_DIR / fname
        if src.exists():
            shutil.copy2(src, dst)
            print(f"  {fname}")
        else:
            print(f"  SKIP (missing): {fname}")

    # Copy tester GM.cfg (public IP, windowed, safe defaults)
    if TESTER_GM_CFG.exists():
        shutil.copy2(TESTER_GM_CFG, DIST_DIR / "GM.cfg")
        print("  GM.cfg (tester config)")
    else:
        # Fallback: copy from Client
        fallback = CLIENT_DIR / "GM.cfg"
        if fallback.exists():
            shutil.copy2(fallback, DIST_DIR / "GM.cfg")
            print("  GM.cfg (WARNING: using local config, not tester config)")

    # Copy directories (filtering out .bak etc.)
    for dname in COPY_DIRS:
        src = CLIENT_DIR / dname
        dst = DIST_DIR / dname
        if not src.is_dir():
            print(f"  SKIP (missing dir): {dname}")
            continue

        file_count = 0
        for dirpath, dirnames, filenames in os.walk(src):
            # Skip backup directories
            dirnames[:] = [d for d in dirnames if d.lower() not in {
                "sprites(original)", ".git", "__pycache__", "save",
            }]

            rel_dir = os.path.relpath(dirpath, src)
            dest_dir = dst / rel_dir if rel_dir != "." else dst
            dest_dir.mkdir(parents=True, exist_ok=True)

            for fname in filenames:
                _, ext = os.path.splitext(fname)
                if ext.lower() in SKIP_PATTERNS:
                    continue
                shutil.copy2(
                    os.path.join(dirpath, fname),
                    dest_dir / fname,
                )
                file_count += 1

        print(f"  {dname}/ ({file_count} files)")

    # Copy updater_config.json if it exists, otherwise create default
    uc = DIST_DIR / "updater_config.json"
    uc_src = Path(__file__).resolve().parent / "updater_config.json"
    if uc_src.exists():
        shutil.copy2(uc_src, uc)
    else:
        with open(uc, "w") as f:
            json.dump({
                "update_url": f"https://api.github.com/repos/{REPO}/releases/latest",
                "channel": "beta",
            }, f, indent=2)
    print("  updater_config.json")

    # Copy Launcher.exe if it exists
    launcher = Path(__file__).resolve().parent / "dist" / "Launcher.exe"
    if launcher.exists():
        shutil.copy2(launcher, DIST_DIR / "Launcher.exe")
        print("  Launcher.exe")
    else:
        print("  SKIP: Launcher.exe not found (build it with PyInstaller first)")


def generate_manifest(version: str, launcher_version: str | None = None):
    """Run the manifest generator."""
    print("\n=== Generating Manifest ===")
    cmd = [
        sys.executable, str(MANIFEST_GENERATOR),
        "--version", version,
        "--source", str(DIST_DIR),
    ]
    if launcher_version:
        cmd += ["--launcher-version", launcher_version]

    result = run(cmd, capture_output=True, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print("MANIFEST GENERATION FAILED:")
        print(result.stderr)
        sys.exit(1)


def create_github_release(version: str, prerelease: bool = False):
    """Create a GitHub Release and upload the manifest + an update pack."""
    print("\n=== Creating GitHub Release ===")

    tag = f"v{version}"
    title = f"Heldenian Journey v{version} Beta"

    # Check if gh CLI is available
    try:
        run(["gh", "--version"], capture_output=True, check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("ERROR: GitHub CLI (gh) not found. Install it from https://cli.github.com/")
        print("Then run: gh auth login")
        sys.exit(1)

    # Create the release
    cmd = [
        "gh", "release", "create", tag,
        "--repo", REPO,
        "--title", title,
        "--notes", f"Beta release v{version}",
    ]
    if prerelease:
        cmd.append("--prerelease")

    result = run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        if "already exists" in result.stderr:
            print(f"  Release {tag} already exists — deleting and recreating...")
            run(["gh", "release", "delete", tag, "--repo", REPO, "--yes"],
                capture_output=True)
            run(["git", "tag", "-d", tag], capture_output=True, cwd=str(PROJECT_ROOT))
            run(["git", "push", "origin", f":refs/tags/{tag}"],
                capture_output=True, cwd=str(PROJECT_ROOT))
            result = run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print("FAILED to create release:")
                print(result.stderr)
                sys.exit(1)
        else:
            print("FAILED to create release:")
            print(result.stderr)
            sys.exit(1)

    release_url = result.stdout.strip()
    print(f"  Release created: {release_url}")

    # Upload manifest.json
    manifest_path = DIST_DIR / "manifest.json"
    print(f"\n  Uploading manifest.json...")
    result = run(
        ["gh", "release", "upload", tag, str(manifest_path),
         "--repo", REPO, "--clobber"],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"  WARNING: manifest upload failed: {result.stderr}")

    # Create a zip of the full distribution for first-time installs
    zip_name = f"HeldenianJourney-v{version}-beta"
    zip_path = PROJECT_ROOT / f"{zip_name}"
    print(f"\n  Creating {zip_name}.zip...")
    shutil.make_archive(str(zip_path), "zip", str(DIST_DIR.parent), DIST_DIR.name)

    zip_file = PROJECT_ROOT / f"{zip_name}.zip"
    zip_size = zip_file.stat().st_size / (1024 * 1024)
    print(f"  Zip size: {zip_size:.1f} MB")

    if zip_size > 2048:
        print("  WARNING: Zip exceeds GitHub's 2GB asset limit!")
        print("  Consider splitting SPRITES into a separate download.")
    else:
        print(f"  Uploading {zip_name}.zip...")
        result = run(
            ["gh", "release", "upload", tag, str(zip_file),
             "--repo", REPO, "--clobber"],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            print(f"  WARNING: zip upload failed: {result.stderr}")
        else:
            print("  Upload complete!")

    # Upload only CHANGED files as individual assets (for incremental updates)
    manifest = json.loads(manifest_path.read_text())
    prev_manifest_path = Path(__file__).resolve().parent / "prev_manifest.json"
    prev_hashes = {}
    if prev_manifest_path.exists():
        prev = json.loads(prev_manifest_path.read_text())
        for f in prev.get("files", []):
            prev_hashes[f["path"]] = f["sha256"]

    changed_files = []
    for finfo in manifest["files"]:
        if finfo["path"] not in prev_hashes or prev_hashes[finfo["path"]] != finfo["sha256"]:
            if finfo["size"] < 100 * 1024 * 1024:  # Under 100MB per asset
                changed_files.append(finfo)

    if changed_files:
        print(f"\n  Uploading {len(changed_files)} changed file(s) for incremental updates...")
        for i, finfo in enumerate(changed_files, 1):
            fpath = DIST_DIR / finfo["path"].replace("/", os.sep)
            asset_name = finfo["path"].replace("/", "_")
            if not fpath.exists():
                continue
            size_mb = finfo["size"] / (1024 * 1024)
            print(f"    [{i}/{len(changed_files)}] {finfo['path']} ({size_mb:.1f} MB)")
            result = run(
                ["gh", "release", "upload", tag, f"{fpath}#{asset_name}",
                 "--repo", REPO, "--clobber"],
                capture_output=True, text=True,
            )
            if result.returncode != 0:
                print(f"      WARNING: upload failed: {result.stderr.strip()}")
    else:
        print("\n  No changed files to upload individually (first release or no diff).")

    # Save current manifest as prev_manifest for next release comparison
    shutil.copy2(manifest_path, prev_manifest_path)
    print(f"  Saved manifest as baseline for next release.")

    print(f"\n=== Release v{version} published! ===")
    print(f"URL: {release_url}")
    return release_url


def main():
    parser = argparse.ArgumentParser(
        description="Build, package, and publish a Helbreath client release."
    )
    parser.add_argument("--version", required=True, help="Version string (e.g. 0.1.0)")
    parser.add_argument("--compile", action="store_true", help="Compile the client first")
    parser.add_argument("--skip-upload", action="store_true", help="Skip GitHub upload (local test)")
    parser.add_argument("--launcher-version", default=LAUNCHER_VERSION, help="Launcher version")
    args = parser.parse_args()

    print(f"Helbreath Release Publisher — v{args.version}")
    print(f"Project root: {PROJECT_ROOT}")

    if args.compile:
        compile_client()

    assemble_dist()
    generate_manifest(args.version, args.launcher_version)

    if args.skip_upload:
        print("\n=== Skipping GitHub upload (--skip-upload) ===")
        print(f"Distribution ready at: {DIST_DIR}")
    else:
        create_github_release(args.version)


if __name__ == "__main__":
    main()
