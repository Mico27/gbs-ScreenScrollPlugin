export const id = "EVENT_AUTO_CONNECT_NEIGHBOUR_SCENE";
export const name = "Auto Connect Neighbour Scenes";
export const groups = ["EVENT_GROUP_SCENE"];

export const autoLabel = (fetchArg) => {
  return `Auto Connect Neighbour Scenes`;
};

export const fields = [
  {
    key: `scene_data_symbol_prefix`,
    label: "Scene data symbol prefix",
    type: "text",
  },
  {
    key: `loop_horizontally`,
    label: "Loop Horizontally",
    description: "Whether to connect scenes on the left and right edges of the map",
    type: "checkbox",
  },
  {
    key: `loop_vertically`,
    label: "Loop Vertically",
    description: "Whether to connect scenes on the top and bottom edges of the map",
    type: "checkbox",
  },
  {
    key: `rounded`,
    label: "Round position to nearest tile",
    description: "Snaps the player's position to the nearest tile grid after each transition. Recommended for Top-Down scenes",
    type: "checkbox",
  },
];

//Direction flags used by set_neighbour_scene (scene_transition.h)
const DIRECTION_UP = 1;
const DIRECTION_RIGHT = 2;
const DIRECTION_DOWN = 4;
const DIRECTION_LEFT = 8;

