import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import os
import threading
import time
import datetime
import queue
import sys
import socket
import re

BASE_DIR = r"C:\Helbreath Project"
LOG_DIR = os.path.join(BASE_DIR, "server_logs")

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

# --- Debug Tool Log Paths ---
DEBUG_LOG_PATHS = {
    "Map Change Log":     os.path.join(BASE_DIR, "Client", "mapchange_debug.txt"),
    "Client Color Debug": os.path.join(BASE_DIR, "Client", "color_debug.txt"),
    "Login Error Log":    os.path.join(BASE_DIR, "Login", "Logs", "Error.log"),
    "Login XSocket Log":  os.path.join(BASE_DIR, "Login", "Logs", "XSocket.log"),
    "Login Hack Log":     os.path.join(BASE_DIR, "Login", "Logs", "Hack.log"),
    "Login MySQL Errors":  os.path.join(BASE_DIR, "Login", "Logs", "MysqlError.log"),
    "Login Query Errors": os.path.join(BASE_DIR, "Login", "Logs", "Query errors.log"),
    "Login Events":       os.path.join(BASE_DIR, "Login", "Events.log"),
    "Towns Events":       os.path.join(BASE_DIR, "Maps", "Towns", "Logs", "Events.log"),
    "Towns Admin":        os.path.join(BASE_DIR, "Maps", "Towns", "Logs", "Admin.log"),
    "Neutrals Events":    os.path.join(BASE_DIR, "Maps", "Neutrals", "Logs", "Events.log"),
    "Neutrals Errors":    os.path.join(BASE_DIR, "Maps", "Neutrals", "Logs", "Error.log"),
    "Neutrals Admin":     os.path.join(BASE_DIR, "Maps", "Neutrals", "Logs", "Admin.log"),
    "Middleland Events":  os.path.join(BASE_DIR, "Maps", "Middleland", "Logs", "Events.log"),
    "Middleland Admin":   os.path.join(BASE_DIR, "Maps", "Middleland", "Logs", "Admin.log"),
    "Events Events":      os.path.join(BASE_DIR, "Maps", "Events", "Logs", "Events.log"),
    "Events Admin":       os.path.join(BASE_DIR, "Maps", "Events", "Logs", "Admin.log"),
}

