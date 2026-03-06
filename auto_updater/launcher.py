#!/usr/bin/env python3
"""Heldenian Journey Launcher — auto-updater + game launcher.

Checks GitHub Releases for a newer manifest, downloads changed files,
then launches Play_Me.exe.

Compile to exe:
    pyinstaller --onefile --windowed --name Launcher --icon=icon.ico launcher.py
"""

import hashlib
import json
import os
import re
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import ttk, messagebox
from urllib.request import urlopen, Request
from urllib.error import URLError, HTTPError

# ---------------------------------------------------------------------------
# Paths — resolve relative to the launcher's own directory
# ---------------------------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(sys.argv[0]))
CONFIG_PATH = os.path.join(BASE_DIR, "updater_config.json")
LOCAL_MANIFEST = os.path.join(BASE_DIR, "manifest.json")
GM_CFG_PATH = os.path.join(BASE_DIR, "GM.cfg")
GAME_EXE = os.path.join(BASE_DIR, "Play_Me.exe")
LAUNCHER_VERSION = "1.1.0"

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
DEFAULT_CONFIG = {
    "update_url": "https://api.github.com/repos/cboultbee-ops/Heldenian-Journey/releases/latest",
    "channel": "beta",
}

RESOLUTIONS = [
    "640x480",
    "800x600",
    "1024x768",
    "1280x720",
    "1280x960",
    "1280x1024",
    "1366x768",
    "1600x900",
    "1920x1080",
    "2560x1440",
    "3840x2160",
]


# ===== GM.cfg helpers ======================================================

def read_gm_cfg() -> dict:
    """Read GM.cfg into a dict of key→value strings."""
    settings = {}
    try:
        with open(GM_CFG_PATH, "r") as f:
            for line in f:
                line = line.strip()
                if "=" in line and not line.startswith("["):
                    key, _, val = line.partition("=")
                    settings[key.strip()] = val.strip()
    except FileNotFoundError:
        pass
    return settings


def write_gm_cfg(settings: dict):
    """Write settings dict back to GM.cfg, preserving key order."""
    lines = ["[CONFIG]\n"]
    for key, val in settings.items():
        lines.append(f"{key}\t\t = {val}\n")
    with open(GM_CFG_PATH, "w") as f:
        f.writelines(lines)


# ===== Helper functions ====================================================

def load_json(path: str) -> dict | None:
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return None


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    try:
        with open(path, "rb") as f:
            while True:
                chunk = f.read(1 << 16)
                if not chunk:
                    break
                h.update(chunk)
    except FileNotFoundError:
        return ""
    return h.hexdigest()


def download_file(url: str, dest: str, progress_cb=None) -> bool:
    """Download *url* to *dest*, calling progress_cb(bytes_so_far, total) periodically."""
    os.makedirs(os.path.dirname(dest) or ".", exist_ok=True)
    tmp = dest + ".tmp"
    retries = 3
    for attempt in range(1, retries + 1):
        try:
            req = Request(url, headers={"User-Agent": "HeldenianJourneyLauncher/1.1"})
            resp = urlopen(req, timeout=30)
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            with open(tmp, "wb") as f:
                while True:
                    chunk = resp.read(1 << 16)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if progress_cb:
                        progress_cb(downloaded, total)
            # Atomic-ish rename
            if os.path.exists(dest):
                os.remove(dest)
            os.rename(tmp, dest)
            return True
        except Exception:
            if os.path.exists(tmp):
                os.remove(tmp)
            if attempt == retries:
                return False
    return False


def fetch_json(url: str) -> dict | None:
    try:
        req = Request(url, headers={
            "User-Agent": "HeldenianJourneyLauncher/1.1",
            "Accept": "application/vnd.github+json",
        })
        resp = urlopen(req, timeout=15)
        return json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        # Store last error for display
        fetch_json._last_error = str(e)
        return None

fetch_json._last_error = ""


def find_asset_url(release: dict, name: str) -> str | None:
    """Find the browser_download_url for an asset by filename."""
    for asset in release.get("assets", []):
        if asset["name"] == name:
            return asset["browser_download_url"]
    return None


# ===== Settings Dialog =====================================================

