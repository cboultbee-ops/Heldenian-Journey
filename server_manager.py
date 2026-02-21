import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import os
import threading
import time
import datetime

BASE_DIR = r"C:\Helbreath Project"

SERVERS = {
    "Login": {
        "exe": os.path.join(BASE_DIR, "Login", "Login.exe"),
        "cwd": os.path.join(BASE_DIR, "Login"),
        "process_name": "Login.exe",
    },
    "Towns": {
        "exe": os.path.join(BASE_DIR, "Maps", "Towns", "HGserver.exe"),
        "cwd": os.path.join(BASE_DIR, "Maps", "Towns"),
        "process_name": "HGserver.exe",
        "settings": os.path.join(BASE_DIR, "Maps", "Towns", "Settings.cfg"),
    },
    "Neutrals": {
        "exe": os.path.join(BASE_DIR, "Maps", "Neutrals", "HGserver.exe"),
        "cwd": os.path.join(BASE_DIR, "Maps", "Neutrals"),
        "process_name": "HGserver.exe",
        "settings": os.path.join(BASE_DIR, "Maps", "Neutrals", "Settings.cfg"),
    },
    "Middleland": {
        "exe": os.path.join(BASE_DIR, "Maps", "Middleland", "HGserver.exe"),
        "cwd": os.path.join(BASE_DIR, "Maps", "Middleland"),
        "process_name": "HGserver.exe",
        "settings": os.path.join(BASE_DIR, "Maps", "Middleland", "Settings.cfg"),
    },
    "Events": {
        "exe": os.path.join(BASE_DIR, "Maps", "Events", "HGserver.exe"),
        "cwd": os.path.join(BASE_DIR, "Maps", "Events"),
        "process_name": "HGserver.exe",
        "settings": os.path.join(BASE_DIR, "Maps", "Events", "Settings.cfg"),
    },
}

CLIENT_EXE = os.path.join(BASE_DIR, "Client", "Play_Me.exe")
CLIENT_CWD = os.path.join(BASE_DIR, "Client")
CLIENT_CFG = os.path.join(BASE_DIR, "Client", "GM.cfg")
MYSQL_BIN = r"C:\Program Files\MySQL\MySQL Server 8.0\bin"
GM_COMMANDS_FILE = os.path.join(BASE_DIR, "GM_COMMANDS.md")

# --- Settings Definitions ---
# (config_key, display_label, min_val, max_val)

SERVER_SETTINGS = [
    ("exp-modifier", "EXP Modifier", 1, 10000),
    ("max-player-level", "Max Player Level", 1, 800),
    ("primary-drop-rate", "Primary Drop Rate", 1, 10000),
    ("secondary-drop-rate", "Secondary Drop Rate", 1, 10000),
    ("character-skill-limit", "Skill Point Limit", 1, 5000),
    ("character-stat-limit", "Stat Limit", 1, 5000),
    ("slate-success-rate", "Slate Success Rate", 1, 100),
    ("enemy-kill-adjust", "Enemy Kill Adjust", 1, 100),
    ("rep-drop-modifier", "Rep Drop Modifier", 0, 100),
]

WEAPON_SPEED_SETTINGS = [
    ("min-speed-axe", "Min Speed (Axe)", 0, 10, 3),
    ("min-speed-longsword", "Min Speed (Long Sword)", 0, 10, 2),
    ("min-speed-fencing", "Min Speed (Fencing)", 0, 10, 1),
    ("min-speed-shortsword", "Min Speed (Short Sword)", 0, 10, 0),
    ("min-speed-archery", "Min Speed (Archery)", 0, 10, 1),
]

MOVEMENT_SETTINGS = [
    ("walk-speed", "Walk Speed (ms/frame)", 10, 200, 70),
    ("run-speed", "Run Speed (ms/frame)", 10, 200, 42),
    ("dash-speed", "Dash Speed (ms/frame)", 10, 200, 30),
]

FREQUENCY_SETTINGS = [
    ("attack-frequency-min", "Attack Freq Min (ms)", 0, 2000, 0),
    ("move-frequency-min", "Move Freq Min (ms)", 0, 1000, 0),
    ("magic-frequency-min", "Magic Freq Min (ms)", 0, 5000, 1500),
]

SUPER_ATTACK_SETTINGS = [
    ("super-attack-multiplier", "Super Attack Multiplier (%)", 0, 500, 100),
]

ATTACK_SPEED_SETTINGS = [
    ("attack-speed-multiplier", "Attack Speed Multiplier (%)", 50, 400, 100),
]


# --- Utility Functions ---

def is_process_running(name):
    try:
        output = subprocess.check_output(
            ["tasklist", "/FI", f"IMAGENAME eq {name}"],
            creationflags=subprocess.CREATE_NO_WINDOW,
            text=True, timeout=5,
        )
        return name.lower() in output.lower()
    except Exception:
        return False


def get_pids(name):
    try:
        output = subprocess.check_output(
            ["tasklist", "/FI", f"IMAGENAME eq {name}", "/FO", "CSV", "/NH"],
            creationflags=subprocess.CREATE_NO_WINDOW,
            text=True, timeout=5,
        )
        pids = []
        for line in output.strip().splitlines():
            parts = line.strip('"').split('","')
            if len(parts) >= 2 and parts[0].lower() == name.lower():
                pids.append(int(parts[1]))
        return pids
    except Exception:
        return []


def read_cfg(path):
    settings = {}
    try:
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if "=" in line and not line.startswith("//") and not line.startswith("["):
                    key, _, val = line.partition("=")
                    settings[key.strip()] = val.strip()
    except FileNotFoundError:
        pass
    return settings


