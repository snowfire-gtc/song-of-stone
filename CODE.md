# Song of Stone - Programmed Manual (CODE.md)

## Project Overview

**Song of Stone** is an epic 2D medieval warfare platformer built with C and raylib. Two teams (Blue and Red) compete in a Capture-the-Flag style game where players must capture the enemy flag and bring it to their throne while defending their own.

### Game Features

- **Multiplayer Support**: Up to 128 players per team on dedicated servers
- **Three Character Classes**: Worker, Warrior, and Archer - each with unique abilities
- **Destructible Terrain**: Full block-based world with mining, building, and physics
- **Network Play**: Client-server architecture with snapshot interpolation and input prediction
- **Dynamic Weather**: Clear, rain, and snow conditions
- **Rich Audio System**: Context-aware sound effects and adaptive music

---

## Build System

### Directory Structure

```
/workspace/
├── Makefile              # Root build wrapper
├── src/                  # Source code directory
│   ├── Makefile          # Main build configuration
│   ├── main.c            # Application entry point
│   ├── logic*.c/h        # Game logic modules
│   ├── draw*.c/h         # Rendering modules
│   ├── net_*.c/h         # Network components
│   └── ...               # Other modules
├── build/                # Compiled objects and binaries
├── include/              # Header files (raylib)
├── libs/                 # Libraries (libraylib.a)
├── data/                 # Assets (textures, sounds, music)
└── docs/                 # Documentation
```

### Building the Project

```bash
# Build release version
make

# Build debug version
make debug

# Build standalone network server
make server

# Clean build artifacts
make clean

# Test build
make test
```

### Build Targets

| Target | Description | Output |
|--------|-------------|--------|
| `make` / `make release` | Full game client (release) | `build/song-of-stone` |
| `make debug` | Full game client (debug) | `build/song-of-stone` |
| `make server` | Standalone dedicated server | `ctf_server` |

---

## Architecture

### Core Modules

#### 1. Main Module (`main.c`, `main.h`)
- Game initialization and main loop
- State machine: Menu → Playing → Paused
- Input handling and rendering coordination

#### 2. Logic System (`logic*.c/h`)
Game logic is modularized by character type and functionality:

| File | Purpose |
|------|---------|
| `logic.c` | Core game logic, respawn system, collision detection |
| `logic_worker.c` | Worker abilities: mining, building, resource gathering |
| `logic_warrior.c` | Warrior combat: sword attacks, bombs, shield mechanics |
| `logic_archer.c` | Archer abilities: shooting, climbing, arrow harvesting |
| `logic_bomb.c` | Bomb physics, explosion, area damage |
| `logic_arrow.c` | Arrow projectile physics and collision |
| `logic_physics.c` | Character movement, gravity, collision response |
| `logic_items.c` | Item drops, pickups, inventory management |
| `logic_flags.c` | Flag capture mechanics and scoring |

#### 3. Rendering System (`draw*.c/h`)
Modular rendering for each game element:

| File | Purpose |
|------|---------|
| `draw.c` | Main render coordinator |
| `draw_warrior.c` | Warrior character sprites and animations |
| `draw_archer.c` | Archer character sprites and animations |
| `draw_worker.c` | Worker character sprites and animations |
| `draw_bomb.c` | Bomb rendering with fuse animation |
| `draw_arrow.c` | Arrow projectile rendering |
| `draw_ui.c` | HUD, health bars, inventory display |
| `draw_ascii.c` | ASCII fallback rendering mode |

#### 4. Network System (`net_*.c/h`)
Client-server networking implementation:

| File | Purpose |
|------|---------|
| `net_server.c` | UDP server: packet handling, snapshots, client management |
| `net_protocol.c` | Packet serialization/deserialization |
| `client.c` | Client-side networking: input sending, state interpolation |
| `local_server.c` | Local single-player server simulation |

#### 5. World Generation (`gen.c`, `gen.h`)
- Procedural terrain generation using Perlin noise
- Resource placement (gold, trees, stone)
- Throne and flag positioning

#### 6. Audio System (`sound.c`, `sound.h`)
- Sound effect playback with positional audio
- Adaptive music intensity based on game state
- Sound categories: mining, combat, environment, UI

#### 7. Particle System (`particles.c`, `particles.h`)
- Block break particles
- Explosion effects
- Environmental particles (rain, snow)

