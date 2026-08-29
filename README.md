# gbs-ScreenScrollPlugin

**Version 4.3.0. Requires GB Studio 4.3.0 or newer.**

Scrolls from one scene into the next when the player walks off the edge, the way The Legend of Zelda: Link's Awakening moves around its overworld. No fade, no loading pause. The screen slides, the new scene comes in behind it, and the player keeps walking.

That turns a project made of separate rooms into one continuous world, which is what an overworld map, a dungeon of connected rooms or a long side-scrolling level wants.

It works in Top-Down, Platformer, Adventure, Point and Click and Shmup scenes. Three events are added to the **Scene** group: **Set Neighbour Scene**, **Auto Connect Neighbour Scenes** and **Assign current scene scroll offset to Variable**.

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [Size Limits and Restrictions](#size-limits-and-restrictions)
4. [Events Reference](#events-reference)
5. [Engine Settings](#engine-settings)
6. [FAQ](#faq)
7. [Memory Footprint](#memory-footprint)
8. [Bank 0 (HOME) Usage](#bank-0-home-usage)
9. [Changelog](#changelog)

---

## Concepts

### How the Scroll Transition Works

GB Studio normally clears the view and the background when a scene changes. This plugin keeps the current scroll position across the load and drives the camera and the player itself while the transition runs. The screen slides in one direction while the new scene's tiles are written, a row or a column at a time, into the part of the background that is off screen.

### The background wraps around

The hardware background is 32 by 32 tiles and only 20 by 18 are on screen. The plugin uses the rest: scrolling right, the new column is written at the far side of the map, which wraps around to just off the right edge of the screen, so nothing pops into view.

The **Scroll offset X** and **Scroll offset Y** values track how far the view has moved in total. They run from 0 to 31 and wrap, matching the size of the background.

---

## Project Setup

### 1. Assign a Common Tileset

Every scene that scrolls into another must share one **common tileset**. Click the puzzle-piece icon on each scene and pick the same one. That keeps the tiles in the same places on both sides of the join, which is what makes it seamless.

<img width="899" height="672" alt="image" src="https://github.com/user-attachments/assets/28ee25f5-b938-4af9-b176-a03d6135bbd1" />

### 2. Design Scenes with Matching Edge Dimensions

Scenes can be larger than the screen, but **the shared edge must be exactly the same length** on both sides:

- Scenes side by side must have the **same height**.
- Scenes above and below each other must have the **same width**.

### 3. Add Set Neighbour Scene to Each Scene's Init Script

In the **On Init** script of every scene that scrolls to a neighbour, add a **Set Neighbour Scene** event for each direction that has one. No triggers are needed on the edges, because the plugin notices the crossing itself.

- Set **Scene** to the neighbour in that direction.
- Set **Direction of scroll** to Up, Down, Left or Right.
- Tick **Round position to nearest tile** in Top-Down scenes so the player lands neatly on the grid.

https://github.com/user-attachments/assets/6544e245-1e59-4194-81b1-e7568e39e8b2

https://github.com/user-attachments/assets/111357fe-5c1b-4a8e-9b9b-e1d281330c75

https://github.com/user-attachments/assets/0c77d5f0-4e99-4337-b7ca-3bd9a135d766

https://github.com/user-attachments/assets/9b7bf82a-9763-4abd-8066-53a8d565579c

---

### HUD Margins

If your game shows a fixed heads-up display on the overlay, the plugin needs its size, so the edges and the camera account for the smaller playing area.

| HUD Position | Setting to adjust |
|---|---|
| Bottom-aligned HUD | Set **Bottom margin** to the HUD height in tiles. |
| Right-aligned HUD | Set **Right margin** to the HUD width in tiles. |
| Top-aligned HUD | Set **Bottom margin** to the HUD height in tiles **and** set **Top scroll offset** to the HUD height in pixels. |

**Bottom margin** shortens the scene height the plugin works with, **Right margin** shortens the width, and **Top scroll offset** pushes the background down in pixels so it starts below a top display.

---

## Size Limits and Restrictions

### Maximum Scene Size is Halved

Because the background wraps, scenes can be at most **128 tiles wide and 128 tall**, half the usual GB Studio maximum. Going beyond that makes the picture wrap onto itself during a transition.

### Matching Edge Sizes

The shared edge must be the same length on both sides. A left to right scroll needs both scenes to have the same number of tile rows. A mismatch leaves a visible step at the join.

### Common Tileset Is Required

Both scenes must use the same common tileset. No tiles are reloaded during a scroll, so any tile the new scene needs that is not in the shared tileset comes out wrong.

### Scripts Are Killed on Transition

Every running script in the current scene is stopped when a transition begins. Variables are left alone. Timers, input events and music events are reset. The new scene's init scripts run once it has loaded.

### Camera Is Unlocked During Transition

The camera lock is cleared when a transition starts and restored once both the camera and the player have arrived. Normal following is suspended for the duration.

### Player Sprite and Tileset Loading

- **Disable player sprite loading on scene scroll**, on by default, skips reloading the player sprite when it is the same in both scenes.
- **Disable tileset loading on scene scroll** skips the tileset reload when both scenes share exactly the same common tileset. Only turn it on when they truly do.
- **Disable loading UI tileset on scene load** skips the interface tileset reload on every scene load. Useful when the interface tiles are part of the common tileset.

---

## Events Reference

All events are in the **Scene** group.

---

### Set Neighbour Scene

Names the scene that lies in a given direction and switches on edge detection for the current scene. Put it in the scene's **On Init** script, once per direction with a neighbour.

| Field | Description |
|-------|-------------|
| Scene | The scene to scroll to when the player exits in the chosen direction. |
| Direction of scroll | Up, Down, Left or Right. The way the screen scrolls when the player crosses that edge. |
| Round position to nearest tile | Puts the player neatly on the tile grid after the transition. Recommended in Top-Down scenes, where being half a tile off is visible. |

---

### Auto Connect Neighbour Scenes

Sets up every **Set Neighbour Scene** call for a whole group of scenes during the build, working out the connections from how you laid the scenes out in the editor. Drag your map into place and this event does the wiring.

Put it once, in the **On Init** script of an empty scene kept aside for the purpose. It adds nothing to your game where it sits.

> **Important:** scenes are built in project order, and this event can only reach scenes built after the one holding it. That scene has to be the **first scene of the project**, which it is when it is the first one you ever added. On an existing project, edit the scene's `.gbsres` file and set its `"_index"` below every other scene's, for instance `-1`. For the same reason the scene holding the event never gets connections of its own, so use one that is not part of the map.

For every scene whose **symbol** starts with the prefix you give, set per scene in the scene's settings, the event finds other matching scenes whose edges touch it in the editor and adds the connections to the start of that scene's On Init.

Because a scroll keeps the player's position along the shared edge exactly, **two scenes connect only when their edges line up**. Side by side neighbours must share a top edge, and stacked ones must share a left edge. Scenes that merely overlap are skipped.

| Field | Description |
|-------|-------------|
| Scene data symbol prefix | Only scenes whose symbol starts with this are considered. Leave it empty to match every scene. |
| Loop Horizontally | Also connects scenes at the left edge of the map to aligned scenes at the right edge, for a world that wraps around. |
| Loop Vertically | The same, top edge to bottom edge. |
| Round position to nearest tile | Applies tile snapping to every connection it makes. Recommended for Top-Down scenes. |

The usual rules still apply to connections made this way: one shared common tileset, matching edge lengths, and a maximum scene size of 128 by 128 tiles.

---

### Assign current scene scroll offset to Variable

Puts how far the view has scrolled, from 0 to 31 on each axis, into two variables. Scripts that draw at fixed screen positions need it to line up with world tiles, for instance when placing something on the background that must sit over a particular tile.

| Field | Description |
|-------|-------------|
| X Offset Variable | Receives the horizontal offset, 0 to 31. |
| Y Offset Variable | Receives the vertical offset, 0 to 31. |

---

## Engine Settings

Found under **Settings**, then **Engine**, then **Screen Scroll**.

### HUD Layout Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| **Right margin** | Slider, 0 to 20 tiles | 0 | Width taken by a display on the right. Shortens the horizontal scrolling area. |
| **Bottom margin** | Slider, 0 to 18 tiles | 0 | Height taken by a display at the top or bottom. Shortens the vertical scrolling area. |
| **Top scroll offset** | Slider, 0 to 144 pixels | 0 | Pushes the background down each frame, so it starts below a display at the top. |

### Player Transition Distance

These are in subpixels, where 256 is one tile of 8 pixels. They set how far into the new scene the player walks before the scroll finishes and the camera locks again.

| Setting | Default (subpx) | Description |
|---------|-----------------|-------------|
| **Player transition right distance** | 512 | Distance the player travels right after crossing the right edge. |
| **Player transition left distance** | 512 | Distance the player travels left after crossing the left edge. |
| **Player transition top distance** | 512 | Distance the player travels upward after crossing the top edge. |
| **Player transition bottom distance** | 512 | Distance the player travels downward after crossing the bottom edge. |

### Transition Trigger Thresholds

These are compared against the player's position in subpixels. A transition starts when the player is nearer the edge than the threshold.

| Setting | Default (subpx) | Description |
|---------|-----------------|-------------|
| **Player transition right threshold** | 512 | Minimum distance from the right edge to trigger a right scroll. |
| **Player transition left threshold** | 0 | Position below which a left scroll triggers. |
| **Player transition top threshold** | 256 | Position above which an upward scroll triggers. |
| **Player transition bottom threshold** | 256 | Minimum distance from the bottom edge to trigger a downward scroll. |

### Performance Flags

| Setting | Default | Description |
|---------|---------|-------------|
| **Disable player sprite loading on scene scroll** | Enabled | Skips reloading the player sprite during a scroll. Safe when the sprite is the same in both scenes. |
| **Disable tileset loading on scene scroll** | Disabled | Skips the tileset reload during a scroll. Only turn it on when both scenes use exactly the same common tileset. |
| **Disable loading UI tileset on scene load** | Disabled | Skips the interface tileset reload on every scene load. Turn it on when the interface tiles are part of the common tileset. |

### Values scripts can read

These are read-only and available through **Engine Field Value**.

| Field | Description |
|-------|-------------|
| **Scene transition enabled** | Not zero once at least one neighbour has been named with **Set Neighbour Scene**. |
| **Is transitioning scene** | While a scroll runs, this holds the direction: 1 up, 2 right, 4 down, 8 left. Zero the rest of the time. |
| **Scroll offset X** | How far the view has scrolled horizontally, 0 to 31. |
| **Scroll offset Y** | How far the view has scrolled vertically, 0 to 31. |

---

## FAQ

**How do I build a Link's Awakening style overworld?**
Give every scene the same common tileset, make neighbouring scenes match along their shared edges,
and lay them out in a grid in the editor. Then use **Auto Connect Neighbour Scenes** once, in the
first scene of the project, and the whole map is wired up.

**Do I have to add a Set Neighbour Scene event to every scene by hand?**
Only if you want to. **Auto Connect Neighbour Scenes** does the whole map from the editor layout.
Use the manual event for one-off connections or a map that is not laid out in a grid.

**My scroll shows garbled tiles in the new scene.**
The two scenes are not using the same common tileset, or the new scene uses tiles the shared
tileset does not have. Nothing is reloaded during a scroll, so both scenes have to draw from the
same set.

**There is a visible step at the join between two scenes.**
Their shared edges are different lengths. Scenes side by side need the same height, and stacked
scenes the same width.

**My auto-connect did nothing.**
Two usual reasons: the scene holding the event is not the first scene in the project, or the scene
symbols do not start with the prefix you gave. Scenes also have to line up exactly, sharing a top
edge for side by side neighbours and a left edge for stacked ones.

**Can my scenes be bigger than 128 by 128 tiles?**
No. That is the limit while this plugin is installed, half the usual maximum, because the
background has to hold two scenes' worth of edge at once.

**My scripts stop running when the screen scrolls.**
Every running script in the leaving scene is stopped. Variables survive. Move anything that must
continue into the new scene's On Init.

**My heads-up display overlaps the playfield or the scroll stops in the wrong place.**
Set **Bottom margin** or **Right margin** to the size of the display in tiles. For a display at the
top, also set **Top scroll offset** to its height in pixels.

**How do I make the world wrap around at the edges?**
Tick **Loop Horizontally** or **Loop Vertically** on the auto-connect event, and scenes at opposite
edges of the map connect to each other.

**How far does the player walk into the new scene?**
512 subpixels by default, which is two tiles. The four transition distance settings change it per
direction.

**Can I speed up the transition?**
Turn on **Disable tileset loading on scene scroll** when both scenes share exactly the same common
tileset, and leave **Disable player sprite loading on scene scroll** on.

**My overlay drawing lands on the wrong tiles after a few scrolls.**
The view has moved. Read **Assign current scene scroll offset to Variable** and add the offset to
your positions.

**Does it work with the MetaTile or ContinuousScene plugins?**
Yes. Compatibility variants ship for both.

---

<!-- SETTINGCOST:BEGIN -->
### What each engine setting costs

Each setting changes what gets compiled. Figures are what you **get back by turning
the setting off**. Rows marked *off by default* show what turning it **on** costs, and
sliders show the cost per step. "none" means that budget does not move.

| Setting | Bank 0 | WRAM | Banked ROM |
|---|---|---|---|
| Disable player sprite loading on scene scroll | none | none | **16 B** |
| Disable tileset loading on scene scroll *(off by default, so this is the cost of turning it on)* | none | none | +7 B |
| Disable loading ui tileset on scene load *(off by default, so this is the cost of turning it on)* | none | none | -8 B |

Turning off every on-by-default switch above frees **16 B** of banked ROM. That is the
span between the plugin at its fullest and stripped to nothing, so treat it as a
ceiling. You keep whatever your game actually uses.

<details><summary>How these were measured</summary>

GB Studio 4.3.0-e1. This plugin's engine code was compiled with the toolchain and
flags GB Studio itself uses, and the size of each part of the result was read back and
sorted into the three budgets: the fixed bank 0, work RAM, and switchable ROM banks.

Two caveats. Only this plugin's own engine sources are measured, so a setting that also
changes a shared data structure can move a few more bytes elsewhere. And each setting is
toggled on its own, so a few measure slightly *negative* when enabling their code lets
the compiler drop a fallback path, and a setting that gates other settings shows only
its own contribution.

</details>
<!-- SETTINGCOST:END -->

## Memory Footprint

Measured against the stock GB Studio **4.3.0-e1** engine at default engine settings, report of 2026-08-13. Figures are the difference against a stock project: a file that replaces a stock engine file counts only the change, which is why a plugin can come out negative. Each event you use also compiles a few bytes of script into your project, on top of the fixed cost below.

| Budget | Cost |
|---|---|
| Bank 0 (HOME) | -152 bytes |
| WRAM | +46 bytes |
| Banked ROM | +3,707 bytes |

- **Bank 0:** the plugin *gives back* 152 bytes, because its replacements for stock engine files compile smaller than the originals. See [Bank 0 (HOME) Usage](#bank-0-home-usage).
- **WRAM:** 46 bytes, mostly working state for the transition.
- **Banked ROM:** 3,707 bytes. 53 of those land in stock files the plugin does not ship, which compile slightly differently once it is installed. It replaces nine stock engine files, so the figure is what is left after subtracting them.
- **Engine WRAM headroom:** a stock GB Studio 4.3.0 project leaves about **854 bytes** of WRAM free (the engine has 7,776 bytes to work with and uses 6,922 of them). With this plugin installed roughly **808 bytes** remain. Adding more global variables to your project does not change that figure, because script memory is a fixed 3,584 byte block at stock engine settings.
- **SRAM:** not used.

---

<!-- BANK0:BEGIN -->
## Bank 0 (HOME) Usage

Bank 0 is the 16 KB fixed ROM bank shared by the GB Studio engine core, the
interrupt handlers and the GBDK runtime. Extra banked ROM is cheap to add,
bank 0 is not, so bank 0 is usually the first thing a project runs out of.

| | Bytes |
|---|---|
| Bank 0 used by this plugin | **-152** |
| Bank 0 free with this plugin installed | **1,603** of 16,384 (90% used) |

**This plugin gives bank 0 space back.** Its replacements for stock engine
files compile smaller than the originals, freeing 152 bytes.

| Module | This plugin | Stock engine | Bank 0 cost |
|---|---|---|---|
| Actor handling | 669 | 871 | -202 |
| Collision | 431 | 401 | +30 |
| Scrolling | 306 | 286 | +20 |

A module that replaces a stock engine file costs only the *difference*, because
the stock version's bank 0 bytes were being spent anyway.

<details><summary>How this was measured</summary>

GB Studio 4.3.0-e1, default engine settings. Each module was compiled with the
toolchain and flags GB Studio itself uses, and the bank 0 size the compiler
recorded was read back. The stock column is the same compile of the engine file
the module replaces.

The "free" figure assumes a stock project with this plugin and nothing else.
Your own number will differ, because other plugins and any engine settings that
change what the core compiles move it too.

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

- Added custom script parameter and stack support to the events.

### 2026-06-08

First published in the official plugin repository. This entry covers everything
developed since the plugin's standalone release in July 2024:

- Added the event that stores the scroll offset in a variable, for editing the background.
- Added script lock support.
- Refined the transition threshold and distance settings.
- Exposed more values for scripts to read.
- Fixed a normal scene load after a scroll, and the small jump when scrolling up with a display margin.