def write_cfg(path, settings):
    lines = []
    try:
        with open(path, "r") as f:
            lines = f.readlines()
    except FileNotFoundError:
        return

    written_keys = set()
    new_lines = []
    for line in lines:
        stripped = line.strip()
        if "=" in stripped and not stripped.startswith("//") and not stripped.startswith("["):
            key = stripped.split("=")[0].strip()
            if key in settings:
                new_lines.append(f"{key}\t\t = {settings[key]}\n")
                written_keys.add(key)
                continue
        new_lines.append(line)

    # Append any new keys not found in file
    for key, val in settings.items():
        if key not in written_keys:
            new_lines.append(f"{key}\t\t = {val}\n")

    with open(path, "w") as f:
        f.writelines(new_lines)


# --- Scrollable Frame Helper ---

class ScrollableFrame(ttk.Frame):
    """A vertically scrollable frame for tkinter."""
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)

        self.canvas = tk.Canvas(self, highlightthickness=0, borderwidth=0)
        self.scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.canvas.yview)
        self.inner = ttk.Frame(self.canvas)

        self.inner.bind("<Configure>",
                        lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))

        self._canvas_window = self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)

        self.canvas.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")

        self.canvas.bind("<Configure>", self._on_canvas_configure)
        self.inner.bind("<Enter>", self._bind_mousewheel)
        self.inner.bind("<Leave>", self._unbind_mousewheel)

    def _on_canvas_configure(self, event):
        self.canvas.itemconfig(self._canvas_window, width=event.width)

    def _bind_mousewheel(self, event):
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)

    def _unbind_mousewheel(self, event):
        self.canvas.unbind_all("<MouseWheel>")

    def _on_mousewheel(self, event):
        self.canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")


# --- Main Application ---