#### 8. Menu System (`menu.c`, `menu.h`)
- Main menu navigation
- Server browser and connection
- Settings (video, audio, controls, appearance)
- Pause menu

#### 9. Settings System (`settings.c`, `settings.h`)
- Configuration file parsing
- Key binding management
- Video/audio settings persistence

#### 10. Debug Console (`debug_console.c`, `debug_console.h`)
- Runtime debugging commands
- World inspection tools
- Cheat commands for testing

---

## Game Mechanics

### Character Types

#### Worker (CHAR_WORKER)
**Abilities:**
- Mine blocks (dirt, stone, gold, wood)
- Build structures: spikes, bridges, ladders, doors
- Plant trees
- Move dirt instantly

**Build Costs:**
- Spikes: 20 wood
- Bridge: 20 wood
- Ladder: 10 wood
- Door: 20 wood

**Mining Speed:**
- Dirt: 0.2 seconds per hit
- Stone: 0.5 seconds per hit
- Gold: 0.3 seconds per hit

#### Warrior (CHAR_WARRIOR)
**Abilities:**
- Sword attack (normal and charged)
- Throw bombs
- Activate shield for defense

**Combat Mechanics:**
- Normal damage: 2 HP
- Charged attack (>0.8s): 4 HP
- Shield reduces melee damage by 50%
- Rocket jump from bomb explosions when shield is active
- Movement speed reduced by 50% when shield is active

**Bomb Mechanics:**
- Cost: 20 coins per bomb
- Explosion radius: 2 blocks
- Fuse time: configurable (default 2 seconds)
- Destroys stone, wood, and structures

#### Archer (CHAR_ARCHER)
**Abilities:**
- Shoot arrows with charge-based power
- Climb trees and vertical walls
- Harvest arrows from wood (2 arrows) and leafs (1 arrow)
- Fall damage negation when landing on leafs

**Arrow Mechanics:**
- Max arrows: 200
- Purchase: 10 coins for 10 arrows
- Power scales with charge time (0.2-1.0s)

### Block Types

| Block | Properties | Interactions |
|-------|------------|--------------|
| BLOCK_AIR | Passable | - |
| BLOCK_WATER | Swimable, flows | Drowns characters without oxygen |
| BLOCK_DIRT | Mineable | All characters can mine |
| BLOCK_LAVA | Damaging | Extinguished by water |
| BLOCK_STONE | Solid, durable | Only workers can mine (10 stone drop) |
| BLOCK_WOOD | Solid, burnable | Grows over time, yields arrows |
| BLOCK_LEAFS | Passable | Archers can climb, harvest arrows |
| BLOCK_GOLD | Valuable | 2 coins per hit for any character |
| BLOCK_SPIKES | Hazard | 1 damage on contact |
| BLOCK_BRIDGE | Team-specific | Only builder's team can walk |
| BLOCK_LADDER | Climbable | All characters can use |
| BLOCK_DOOR | Team-specific | Only builder's team can pass |
| BLOCK_GRASS | Decorative | Single-hit removal |
| BLOCK_BOMB | Explosive | Placed by warriors |
| BLOCK_SAND | Falling | Physics-enabled |
| BLOCK_GRAVEL | Falling | Physics-enabled |

### Health and Damage

- **Max HP**: 6 (displayed as 3 hearts)
- **Fall Damage**: 1 HP per 4 blocks fallen beyond 10 blocks
- **Lethal Fall Height**: 34+ blocks
- **Invulnerability**: 5 seconds after respawn (visualized by flickering)

### Oxygen System

- **Max Oxygen**: 30 units
- **Depletion Rate**: 1 unit/second when fully submerged
- **Recovery**: Regenerates when at least one block is above water

### Respawn System

- **Respawn Time**: Proportional to distance from throne at death
- **Maximum Wait**: 120 seconds (2 minutes)
- **Camera**: Shifts to throne view during wait

---

## Network Architecture

### Protocol Design

**Transport**: UDP for low-latency gameplay

**Packet Types:**
- `PKT_HELLO`: Client connection handshake
- `PKT_INPUT`: Player input commands
- `PKT_ACTION`: Special actions (attack, build, etc.)
- `PKT_SNAPSHOT`: Server state updates (30 Hz)
- `PKT_WORLD_STATE`: Initial world synchronization
- `PKT_CHAT`: Chat messages
- `PKT_DISCONNECT`: Graceful disconnection