export const compile = (input, helpers) => {
    const { options } = helpers;

    const { scenes } = options;

    const prefix = input.scene_data_symbol_prefix || "";

    let left_most_scene_left_edge = Infinity;
    let right_most_scene_right_edge = -Infinity;
    let top_most_scene_top_edge = Infinity;
    let bottom_most_scene_bottom_edge = -Infinity;
    let scene_connections = [];
    //For each scene, check for any other scenes that are right next to it and create a connection object for them.
    //Unlike the ContinuousScene version, the scroll transition preserves the player's position on the
    //perpendicular axis with no offset correction, so edges must be exactly aligned to connect.
    scenes.forEach((scene) => {
        //if scene gbvm symbol doesnt start by scene_data_symbol_prefix, skip it
        if (!scene.symbol.startsWith(prefix)) {
            return;
        }
        const connections = [];
        const scene_left = scene.x >> 3;
        const scene_right = scene_left + scene.width;
        const scene_top = scene.y >> 3;
        const scene_bottom = scene_top + scene.height;
        if (scene_left < left_most_scene_left_edge) {
            left_most_scene_left_edge = scene_left;
        }
        if (scene_right > right_most_scene_right_edge) {
            right_most_scene_right_edge = scene_right;
        }
        if (scene_top < top_most_scene_top_edge) {
            top_most_scene_top_edge = scene_top;
        }
        if (scene_bottom > bottom_most_scene_bottom_edge) {
            bottom_most_scene_bottom_edge = scene_bottom;
        }
        scenes.forEach((other_scene) => {
            if (scene.id === other_scene.id) {
                return;
            }
            if (!other_scene.symbol.startsWith(prefix)) {
                return;
            }
            const other_scene_left = other_scene.x >> 3;
            const other_scene_right = other_scene_left + other_scene.width;
            const other_scene_top = other_scene.y >> 3;
            const other_scene_bottom = other_scene_top + other_scene.height;
            if (scene_right === other_scene_left && scene_top === other_scene_top) {
                connections.push({
                    scene_symbol: other_scene.symbol,
                    direction: DIRECTION_RIGHT,
                });
            } else if (scene_left === other_scene_right && scene_top === other_scene_top) {
                connections.push({
                    scene_symbol: other_scene.symbol,
                    direction: DIRECTION_LEFT,
                });
            } else if (scene_bottom === other_scene_top && scene_left === other_scene_left) {
                connections.push({
                    scene_symbol: other_scene.symbol,
                    direction: DIRECTION_DOWN,
                });
            } else if (scene_top === other_scene_bottom && scene_left === other_scene_left) {
                connections.push({
                    scene_symbol: other_scene.symbol,
                    direction: DIRECTION_UP,
                });
            }
        });
        scene_connections.push({
            scene: scene,
            scene_symbol: scene.symbol,
            connections,
        });
    });
    const hasConnection = (scene_connection, direction) =>
        scene_connection.connections.some((c) => c.direction === direction);
    if (input.loop_horizontally) {
        //connect left most scenes to right most scenes
        const left_most_scenes = scenes.filter((scene) => (scene.x >> 3) === left_most_scene_left_edge && scene.symbol.startsWith(prefix));
        const right_most_scenes = scenes.filter((scene) => ((scene.x >> 3) + (scene.width)) === right_most_scene_right_edge && scene.symbol.startsWith(prefix));
        left_most_scenes.forEach((left_scene) => {
            const left_scene_top = left_scene.y >> 3;
            right_most_scenes.forEach((right_scene) => {
                const right_scene_top = right_scene.y >> 3;
                if (left_scene_top === right_scene_top) {
                    const left_connection = scene_connections.find((s) => s.scene_symbol === left_scene.symbol);
                    const right_connection = scene_connections.find((s) => s.scene_symbol === right_scene.symbol);
                    if (!hasConnection(left_connection, DIRECTION_LEFT)) {
                        left_connection.connections.push({
                            scene_symbol: right_scene.symbol,
                            direction: DIRECTION_LEFT,
                        });
                    }
                    if (!hasConnection(right_connection, DIRECTION_RIGHT)) {
                        right_connection.connections.push({
                            scene_symbol: left_scene.symbol,
                            direction: DIRECTION_RIGHT,
                        });
                    }
                }
            });
        });
    }
    if (input.loop_vertically) {
        //connect top most scenes to bottom most scenes
        const top_most_scenes = scenes.filter((scene) => (scene.y >> 3) === top_most_scene_top_edge && scene.symbol.startsWith(prefix));
        const bottom_most_scenes = scenes.filter((scene) => ((scene.y >> 3) + (scene.height)) === bottom_most_scene_bottom_edge && scene.symbol.startsWith(prefix));
        top_most_scenes.forEach((top_scene) => {
            const top_scene_left = top_scene.x >> 3;
            bottom_most_scenes.forEach((bottom_scene) => {
                const bottom_scene_left = bottom_scene.x >> 3;
                if (top_scene_left === bottom_scene_left) {
                    const top_connection = scene_connections.find((s) => s.scene_symbol === top_scene.symbol);
                    const bottom_connection = scene_connections.find((s) => s.scene_symbol === bottom_scene.symbol);
                    if (!hasConnection(top_connection, DIRECTION_UP)) {
                        top_connection.connections.push({
                            scene_symbol: bottom_scene.symbol,
                            direction: DIRECTION_UP,
                        });
                    }
                    if (!hasConnection(bottom_connection, DIRECTION_DOWN)) {
                        bottom_connection.connections.push({
                            scene_symbol: top_scene.symbol,
                            direction: DIRECTION_DOWN,
                        });
                    }
                }
            });
        });
    }

    //insert a GBVM event at the start of each connected scene's init script that registers
    //its neighbours with the set_neighbour_scene native (neighbours are cleared on every
    //scene load by scene_transition_reset, so each scene has to re-register its own)
    const rounded = input.rounded ? 1 : 0;
    let sceneConnectionCounter = 0;
    scene_connections.forEach((scene_connection) => {
        if (scene_connection.connections.length === 0) {
            return;
        }
        scene_connection.connections.sort((a, b) => a.direction - b.direction);
        const script = scene_connection.connections.map((connection) =>
            `VM_PUSH_CONST ${rounded}\nVM_PUSH_CONST ${connection.direction}\nVM_PUSH_CONST _${connection.scene_symbol}\nVM_PUSH_CONST ___bank_${connection.scene_symbol}\nVM_CALL_NATIVE b_set_neighbour_scene, _set_neighbour_scene\nVM_POP 4\n`
        ).join("");
        const setNeighbourScenesEvent = {
          id: `auto_connect_neighbour_scenes_${prefix}_${sceneConnectionCounter}`,
          command: "EVENT_GBVM_SCRIPT",
          args: {
            "script": script,
          },
        };
        scene_connection.scene.script.unshift(setNeighbourScenesEvent);
        sceneConnectionCounter++;
    });
};
