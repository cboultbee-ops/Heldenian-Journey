# Helbreath GM Commands Reference

All commands are typed in chat with `/` prefix. Requires admin level set in database.

---

## Player Management
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/who` | Show online players | 1+ |
| `/summonplayer [name]` | Teleport a player to you | 1+ |
| `/closeconn [name]` | Disconnect a player | 1+ |
| `/shutup [name]` | Mute a player | 1+ |
| `/ban [name]` | Ban a player | 1+ |
| `/checkip [name]` | Check player's IP | 1+ |
| `/disconnectall` | Disconnect all players | 1+ |

## Teleportation
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/goto [map]` | Teleport to a map | 1+ |
| `/tp [name]` | Teleport to a player | 1+ |

## Items & Stats
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/ci [itemname]` | Create an item | 2+ |
| `/createitem [itemname]` | Create an item (alias) | 2+ |
| `/sethp` | Set HP | 1+ |
| `/setmp` | Set MP | 1+ |
| `/setmag` | Set magic | 1+ |
| `/setattackmode [mode]` | Change attack mode | 1+ |

## Character
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/setinvi` or `/invi` | Set invisible | 1+ |
| `/invincible` | Toggle invincibility | 1+ |
| `/noaggro` | Toggle monster aggro | 1+ |
| `/obs` | Observer mode | 3+ |
| `/polymorph [npc]` | Transform into an NPC | 1+ |
| `/rep+ [name]` | Increase reputation | 1+ |
| `/rep- [name]` | Decrease reputation | 1+ |

## Map & Events
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/clearmap` | Clear current map | 1+ |
| `/summon [npc]` | Summon an NPC/monster | 1+ |
| `/unsummonall` | Remove all summoned NPCs | 1+ |
| `/attack [target]` | Force attack | 1+ |
| `/createfish [type]` | Spawn fish | 1+ |
| `/energysphere [value]` | Energy sphere event | 1+ |

## Server Events (Admin 4+)
| Command | Description |
|---------|-------------|
| `/crusade` | Start crusade |
| `/endcrusade` | End crusade |
| `/apocalypse` | Start apocalypse event |
| `/endapocalypse` | End apocalypse |
| `/heldenian` | Start Heldenian |
| `/endheldenian` | End Heldenian |
| `/astoria [param]` | Astoria event |
| `/endastoria` | End Astoria |
| `/eventspell` | Spell event |
| `/eventarmor` | Armor event |
| `/eventshield` | Shield event |
| `/eventchat` | Chat event |
| `/eventparty` | Party event |
| `/eventreset` | Reset events |
| `/eventtp` | Teleport event |
| `/eventillusion` | Illusion event |
| `/reservefightzone` | Reserve fight zone |
| `/shutdownthisserverrightnow [delay]` | Shutdown server |

## Party
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/joinparty [name]` | Join party | 1+ |
| `/dismissparty` | Dismiss party | 1+ |
| `/getpartyinfo` | Get party info | 1+ |
| `/deleteparty` | Delete party | 1+ |

## Other
| Command | Description | Admin Level |
|---------|-------------|-------------|
| `/mcount` | Monster count | 1+ |
| `/send [message]` | Send server message | 1+ |
| `/afk` | Toggle AFK | 1+ |
| `/checkrep` | Check reputation | 1+ |
| `/setpf [value]` | Set profile | 1+ |
| `/pf [name]` | View profile | 1+ |
| `/hold` | Hold/freeze | 1+ |
| `/free` | Unfreeze | 1+ |
| `/setforcerecalltime [seconds]` | Set force recall timer | 1+ |
| `/getticket` | Get ticket | 2+ |

---

## Setting Admin Level

The admin level is stored at byte offset 242 in the character save data (`m_iAdminUserLevel`).
- Level 1+: Basic GM commands
- Level 2+: Item creation
- Level 3+: Observer mode
- Level 4+: Server events, shutdown