### Server Architecture

**GameServer Structure:**
```c
typedef struct {
    int socket_fd;
    ServerConfig config;
    WorldState* world;
    ClientInfo clients[MAX_CLIENTS];
    Character characters[MAX_CLIENTS];
    int client_count;
    double game_time;
    unsigned long total_ticks;
    Weather weather;
} GameServer;
```

**Tick Rate**: 60 Hz server logic, 30 Hz snapshot delivery

**Client Management:**
- Rate limiting: 60 packets/second per client
- Address-based client identification (UDP connectionless)
- Automatic timeout detection

### Client Architecture

**Interpolation**: Smooth rendering between received snapshots
**Extrapolation**: Predict entity positions between updates
**Input Buffering**: Queue inputs for reliable delivery

---

## World Parameters

### Configurable Settings

```c
typedef struct {
    int width_blocks;           // Map width
    int height_blocks;          // Map height
    float gravity;              // Gravity acceleration
    float max_speed;            // Maximum movement speed
    float friction;             // Ground friction
    int bomb_fuse_time_seconds; // Bomb timer
    float respawn_time_per_block; // Sec/block respawn delay
    float dropped_coin_ratio;   // Coin drop percentage on death
    float flag_capture_share;   // Token share for flag capture
    bool enable_falling_blocks; // Enable block physics
    bool enable_sliding_blocks; // Enable block sliding
} WorldParams;
```

### Default Constants

| Constant | Value | Description |
|----------|-------|-------------|
| WORLD_MAX_WIDTH | 1024 | Maximum map width (blocks) |
| WORLD_MAX_HEIGHT | 256 | Maximum map height (blocks) |
| MAX_PLAYERS | 128 | Maximum players per team |
| PLAYER_MAX_HP | 6 | Maximum health points |
| PLAYER_OXYGEN_MAX | 30 | Maximum oxygen units |
| BOMB_EXPLOSION_RADIUS | 2 | Explosion radius (blocks) |
| FLAG_RETURN_RADIUS | 2 | Flag auto-return distance |

---

## Controls

### Default Key Bindings

| Action | Key/Button |
|--------|-----------|
| Move Left | A / Left Arrow |
| Move Right | D / Right Arrow |
| Jump | W / Space |
| Attack | Left Mouse Button |
| Build/Action | Right Mouse Button |
| Shield Toggle | S (Warrior) |
| Climb | W/S near climbable surface (Archer) |
| Debug Console | ~ (tilde) |

---

## Audio System

### Sound Categories

1. **Mining Sounds**
   - `SOUND_DIG_GRASS`: Dirt/grass mining
   - `SOUND_DIG_STONE`: Stone mining
   - `SOUND_DIG_GOLD`: Gold mining
   - `SOUND_DIG_WOOD`: Wood chopping

2. **Combat Sounds**
   - `SOUND_SWORD_HIT_WOOD`: Sword vs wood
   - `SOUND_SWORD_HIT_STONE`: Sword vs stone
   - `SOUND_SWORD_HIT_DIRT`: Sword vs dirt
   - `SOUND_ARROW_FLY`: Arrow shot
   - `SOUND_BOMB_EXPLODE`: Bomb explosion
   - `SOUND_BOMB_THROW`: Bomb throw

3. **Environment Sounds**
   - `SOUND_BUILD`: Construction
   - `SOUND_BUILD_DOOR`: Door placement
   - `SOUND_PLANT_TREE`: Tree planting

4. **Character Sounds**
   - `SOUND_PLAYER_HURT`: Damage taken
   - `SOUND_FALL`: Fall damage

5. **Harvest Sounds**
   - `SOUND_ARROW_HARVEST_WOOD`: Arrow from wood
   - `SOUND_ARROW_HARVEST_LEAFS`: Arrow from leafs

### Music System

- **Format**: OGG
- **Adaptive Intensity**: Transitions between peaceful and combat themes
- **Intensity Factors**: 
  - Number of nearby enemies
  - Recent combat events
  - Flag carrier proximity

---

## Visual Effects

### Character Animations

| Animation | Trigger |
|-----------|---------|
| ANIM_IDLE | Standing still |
| ANIM_WALK | Moving horizontally |
| ANIM_JUMP | In air |
| ANIM_ATTACK | Attacking |
| ANIM_DIG | Mining/building |
| ANIM_SHIELD | Shield active (Warrior) |
| ANIM_SWIM | In water |
| ANIM_CLIMB | Climbing (Archer) |
| ANIM_HURT | Taking damage |
| ANIM_IDLE_FUNNY | Idle for 5+ seconds |

