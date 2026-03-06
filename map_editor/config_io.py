"""
Helbreath Map Editor — .txt map config file reader/writer.
Format: [MAP-INFO] ... [END-MAP-INFO]
"""
import re


class MapConfig:
    def __init__(self):
        self.map_location = ""
        self.initial_points = []       # [(index, x, y)]
        self.level_limit = None
        self.upper_level_limit = None
        self.teleport_locs = []        # [(sx, sy, dest_map, dx, dy, direction)]
        self.waypoints = []            # [(index, x, y)]
        self.npcs = []                 # [(name, move_type, waypoint_indices, prefix)]
        self.spot_mob_generators = []  # [(idx, type, l, t, r, b, mob_type, max_mobs)]
        self.no_attack_areas = []      # [(idx, l, t, r, b)]
        self.maximum_object = 350
        self.random_mob_generator = None  # (enabled, level) or None
        self.fixed_dayornight = None
        self.fish_points = []          # [(index, x, y)]
        self.max_fish = None
        self.mineral_points = []       # [(index, x, y)]
        self.max_mineral = None
        self.map_type = None

    @staticmethod
    def read(filepath):
        """Parse a map config .txt file."""
        cfg = MapConfig()
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('//'):
                    continue
                if line.startswith('[MAP-INFO]') or line.startswith('[END-MAP-INFO]'):
                    continue

                # Parse key = value
                parts = line.split('=', 1)
                if len(parts) != 2:
                    continue
                key = parts[0].strip().lower()
                val = parts[1].strip()
                tokens = val.split()

                try:
                    if key == 'map-location':
                        cfg.map_location = tokens[0] if tokens else ""
                    elif key == 'initial-point' and len(tokens) >= 3:
                        cfg.initial_points.append((int(tokens[0]), int(tokens[1]), int(tokens[2])))
                    elif key == 'level-limit' and tokens:
                        cfg.level_limit = int(tokens[0])
                    elif key == 'upper-level-limit' and tokens:
                        cfg.upper_level_limit = int(tokens[0])
                    elif key == 'teleport-loc' and len(tokens) >= 6:
                        cfg.teleport_locs.append((
                            int(tokens[0]), int(tokens[1]),
                            tokens[2],
                            int(tokens[3]), int(tokens[4]),
                            int(tokens[5])
                        ))
                    elif key == 'waypoint' and len(tokens) >= 3:
                        cfg.waypoints.append((int(tokens[0]), int(tokens[1]), int(tokens[2])))
                    elif key == 'spot-mob-generator' and len(tokens) >= 8:
                        cfg.spot_mob_generators.append((
                            int(tokens[0]), int(tokens[1]),
                            int(tokens[2]), int(tokens[3]),
                            int(tokens[4]), int(tokens[5]),
                            int(tokens[6]), int(tokens[7])
                        ))
                    elif key == 'no-attack-area' and len(tokens) >= 5:
                        cfg.no_attack_areas.append((
                            int(tokens[0]),
                            int(tokens[1]), int(tokens[2]),
                            int(tokens[3]), int(tokens[4])
                        ))
                    elif key == 'maximum-object' and tokens:
                        cfg.maximum_object = int(tokens[0])
                    elif key == 'random-mob-generator' and len(tokens) >= 2:
                        cfg.random_mob_generator = (int(tokens[0]), int(tokens[1]))
                    elif key == 'fixed-dayornight-mode' and tokens:
                        cfg.fixed_dayornight = int(tokens[0])
                    elif key == 'fish-point' and len(tokens) >= 3:
                        cfg.fish_points.append((int(tokens[0]), int(tokens[1]), int(tokens[2])))
                    elif key == 'max-fish' and tokens:
                        cfg.max_fish = int(tokens[0])
                    elif key == 'mineral-point' and len(tokens) >= 3:
                        cfg.mineral_points.append((int(tokens[0]), int(tokens[1]), int(tokens[2])))
                    elif key == 'max-mineral' and tokens:
                        cfg.max_mineral = int(tokens[0])
                    elif key == 'type' and tokens:
                        cfg.map_type = int(tokens[0])
                except (ValueError, IndexError):
                    continue
        return cfg

    def write(self, filepath):
        """Write config to .txt file."""
        lines = []
        lines.append("[MAP-INFO]")
        lines.append("")
        lines.append(f"map-location  = {self.map_location}")

        for idx, x, y in self.initial_points:
            lines.append(f"initial-point = {idx}  {x} {y}")

        if self.level_limit is not None:
            lines.append(f"level-limit = {self.level_limit}")
        if self.upper_level_limit is not None:
            lines.append(f"upper-level-limit = {self.upper_level_limit}")
        if self.map_type is not None:
            lines.append(f"type = {self.map_type}")

        if self.teleport_locs:
            lines.append("")
            lines.append("//-Teleportation-Set----SX-----SY-----DestMapName---------DX-----DY----Dir---;")
            for sx, sy, dest, dx, dy, d in self.teleport_locs:
                lines.append(f"teleport-loc     =      {sx:<6} {sy:<6} {dest:<20}{dx:<6} {dy:<6} {d}")

        if self.waypoints:
            lines.append("")
            lines.append("//-WayPoint-Set---------Num-----X-----Y----;")
            for idx, x, y in self.waypoints:
                lines.append(f"waypoint = {idx}  {x} {y}")

        if self.no_attack_areas:
            lines.append("")
            lines.append("//-No-Attack-Area---------Num-----RECT-------------")
            for idx, l, t, r, b in self.no_attack_areas:
                lines.append(f"no-attack-area          = {idx}       {l} {t} {r} {b}")

        lines.append("")
        lines.append(f"maximum-object\t\t= {self.maximum_object}")

        if self.random_mob_generator:
            lines.append(f"random-mob-generator\t= {self.random_mob_generator[0]}   {self.random_mob_generator[1]}")

        if self.spot_mob_generators:
            lines.append("")
            lines.append("//------------------------Num-Type-waypoints--------------Mob-MobNum")
            for idx, t, l, top, r, b, mob, cnt in self.spot_mob_generators:
                lines.append(f"spot-mob-generator\t= {idx}   {t}    {l}  {top}  {r}  {b}         {mob}  {cnt}")

        if self.fixed_dayornight is not None:
            lines.append(f"fixed-dayornight-mode = {self.fixed_dayornight}")

        if self.max_fish is not None:
            lines.append(f"max-fish = {self.max_fish}")
        for idx, x, y in self.fish_points:
            lines.append(f"fish-point = {idx}  {x} {y}")

        if self.max_mineral is not None:
            lines.append(f"max-mineral = {self.max_mineral}")
        for idx, x, y in self.mineral_points:
            lines.append(f"mineral-point = {idx}  {x} {y}")

        lines.append("")
        lines.append("[END-MAP-INFO]")
        lines.append("")

        with open(filepath, 'w') as f:
            f.write('\n'.join(lines))

    @staticmethod
    def generate_template(map_name, width, height):
        """Generate a starter config template."""
        cfg = MapConfig()
        cfg.map_location = map_name
        cfg.initial_points = [(1, width // 2, height // 2)]
        cfg.maximum_object = 350
        return cfg