class SettingsDialog:
    def __init__(self, parent):
        self.result = None
        self.win = tk.Toplevel(parent)
        self.win.title("Display Settings")
        self.win.resizable(False, False)
        self.win.configure(bg="#1a1a2e")
        self.win.transient(parent)
        self.win.grab_set()

        w, h = 340, 220
        sx = parent.winfo_x() + (parent.winfo_width() - w) // 2
        sy = parent.winfo_y() + (parent.winfo_height() - h) // 2
        self.win.geometry(f"{w}x{h}+{sx}+{sy}")

        bg = "#1a1a2e"
        fg = "#e0e0e0"

        # Load current settings
        cfg = read_gm_cfg()
        current_res = cfg.get("resolution", "1280x960")
        current_mode = cfg.get("display-mode", "windowed")

        # Title
        tk.Label(
            self.win, text="Display Settings",
            font=("Segoe UI", 13, "bold"), bg=bg, fg="#c9a84c",
        ).pack(pady=(15, 10))

        # Resolution
        res_frame = tk.Frame(self.win, bg=bg)
        res_frame.pack(pady=5, padx=20, fill="x")
        tk.Label(res_frame, text="Resolution:", font=("Segoe UI", 10),
                 bg=bg, fg=fg, width=12, anchor="w").pack(side="left")
        self.res_var = tk.StringVar(value=current_res)
        res_combo = ttk.Combobox(res_frame, textvariable=self.res_var,
                                 values=RESOLUTIONS, state="readonly", width=14)
        res_combo.pack(side="left", padx=(5, 0))
        # If current res isn't in list, add it
        if current_res not in RESOLUTIONS:
            res_combo["values"] = [current_res] + RESOLUTIONS

        # Display mode
        mode_frame = tk.Frame(self.win, bg=bg)
        mode_frame.pack(pady=5, padx=20, fill="x")
        tk.Label(mode_frame, text="Display Mode:", font=("Segoe UI", 10),
                 bg=bg, fg=fg, width=12, anchor="w").pack(side="left")
        self.mode_var = tk.StringVar(value=current_mode)
        mode_combo = ttk.Combobox(mode_frame, textvariable=self.mode_var,
                                  values=["windowed", "fullscreen"], state="readonly", width=14)
        mode_combo.pack(side="left", padx=(5, 0))

        # Buttons
        btn_frame = tk.Frame(self.win, bg=bg)
        btn_frame.pack(pady=(20, 10))

        tk.Button(
            btn_frame, text="Save", font=("Segoe UI", 10, "bold"),
            bg="#2d6a2d", fg="white", activebackground="#3d8a3d",
            width=10, command=self._save,
        ).pack(side="left", padx=8)

        tk.Button(
            btn_frame, text="Cancel", font=("Segoe UI", 10),
            bg="#4a4a4a", fg="white", activebackground="#5a5a5a",
            width=10, command=self.win.destroy,
        ).pack(side="left", padx=8)

    def _save(self):
        cfg = read_gm_cfg()
        cfg["resolution"] = self.res_var.get()
        cfg["display-mode"] = self.mode_var.get()
        write_gm_cfg(cfg)
        self.result = True
        self.win.destroy()


# ===== Launcher GUI ========================================================

class LauncherApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Heldenian Journey Launcher")
        self.root.resizable(False, False)
        self.root.configure(bg="#1a1a2e")

        # Center window
        w, h = 500, 350
        sx = (self.root.winfo_screenwidth() - w) // 2
        sy = (self.root.winfo_screenheight() - h) // 2
        self.root.geometry(f"{w}x{h}+{sx}+{sy}")

        self._build_ui()
        self._update_thread = None

    # ----- UI setup --------------------------------------------------------

    def _build_ui(self):
        bg = "#1a1a2e"
        fg = "#e0e0e0"

        # Title
        tk.Label(
            self.root, text="Heldenian Journey",
            font=("Segoe UI", 18, "bold"), bg=bg, fg="#c9a84c",
        ).pack(pady=(25, 5))

        tk.Label(
            self.root, text="Beta Launcher",
            font=("Segoe UI", 10), bg=bg, fg="#808080",
        ).pack()

        # Current settings display
        cfg = read_gm_cfg()
        res = cfg.get("resolution", "1280x960")
        mode = cfg.get("display-mode", "windowed")
        self.settings_var = tk.StringVar(value=f"{res}  {mode}")
        tk.Label(
            self.root, textvariable=self.settings_var,
            font=("Segoe UI", 9), bg=bg, fg="#607090",
        ).pack()

        # Status
        self.status_var = tk.StringVar(value="Initializing...")
        tk.Label(
            self.root, textvariable=self.status_var,
            font=("Segoe UI", 10), bg=bg, fg=fg,
        ).pack(pady=(15, 5))

        # File progress
        self.file_var = tk.StringVar(value="")
        tk.Label(
            self.root, textvariable=self.file_var,
            font=("Segoe UI", 9), bg=bg, fg="#a0a0a0",
        ).pack()

        # Progress bar
        style = ttk.Style()
        style.theme_use("clam")
        style.configure(
            "Gold.Horizontal.TProgressbar",
            troughcolor="#2a2a4a", background="#c9a84c", thickness=20,
        )
        self.progress = ttk.Progressbar(
            self.root, style="Gold.Horizontal.TProgressbar",
            orient="horizontal", length=420, mode="determinate",
        )
        self.progress.pack(pady=(10, 5))

        # Overall label
        self.overall_var = tk.StringVar(value="")
        tk.Label(
            self.root, textvariable=self.overall_var,
            font=("Segoe UI", 9), bg=bg, fg="#a0a0a0",
        ).pack()

        # Buttons frame
        btn_frame = tk.Frame(self.root, bg=bg)
        btn_frame.pack(pady=(15, 10))

        self.play_btn = tk.Button(
            btn_frame, text="Play", font=("Segoe UI", 12, "bold"),
            bg="#2d6a2d", fg="white", activebackground="#3d8a3d",
            width=10, state="disabled", command=self._launch_game,
        )
        self.play_btn.pack(side="left", padx=8)

        self.settings_btn = tk.Button(
            btn_frame, text="Settings", font=("Segoe UI", 10),
            bg="#3a3a6a", fg="white", activebackground="#4a4a8a",
            width=8, command=self._open_settings,
        )
        self.settings_btn.pack(side="left", padx=8)

        self.quit_btn = tk.Button(
            btn_frame, text="Quit", font=("Segoe UI", 10),
            bg="#4a2020", fg="white", activebackground="#6a3030",
            width=8, command=self.root.destroy,
        )
        self.quit_btn.pack(side="left", padx=8)

        # Version label
        tk.Label(
            self.root, text=f"Launcher v{LAUNCHER_VERSION}",
            font=("Segoe UI", 8), bg=bg, fg="#505050",
        ).pack(side="bottom", pady=(0, 5))

    # ----- Settings --------------------------------------------------------

    def _open_settings(self):
        dlg = SettingsDialog(self.root)
        self.root.wait_window(dlg.win)
        if dlg.result:
            cfg = read_gm_cfg()
            res = cfg.get("resolution", "1280x960")
            mode = cfg.get("display-mode", "windowed")
            self.settings_var.set(f"{res}  {mode}")

    # ----- Update logic (runs in background thread) ------------------------

    def start(self):
        self._update_thread = threading.Thread(target=self._do_update, daemon=True)
        self._update_thread.start()
        self.root.mainloop()

    def _set_status(self, text):
        self.root.after(0, lambda: self.status_var.set(text))

    def _set_file(self, text):
        self.root.after(0, lambda: self.file_var.set(text))

    def _set_overall(self, text):
        self.root.after(0, lambda: self.overall_var.set(text))

    def _set_progress(self, value):
        self.root.after(0, lambda: self.progress.configure(value=value))

    def _enable_play(self):
        self.root.after(0, lambda: self.play_btn.configure(state="normal"))

    def _do_update(self):
        # Load config
        config = load_json(CONFIG_PATH) or DEFAULT_CONFIG
        update_url = config.get("update_url", DEFAULT_CONFIG["update_url"])

        # Fetch latest release
        self._set_status("Checking for updates...")
        release = fetch_json(update_url)

        if release is None:
            err = fetch_json._last_error
            self._set_status("Could not reach update server")
            self._set_file(f"Playing with current files  ({err[:60]})" if err else "Playing with current files...")
            self._enable_play()
            return

        # Find manifest asset
        manifest_url = find_asset_url(release, "manifest.json")
        if manifest_url is None:
            self._set_status("No manifest found in latest release")
            self._set_file("Playing with current files...")
            self._enable_play()
            return

        # Download remote manifest
        remote_manifest = fetch_json(manifest_url)
        if remote_manifest is None:
            self._set_status("Failed to download manifest")
            self._set_file("Playing with current files...")
            self._enable_play()
            return

        remote_version = remote_manifest.get("version", "?")
        local_manifest = load_json(LOCAL_MANIFEST)
        local_version = local_manifest.get("version", "0.0.0") if local_manifest else "0.0.0"

        # Build lookup of local file hashes
        local_hashes = {}
        if local_manifest:
            for f in local_manifest.get("files", []):
                local_hashes[f["path"]] = f["sha256"]

        # Determine which files need updating
        to_download = []
        for rf in remote_manifest.get("files", []):
            rpath = rf["path"]
            local_path = os.path.join(BASE_DIR, rpath.replace("/", os.sep))

            needs_update = False
            if rpath not in local_hashes:
                needs_update = True
            elif local_hashes[rpath] != rf["sha256"]:
                needs_update = True
            elif not os.path.exists(local_path):
                needs_update = True
            else:
                # Verify actual file hash matches manifest
                actual_hash = sha256_file(local_path)
                if actual_hash != rf["sha256"]:
                    needs_update = True

            if needs_update:
                to_download.append(rf)

        if not to_download:
            self._set_status(f"Up to date! (v{remote_version})")
            self._set_file("")
            self._set_progress(100)
            self._enable_play()
            # Save remote manifest locally (version sync)
            with open(LOCAL_MANIFEST, "w", encoding="utf-8") as f:
                json.dump(remote_manifest, f, indent=2)
            return

        # Download changed files
        total_bytes = sum(f["size"] for f in to_download)
        downloaded_bytes = 0
        n_files = len(to_download)

        self._set_status(f"Updating to v{remote_version}  ({n_files} file{'s' if n_files != 1 else ''})")

        for i, rf in enumerate(to_download, 1):
            rpath = rf["path"]
            fname = os.path.basename(rpath)
            self._set_file(f"Downloading {fname} ({i}/{n_files})")

            # Find asset URL in the release — try flattened path name first
            asset_url = find_asset_url(release, rpath.replace("/", "_"))
            if asset_url is None:
                # Try the original filename as-is
                asset_url = find_asset_url(release, os.path.basename(rpath))
            if asset_url is None:
                asset_url = find_asset_url(release, rpath)

            if asset_url is None:
                self._set_file(f"Skipped {fname} (not in release assets)")
                downloaded_bytes += rf["size"]
                continue

            dest = os.path.join(BASE_DIR, rpath.replace("/", os.sep))

            file_start = downloaded_bytes

            def on_progress(so_far, total_file, _base=file_start, _total=total_bytes):
                overall = _base + so_far
                pct = (overall / _total * 100) if _total > 0 else 0
                self._set_progress(min(pct, 100))
                self._set_overall(f"{overall / (1024*1024):.1f} / {_total / (1024*1024):.1f} MB")

            ok = download_file(asset_url, dest, progress_cb=on_progress)
            if not ok:
                self._set_file(f"Failed to download {fname} — skipping")

            downloaded_bytes += rf["size"]

        # Save updated manifest
        with open(LOCAL_MANIFEST, "w", encoding="utf-8") as f:
            json.dump(remote_manifest, f, indent=2)

        self._set_status(f"Updated to v{remote_version}!")
        self._set_file("")
        self._set_progress(100)
        self._set_overall("")
        self._enable_play()

    # ----- Launch game -----------------------------------------------------

    def _launch_game(self):
        if not os.path.exists(GAME_EXE):
            messagebox.showerror(
                "Error",
                f"Game executable not found:\n{GAME_EXE}\n\n"
                "Please redownload the full client package.",
            )
            return
        try:
            subprocess.Popen([GAME_EXE], cwd=BASE_DIR)
        except Exception as e:
            messagebox.showerror("Launch Error", str(e))
            return
        self.root.destroy()


# ===== Entry point =========================================================

def main():
    app = LauncherApp()
    app.start()


if __name__ == "__main__":
    main()