### Particle Effects

- Block break particles (type-specific colors)
- Explosion particles
- Water splash
- Blood splatter on damage
- Environmental particles (rain, snow)

### Weather System

| Weather | Effect |
|---------|--------|
| WEATHER_CLEAR | No precipitation |
| WEATHER_RAIN | Rain particles, reduced visibility |
| WEATHER_SNOW | Snow particles, ground accumulation |

---

## Data Structures

### Character Structure

```c
typedef struct {
    int x, y;                 // Position (pixels)
    float vx, vy;             // Velocity
    CharacterType type;       // Worker/Warrior/Archer
    Team team;                // Blue/Red/None
    int hp;                   // Health points
    bool is_shield_active;    // Warrior shield
    bool is_charging;         // Attack charge
    float charge_time;        // Charge duration
    AnimationState anim_state;
    int frame_counter;
    bool is_invulnerable;     // Post-respawn invincibility
    float invuln_timer;
    bool is_holding_flag;     // Flag carrier status
    char name[MAX_NAME_LEN];
    int player_id;
    bool facing_right;        // Facing direction
    
    // Inventory
    int coins;
    int wood;
    int stone;
    int arrows;
    int bombs;
    
    // Oxygen
    int oxygen;
    
    // Visual customization
    int head_style;
    int costume_style;
    
    // Archer-specific
    bool is_aiming;
    float aim_time;
    bool is_climbing;
    int climbing_block_x;
} Character;
```

### World State

```c
typedef struct {
    Block blocks[WORLD_MAX_HEIGHT][WORLD_MAX_WIDTH];
    Character characters[MAX_PLAYERS];
    DroppedItem items[MAX_PLAYERS * 4];
    int char_count;
    int item_count;
    Vector2 throne_blue;
    Vector2 throne_red;
    Vector2 flag_blue_pos;
    Vector2 flag_red_pos;
    bool flag_blue_carried;
    bool flag_red_carried;
    int flag_carrier_id;
    WorldParams params;
    bool game_over;
    Team winner;
    bool is_multiplayer;
    int local_player_id;
    Bomb bombs[MAX_PLAYERS * 2];
    Arrow arrows[MAX_PLAYERS * 10];
    int bomb_count;
    int arrow_count;
    FlagAnimation flag_anim;
} WorldState;
```

### Bomb Structure

```c
typedef struct {
    int x, y;           // Position (pixels)
    float vx, vy;       // Velocity
    BombState state;    // Flying/Planted/Exploding
    float timer;        // Fuse time remaining
    int owner_id;       // Throwing player ID
    Team owner_team;
    bool exploded;
} Bomb;
```

### Arrow Structure

```c
typedef struct {
    int x, y;           // Position (pixels)
    float vx, vy;       // Velocity
    Team owner_team;    // Shooter's team
    bool hit;           // Collision occurred
    float rotation;     // Flight angle (radians)
} Arrow;
```

---

## Scoring System

### Point Allocation

| Action | Points | Recipient |
|--------|--------|-----------|
| Flag Capture | 100 | Entire team |
| Enemy Kill | 1 | Killer |
| Flag Return | Bonus | Helper team members |

### Token Economy (Optional)

**Mode 1: Tournament**
- Players stake tokens on victory
- Winning team splits all tokens

**Mode 2: Persistent Server**
- Flag capture: Enemy team loses tokens, capturer gains
- Capturer shares portion with teammates

**Mode 3: Casual**
- No token economy
- Score-only tracking

---

## Debug Features

### Debug Console Commands

Access with `~` key during gameplay.

**Information Queries:**
- `pos` - Show character coordinates
- `inventory` - Display current inventory
- `block <x> <y>` - Query block type at position
- `netstat` - Network connection statistics

**World Modification:**
- `gravity <value>` - Set gravity
- `spawn <item> [amount]` - Spawn items
- `destroy <x> <y>` - Remove block at position
- `weather <clear|rain|snow>` - Change weather

**Character Modification:**
- `hp <value>` - Set health
- `coins <value>` - Set coin count
- `god` - Toggle invincibility

---

## File Organization Standards

### Naming Conventions

