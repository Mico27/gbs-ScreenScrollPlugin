# gbs-ScreenScrollPlugin

**Version 4.3.0 — Requires GB Studio ≥ 4.3.0**

A GB Studio engine plugin that enables seamless screen-scrolling transitions between scenes, similar to the overworld navigation in *The Legend of Zelda: Link's Awakening*. When the player walks off the edge of a scene, the screen scrolls in that direction and loads the neighbouring scene without a fade. The player and camera glide smoothly across the boundary, and the game loop stays fully active throughout.

All supported scene types (Top-Down, Platformer, Adventure, Point & Click, SHMUP) work with the plugin. Three events are added to the **Scene** group: **Set Neighbour Scene**, **Auto Connect Neighbour Scenes** and **Assign current scene scroll offset to Variable**.

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [Size Limits and Restrictions](#size-limits-and-restrictions)
4. [Events Reference](#events-reference)
5. [Engine Settings](#engine-settings)
6. [Memory Footprint](#memory-footprint)
7. [Bank 0 (HOME) Usage](#bank-0-home-usage)
8. [Changelog](#changelog)

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

### HUD Margins

If your game displays a fixed HUD on the overlay/window layer, the plugin needs to know its size so the scroll boundaries and camera calculations account for the reduced playfield area.

| HUD Position | Setting to adjust |
|---|---|
| Bottom-aligned HUD | Set **Bottom margin** to the HUD height in tiles. |
| Right-aligned HUD | Set **Right margin** to the HUD width in tiles. |
| Top-aligned HUD | Set **Bottom margin** to the HUD height in tiles **and** set **Top scroll offset** to the HUD height in pixels. |

The **Bottom margin** shrinks the effective scene height used for scroll boundary calculations. The **Right margin** shrinks the effective width. The **Top scroll offset** shifts the draw scroll Y downward in pixels so the background origin aligns below the top HUD.

---

## Size Limits and Restrictions

### Maximum Scene Size is Halved

Due to the ring-buffer nature of the VRAM tilemap, the usable scene dimensions are limited to **128 tiles wide and 128 tiles tall** (half of the standard GB Studio maximum of 256×256). Exceeding this causes visual wrap-around corruption during transitions.

### Matching Edge Sizes

The dimension perpendicular to the scroll direction must be identical on both sides of the boundary. For example, a left-right scroll requires both scenes to have the same number of tile rows. Mismatched sizes produce an offset seam.

### Common Tileset Is Required

Both connecting scenes must use the same common tileset. Because no tileset reload happens during a scroll transition (the VRAM tile data stays unchanged), any tile in the new scene that is not present in the shared tileset will display incorrectly.

### Scripts Are Killed on Transition

When a transition begins, every running script in the current scene is terminated — **without** clearing variables. Timers, input events and music events are reset too. The new scene's init scripts run once the scene has loaded.

### Camera Is Unlocked During Transition

The camera lock is cleared at the start of a transition and restored once both the camera and the player have reached their target positions. Normal camera following is suspended for the duration, so the transition owns the camera.

### Player Sprite and Tileset Loading

- **Disable player sprite loading on scene scroll** (enabled by default): prevents redundant VRAM writes if the player sprite is the same in both scenes.
- **Disable tileset loading on scene scroll**: prevents the tileset from being reloaded if both scenes share the same common tileset fully (saves time but must only be used when truly identical).
- **Disable loading UI tileset on scene load**: prevents the UI tileset reload on every scene load; useful if the UI is part of the common tileset.

---

## Events Reference

All events are in the **Scene** group.

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

### Auto Connect Neighbour Scenes

**`EVENT_AUTO_CONNECT_NEIGHBOUR_SCENE`**

Automatically wires up **Set Neighbour Scene** calls for a whole group of scenes at **compile time**, based on how the scenes are laid out in the GB Studio editor. Place this event once, in the **On Init** script of a dedicated empty "compiler" scene — the event itself emits no runtime code where it is placed.

> **Important:** scene scripts are compiled in project scene order, and this event can only inject into scenes that are compiled *after* the scene containing it. The scene holding the event must therefore be the **first scene of the project** (it is first when it is the first scene ever added; on an existing project, edit the scene's `.gbsres` file and set its `"_index"` lower than every other scene's, e.g. `-1`). For the same reason the hosting scene itself never receives auto-connections — use a scene that is not part of the connected map.

For every scene whose **GBVM symbol** starts with the given prefix (set the symbol per scene under the scene's settings), the event looks for other matching scenes whose edges touch it in the editor and injects a script at the start of that scene's On Init that registers each detected neighbour via `set_neighbour_scene`. Scene positions are compared in tiles (editor pixel position ÷ 8).

Because the scroll transition preserves the player's position along the shared edge with no offset correction, **two scenes are only connected when their edges are exactly aligned**: left/right neighbours must have the same top edge, up/down neighbours the same left edge. Scenes that merely overlap at an offset are skipped.

| Field | Description |
|-------|-------------|
| Scene data symbol prefix | Only scenes whose GBVM symbol starts with this prefix are considered for connection. Leave empty to match every scene. |
| Loop Horizontally | Additionally connects scenes on the left-most map edge to aligned scenes on the right-most map edge (wrap-around world). |
| Loop Vertically | Additionally connects scenes on the top-most map edge to aligned scenes on the bottom-most map edge. |
| Round position to nearest tile | Applies the tile-snap flag to every generated connection. Recommended for Top-Down scenes. |

The usual plugin restrictions still apply to auto-connected scenes: shared common tileset, matching edge dimensions, and a maximum scene size of 128×128 tiles.

---

### Assign current scene scroll offset to Variable

**`EVENT_GET_SCROLL_OFFSET`**

Reads the current accumulated background offset (`bkg_offset_x`, `bkg_offset_y`) and stores the values, masked to 0–31, into two variables. This is useful for scripts that need to compensate for the viewport shift when drawing to fixed screen positions (e.g. placing overlay elements that must align with world tiles).

| Field | Description |
|-------|-------------|
| X Offset Variable | Destination variable for the horizontal tile offset (0–31). |
| Y Offset Variable | Destination variable for the vertical tile offset (0–31). |

---

## Engine Settings

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

<!-- SETTINGCOST:BEGIN -->
### What each engine setting costs

Every setting here changes what gets compiled. Figures are what you **get back by
turning the setting off**; rows marked *off by default* show what turning it **on**
costs instead, and sliders show the cost per step. A dash means that budget does not
move.

| Setting | Bank 0 | WRAM | Banked ROM |
|---|---|---|---|
| Disable player sprite loading on scene scroll | — | — | **16 B** |
| Disable tileset loading on scene scroll *(off by default — cost of turning it on)* | — | — | +7 B |
| Disable loading ui tileset on scene load *(off by default — cost of turning it on)* | — | — | −8 B |

Turning off every on-by-default switch above frees **16 B** of banked ROM — the full
span between this plugin at its fullest and stripped to nothing. Treat it as a
ceiling rather than a recipe: you keep whatever your game actually uses.

<details><summary>How these were measured</summary>

GB Studio 4.3.0-e1. This plugin's `engine/src/**/*.c` was compiled with the
toolchain and flags GB Studio itself uses (`lcc -msm83:gb -Wf--max-allocs-per-node 3000
-DHUGE_TRACKER -DRUMBLE_ENABLE=0x08u`) against a merged include tree, and the SDCC object
files' area records were read: `_HOME` is bank 0, `_DATA`/`_INITIALIZED`/`_BSS` are WRAM,
and `_CODE*`/`_CONST`/`_LIT`/`_INITIALIZER` are banked ROM.

Two caveats. Only this plugin's own engine sources are measured, so a setting that also
changes a struct shared with stock engine files can move a few more bytes in files the
plugin does not ship. And each setting is toggled on its own: a handful measure slightly
*negative* because enabling their code lets the compiler drop a fallback path elsewhere,
and settings that gate other settings only show their own contribution.

</details>
<!-- SETTINGCOST:END -->

## Memory Footprint

Measured against the stock GB Studio **4.3.0-e1** engine by `measure_plugin_memory.js` (per-file SDCC compile with GB Studio's own build flags, at default engine settings; report of 2026-08-13). Figures are this plugin's *delta* versus stock — a file that replaces a stock engine file counts only the difference, which is why a plugin can come out negative. Using the plugin's events additionally compiles a few bytes of GBVM script per call into your project's script banks, on top of the fixed cost below.

| Budget | Cost |
|---|---|
| Bank 0 (HOME) | −152 bytes |
| WRAM | +46 bytes |
| Banked ROM | +3,707 bytes |

- **Bank 0:** the plugin *gives back* 152 bytes — its replacements for stock engine files compile smaller than the originals. See [Bank 0 (HOME) Usage](#bank-0-home-usage).
- **WRAM:** 46 bytes, mostly scene-transition scratch state.
- **Banked ROM:** 3,707 bytes, 53 of which land in stock files the plugin does not ship but which recompile differently because it overrides `camera.h`, `collision.h`, `scroll.h` and `ui.h`. It replaces nine stock engine files, so the figure is a net one.
- **Engine WRAM headroom:** a stock GB Studio 4.3.0 project leaves about **854 bytes** of WRAM free (usable engine WRAM is 7,776 bytes at 0xC0A0–0xDF00; the stock engine uses 6,922). With this plugin installed roughly **808 bytes** remain. That does not change with the number of global variables your project defines: the script memory array is a fixed 3,584 bytes at stock engine settings (VM_HEAP_SIZE + VM_MAX_CONTEXTS × VM_CONTEXT_STACK_SIZE = 768 + 16 × 64 words).
- **SRAM:** not used.

---

<!-- BANK0:BEGIN -->
## Bank 0 (HOME) Usage

Bank 0 is the 16 KB non-switchable ROM bank that the GB Studio engine core,
the interrupt handlers and the GBDK runtime all share. Banked ROM is cheap
(add another bank), bank 0 is not, so it is usually the first thing a project
runs out of.

| | Bytes |
|---|---|
| Bank 0 used by this plugin | **−152** |
| Bank 0 free with this plugin installed | **1,603** of 16,384 (90% used) |

**This plugin gives bank 0 space back.** Its replacements for stock engine
files compile smaller than the originals, freeing 152 bytes.

| Module | This plugin | Stock engine | Bank 0 cost |
|---|---|---|---|
| `core/actor.c` | 669 | 871 | −202 |
| `core/collision.c` | 431 | 401 | +30 |
| `core/scroll.c` | 306 | 286 | +20 |

Modules that replace or patch a stock engine file only cost the *difference*:
the stock version's bank 0 bytes were being spent anyway.

<details><summary>How this was measured</summary>

GB Studio 4.3.0-e1, default engine settings. Each module is compiled with the
toolchain and flags GB Studio itself uses, and the `A _HOME size` record SDCC
writes into the resulting `.rel` object is read back; the stock column is the
same compile of the engine file this module replaces.

The "free" figure is a stock project with this plugin and nothing else. Your
own number will differ: other plugins, and any engine settings that change what
the core compiles, move it independently of this plugin.

</details>
<!-- BANK0:END -->

## Changelog

Grouped by the date each change was merged into the official
[gb-studio-plugins](https://github.com/gb-studio-dev/gb-studio-plugins) repository.

Only bug fixes, new features and feature changes are listed. Engine version
bumps, patch regeneration, packaging fixes and documentation edits are omitted.

### 2026-08-02

- Implemented the ContinuousScene plugin's auto-connect event variant for ScreenScroll.

### 2026-06-28

- Added ContinuousScenePlugin compatibility.

### 2026-06-14

- Added custom script parameter / stack support to the events.

### 2026-06-08

First published in the official plugin repository. This entry covers everything
developed since the plugin's standalone release in July 2024:

- New event to store the scroll offset in a variable, for tilemap editing.
- Script lock support.
- Refined the `#define` settings for the transition threshold and distance.
- Exposed additional engine fields.
- Fixes: normal scene load after a scene scroll, and the small blip when scrolling up with a HUD margin.