class ServerManager(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Helbreath Server Manager")
        self.geometry("680x900")
        self.minsize(600, 800)

        self.status_dots = {}
        self.setting_vars = {}
        self.weapon_vars = {}
        self.movement_vars = {}
        self.frequency_vars = {}
        self.super_attack_vars = {}
        self.attack_speed_vars = {}
        self.display_mode_var = tk.StringVar(value="windowed")
        self.resolution_var = tk.StringVar(value="1280x960")
        self.logout_timer_var = tk.StringVar(value="10")
        self.instant_logout_var = tk.IntVar(value=0)

        self._build_top_bar()
        self._build_tabs()
        self._build_log_area()

        self._refresh_status()

    # ---- Top Bar (always visible) ----
    def _build_top_bar(self):
        frame = ttk.Frame(self, padding=(10, 8, 10, 4))
        frame.pack(fill="x")

        # Buttons row
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill="x")

        start_btn = ttk.Button(btn_frame, text="Start All", command=self._start_all)
        start_btn.pack(side="left", padx=3)
        stop_btn = ttk.Button(btn_frame, text="Stop All", command=self._stop_all)
        stop_btn.pack(side="left", padx=3)
        restart_btn = ttk.Button(btn_frame, text="Restart All", command=self._restart_all)
        restart_btn.pack(side="left", padx=3)
        ttk.Button(btn_frame, text="Launch Client", command=self._launch_client).pack(side="right", padx=3)

        # Status indicators row
        status_frame = ttk.Frame(frame)
        status_frame.pack(fill="x", pady=(6, 0))

        for name in ["Login", "Towns", "Neutrals", "Middleland", "Events"]:
            inner = ttk.Frame(status_frame)
            inner.pack(side="left", padx=12)
            dot = tk.Canvas(inner, width=14, height=14, highlightthickness=0)
            dot.create_oval(2, 2, 12, 12, fill="gray", tags="dot")
            dot.pack(side="left", padx=(0, 4))
            ttk.Label(inner, text=name, font=("Segoe UI", 8)).pack(side="left")
            self.status_dots[name] = dot

        ttk.Separator(self, orient="horizontal").pack(fill="x", padx=10, pady=4)

    # ---- Tabbed Notebook ----
    def _build_tabs(self):
        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=(0, 4))

        self._build_tab_server_settings()
        self._build_tab_gameplay_tuning()
        self._build_tab_display()
        self._build_tab_quick_actions()
        self._build_tab_gm_commands()

    # ---- Tab 1: Server Settings ----
    def _build_tab_server_settings(self):
        tab = ttk.Frame(self.notebook, padding=10)
        self.notebook.add(tab, text="Server Settings")

        ttk.Label(tab, text="Applied to all 4 game servers (Towns, Neutrals, Middleland, Events)",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w")

        # Read reference settings
        ref = read_cfg(SERVERS["Neutrals"]["settings"])

        settings_frame = ttk.Frame(tab)
        settings_frame.pack(fill="x", pady=(6, 0))

        for i, (key, label, lo, hi) in enumerate(SERVER_SETTINGS):
            row = ttk.Frame(settings_frame)
            row.pack(fill="x", pady=2)

            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            var = tk.StringVar(value=ref.get(key, ""))
            entry = ttk.Entry(row, textvariable=var, width=10)
            entry.pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo} - {hi})", foreground="gray",
                      font=("Segoe UI", 8)).pack(side="left")
            self.setting_vars[key] = var

        btn_frame = ttk.Frame(tab)
        btn_frame.pack(fill="x", pady=(12, 0))
        ttk.Button(btn_frame, text="Save Settings", command=self._save_server_settings).pack(side="left", padx=3)
        ttk.Button(btn_frame, text="Save & Restart", command=self._save_and_restart_server).pack(side="left", padx=3)

    # ---- Tab 2: Gameplay Tuning ----
    def _build_tab_gameplay_tuning(self):
        tab = ttk.Frame(self.notebook, padding=0)
        self.notebook.add(tab, text="Gameplay Tuning")

        # Pinned buttons at top (always visible)
        btn_frame = ttk.Frame(tab, padding=(10, 8, 10, 4))
        btn_frame.pack(fill="x")
        ttk.Button(btn_frame, text="Save Settings", command=self._save_gameplay_settings).pack(side="left", padx=3)
        ttk.Button(btn_frame, text="Save & Restart", command=self._save_and_restart_gameplay).pack(side="left", padx=3)
        ttk.Label(btn_frame, text="Settings applied to all 4 game servers",
                  foreground="gray", font=("Segoe UI", 8)).pack(side="right", padx=5)

        ttk.Separator(tab, orient="horizontal").pack(fill="x", padx=10)

        # Scrollable content area
        scroll = ScrollableFrame(tab)
        scroll.pack(fill="both", expand=True, padx=5, pady=5)
        content = scroll.inner

        # Read reference settings
        ref = read_cfg(SERVERS["Neutrals"]["settings"])
        client_cfg = read_cfg(CLIENT_CFG)

        # --- Dev Toggles Section ---
        toggle_frame = ttk.LabelFrame(content, text="Dev Toggles", padding=8)
        toggle_frame.pack(fill="x", padx=5, pady=(5, 4))

        ttk.Label(toggle_frame,
                  text="Quick toggles for testing. Requires server restart to take effect.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        self.recall_timer_var = tk.IntVar(value=int(ref.get("recall-damage-timer", "1")))
        self.recall_timer_btn = ttk.Checkbutton(
            toggle_frame, text="Recall Damage Timer",
            variable=self.recall_timer_var,
            command=self._on_recall_timer_toggle)
        self.recall_timer_btn.pack(anchor="w", pady=1)

        self.recall_timer_label = ttk.Label(toggle_frame, text="", font=("Segoe UI", 8))
        self.recall_timer_label.pack(anchor="w", padx=(20, 0))
        self._update_recall_timer_label()

        # --- Combat Speed Section ---
        combat_frame = ttk.LabelFrame(content, text="Combat Speed (per-weapon minimums)", padding=8)
        combat_frame.pack(fill="x", padx=5, pady=4)

        ttk.Label(combat_frame,
                  text="Higher = slower attack. 0 = fastest possible.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        for key, label, lo, hi, default in WEAPON_SPEED_SETTINGS:
            row = ttk.Frame(combat_frame)
            row.pack(fill="x", pady=1)
            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            var = tk.StringVar(value=ref.get(key, str(default)))
            ttk.Entry(row, textvariable=var, width=6).pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo}-{hi})  def: {default}",
                      foreground="gray", font=("Segoe UI", 8)).pack(side="left")
            self.weapon_vars[key] = var

        # --- Super Attack Section ---
        sa_frame = ttk.LabelFrame(content, text="Super Attack", padding=8)
        sa_frame.pack(fill="x", padx=5, pady=4)

        ttk.Label(sa_frame,
                  text="100 = normal. 200 = double. 50 = half.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        for key, label, lo, hi, default in SUPER_ATTACK_SETTINGS:
            row = ttk.Frame(sa_frame)
            row.pack(fill="x", pady=1)
            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            var = tk.StringVar(value=ref.get(key, str(default)))
            ttk.Entry(row, textvariable=var, width=6).pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo}-{hi})  def: {default}",
                      foreground="gray", font=("Segoe UI", 8)).pack(side="left")
            self.super_attack_vars[key] = var

        # --- Attack Speed Section ---
        atk_frame = ttk.LabelFrame(content, text="Attack Speed (server + client)", padding=8)
        atk_frame.pack(fill="x", padx=5, pady=4)

        ttk.Label(atk_frame,
                  text="100 = normal. 200 = double speed. 400 = quad speed. Applies to all weapons.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        for key, label, lo, hi, default in ATTACK_SPEED_SETTINGS:
            row = ttk.Frame(atk_frame)
            row.pack(fill="x", pady=1)
            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            # Read from client cfg (GM.cfg) since it's written there too
            combined_default = client_cfg.get(key, ref.get(key, str(default)))
            var = tk.StringVar(value=combined_default)
            ttk.Entry(row, textvariable=var, width=6).pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo}-{hi})  def: {default}",
                      foreground="gray", font=("Segoe UI", 8)).pack(side="left")
            self.attack_speed_vars[key] = var

        # --- Movement Section ---
        move_frame = ttk.LabelFrame(content, text="Movement & Dash Speed (client)", padding=8)
        move_frame.pack(fill="x", padx=5, pady=4)

        ttk.Label(move_frame,
                  text="Lower = faster. Saved to GM.cfg. Restart client to apply.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        for key, label, lo, hi, default in MOVEMENT_SETTINGS:
            row = ttk.Frame(move_frame)
            row.pack(fill="x", pady=1)
            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            var = tk.StringVar(value=client_cfg.get(key, str(default)))
            ttk.Entry(row, textvariable=var, width=6).pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo}-{hi})  def: {default}",
                      foreground="gray", font=("Segoe UI", 8)).pack(side="left")
            self.movement_vars[key] = var

        # Logout timer (numeric entry)
        row_lt = ttk.Frame(move_frame)
        row_lt.pack(fill="x", pady=1)
        ttk.Label(row_lt, text="Logout Timer (seconds)", width=24, anchor="w").pack(side="left")
        ttk.Entry(row_lt, textvariable=self.logout_timer_var, width=6).pack(side="left", padx=5)
        ttk.Label(row_lt, text="(1-60)  def: 10", foreground="gray",
                  font=("Segoe UI", 8)).pack(side="left")

        # Instant logout checkbox (underneath timer entry)
        timer_val = client_cfg.get("logout-timer", "10")
        self.logout_timer_var.set(timer_val)
        self.instant_logout_var.set(1 if timer_val in ("0", "1") else 0)
        row_instant = ttk.Frame(move_frame)
        row_instant.pack(fill="x", pady=(1, 4))
        ttk.Label(row_instant, text="", width=24).pack(side="left")  # spacer to align with entry
        ttk.Checkbutton(row_instant, text="Instant Logout (1s)",
                        variable=self.instant_logout_var,
                        command=self._on_instant_logout_toggle).pack(side="left", padx=5)

        # --- Frequency Checks Section ---
        freq_frame = ttk.LabelFrame(content, text="Speed Check Thresholds (server)", padding=8)
        freq_frame.pack(fill="x", padx=5, pady=4)

        ttk.Label(freq_frame,
                  text="Anti-speedhack. 0 = disabled.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 4))

        for key, label, lo, hi, default in FREQUENCY_SETTINGS:
            row = ttk.Frame(freq_frame)
            row.pack(fill="x", pady=1)
            ttk.Label(row, text=label, width=24, anchor="w").pack(side="left")
            var = tk.StringVar(value=ref.get(key, str(default)))
            ttk.Entry(row, textvariable=var, width=6).pack(side="left", padx=5)
            ttk.Label(row, text=f"({lo}-{hi})  def: {default}",
                      foreground="gray", font=("Segoe UI", 8)).pack(side="left")
            self.frequency_vars[key] = var


    # ---- Tab: Display ----
    def _build_tab_display(self):
        tab = ttk.Frame(self.notebook, padding=10)
        self.notebook.add(tab, text="Display")

        client_cfg = read_cfg(CLIENT_CFG)

        ttk.Label(tab, text="Client display settings. Saved to GM.cfg. Restart client to apply.",
                  foreground="gray", font=("Segoe UI", 8)).pack(anchor="w", pady=(0, 8))

        # --- Display Mode ---
        mode_frame = ttk.LabelFrame(tab, text="Display Mode", padding=8)
        mode_frame.pack(fill="x", pady=(0, 8))

        self.display_mode_var.set(client_cfg.get("display-mode", "windowed"))
        ttk.Radiobutton(mode_frame, text="Windowed",
                        variable=self.display_mode_var, value="windowed",
                        command=self._on_display_mode_change).pack(anchor="w", pady=2)
        ttk.Radiobutton(mode_frame, text="Borderless Fullscreen",
                        variable=self.display_mode_var, value="fullscreen",
                        command=self._on_display_mode_change).pack(anchor="w", pady=2)

        # --- Resolution ---
        res_frame = ttk.LabelFrame(tab, text="Resolution (windowed mode)", padding=8)
        res_frame.pack(fill="x", pady=(0, 8))

        self.res_note_label = ttk.Label(res_frame,
                  text="Select window size. Ignored in fullscreen (uses monitor native resolution).",
                  foreground="gray", font=("Segoe UI", 8))
        self.res_note_label.pack(anchor="w", pady=(0, 4))

        RESOLUTIONS = [
            "640x480",
            "1280x960",
            "1600x1200",
            "1920x1080",
            "1920x1440",
            "2560x1440",
        ]

        self.resolution_var.set(client_cfg.get("resolution", "1280x960"))
        row = ttk.Frame(res_frame)
        row.pack(fill="x", pady=2)
        ttk.Label(row, text="Resolution", width=16, anchor="w").pack(side="left")
        self.res_combo = ttk.Combobox(row, textvariable=self.resolution_var,
                                       values=RESOLUTIONS, width=14, state="readonly")
        self.res_combo.pack(side="left", padx=5)

        # Ensure current value is in the list; if not, add it
        current_res = self.resolution_var.get()
        if current_res not in RESOLUTIONS:
            self.res_combo["values"] = RESOLUTIONS + [current_res]

        self._on_display_mode_change()  # Set initial state

        # --- Save Button ---
        btn_frame = ttk.Frame(tab)
        btn_frame.pack(fill="x", pady=(12, 0))
        ttk.Button(btn_frame, text="Save Display Settings",
                   command=self._save_display_settings).pack(side="left", padx=3)

    def _on_display_mode_change(self):
        is_windowed = self.display_mode_var.get() == "windowed"
        state = "readonly" if is_windowed else "disabled"
        self.res_combo.config(state=state)

    def _save_display_settings(self):
        settings = {
            "display-mode": self.display_mode_var.get(),
            "resolution": self.resolution_var.get(),
        }
        write_cfg(CLIENT_CFG, settings)
        mode = self.display_mode_var.get()
        res = self.resolution_var.get()
        if mode == "fullscreen":
            self._log(f"Display: borderless fullscreen (saved to GM.cfg)")
        else:
            self._log(f"Display: windowed {res} (saved to GM.cfg)")

    def _on_instant_logout_toggle(self):
        if self.instant_logout_var.get():
            self.logout_timer_var.set("1")
        else:
            self.logout_timer_var.set("10")
        write_cfg(CLIENT_CFG, {"logout-timer": self.logout_timer_var.get()})
        self._log(f"Logout timer set to {self.logout_timer_var.get()}s (saved to GM.cfg)")

    def _on_recall_timer_toggle(self):
        val = self.recall_timer_var.get()
        for name in ["Towns", "Neutrals", "Middleland", "Events"]:
            path = SERVERS[name].get("settings")
            if path:
                write_cfg(path, {"recall-damage-timer": str(val)})
        self._update_recall_timer_label()
        state = "ON" if val else "OFF"
        self._log(f"Recall Damage Timer: {state} (saved to all Settings.cfg — restart server to apply)")

    def _update_recall_timer_label(self):
        if self.recall_timer_var.get():
            self.recall_timer_label.config(
                text="ON — 10s delay after damage before Recall works (normal)",
                foreground="#2ecc40")
        else:
            self.recall_timer_label.config(
                text="OFF — Instant Recall even after taking damage (testing)",
                foreground="#ff4136")

    # ---- Tab 3: Quick Actions ----
    def _build_tab_quick_actions(self):
        tab = ttk.Frame(self.notebook, padding=10)
        self.notebook.add(tab, text="Quick Actions")

        actions = [
            ("Open Server Logs", self._open_logs),
            ("Open Config Files", self._open_configs),
            ("Open Source Code", self._open_source),
            ("Database Backup", self._db_backup),
            ("Git Commit & Push", self._git_commit_push),
            ("Open GM Commands", self._open_gm_commands),
        ]

        for label, cmd in actions:
            btn = ttk.Button(tab, text=label, command=cmd, width=30)
            btn.pack(pady=3, anchor="w")

    # ---- Tab 4: GM Commands ----
    def _build_tab_gm_commands(self):
        tab = ttk.Frame(self.notebook, padding=0)
        self.notebook.add(tab, text="GM Commands")

        # Search bar at top (always visible)
        search_frame = ttk.Frame(tab, padding=(10, 8, 10, 4))
        search_frame.pack(fill="x")
        ttk.Label(search_frame, text="Filter:").pack(side="left", padx=(0, 5))
        self.gm_search_var = tk.StringVar()
        self.gm_search_var.trace_add("write", self._filter_gm_commands)
        search_entry = ttk.Entry(search_frame, textvariable=self.gm_search_var, width=30)
        search_entry.pack(side="left", padx=(0, 8))
        ttk.Label(search_frame, text="Type to search commands",
                  foreground="gray", font=("Segoe UI", 8)).pack(side="left")

        ttk.Separator(tab, orient="horizontal").pack(fill="x", padx=10)

        # Scrollable text area
        text_frame = ttk.Frame(tab)
        text_frame.pack(fill="both", expand=True, padx=10, pady=(4, 10))

        scrollbar = ttk.Scrollbar(text_frame, orient="vertical")
        scrollbar.pack(side="right", fill="y")

        self.gm_text = tk.Text(
            text_frame, font=("Consolas", 10), wrap="word",
            bg="#1e1e1e", fg="#cccccc", insertbackground="#cccccc",
            state="disabled", yscrollcommand=scrollbar.set,
            padx=10, pady=10, spacing1=1, spacing3=1,
        )
        self.gm_text.pack(fill="both", expand=True)
        scrollbar.config(command=self.gm_text.yview)

        # Configure text tags for formatting
        self.gm_text.tag_configure("category", font=("Segoe UI", 11, "bold"),
                                   foreground="#4fc3f7", spacing1=10, spacing3=4)
        self.gm_text.tag_configure("subcategory", font=("Segoe UI", 9, "bold"),
                                   foreground="#81c784", spacing1=6, spacing3=2)
        self.gm_text.tag_configure("command", font=("Consolas", 10, "bold"),
                                   foreground="#ffcc80")
        self.gm_text.tag_configure("description", font=("Consolas", 9),
                                   foreground="#aaaaaa")
        self.gm_text.tag_configure("note", font=("Segoe UI", 9, "italic"),
                                   foreground="#888888", lmargin1=20, lmargin2=20)
        self.gm_text.tag_configure("separator", font=("Consolas", 8),
                                   foreground="#555555")

        # Store command data for filtering
        self._gm_command_data = self._get_gm_commands()
        self._render_gm_commands(self._gm_command_data)

        # Bind mousewheel for scrolling
        self.gm_text.bind("<Enter>", lambda e: self.gm_text.bind_all("<MouseWheel>",
                          lambda ev: self.gm_text.yview_scroll(int(-1 * (ev.delta / 120)), "units")))
        self.gm_text.bind("<Leave>", lambda e: self.gm_text.unbind_all("<MouseWheel>"))

    def _get_gm_commands(self):
        """Return GM command data as a list of (category, entries) tuples.
        Each entry is (command_syntax, description) or a special tuple for notes."""
        return [
            ("Item & NPC Management", {
                "note": "Requires AdminLevel >= 1",
                "commands": [
                    ("/createitem <item> <attr> <manuendu> <amount>",
                     "Create an item (full syntax)"),
                    ("/ci <item> <attr> <manuendu> <amount>",
                     "Create an item (short version)"),
                    ("/summon <npcname>",
                     "Spawn an NPC or monster"),
                    ("/unsummonall",
                     "Remove all summoned NPCs"),
                    ("/unsummonboss",
                     "Remove summoned bosses"),
                    ("/createfish <type>",
                     "Spawn a fish"),
                ],
                "extra": [
                    ("createitem attribute values:", [
                        "0 = normal item",
                        "1 = manufactured (manuendu = quality 1-200, 100=normal, 200=best)",
                        "",
                        "For non-manufactured items, attribute is a hex bitmask:",
                        "  Bits 20-23 encode the prefix type:",
                        "  1048576   (1<<20)  = Poisoning (green)",
                        "  6291456   (6<<20)  = Ancient",
                        "  8388608   (8<<20)  = Light",
                        "  9437184   (9<<20)  = Sharp",
                        "  10485760  (10<<20) = Strong",
                        "",
                        "Examples:",
                        "  /ci GiantSword 1048576 0 1   Poisoning Giant Sword",
                        "  /ci GiantSword 6291456 0 1   Ancient Giant Sword",
                        "  /ci GiantSword 8388608 0 1   Light Giant Sword",
                    ]),
                ],
            }),
            ("Player Management", {
                "note": "Requires AdminLevel >= 1",
                "commands": [
                    ("/who",
                     "Show total online players"),
                    ("/goto <mapname> <x> <y>",
                     "Teleport yourself to location"),
                    ("/teleport <player>  |  /tp <player>",
                     "Teleport to a player"),
                    ("/summonplayer <player>",
                     "Bring a player to you"),
                    ("/send <player> <map> <x> <y>",
                     "Teleport a player somewhere"),
                    ("/closeconn <player>",
                     "Disconnect a player"),
                    ("/disconnectall",
                     "Disconnect all players"),
                    ("/shutup <player> <minutes>",
                     "Mute a player"),
                    ("/checkrep <player>",
                     "Check player reputation"),
                    ("/checkip <player>",
                     "Check player IP address"),
                    ("/ban <player>",
                     "Ban a player"),
                ],
            }),
            ("Character Modification", {
                "note": "Requires AdminLevel >= 1",
                "commands": [
                    ("/sethp <value>",
                     "Set your HP"),
                    ("/setmp <value>",
                     "Set your MP"),
                    ("/setmag <value>",
                     "Set your Magic stat"),
                    ("/polymorph <npcname>",
                     "Transform into an NPC"),
                    ("/setattackmode <mode>",
                     "Change attack mode"),
                    ("/rep+ <player> <amount>",
                     "Increase player reputation"),
                    ("/rep- <player> <amount>",
                     "Decrease player reputation"),
                ],
            }),
            ("GM Mode Toggles", {
                "note": "Requires AdminLevel >= 1",
                "commands": [
                    ("/noaggro",
                     "Toggle monster aggro on/off"),
                    ("/invincible",
                     "Toggle invincibility"),
                    ("/setinvi  |  /invi",
                     "Toggle invisibility"),
                ],
            }),
            ("Server Events", {
                "note": "AdminLevel requirements vary (noted per command)",
                "commands": [
                    ("/setforcerecalltime <minutes>",
                     "Set forced recall timer"),
                    ("/crusade",
                     "Start crusade (AdminLevel >= 4)"),
                    ("/apocalypse",
                     "Start apocalypse (AdminLevel >= 4)"),
                    ("/endapocalypse",
                     "End apocalypse (AdminLevel >= 4)"),
                    ("/heldenian",
                     "Start heldenian (AdminLevel >= 4)"),
                    ("/endheldenian",
                     "End heldenian (AdminLevel >= 4)"),
                    ("/energysphere",
                     "Spawn energy sphere (AdminLevel >= 2)"),
                    ("/reservefightzone",
                     "Reserve fight zone"),
                    ("/clearmap",
                     "Clear map of all NPCs"),
                    ("/mcount",
                     "Count apocalypse monsters"),
                ],
            }),
            ("Event System", {
                "note": "Requires AdminLevel >= 1",
                "commands": [
                    ("/eventspell <params>",
                     "Event: give spell"),
                    ("/eventarmor",
                     "Event: give armor"),
                    ("/eventshield",
                     "Event: give shield"),
                    ("/eventchat",
                     "Event: chat event"),
                    ("/eventparty",
                     "Event: party event"),
                    ("/eventreset",
                     "Event: reset events"),
                    ("/eventtp",
                     "Event: teleport event"),
                    ("/eventillusion",
                     "Event: illusion event"),
                ],
            }),
            ("Player Commands", {
                "note": "No admin level required -- available to all players",
                "commands": [
                    ("/afk",
                     "Toggle AFK status"),
                    ("/to <player>",
                     "Whisper toggle"),
                    ("/setpf <text>",
                     "Set player profile"),
                    ("/pf <player>",
                     "View player profile"),
                    ("/fi <player>",
                     "Check if player is online"),
                    ("/hold",
                     "Hold summoned mob"),
                    ("/tgt <target>",
                     "Set summoned mob target"),
                    ("/free",
                     "Free summoned mob"),
                    ("/ban <guildmember>",
                     "Ban from guild (guild master only)"),
                    ("/dissmiss <guildmember>",
                     "Dismiss from guild"),
                ],
            }),
        ]

    def _render_gm_commands(self, data, filter_text=""):
        """Render GM commands into the text widget."""
        self.gm_text.config(state="normal")
        self.gm_text.delete("1.0", "end")

        filter_lower = filter_text.lower().strip()
        any_match = False

        for category_name, category_data in data:
            commands = category_data.get("commands", [])
            note = category_data.get("note", "")
            extra = category_data.get("extra", [])

            # If filtering, check if any command in this category matches
            if filter_lower:
                matching_cmds = [
                    (cmd, desc) for cmd, desc in commands
                    if filter_lower in cmd.lower() or filter_lower in desc.lower()
                ]
                matching_extra = []
                for title, lines in extra:
                    if filter_lower in title.lower() or any(filter_lower in l.lower() for l in lines):
                        matching_extra.append((title, lines))
                if not matching_cmds and not matching_extra and filter_lower not in category_name.lower():
                    continue
                if not matching_cmds and filter_lower not in category_name.lower():
                    # Only extra matched, show all commands for context
                    matching_cmds = commands
            else:
                matching_cmds = commands
                matching_extra = extra

            any_match = True

            # Category header
            self.gm_text.insert("end", f"\n{category_name}\n", "category")
            if note:
                self.gm_text.insert("end", f"  {note}\n", "note")
            self.gm_text.insert("end", "  " + "-" * 60 + "\n", "separator")

            # Commands
            for cmd, desc in matching_cmds:
                self.gm_text.insert("end", f"  {cmd:<45}", "command")
                self.gm_text.insert("end", f"  {desc}\n", "description")

            # Extra info sections (like createitem attribute values)
            if not filter_lower:
                matching_extra = extra
            for title, lines in matching_extra:
                self.gm_text.insert("end", f"\n  {title}\n", "subcategory")
                for line in lines:
                    self.gm_text.insert("end", f"    {line}\n", "note")

            self.gm_text.insert("end", "\n")

        if not any_match and filter_lower:
            self.gm_text.insert("end", f"\n  No commands matching \"{filter_text.strip()}\"\n",
                                "description")

        self.gm_text.config(state="disabled")

    def _filter_gm_commands(self, *args):
        """Called when the search box text changes."""
        filter_text = self.gm_search_var.get()
        self._render_gm_commands(self._gm_command_data, filter_text)

    # ---- Log Area (always visible at bottom) ----
    def _build_log_area(self):
        ttk.Separator(self, orient="horizontal").pack(fill="x", padx=10)
        log_frame = ttk.Frame(self, padding=(10, 4, 10, 10))
        log_frame.pack(fill="both", expand=False)

        ttk.Label(log_frame, text="Log", font=("Segoe UI", 8, "bold")).pack(anchor="w")
        self.log_text = tk.Text(log_frame, height=8, font=("Consolas", 9),
                                state="disabled", bg="#1e1e1e", fg="#cccccc",
                                wrap="word")
        self.log_text.pack(fill="both")

    def _log(self, msg):
        def update():
            ts = datetime.datetime.now().strftime("%H:%M:%S")
            self.log_text.config(state="normal")
            self.log_text.insert("end", f"[{ts}] {msg}\n")
            self.log_text.see("end")
            self.log_text.config(state="disabled")
        self.after(0, update)

    # ---- Status Refresh ----
    def _refresh_status(self):
        def check():
            login_running = is_process_running("Login.exe")
            hg_count = len(get_pids("HGserver.exe"))
            self.after(0, lambda: self._update_dots(login_running, hg_count))
        threading.Thread(target=check, daemon=True).start()
        self.after(3000, self._refresh_status)

    def _update_dots(self, login_running, hg_count):
        color = "#2ecc40" if login_running else "#ff4136"
        self.status_dots["Login"].itemconfig("dot", fill=color)

        game_names = ["Towns", "Neutrals", "Middleland", "Events"]
        if hg_count >= 4:
            for name in game_names:
                self.status_dots[name].itemconfig("dot", fill="#2ecc40")
        elif hg_count == 0:
            for name in game_names:
                self.status_dots[name].itemconfig("dot", fill="#ff4136")
        else:
            for i, name in enumerate(game_names):
                color = "#2ecc40" if i < hg_count else "#ffdc00"
                self.status_dots[name].itemconfig("dot", fill=color)

    # ---- Server Control ----
    def _start_all(self):
        def run():
            self._log("Starting Login server...")
            try:
                subprocess.Popen(
                    [SERVERS["Login"]["exe"]],
                    cwd=SERVERS["Login"]["cwd"],
                    creationflags=subprocess.CREATE_NEW_CONSOLE,
                )
                self._log("Login server started")
            except Exception as e:
                self._log(f"Failed to start Login: {e}")
                return

            self._log("Waiting 3s for Login to initialize...")
            time.sleep(1)

            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                try:
                    subprocess.Popen(
                        [SERVERS[name]["exe"]],
                        cwd=SERVERS[name]["cwd"],
                        creationflags=subprocess.CREATE_NEW_CONSOLE,
                    )
                    self._log(f"{name} server started")
                except Exception as e:
                    self._log(f"Failed to start {name}: {e}")

            self._log("All servers started")
        threading.Thread(target=run, daemon=True).start()

    def _stop_all(self):
        def run():
            self._log("Stopping all servers...")
            for proc_name in ["HGserver.exe", "Login.exe"]:
                try:
                    subprocess.run(
                        ["taskkill", "/F", "/IM", proc_name],
                        creationflags=subprocess.CREATE_NO_WINDOW,
                        capture_output=True, timeout=10,
                    )
                    self._log(f"Killed {proc_name}")
                except Exception as e:
                    self._log(f"Could not kill {proc_name}: {e}")
            self._log("All servers stopped")
        threading.Thread(target=run, daemon=True).start()

    def _restart_all(self):
        def run():
            self._log("Restarting all servers...")
            for proc_name in ["HGserver.exe", "Login.exe"]:
                try:
                    subprocess.run(
                        ["taskkill", "/F", "/IM", proc_name],
                        creationflags=subprocess.CREATE_NO_WINDOW,
                        capture_output=True, timeout=10,
                    )
                except Exception:
                    pass

            time.sleep(2)
            self._log("Servers stopped, starting...")

            try:
                subprocess.Popen(
                    [SERVERS["Login"]["exe"]],
                    cwd=SERVERS["Login"]["cwd"],
                    creationflags=subprocess.CREATE_NEW_CONSOLE,
                )
            except Exception as e:
                self._log(f"Failed to start Login: {e}")
                return

            time.sleep(1)

            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                try:
                    subprocess.Popen(
                        [SERVERS[name]["exe"]],
                        cwd=SERVERS[name]["cwd"],
                        creationflags=subprocess.CREATE_NEW_CONSOLE,
                    )
                except Exception:
                    pass

            self._log("All servers restarted")
        threading.Thread(target=run, daemon=True).start()

    def _launch_client(self):
        try:
            subprocess.Popen(
                [CLIENT_EXE],
                cwd=CLIENT_CWD,
                creationflags=subprocess.CREATE_NEW_CONSOLE,
            )
            self._log("Client launched")
        except Exception as e:
            self._log(f"Failed to launch client: {e}")

    # ---- Save Functions ----
    def _save_server_settings(self):
        new_settings = {}
        for key, label, lo, hi in SERVER_SETTINGS:
            val = self.setting_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    new_settings[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        for name in ["Towns", "Neutrals", "Middleland", "Events"]:
            path = SERVERS[name].get("settings")
            if path:
                write_cfg(path, new_settings)

        self._log("Server settings saved to all 4 configs")

    def _save_gameplay_settings(self):
        # Weapon speeds -> Settings.cfg
        weapon_settings = {}
        for key, label, lo, hi, default in WEAPON_SPEED_SETTINGS:
            val = self.weapon_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    weapon_settings[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        # Frequency checks -> Settings.cfg
        for key, label, lo, hi, default in FREQUENCY_SETTINGS:
            val = self.frequency_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    weapon_settings[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        # Super attack -> Settings.cfg
        for key, label, lo, hi, default in SUPER_ATTACK_SETTINGS:
            val = self.super_attack_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    weapon_settings[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        # Attack speed -> Settings.cfg AND GM.cfg
        attack_speed_for_client = {}
        for key, label, lo, hi, default in ATTACK_SPEED_SETTINGS:
            val = self.attack_speed_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    weapon_settings[key] = str(num)
                    attack_speed_for_client[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        if weapon_settings:
            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                path = SERVERS[name].get("settings")
                if path:
                    write_cfg(path, weapon_settings)
            self._log("Server gameplay settings saved to all 4 configs")

        # Movement speeds -> GM.cfg
        client_settings = {}
        # Include attack speed in client settings too
        client_settings.update(attack_speed_for_client)
        for key, label, lo, hi, default in MOVEMENT_SETTINGS:
            val = self.movement_vars[key].get().strip()
            if val:
                try:
                    num = int(val)
                    if num < lo or num > hi:
                        messagebox.showwarning("Invalid Value",
                                               f"{label} must be between {lo} and {hi}")
                        return
                    client_settings[key] = str(num)
                except ValueError:
                    messagebox.showwarning("Invalid Value", f"{label} must be a number")
                    return

        # Logout timer -> GM.cfg
        timer_val = self.logout_timer_var.get().strip()
        try:
            timer_num = int(timer_val)
            if timer_num < 0 or timer_num > 60:
                messagebox.showwarning("Invalid Value", "Logout timer must be between 0 and 60")
                return
            client_settings["logout-timer"] = str(timer_num)
        except ValueError:
            messagebox.showwarning("Invalid Value", "Logout timer must be a number")
            return

        write_cfg(CLIENT_CFG, client_settings)
        self._log(f"Client settings saved to GM.cfg (logout: {timer_num}s)")

    def _save_and_restart_server(self):
        self._save_server_settings()
        self._restart_all()

    def _save_and_restart_gameplay(self):
        self._save_gameplay_settings()
        self._restart_all()

    # ---- Quick Actions ----
    def _open_logs(self):
        log_dir = os.path.join(BASE_DIR, "Maps", "Neutrals", "Logs")
        if not os.path.exists(log_dir):
            log_dir = os.path.join(BASE_DIR, "Maps")
        os.startfile(log_dir)

    def _open_configs(self):
        os.startfile(os.path.join(BASE_DIR, "Maps"))

    def _open_source(self):
        os.startfile(os.path.join(BASE_DIR, "Sources"))

    def _open_gm_commands(self):
        if os.path.exists(GM_COMMANDS_FILE):
            os.startfile(GM_COMMANDS_FILE)
        else:
            self._log("GM_COMMANDS.md not found")

    def _db_backup(self):
        def run():
            mysqldump = os.path.join(MYSQL_BIN, "mysqldump.exe")
            if not os.path.exists(mysqldump):
                self._log("ERROR: mysqldump not found at " + mysqldump)
                return

            backup_dir = os.path.join(BASE_DIR, "Backups")
            os.makedirs(backup_dir, exist_ok=True)

            ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            outfile = os.path.join(backup_dir, f"helbreath_backup_{ts}.sql")

            self._log("Running database backup...")
            try:
                result = subprocess.run(
                    [mysqldump, "-u", "root", "-palpha123", "helbreath"],
                    capture_output=True, text=True, timeout=60,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                if result.returncode == 0:
                    with open(outfile, "w") as f:
                        f.write(result.stdout)
                    size_kb = os.path.getsize(outfile) // 1024
                    self._log(f"Backup saved: {outfile} ({size_kb} KB)")
                else:
                    self._log(f"Backup failed: {result.stderr.strip()}")
            except Exception as e:
                self._log(f"Backup error: {e}")
        threading.Thread(target=run, daemon=True).start()

    def _git_commit_push(self):
        def run():
            self._log("Running git commit & push...")
            ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
            try:
                result = subprocess.run(
                    ["git", "add", "."],
                    cwd=BASE_DIR, capture_output=True, text=True, timeout=30,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                if result.returncode != 0:
                    self._log(f"git add failed: {result.stderr.strip()}")
                    return

                result = subprocess.run(
                    ["git", "commit", "-m", f"[auto] {ts}"],
                    cwd=BASE_DIR, capture_output=True, text=True, timeout=30,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                if result.returncode != 0:
                    self._log(f"git commit: {result.stdout.strip() or result.stderr.strip()}")
                    return

                self._log("Commit created, pushing...")
                result = subprocess.run(
                    ["git", "push"],
                    cwd=BASE_DIR, capture_output=True, text=True, timeout=60,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                if result.returncode == 0:
                    self._log("Git push successful")
                else:
                    self._log(f"git push: {result.stderr.strip()}")
            except Exception as e:
                self._log(f"Git error: {e}")
        threading.Thread(target=run, daemon=True).start()


if __name__ == "__main__":
    app = ServerManager()
    app.mainloop()
