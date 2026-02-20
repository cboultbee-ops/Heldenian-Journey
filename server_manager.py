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
        self._build_tab_quick_actions()

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

        # Logout toggle row
        row_logout = ttk.Frame(toggle_frame)
        row_logout.pack(fill="x", pady=(6, 1))
        timer_val = client_cfg.get("logout-timer", "10")
        self.logout_timer_var.set(timer_val)
        self.instant_logout_var.set(1 if timer_val in ("0", "1") else 0)
        ttk.Checkbutton(row_logout, text="Instant Logout (Testing)",
                        variable=self.instant_logout_var,
                        command=self._on_instant_logout_toggle).pack(side="left")
        ttk.Label(row_logout, text="Sets timer to 1s (client-side, GM.cfg)",
                  foreground="gray", font=("Segoe UI", 8)).pack(side="left", padx=8)

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
            time.sleep(3)

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

            time.sleep(3)

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

        if weapon_settings:
            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                path = SERVERS[name].get("settings")
                if path:
                    write_cfg(path, weapon_settings)
            self._log("Server gameplay settings saved to all 4 configs")

        # Movement speeds -> GM.cfg
        client_settings = {}
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