# Server ports for status checking
SERVER_PORTS = {
    "Login":      4000,
    "GateServer": 5656,
    "Towns":      3002,
    "Neutrals":   3008,
    "Middleland": 3007,
    "Events":     3001,
}

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

        # Debug console state
        self._server_procs = {}      # name -> subprocess.Popen
        self._log_queue = queue.Queue()
        self._log_file = None
        self._log_file_path = None
        self._debug_mode = False     # True = capture output, False = legacy consoles
        self._debug_filter_var = tk.StringVar(value="")
        self._debug_paused = False
        self._debug_buffer = []      # all lines for filtering
        self._max_buffer = 50000     # max lines to keep

        self._build_top_bar()
        self._build_tabs()
        self._build_log_area()

        self._refresh_status()
        self._poll_log_queue()

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

        self._build_tab_debug_console()
        self._build_tab_debug_tools()
        self._build_tab_server_settings()
        self._build_tab_gameplay_tuning()
        self._build_tab_display()
        self._build_tab_quick_actions()
        self._build_tab_gm_commands()

    # ---- Tab: Debug Console ----
    def _build_tab_debug_console(self):
        tab = ttk.Frame(self.notebook, padding=0)
        self.notebook.add(tab, text="Debug Console")

        # Toolbar at top
        toolbar = ttk.Frame(tab, padding=(8, 6, 8, 4))
        toolbar.pack(fill="x")

        ttk.Button(toolbar, text="Clear", command=self._debug_clear).pack(side="left", padx=2)
        ttk.Button(toolbar, text="Save Log", command=self._debug_save_log).pack(side="left", padx=2)
        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=6)
        ttk.Button(toolbar, text="Save for Claude", command=self._save_for_claude).pack(side="left", padx=2)

        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=8)

        # Filter
        ttk.Label(toolbar, text="Filter:").pack(side="left", padx=(0, 4))
        self._debug_filter_var.trace_add("write", self._debug_on_filter)
        filter_entry = ttk.Entry(toolbar, textvariable=self._debug_filter_var, width=20)
        filter_entry.pack(side="left", padx=(0, 6))

        # Server toggle checkbuttons
        self._debug_server_vars = {}
        for name in ["Login", "Towns", "Neutrals", "Middleland", "Events"]:
            var = tk.IntVar(value=1)
            cb = ttk.Checkbutton(toolbar, text=name[:3], variable=var,
                                 command=self._debug_refilter)
            cb.pack(side="left", padx=1)
            self._debug_server_vars[name] = var

        # Log file indicator
        self._debug_logfile_label = ttk.Label(toolbar, text="", font=("Segoe UI", 7),
                                               foreground="gray")
        self._debug_logfile_label.pack(side="right", padx=4)

        ttk.Separator(tab, orient="horizontal").pack(fill="x")

        # Console text area
        console_frame = ttk.Frame(tab)
        console_frame.pack(fill="both", expand=True)

        scrollbar = ttk.Scrollbar(console_frame, orient="vertical")
        scrollbar.pack(side="right", fill="y")

        self._debug_text = tk.Text(
            console_frame, font=("Consolas", 9), wrap="none",
            bg="#0c0c0c", fg="#cccccc", insertbackground="#cccccc",
            state="disabled", yscrollcommand=scrollbar.set,
            padx=6, pady=4,
        )
        self._debug_text.pack(fill="both", expand=True)
        scrollbar.config(command=self._debug_text.yview)

        # Horizontal scrollbar
        hscroll = ttk.Scrollbar(tab, orient="horizontal", command=self._debug_text.xview)
        hscroll.pack(fill="x")
        self._debug_text.config(xscrollcommand=hscroll.set)

        # Configure color tags per server
        server_colors = {
            "Login":      "#4fc3f7",  # light blue
            "Towns":      "#81c784",  # green
            "Neutrals":   "#ffcc80",  # orange
            "Middleland": "#ce93d8",  # purple
            "Events":     "#ef5350",  # red
            "System":     "#888888",  # gray
        }
        for name, color in server_colors.items():
            self._debug_text.tag_configure(f"srv_{name}", foreground=color)
        # Special tags for debug markers
        self._debug_text.tag_configure("tag_error",
            foreground="#ff5252", font=("Consolas", 9, "bold"))
        self._debug_text.tag_configure("tag_admin",
            foreground="#ffeb3b")
        self._debug_text.tag_configure("tag_connect",
            foreground="#69f0ae")
        self._debug_text.tag_configure("tag_teleport",
            foreground="#40c4ff")
        self._debug_text.tag_configure("tag_item",
            foreground="#ff80ab")
        self._debug_text.tag_configure("tag_timestamp",
            foreground="#666666")

        # Mousewheel binding
        self._debug_text.bind("<Enter>", lambda e: self._debug_text.bind_all(
            "<MouseWheel>", lambda ev: self._debug_text.yview_scroll(
                int(-1 * (ev.delta / 120)), "units")))
        self._debug_text.bind("<Leave>", lambda e: self._debug_text.unbind_all("<MouseWheel>"))

    def _launch_server_debug(self, name):
        """Launch a single server with stdout/stderr capture."""
        info = SERVERS[name]
        exe = info["exe"]
        cwd = info["cwd"]

        if not os.path.exists(exe):
            self._debug_append("System", f"ERROR: {exe} not found!")
            return

        try:
            proc = subprocess.Popen(
                [exe],
                cwd=cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                creationflags=subprocess.CREATE_NO_WINDOW,
            )
            self._server_procs[name] = proc
            self._debug_append("System", f"Started {name} (PID {proc.pid})")

            # Start reader thread
            t = threading.Thread(target=self._reader_thread, args=(name, proc),
                               daemon=True)
            t.start()
        except Exception as e:
            self._debug_append("System", f"ERROR starting {name}: {e}")

    def _reader_thread(self, name, proc):
        """Background thread that reads server stdout line by line."""
        try:
            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                text = line.decode("utf-8", errors="replace").rstrip("\r\n")
                if text:
                    self._log_queue.put((name, text))
        except Exception as e:
            self._log_queue.put((name, f"[Reader error: {e}]"))
        finally:
            ret = proc.poll()
            self._log_queue.put((name, f"[Process exited, code={ret}]"))
            self._server_procs.pop(name, None)

    def _poll_log_queue(self):
        """Drain the queue and update the debug console (runs on main thread)."""
        batch = []
        try:
            while True:
                batch.append(self._log_queue.get_nowait())
        except queue.Empty:
            pass

        if batch:
            for server_name, text in batch:
                self._debug_append(server_name, text)

        self.after(50, self._poll_log_queue)  # 50ms polling = responsive

    def _debug_append(self, server_name, text):
        """Append a line to the debug console and log file."""
        ts = datetime.datetime.now().strftime("%H:%M:%S.") + \
             f"{datetime.datetime.now().microsecond // 1000:03d}"
        line = f"[{ts}] [{server_name:11s}] {text}"

        # Write to log file
        if self._log_file:
            try:
                self._log_file.write(line + "\n")
                self._log_file.flush()
            except Exception:
                pass

        # Store in buffer
        self._debug_buffer.append((server_name, line))
        if len(self._debug_buffer) > self._max_buffer:
            self._debug_buffer = self._debug_buffer[-self._max_buffer:]

        # Check filter
        if not self._debug_should_show(server_name, text):
            return

        # Determine tag based on content
        tags = [f"srv_{server_name}"]
        text_upper = text.upper()
        if "ERROR" in text_upper or "FAIL" in text_upper or "CRITICAL" in text_upper:
            tags = ["tag_error"]
        elif text.startswith("[Admin]") or text.startswith("[ChatMsg]"):
            tags.append("tag_admin")
        elif text.startswith("[Connect]") or text.startswith("[Disconnect]"):
            tags.append("tag_connect")
        elif text.startswith("[Teleport]"):
            tags.append("tag_teleport")
        elif text.startswith("[CreateItem]") or text.startswith("[Summon]"):
            tags.append("tag_item")

        # Append to text widget
        self._debug_text.config(state="normal")
        self._debug_text.insert("end", line + "\n", tuple(tags))
        self._debug_text.see("end")
        self._debug_text.config(state="disabled")

    def _debug_should_show(self, server_name, text):
        """Check if a line passes the current filter."""
        # Server filter
        if server_name in self._debug_server_vars:
            if not self._debug_server_vars[server_name].get():
                return False
        # Text filter
        ft = self._debug_filter_var.get().strip().lower()
        if ft and ft not in text.lower() and ft not in server_name.lower():
            return False
        return True

    def _debug_on_filter(self, *args):
        """Re-render when filter text changes."""
        self._debug_refilter()

    def _debug_refilter(self):
        """Re-render all buffered lines with current filter."""
        self._debug_text.config(state="normal")
        self._debug_text.delete("1.0", "end")

        for server_name, line in self._debug_buffer:
            # Extract text portion after the timestamp and server tag
            parts = line.split("] ", 2)
            text = parts[2] if len(parts) > 2 else line
            if not self._debug_should_show(server_name, text):
                continue

            tags = [f"srv_{server_name}"]
            text_upper = text.upper()
            if "ERROR" in text_upper or "FAIL" in text_upper or "CRITICAL" in text_upper:
                tags = ["tag_error"]
            elif "[Admin]" in line or "[ChatMsg]" in line:
                tags.append("tag_admin")
            elif "[Connect]" in line or "[Disconnect]" in line:
                tags.append("tag_connect")
            elif "[Teleport]" in line:
                tags.append("tag_teleport")
            elif "[CreateItem]" in line or "[Summon]" in line:
                tags.append("tag_item")

            self._debug_text.insert("end", line + "\n", tuple(tags))

        self._debug_text.see("end")
        self._debug_text.config(state="disabled")

    def _debug_clear(self):
        """Clear the debug console."""
        self._debug_buffer.clear()
        self._debug_text.config(state="normal")
        self._debug_text.delete("1.0", "end")
        self._debug_text.config(state="disabled")

    def _debug_save_log(self):
        """Save current buffer to a new log file."""
        if not self._debug_buffer:
            self._log("No debug output to save")
            return
        os.makedirs(LOG_DIR, exist_ok=True)
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        path = os.path.join(LOG_DIR, f"snapshot_{ts}.log")
        with open(path, "w", encoding="utf-8") as f:
            for _, line in self._debug_buffer:
                f.write(line + "\n")
        self._log(f"Log snapshot saved: {path}")

    def _save_for_claude(self):
        """Save comprehensive diagnostic dump for Claude Code to analyze."""
        os.makedirs(LOG_DIR, exist_ok=True)
        path = os.path.join(LOG_DIR, "claude_review.log")

        def _read_tail(filepath, max_lines=200):
            """Read last N lines of a file, return as string."""
            try:
                with open(filepath, "r", encoding="utf-8", errors="replace") as f:
                    lines = f.readlines()
                if not lines:
                    return "(empty file)\n"
                if len(lines) > max_lines:
                    return f"--- last {max_lines} of {len(lines)} lines ---\n" + "".join(lines[-max_lines:])
                return "".join(lines)
            except FileNotFoundError:
                return "(file not found)\n"
            except Exception as e:
                return f"(error: {e})\n"

        def _check_port(port):
            """Check if a TCP port is listening on localhost."""
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.5)
                result = sock.connect_ex(("127.0.0.1", port))
                sock.close()
                return "LISTENING" if result == 0 else "NOT LISTENING"
            except Exception:
                return "ERROR"

        with open(path, "w", encoding="utf-8") as f:
            now = datetime.datetime.now()
            f.write(f"{'='*80}\n")
            f.write(f"HELBREATH DIAGNOSTIC DUMP\n")
            f.write(f"Generated: {now.isoformat()}\n")
            f.write(f"{'='*80}\n\n")

            # --- Section 1: Port Status ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 1: SERVER PORT STATUS\n")
            f.write(f"{'='*60}\n")
            for name, port in SERVER_PORTS.items():
                status = _check_port(port)
                f.write(f"  {name:<14} port {port:<6} {status}\n")
            f.write("\n")

            # --- Section 2: Process Status ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 2: PROCESS STATUS\n")
            f.write(f"{'='*60}\n")
            f.write(f"  Login.exe running: {is_process_running('Login.exe')}\n")
            hg_pids = get_pids("HGserver.exe")
            f.write(f"  HGserver.exe count: {len(hg_pids)} (PIDs: {hg_pids})\n")
            f.write(f"  Play_Me.exe running: {is_process_running('Play_Me.exe')}\n")
            f.write("\n")

            # --- Section 3: Client Map Change Debug ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 3: CLIENT MAP CHANGE LOG (mapchange_debug.txt)\n")
            f.write(f"{'='*60}\n")
            f.write(_read_tail(os.path.join(BASE_DIR, "Client", "mapchange_debug.txt"), 100))
            f.write("\n")

            # --- Section 4: Server Debug Connect Logs ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 4: SERVER DEBUG CONNECT LOGS\n")
            f.write(f"(File-based diagnostic logging from HG and Login servers)\n")
            f.write(f"{'='*60}\n")
            debug_log_dirs = {
                "Login": os.path.join(BASE_DIR, "Login", "Logs", "debug_connect.log"),
                "Towns": os.path.join(BASE_DIR, "Maps", "Towns", "Logs", "debug_connect.log"),
                "Neutrals": os.path.join(BASE_DIR, "Maps", "Neutrals", "Logs", "debug_connect.log"),
                "Middleland": os.path.join(BASE_DIR, "Maps", "Middleland", "Logs", "debug_connect.log"),
                "Events": os.path.join(BASE_DIR, "Maps", "Events", "Logs", "debug_connect.log"),
            }
            for name, logpath in debug_log_dirs.items():
                f.write(f"\n--- {name} debug_connect.log ---\n")
                f.write(_read_tail(logpath, 150))
            f.write("\n")

            # --- Section 5: Login Server Error Log ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 5: LOGIN SERVER ERROR LOG\n")
            f.write(f"{'='*60}\n")
            f.write(_read_tail(os.path.join(BASE_DIR, "Login", "Logs", "Error.log"), 100))
            f.write("\n")

            # --- Section 6: Login XSocket Log ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 6: LOGIN XSOCKET LOG\n")
            f.write(f"{'='*60}\n")
            f.write(_read_tail(os.path.join(BASE_DIR, "Login", "Logs", "XSocket.log"), 50))
            f.write("\n")

            # --- Section 7: Game Server Error/Events Logs ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 7: GAME SERVER LOGS (Events + Errors)\n")
            f.write(f"{'='*60}\n")
            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                log_dir = os.path.join(BASE_DIR, "Maps", name, "Logs")
                for logname in ["Events.log", "Error.log"]:
                    logpath = os.path.join(log_dir, logname)
                    if os.path.exists(logpath):
                        f.write(f"\n--- {name}/{logname} ---\n")
                        f.write(_read_tail(logpath, 50))
            f.write("\n")

            # --- Section 8: Debug Console Buffer ---
            f.write(f"{'='*60}\n")
            f.write(f"SECTION 8: SERVER MANAGER DEBUG CONSOLE\n")
            f.write(f"(Server stdout capture)\n")
            f.write(f"{'='*60}\n")
            recent = self._debug_buffer[-500:] if len(self._debug_buffer) > 500 else self._debug_buffer
            if recent:
                for _, line in recent:
                    f.write(line + "\n")
            else:
                f.write("(no debug console output captured)\n")
            f.write("\n")

            # --- Section 9: Client Color Debug ---
            color_path = os.path.join(BASE_DIR, "Client", "color_debug.txt")
            if os.path.exists(color_path):
                f.write(f"{'='*60}\n")
                f.write(f"SECTION 9: CLIENT COLOR DEBUG\n")
                f.write(f"{'='*60}\n")
                f.write(_read_tail(color_path, 50))
                f.write("\n")

            f.write(f"{'='*80}\n")
            f.write(f"END OF DIAGNOSTIC DUMP\n")
            f.write(f"{'='*80}\n")

        try:
            size_kb = os.path.getsize(path) / 1024
        except Exception:
            size_kb = 0
        self._log(f"Diagnostic dump saved: {path} ({size_kb:.1f} KB)")
        self._debug_append("System", f"Diagnostic dump saved for Claude: {path} ({size_kb:.1f} KB)")

    # ---- Tab: Debug Tools ----
    def _build_tab_debug_tools(self):
        tab = ttk.Frame(self.notebook, padding=0)
        self.notebook.add(tab, text="Debug Tools")

        # Toolbar row 1: log selector + refresh controls
        toolbar = ttk.Frame(tab, padding=(8, 6, 8, 2))
        toolbar.pack(fill="x")

        ttk.Label(toolbar, text="Log:").pack(side="left", padx=(0, 4))
        self._dbg_log_var = tk.StringVar(value="Map Change Log")
        log_choices = list(DEBUG_LOG_PATHS.keys())
        combo = ttk.Combobox(toolbar, textvariable=self._dbg_log_var,
                             values=log_choices, width=22, state="readonly")
        combo.pack(side="left", padx=(0, 6))
        combo.bind("<<ComboboxSelected>>", lambda e: self._dbg_refresh_log())

        ttk.Button(toolbar, text="Refresh", command=self._dbg_refresh_log).pack(side="left", padx=2)

        self._dbg_auto_var = tk.IntVar(value=0)
        ttk.Checkbutton(toolbar, text="Auto (3s)", variable=self._dbg_auto_var,
                         command=self._dbg_toggle_auto).pack(side="left", padx=4)

        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=6)

        ttk.Button(toolbar, text="Tail 50", command=lambda: self._dbg_refresh_log(tail=50)).pack(side="left", padx=2)
        ttk.Button(toolbar, text="Full", command=lambda: self._dbg_refresh_log(tail=0)).pack(side="left", padx=2)

        ttk.Separator(toolbar, orient="vertical").pack(side="left", fill="y", padx=6)

        ttk.Button(toolbar, text="Clear File", command=self._dbg_clear_file).pack(side="left", padx=2)

        # Toolbar row 2: special tools
        toolbar2 = ttk.Frame(tab, padding=(8, 2, 8, 4))
        toolbar2.pack(fill="x")

        ttk.Button(toolbar2, text="Port Check", command=self._dbg_port_check).pack(side="left", padx=2)
        ttk.Button(toolbar2, text="Connection Trace", command=self._dbg_connection_trace).pack(side="left", padx=2)
        ttk.Button(toolbar2, text="Open Log Folder", command=self._dbg_open_log_folder).pack(side="left", padx=2)

        ttk.Separator(toolbar2, orient="vertical").pack(side="left", fill="y", padx=6)

        # Filter
        ttk.Label(toolbar2, text="Filter:").pack(side="left", padx=(0, 4))
        self._dbg_filter_var = tk.StringVar(value="")
        filter_entry = ttk.Entry(toolbar2, textvariable=self._dbg_filter_var, width=18)
        filter_entry.pack(side="left", padx=(0, 4))
        filter_entry.bind("<Return>", lambda e: self._dbg_refresh_log())

        # Info label (right-aligned)
        self._dbg_info_label = ttk.Label(toolbar2, text="", font=("Segoe UI", 8),
                                          foreground="gray")
        self._dbg_info_label.pack(side="right", padx=4)

        ttk.Separator(tab, orient="horizontal").pack(fill="x")

        # Text area
        text_frame = ttk.Frame(tab)
        text_frame.pack(fill="both", expand=True)

        scrollbar = ttk.Scrollbar(text_frame, orient="vertical")
        scrollbar.pack(side="right", fill="y")

        self._dbg_text = tk.Text(
            text_frame, font=("Consolas", 9), wrap="none",
            bg="#0c0c0c", fg="#cccccc", insertbackground="#cccccc",
            state="disabled", yscrollcommand=scrollbar.set,
            padx=6, pady=4,
        )
        self._dbg_text.pack(fill="both", expand=True)
        scrollbar.config(command=self._dbg_text.yview)

        hscroll = ttk.Scrollbar(tab, orient="horizontal", command=self._dbg_text.xview)
        hscroll.pack(fill="x")
        self._dbg_text.config(xscrollcommand=hscroll.set)

        # Color tags for mapchange log
        self._dbg_text.tag_configure("dbg_teleport", foreground="#40c4ff")
        self._dbg_text.tag_configure("dbg_connect", foreground="#69f0ae")
        self._dbg_text.tag_configure("dbg_closed", foreground="#ff5252", font=("Consolas", 9, "bold"))
        self._dbg_text.tag_configure("dbg_confirm", foreground="#ffcc80")
        self._dbg_text.tag_configure("dbg_suppress", foreground="#ce93d8")
        self._dbg_text.tag_configure("dbg_error", foreground="#ff5252", font=("Consolas", 9, "bold"))
        self._dbg_text.tag_configure("dbg_warn", foreground="#ffeb3b")
        self._dbg_text.tag_configure("dbg_info", foreground="#cccccc")
        self._dbg_text.tag_configure("dbg_header", foreground="#4fc3f7", font=("Consolas", 10, "bold"))
        self._dbg_text.tag_configure("dbg_success", foreground="#69f0ae", font=("Consolas", 9, "bold"))
        self._dbg_text.tag_configure("dbg_fail", foreground="#ff5252", font=("Consolas", 9, "bold"))
        self._dbg_text.tag_configure("dbg_port_ok", foreground="#69f0ae")
        self._dbg_text.tag_configure("dbg_port_bad", foreground="#ff5252")
        self._dbg_text.tag_configure("dbg_dim", foreground="#666666")
        self._dbg_text.tag_configure("dbg_stale", foreground="#ff80ab")

        # Mousewheel binding
        self._dbg_text.bind("<Enter>", lambda e: self._dbg_text.bind_all(
            "<MouseWheel>", lambda ev: self._dbg_text.yview_scroll(
                int(-1 * (ev.delta / 120)), "units")))
        self._dbg_text.bind("<Leave>", lambda e: self._dbg_text.unbind_all("<MouseWheel>"))

        self._dbg_auto_timer = None
        self._dbg_tail_mode = 50  # default tail lines

        # Load initial content
        self.after(100, self._dbg_refresh_log)

    def _dbg_refresh_log(self, tail=None):
        """Read the selected log file and display it with color coding."""
        if tail is not None:
            self._dbg_tail_mode = tail

        log_name = self._dbg_log_var.get()
        path = DEBUG_LOG_PATHS.get(log_name, "")

        self._dbg_text.config(state="normal")
        self._dbg_text.delete("1.0", "end")

        if not path or not os.path.exists(path):
            self._dbg_text.insert("end", f"File not found: {path}\n", "dbg_error")
            self._dbg_info_label.config(text="File not found")
            self._dbg_text.config(state="disabled")
            return

        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                lines = f.readlines()
        except Exception as e:
            self._dbg_text.insert("end", f"Error reading file: {e}\n", "dbg_error")
            self._dbg_info_label.config(text="Read error")
            self._dbg_text.config(state="disabled")
            return

        # Apply filter
        ft = self._dbg_filter_var.get().strip().lower()
        if ft:
            lines = [l for l in lines if ft in l.lower()]

        total = len(lines)
        if self._dbg_tail_mode > 0 and total > self._dbg_tail_mode:
            lines = lines[-self._dbg_tail_mode:]
            self._dbg_text.insert("end",
                f"--- Showing last {self._dbg_tail_mode} of {total} lines ---\n\n", "dbg_dim")

        # Detect which log type for color coding
        is_mapchange = "mapchange" in log_name.lower() or "map change" in log_name.lower()

        for line in lines:
            line = line.rstrip("\r\n")
            if not line:
                self._dbg_text.insert("end", "\n")
                continue

            tag = self._dbg_classify_line(line, is_mapchange)
            self._dbg_text.insert("end", line + "\n", tag)

        # File info
        try:
            mtime = os.path.getmtime(path)
            mod_str = datetime.datetime.fromtimestamp(mtime).strftime("%Y-%m-%d %H:%M:%S")
            size_kb = os.path.getsize(path) / 1024
            shown = min(total, self._dbg_tail_mode) if self._dbg_tail_mode > 0 else total
            info = f"{shown}/{total} lines  |  {size_kb:.1f} KB  |  Modified: {mod_str}"
        except Exception:
            info = f"{total} lines"
        if ft:
            info = f"[filter: {ft}] " + info
        self._dbg_info_label.config(text=info)

        self._dbg_text.see("end")
        self._dbg_text.config(state="disabled")

    def _dbg_classify_line(self, line, is_mapchange):
        """Return a tag name based on line content."""
        upper = line.upper()

        if is_mapchange:
            if "TELEPORT_REQ" in line:
                return "dbg_teleport"
            if "GSOCK_CLOSED" in line and "suppressed" in line.lower():
                return "dbg_suppress"
            if "GSOCK_CLOSED" in line:
                return "dbg_closed"
            if "ENTERGAME_RESP=CONFIRM" in line:
                return "dbg_confirm"
            if "CONNECT_OK" in line or "CONN_ESTABLISHED" in line:
                return "dbg_connect"
            if "STALE_DRAIN" in line:
                return "dbg_stale"
            return "dbg_info"

        # Generic log classification
        if "ERROR" in upper or "FAIL" in upper or "CRITICAL" in upper or "REJECT" in upper:
            return "dbg_error"
        if "WARN" in upper:
            return "dbg_warn"
        if "HACK" in upper or "CHEAT" in upper:
            return "dbg_fail"
        if "[Connect]" in line or "[Disconnect]" in line:
            return "dbg_connect"
        if "[Teleport]" in line:
            return "dbg_teleport"
        if "[Admin]" in line or "[ChatMsg]" in line:
            return "dbg_confirm"
        return "dbg_info"

    def _dbg_toggle_auto(self):
        """Toggle auto-refresh timer."""
        if self._dbg_auto_var.get():
            self._dbg_auto_tick()
        else:
            if self._dbg_auto_timer:
                self.after_cancel(self._dbg_auto_timer)
                self._dbg_auto_timer = None

    def _dbg_auto_tick(self):
        """Auto-refresh callback."""
        if self._dbg_auto_var.get():
            self._dbg_refresh_log()
            self._dbg_auto_timer = self.after(3000, self._dbg_auto_tick)

    def _dbg_clear_file(self):
        """Clear the currently selected log file."""
        log_name = self._dbg_log_var.get()
        path = DEBUG_LOG_PATHS.get(log_name, "")
        if not path or not os.path.exists(path):
            self._log("File not found, nothing to clear")
            return
        if not messagebox.askyesno("Clear File",
                f"Clear all contents of:\n{path}\n\nThis cannot be undone."):
            return
        try:
            with open(path, "w") as f:
                f.truncate(0)
            self._log(f"Cleared: {path}")
            self._dbg_refresh_log()
        except Exception as e:
            self._log(f"Error clearing file: {e}")

    def _dbg_port_check(self):
        """Check which server ports are listening and display results."""
        self._dbg_text.config(state="normal")
        self._dbg_text.delete("1.0", "end")

        self._dbg_text.insert("end", "Server Port Status\n", "dbg_header")
        self._dbg_text.insert("end", "=" * 50 + "\n\n", "dbg_dim")

        all_ok = True
        for name, port in SERVER_PORTS.items():
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.5)
                result = sock.connect_ex(("127.0.0.1", port))
                sock.close()
                if result == 0:
                    self._dbg_text.insert("end", f"  {name:<14} port {port:<6} ", "dbg_info")
                    self._dbg_text.insert("end", "LISTENING\n", "dbg_port_ok")
                else:
                    self._dbg_text.insert("end", f"  {name:<14} port {port:<6} ", "dbg_info")
                    self._dbg_text.insert("end", "NOT LISTENING\n", "dbg_port_bad")
                    all_ok = False
            except Exception as e:
                self._dbg_text.insert("end", f"  {name:<14} port {port:<6} ", "dbg_info")
                self._dbg_text.insert("end", f"ERROR: {e}\n", "dbg_error")
                all_ok = False

        self._dbg_text.insert("end", "\n" + "=" * 50 + "\n", "dbg_dim")
        if all_ok:
            self._dbg_text.insert("end", "  All server ports are responding.\n", "dbg_success")
        else:
            self._dbg_text.insert("end", "  Some ports are not responding. Check server status.\n", "dbg_fail")

        ts = datetime.datetime.now().strftime("%H:%M:%S")
        self._dbg_info_label.config(text=f"Port check at {ts}")
        self._dbg_text.config(state="disabled")

    def _dbg_connection_trace(self):
        """Parse mapchange_debug.txt and show a high-level connection trace."""
        path = DEBUG_LOG_PATHS.get("Map Change Log", "")
        if not os.path.exists(path):
            self._log("mapchange_debug.txt not found")
            return

        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                lines = f.readlines()
        except Exception as e:
            self._log(f"Error reading mapchange log: {e}")
            return

        self._dbg_text.config(state="normal")
        self._dbg_text.delete("1.0", "end")

        self._dbg_text.insert("end", "Connection Trace Analysis\n", "dbg_header")
        self._dbg_text.insert("end", "=" * 70 + "\n", "dbg_dim")
        self._dbg_text.insert("end", "Parses mapchange_debug.txt into connection sequences\n\n", "dbg_dim")

        # Parse into connection sequences
        sequences = []
        current_seq = None

        for raw_line in lines:
            line = raw_line.strip()
            if not line:
                continue

            # Extract timestamp
            ts_match = re.search(r'\[(\d+)\]', line)
            ts = int(ts_match.group(1)) if ts_match else 0

            if "TELEPORT_REQ" in line:
                # Extract map and position
                map_match = re.search(r'map=(\S+)', line)
                pos_match = re.search(r'pos=(\S+)', line)
                map_name = map_match.group(1) if map_match else "?"
                pos = pos_match.group(1) if pos_match else "?"
                current_seq = {
                    "type": "teleport",
                    "ts": ts,
                    "map": map_name,
                    "pos": pos,
                    "events": [("TELEPORT_REQ", ts)],
                    "result": "pending",
                    "port": None,
                    "enter_type": None,
                }
                sequences.append(current_seq)

            elif "LSOCK_CONNECT_OK" in line:
                if current_seq is None:
                    # Initial login sequence
                    current_seq = {
                        "type": "login",
                        "ts": ts,
                        "map": "initial",
                        "pos": "",
                        "events": [],
                        "result": "pending",
                        "port": None,
                        "enter_type": None,
                    }
                    sequences.append(current_seq)
                current_seq["events"].append(("LSOCK_CONNECT_OK", ts))

            elif "CONN_ESTABLISHED" in line and current_seq:
                et_match = re.search(r'enterType=(\d+)', line)
                enter_type = et_match.group(1) if et_match else "?"
                current_seq["enter_type"] = enter_type
                current_seq["events"].append(("CONN_ESTABLISHED", ts))

            elif "ENTERGAME_RESP=CONFIRM" in line and current_seq:
                port_match = re.search(r'port=(\d+)', line)
                port = port_match.group(1) if port_match else "?"
                current_seq["port"] = port
                # Identify server from port
                server_name = "?"
                for sname, sport in SERVER_PORTS.items():
                    if str(sport) == port:
                        server_name = sname
                        break
                current_seq["server"] = server_name
                current_seq["events"].append(("ENTERGAME_CONFIRM", ts))

            elif "GSOCK_CONNECT_OK" in line and current_seq:
                current_seq["events"].append(("GSOCK_CONNECT_OK", ts))

            elif "GSOCK_CLOSED" in line and current_seq:
                if "suppressed" in line.lower():
                    current_seq["events"].append(("GSOCK_CLOSED_SUPPRESSED", ts))
                    current_seq["result"] = "failed_suppressed"
                else:
                    current_seq["events"].append(("GSOCK_CLOSED", ts))
                    current_seq["result"] = "failed"
                # After a failed sequence, reset so next line starts fresh
                current_seq = None

        # Mark sequences that didn't fail as success
        for seq in sequences:
            if seq["result"] == "pending":
                # Check if it has GSOCK_CONNECT_OK without a subsequent GSOCK_CLOSED
                has_gsock = any(e[0] == "GSOCK_CONNECT_OK" for e in seq["events"])
                if has_gsock:
                    seq["result"] = "success"

        # Render sequences
        for i, seq in enumerate(sequences):
            seq_type = seq["type"].upper()
            result = seq["result"]

            # Header line
            if seq_type == "TELEPORT":
                header = f"#{i+1}  TELEPORT  map={seq['map']}  pos={seq['pos']}"
            else:
                header = f"#{i+1}  LOGIN (initial connection)"

            self._dbg_text.insert("end", f"\n{header}\n", "dbg_header")

            # Details
            port_info = f"  Server: {seq.get('server', '?')} (port {seq['port']})" if seq['port'] else ""
            et_info = f"  EnterType: {seq['enter_type']}" if seq['enter_type'] else ""
            if port_info:
                self._dbg_text.insert("end", port_info + "\n", "dbg_info")
            if et_info:
                is_serverchange = seq['enter_type'] in ("3874",)
                tag = "dbg_teleport" if is_serverchange else "dbg_info"
                label = " (CROSS-SERVER)" if is_serverchange else " (same-server/login)"
                self._dbg_text.insert("end", et_info + label + "\n", tag)

            # Timeline
            if len(seq["events"]) > 1:
                t0 = seq["events"][0][1]
                for ename, ets in seq["events"]:
                    delta = ets - t0
                    delta_str = f"+{delta}ms" if delta > 0 else "  0ms"
                    tag = "dbg_info"
                    if "CLOSED" in ename:
                        tag = "dbg_closed" if "SUPPRESSED" not in ename else "dbg_suppress"
                    elif "CONNECT_OK" in ename or "ESTABLISHED" in ename:
                        tag = "dbg_connect"
                    elif "CONFIRM" in ename:
                        tag = "dbg_confirm"
                    elif "TELEPORT" in ename:
                        tag = "dbg_teleport"
                    self._dbg_text.insert("end", f"    {delta_str:>8}  {ename}\n", tag)

            # Result
            if result == "success":
                self._dbg_text.insert("end", "  Result: ", "dbg_info")
                self._dbg_text.insert("end", "SUCCESS\n", "dbg_success")
            elif "failed" in result:
                self._dbg_text.insert("end", "  Result: ", "dbg_info")
                self._dbg_text.insert("end", "FAILED", "dbg_fail")
                if "suppressed" in result:
                    self._dbg_text.insert("end", " (GSOCK_CLOSED during server change)\n", "dbg_suppress")
                else:
                    self._dbg_text.insert("end", " (connection dropped)\n", "dbg_fail")
            else:
                self._dbg_text.insert("end", "  Result: ", "dbg_info")
                self._dbg_text.insert("end", "INCOMPLETE\n", "dbg_warn")

        # Summary
        total = len(sequences)
        successes = sum(1 for s in sequences if s["result"] == "success")
        failures = sum(1 for s in sequences if "failed" in s["result"])
        teleports = sum(1 for s in sequences if s["type"] == "teleport")
        cross_server = sum(1 for s in sequences
                          if s["type"] == "teleport" and s["enter_type"] == "3874")

        self._dbg_text.insert("end", "\n" + "=" * 70 + "\n", "dbg_dim")
        self._dbg_text.insert("end", "Summary\n", "dbg_header")
        self._dbg_text.insert("end", f"  Total sequences: {total}\n", "dbg_info")
        self._dbg_text.insert("end", f"  Teleports: {teleports}  (cross-server: {cross_server})\n", "dbg_info")
        self._dbg_text.insert("end", f"  Successes: ", "dbg_info")
        self._dbg_text.insert("end", f"{successes}\n", "dbg_success" if successes else "dbg_info")
        self._dbg_text.insert("end", f"  Failures:  ", "dbg_info")
        self._dbg_text.insert("end", f"{failures}\n", "dbg_fail" if failures else "dbg_info")

        if failures > 0 and cross_server > 0:
            cross_fails = sum(1 for s in sequences
                             if "failed" in s["result"] and s["enter_type"] == "3874")
            if cross_fails == failures:
                self._dbg_text.insert("end",
                    f"\n  All {failures} failures are cross-server teleports (enterType=3874).\n",
                    "dbg_warn")
                self._dbg_text.insert("end",
                    "  The new game server is closing the connection after GSOCK_CONNECT_OK.\n",
                    "dbg_warn")

        ts = datetime.datetime.now().strftime("%H:%M:%S")
        self._dbg_info_label.config(text=f"Connection trace at {ts}  |  {total} sequences parsed")
        self._dbg_text.config(state="disabled")

    def _dbg_open_log_folder(self):
        """Open the folder containing the currently selected log file."""
        log_name = self._dbg_log_var.get()
        path = DEBUG_LOG_PATHS.get(log_name, "")
        if path and os.path.exists(path):
            os.startfile(os.path.dirname(path))
        elif path:
            # Try parent dir even if file doesn't exist
            parent = os.path.dirname(path)
            if os.path.exists(parent):
                os.startfile(parent)
            else:
                self._log(f"Folder not found: {parent}")
        else:
            self._log("No log selected")

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
        # Auto-save display settings when radio button changes
        self._save_display_settings()

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
        """Start all servers with stdout/stderr capture to Debug Console."""
        if self._server_procs:
            self._log("Servers already running. Stop first.")
            return

        self._debug_mode = True

        # Switch to Debug Console tab
        self.notebook.select(0)

        # Create log directory and file
        os.makedirs(LOG_DIR, exist_ok=True)
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        self._log_file_path = os.path.join(LOG_DIR, f"debug_{ts}.log")
        self._log_file = open(self._log_file_path, "w", encoding="utf-8", buffering=1)
        self._debug_logfile_label.config(text=f"Log: debug_{ts}.log")

        self._debug_append("System", f"--- Debug session started: {self._log_file_path} ---")
        self._log(f"Debug log: {self._log_file_path}")

        # Start Login first
        self._launch_server_debug("Login")

        def start_game_servers():
            time.sleep(5)  # Wait for Login to fully initialize (gate port ready)
            for name in ["Towns", "Neutrals", "Middleland", "Events"]:
                self._launch_server_debug(name)
                time.sleep(1.5)  # Allow each server to register before starting next
            self._debug_append("System", "All servers launched")
            self._log("All servers started")
            self.after(0, self._launch_client)

        threading.Thread(target=start_game_servers, daemon=True).start()

    def _stop_all(self):
        def run():
            self._log("Stopping all servers...")

            # If we have tracked debug processes, terminate them directly
            if self._server_procs:
                for name, proc in list(self._server_procs.items()):
                    try:
                        proc.terminate()
                        proc.wait(timeout=5)
                        self._debug_append("System", f"Stopped {name} (PID {proc.pid})")
                    except Exception as e:
                        try:
                            proc.kill()
                        except Exception:
                            pass
                        self._debug_append("System", f"Force-killed {name}: {e}")
                self._server_procs.clear()
            else:
                # Legacy: kill by image name
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

            # Close log file
            if self._log_file:
                self._debug_append("System", "--- Debug session ended ---")
                try:
                    self._log_file.close()
                except Exception:
                    pass
                self._log_file = None

            self._debug_mode = False
            self._log("All servers stopped")
        threading.Thread(target=run, daemon=True).start()

    def _restart_all(self):
        def run():
            self._log("Restarting all servers...")

            # Stop existing processes
            if self._server_procs:
                for name, proc in list(self._server_procs.items()):
                    try:
                        proc.terminate()
                        proc.wait(timeout=5)
                    except Exception:
                        try:
                            proc.kill()
                        except Exception:
                            pass
                self._server_procs.clear()
            else:
                for proc_name in ["HGserver.exe", "Login.exe"]:
                    try:
                        subprocess.run(
                            ["taskkill", "/F", "/IM", proc_name],
                            creationflags=subprocess.CREATE_NO_WINDOW,
                            capture_output=True, timeout=10,
                        )
                    except Exception:
                        pass

            # Close previous log file
            if self._log_file:
                self._debug_append("System", "--- Debug session ended (restart) ---")
                try:
                    self._log_file.close()
                except Exception:
                    pass
                self._log_file = None
            self._debug_mode = False

            time.sleep(2)
            self._log("Servers stopped, restarting...")

            # Always restart in debug mode
            self.after(0, self._start_all)
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


    def destroy(self):
        """Clean up on window close."""
        # Close log file
        if self._log_file:
            try:
                self._log_file.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] [System     ] --- Manager closed ---\n")
                self._log_file.close()
            except Exception:
                pass
            self._log_file = None
        super().destroy()


if __name__ == "__main__":
    app = ServerManager()
    app.mainloop()
