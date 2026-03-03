# gbs-ScreenScrollPlugin
 Screen Scroll plugin between scenes for GBS

How to use:
all default scene types are supported. Due to how scene scrolling works, the maximum size of a scene is halved (max width = 128 tiles and max height = 128 tiles)
Scenes can be bigger than the screen size, however, neighboring scenes must match the same size of the side it is connected to.
If you are displaying a hud at the bottom, you can set in the screenscroll settings a bottom margin the size of the height of your hud in tile.
For a hud displayed to the right side, add the right margin to the width of your hud in tile.
For a hud displayed on top, set the bottom margin to the size of the height of your hud in tile and set the top scroll offset to the size of the height of the hud in pixels.

<img width="899" height="672" alt="image" src="https://github.com/user-attachments/assets/28ee25f5-b938-4af9-b176-a03d6135bbd1" />

To be able to make a scene able to screen scroll into another scene, add the "add neighbour scene" event in the init of the scene.
There is no need to add a trigger on the edge of the scene with a "change scene event" on it. The plugin takes care of it.
Click the rounding tile checkbox if you are using the TopDown scene type.

Also make sure you use a common tileset between the scene that you are scrolling to. (Click on the puzzle piece on a scene to assign a common tileset)

https://github.com/user-attachments/assets/6544e245-1e59-4194-81b1-e7568e39e8b2

https://github.com/user-attachments/assets/111357fe-5c1b-4a8e-9b9b-e1d281330c75

https://github.com/user-attachments/assets/0c77d5f0-4e99-4337-b7ca-3bd9a135d766

https://github.com/user-attachments/assets/9b7bf82a-9763-4abd-8066-53a8d565579c
