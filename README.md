# gbs-ScreenScrollPlugin

**Version 4.3.0 — Requires GB Studio ≥ 4.3.0**

A GB Studio engine plugin that enables seamless screen-scrolling transitions between scenes, similar to the overworld navigation in *The Legend of Zelda: Link's Awakening*. When the player walks off the edge of a scene, the screen scrolls in that direction and loads the neighbouring scene without a fade. The player and camera glide smoothly across the boundary, and the game loop stays fully active throughout.

All supported scene types (Top-Down, Platformer, Adventure, Point & Click, SHMUP) work with the plugin. Two events are added to the **Scene** group: **Set Neighbour Scene** and **Assign current scene scroll offset to Variable**.

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [HUD Margins](#hud-margins)
4. [Technicalities and Restrictions](#technicalities-and-restrictions)
5. [Events Reference](#events-reference)
6. [Engine Fields and Settings](#engine-fields-and-settings)
7. [Inner Workings](#inner-workings)

---

## Concepts

### How the Scroll Transition Works

GB Studio normally resets the viewport and tilemap when changing scenes. This plugin sidesteps the reset by keeping the `bkg_offset_x`/`bkg_offset_y` accumulators alive across scene loads and by managing the camera and player positions manually during the transition. The net result is that the screen content slides continuously in one direction while the new scene's tiles load row-by-row or column-by-column into the off-screen portion of the VRAM background map.

### The VRAM Tilemap as a Ring Buffer

The GB hardware background tilemap is 32×32 tiles but only 20×18 tiles are visible at once. The plugin exploits this by treating the map as a wrap-around ring buffer: when scrolling right, the new scene's column data is written into the left edge of the VRAM map (which is off-screen on the right side thanks to the SCX register), so no visual pop occurs.

The `bkg_offset_x` and `bkg_offset_y` fields accumulate the total tile displacement across all transitions. They are masked to 5 bits (`& 31`) to stay within the 32-tile VRAM map dimension.

---

## Project Setup

### 1. Assign a Common Tileset

All scenes that scroll into each other must share the same **common tileset**. Click the puzzle-piece icon on each scene in GB Studio and assign the same common tileset asset. This ensures tile indices are consistent across scene boundaries so that the visual join is seamless.

<img width="899" height="672" alt="image" src="https://github.com/user-attachments/assets/28ee25f5-b938-4af9-b176-a03d6135bbd1" />

### 2. Design Scenes with Matching Edge Dimensions

Scenes can be larger than the screen, but **the dimension along the shared edge must match exactly** between the two connecting scenes:

- A scene to the left/right of another must have the **same height**.
- A scene above/below another must have the **same width**.

### 3. Add Set Neighbour Scene to Each Scene's Init Script

In the **On Init** script of every scene that can scroll to a neighbour, add a **Set Neighbour Scene** event for each direction that has a neighbour. There is no need to place triggers on scene edges — the plugin detects boundary crossing automatically.

- Set the **Scene** to the neighbour scene in that direction.
- Set the **Direction of scroll** (Up, Down, Left, Right).
- Check **Round position to nearest tile** if using the Top-Down scene type so the player snaps cleanly to the tile grid after crossing.

https://github.com/user-attachments/assets/6544e245-1e59-4194-81b1-e7568e39e8b2

https://github.com/user-attachments/assets/111357fe-5c1b-4a8e-9b9b-e1d281330c75

https://github.com/user-attachments/assets/0c77d5f0-4e99-4337-b7ca-3bd9a135d766

https://github.com/user-attachments/assets/9b7bf82a-9763-4abd-8066-53a8d565579c

---

## HUD Margins

If your game displays a fixed HUD on the overlay/window layer, the plugin needs to know its size so the scroll boundaries and camera calculations account for the reduced playfield area.

| HUD Position | Setting to adjust |
|---|---|
| Bottom-aligned HUD | Set **Bottom margin** to the HUD height in tiles. |
| Right-aligned HUD | Set **Right margin** to the HUD width in tiles. |
| Top-aligned HUD | Set **Bottom margin** to the HUD height in tiles **and** set **Top scroll offset** to the HUD height in pixels. |

The **Bottom margin** shrinks the effective scene height used for scroll boundary calculations. The **Right margin** shrinks the effective width. The **Top scroll offset** shifts the draw scroll Y downward in pixels so the background origin aligns below the top HUD.

---

## Technicalities and Restrictions

### Maximum Scene Size is Halved

Due to the ring-buffer nature of the VRAM tilemap, the usable scene dimensions are limited to **128 tiles wide and 128 tiles tall** (half of the standard GB Studio maximum of 256×256). Exceeding this causes visual wrap-around corruption during transitions.

### Matching Edge Sizes

The dimension perpendicular to the scroll direction must be identical on both sides of the boundary. For example, a left-right scroll requires both scenes to have the same number of tile rows. Mismatched sizes produce an offset seam.

### Common Tileset Is Required

Both connecting scenes must use the same common tileset. Because no tileset reload happens during a scroll transition (the VRAM tile data stays unchanged), any tile in the new scene that is not present in the shared tileset will display incorrectly.

### Scripts Are Killed on Transition

When a transition begins, `script_runner_init(FALSE)` is called, which terminates all running script contexts in the current scene **without** clearing variables. Timers, input events, and music events are also reset (`timers_init(FALSE)`, `events_init(FALSE)`, `music_init_events(FALSE)`). The new scene's init scripts run after the scene loads.

### Camera Is Unlocked During Transition

The `CAMERA_LOCK_FLAG` is cleared at transition start and restored once both the camera and player have reached their target positions. The `camera_update` function returns immediately while `is_transitioning_scene` is non-zero, leaving camera movement entirely to the `transition_camera_to` step-interpolation function.

### Player Sprite and Tileset Loading

- **Disable player sprite loading on scene scroll** (enabled by default): prevents redundant VRAM writes if the player sprite is the same in both scenes.
- **Disable tileset loading on scene scroll**: prevents the tileset from being reloaded if both scenes share the same common tileset fully (saves time but must only be used when truly identical).
- **Disable loading UI tileset on scene load**: prevents the UI tileset reload on every scene load; useful if the UI is part of the common tileset.

---

## Events Reference

Both events are in the **Scene** group.

---

### Set Neighbour Scene

**`EVENT_SET_NEIGHBOUR_SCENE`**

Registers a scene as the neighbour in a given direction and enables boundary-crossing detection for the current scene. Must be called in the scene's **On Init** script. Can be called up to four times (once per direction) to register all neighbours.

| Field | Description |
|-------|-------------|
| Scene | The scene to scroll to when the player exits in the chosen direction. |
| Direction of scroll | Up, Down, Left, or Right — the direction the screen will scroll when the boundary is crossed. |
| Round position to nearest tile | Snaps the player's position to the nearest tile grid after the transition completes. Recommended for Top-Down scenes to prevent sub-tile misalignment. |

---

### Assign current scene scroll offset to Variable

**`EVENT_GET_SCROLL_OFFSET`**

Reads the current accumulated background offset (`bkg_offset_x`, `bkg_offset_y`) and stores the values, masked to 0–31, into two variables. This is useful for scripts that need to compensate for the viewport shift when drawing to fixed screen positions (e.g. placing overlay elements that must align with world tiles).

| Field | Description |
|-------|-------------|
| X Offset Variable | Destination variable for the horizontal tile offset (0–31). |
| Y Offset Variable | Destination variable for the vertical tile offset (0–31). |

---

## Engine Fields and Settings

These settings are found under **Settings → Engine Fields → Screen Scroll**.

### HUD Layout Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| **Right margin** (`scroll_right_margin`) | Slider (0–20 tiles) | 0 | Width in tiles reserved by a right-aligned HUD. Shrinks the effective horizontal scroll area. |
| **Bottom margin** (`scroll_bottom_margin`) | Slider (0–18 tiles) | 0 | Height in tiles reserved by a bottom- or top-aligned HUD. Shrinks the effective vertical scroll area. |
| **Top scroll offset** (`scroll_top_offset`) | Slider (0–144 px) | 0 | Pixel offset applied to the draw scroll Y each frame. Use to push the background origin below a top-aligned HUD. |

### Player Transition Distance

These values are in **sub-pixels** (256 sub-pixels = 1 tile = 8 px). They control how far into the new scene the player walks before the scroll animation ends and the camera re-locks.

| Setting | Default (subpx) | Description |
|---------|-----------------|-------------|
| **Player transition right distance** | 512 | Distance the player travels right after crossing the right edge. |
| **Player transition left distance** | 512 | Distance the player travels left after crossing the left edge. |
| **Player transition top distance** | 512 | Distance the player travels upward after crossing the top edge. |
| **Player transition bottom distance** | 512 | Distance the player travels downward after crossing the bottom edge. |

### Transition Trigger Thresholds

The threshold is compared against the player's position in sub-pixels. A transition triggers when the player's coordinate is **less than** the threshold (for top/left) or **greater than** `scene_size − threshold` (for bottom/right).

| Setting | Default (subpx) | Description |
|---------|-----------------|-------------|
| **Player transition right threshold** | 512 | Minimum distance from the right edge to trigger a right scroll. |
| **Player transition left threshold** | 0 | Position below which a left scroll triggers. |
| **Player transition top threshold** | 256 | Position above which an upward scroll triggers. |
| **Player transition bottom threshold** | 256 | Minimum distance from the bottom edge to trigger a downward scroll. |

### Performance Flags

| Setting | Default | Description |
|---------|---------|-------------|
| **Disable player sprite loading on scene scroll** | Enabled | Skips re-uploading the player sprite VRAM data on scroll transitions. Safe when the player sprite is unchanged between scenes. |
| **Disable tileset loading on scene scroll** | Disabled | Skips full tileset VRAM reload on scroll transitions. Only enable if both scenes use an identical common tileset. |
| **Disable loading UI tileset on scene load** | Disabled | Skips the UI tileset reload on every scene load. Enable if the UI tiles are baked into the common tileset. |

### Runtime-Only Fields

These are read-only engine fields accessible via **Engine Field Value** in scripts.

| Field | Description |
|-------|-------------|
| `scene_transition_enabled` | Non-zero when at least one neighbour scene has been registered (i.e. after any **Set Neighbour Scene** call). |
| `is_transitioning_scene` | Non-zero and equal to the direction flag while a scroll is in progress (1=Up, 2=Right, 4=Down, 8=Left). Zero when idle. |
| `bkg_offset_x` | Accumulated horizontal tile offset of the viewport (0–31). Updated on every transition. |
| `bkg_offset_y` | Accumulated vertical tile offset of the viewport (0–31). Updated on every transition. |

---

## Inner Workings

### Boundary Detection (`check_transition_to_scene_collision`)

Each frame, the state update loop calls `check_transition_to_scene_collision` when `scene_transition_enabled` is set and no transition is already in progress. It compares the player's current X and Y positions against four thresholds:

- Left: `PLAYER.pos.x < player_transition_left_threshold`
- Right: `PLAYER.pos.x > TILE_TO_SUBPX(image_tile_width) - player_transition_right_threshold`
- Top: `PLAYER.pos.y < player_transition_top_threshold`
- Bottom: `PLAYER.pos.y > TILE_TO_SUBPX(image_tile_height) - player_transition_bottom_threshold`

A position-change guard (`transitioning_player_pos_x/y != PLAYER.pos.x/y`) prevents the same crossing from triggering multiple frames in a row. When a threshold is crossed and the corresponding `far_ptr_t` has a non-zero bank, `transition_to_scene_modal` is called with the matching direction flag.

### Scene Load Phase (`transition_load_scene`)

Before loading the new scene this function:

1. Hides all active actors (except the player) and moves them to the overlay layer temporarily.
2. Clears all active projectiles.
3. Renders one final OAM frame so sprites disappear cleanly.
4. Adjusts `camera_x`/`camera_y`, `PLAYER.pos`, and `bkg_offset_x`/`bkg_offset_y` for the scroll direction:
   - **Right scroll**: camera jumps left by one screen width; player X is decremented by the scene width; `bkg_offset_x` is incremented by the scene tile width.
   - **Down scroll**: camera jumps up by one screen height; player Y is decremented by the scene height; `bkg_offset_y` is incremented.
   - **Left/Up**: The new scene is loaded first, then the camera and player are offset in the opposite direction and `bkg_offset` is decremented.
5. Kills all running scripts (`script_runner_init(FALSE)`) and resets timers and event handlers.
6. Calls `load_scene` for the new scene. Because the tile offsets were pre-adjusted and `is_transitioning_scene` is set, `scroll_reset` inside `scroll_init` skips clearing `scroll_x/y` and `bkg_offset_x/y`, preserving the cross-scene continuity.
7. If a position-rounding flag is set for the direction, the target player position is snapped to the nearest tile boundary (clearing the lower 8 bits and optionally adding one tile for Up/Left transitions).

### Scroll Animation Phase (`transition_to_scene_modal`)

After `transition_load_scene` returns, the function enters a loop that runs every frame:

1. `script_runner_update` ticks the new scene's init scripts until the VM is no longer locked.
2. `transition_camera_to` steps `camera_x`/`camera_y` toward `transitioning_cam_pos_x/y` by up to `SCROLL_CAM_SPEED` (128 sub-pixels) per frame.
3. `transition_player_to` steps the player's position toward `transitioning_player_pos_x/y` by up to `SCROLL_PLAYER_SPEED` (16 sub-pixels) per frame.
4. The normal game-loop updates (`camera_update`, `scroll_update`, `actors_update`, OAM, VBlank) all run. Because `is_transitioning_scene` is non-zero, `camera_update` is skipped (the camera is driven by `transition_camera_to`) and `scroll_update` bypasses its scroll-clamp logic for the transition axis, allowing the viewport to travel beyond the normal scene bounds.
5. When both `transition_camera_to` and `transition_player_to` return `TRUE`, `CAMERA_LOCK_FLAG` is restored and `is_transitioning_scene` is cleared, ending the loop and returning control to the standard game loop.

### Scroll Margin Rendering

At the start of the animation phase, before entering the loop, any HUD margin rows/columns are rendered into the correct VRAM positions. For example, on a downward scroll, `scroll_render_rows` fills the top `scroll_bottom_margin` rows of the new viewport so the HUD area is correctly populated from the start of the slide.

### `bkg_offset` and Tile Alignment

`bkg_scroll_x` and `bkg_scroll_y` (the actual SCX/SCY values written to hardware) are computed each frame as:

```
bkg_scroll_x = draw_scroll_x + TILE_TO_PX(bkg_offset_x)
bkg_scroll_y = draw_scroll_y + TILE_TO_PX(bkg_offset_y)
```

The `bkg_offset` values shift the VRAM map origin so that tile data written into a specific VRAM ring-buffer slot always appears at the correct screen position, regardless of how many transitions have occurred. Without this accumulation, consecutive scroll transitions in the same direction would progressively misalign the background.