1. **Code Style**: snake_case for all identifiers
2. **Enum Prefixes**: Type-specific prefixes required
   - `BLOCK_*` for block types
   - `CHAR_*` for character types
   - `TEAM_*` for teams
   - `ITEM_*` for items
   - `ANIM_*` for animations
   - `BOMB_STATE_*` for bomb states

3. **File Naming**: Descriptive prefixes
   - `logic_*` for game logic
   - `draw_*` for rendering
   - `net_*` for networking
   - Avoid system header conflicts (e.g., no `types.h`)

### Character Names

| Role | Name |
|------|------|
| Warrior | warrior |
| Archer | archer |
| Worker | worker |
| Air | air |
| Dirt | dirt |
| Wood | wood |
| Stone | stone |
| Gold | gold |
| Lava | lava |
| Leafs | leafs |
| Grass | grass |
| Spikes | spikes |
| Door | door |
| Bridge | bridge |
| Ladder | ladder |

---

## Platform Requirements

### Supported Platforms

- **Linux** (primary development platform)
- Windows (via MinGW or MSVC)
- macOS (with Xcode toolchain)

### Dependencies

- **raylib 5.x** - Graphics, input, audio
- **GCC/Clang** - C11 compatible compiler
- **Make** - Build system
- **pthread** - Threading support
- **X11** - Linux windowing (or Wayland)

### Build Flags

```bash
# Release build
-O2 -DNDEBUG -flto

# Debug build
-g -O0 -DDEBUG

# Platform-specific
-D_POSIX_C_SOURCE=200809L  # POSIX compatibility
-DSTANDALONE_SERVER       # Dedicated server build
```

---

## Performance Considerations

### Optimization Strategies

1. **Rendering**
   - View frustum culling (only render visible blocks)
   - Sprite batching for characters
   - Level-of-detail for distant objects

2. **Physics**
   - Spatial partitioning for collision detection
   - Fixed timestep for deterministic simulation
   - Sleeping bodies for stationary objects

3. **Network**
   - Delta compression for snapshots
   - Entity interest management
   - Packet coalescing for small updates

4. **Memory**
   - Static allocation for game objects
   - Pool allocators for particles/items
   - Cache-friendly data layouts

### Target Performance

| Metric | Target |
|--------|--------|
| Frame Rate | 60 FPS |
| Server Tick Rate | 60 Hz |
| Snapshot Rate | 30 Hz |
| Max Players | 128 per team |
| Network Latency | <100ms recommended |

---

## Testing Guidelines

### Unit Testing

Test individual modules:
- Physics calculations
- Collision detection
- Packet serialization
- State transitions

### Integration Testing

Test subsystem interactions:
- Client-server synchronization
- Character ability combinations
- Multi-player scenarios

### Stress Testing

- Maximum player count
- High-frequency action sequences
- Network packet loss simulation
- Extended play sessions

---

## Troubleshooting

### Common Issues

**Build Failures:**
- Missing raylib: Ensure `libs/libraylib.a` exists
- Include path errors: Verify `-I../include` in Makefile
- Linker errors: Check library order (-lraylib before -lm)

**Runtime Issues:**
- Black screen: Check texture loading paths
- No audio: Verify OGG files in `data/sounds/`
- Network failures: Confirm firewall allows UDP port

**Performance Issues:**
- Low FPS: Reduce render distance, disable particles
- Network lag: Lower tick rate, reduce snapshot frequency
- Memory leaks: Use valgrind for detection

---

## Future Enhancements

### Planned Features

1. **Additional Game Modes**
   - Team Deathmatch
   - King of the Hill
   - Resource Race

2. **New Characters**
   - Engineer (advanced structures)
   - Mage (magical abilities)
   - Scout (stealth mechanics)

3. **Enhanced Graphics**
   - Dynamic lighting
   - Parallax scrolling backgrounds
   - Animated water/lava

4. **Advanced Networking**
   - Server browser with favorites
   - Replay system
   - Anti-cheat measures

5. **Modding Support**
   - Custom map editor
   - Scriptable game logic
   - Asset replacement

---

## License

See LICENSE file for terms and conditions.

---

## Credits

**Engine**: raylib (https://www.raylib.com/)  
**Language**: C11  
**Platform**: Linux/Windows/macOS  

---

*Document Version: 1.0*  
*Last Updated: 2024*  
*Maintained by: Development Team*
