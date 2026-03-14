export const id = "EVENT_SET_NEIGHBOUR_SCENE";
export const name = "Set Neighbour Scene";
export const groups = ["EVENT_GROUP_SCENE"];

export const autoLabel = (fetchArg) => {
  return `Set Neighbour Scene`;
};

export const fields = [
  {
    key: "sceneId",
    label: "Scene",
    type: "scene",
    defaultValue: "LAST_SCENE",
  },
  {
    key: "direction",
    label: "Direction of scroll",
    type: "direction",
    defaultValue: "",
  },
  {
    key: "rounded",
    type: "checkbox",
    label: "Round position to nearest tile",
    defaultValue: false,
  },
];


export const compile = (input, helpers) => {
	const { _addComment, options, _stackPushConst, _callNative, _addNL, _stackPop } = helpers;
	
	_addComment("Set neighbour scene");
	const { scenes } = options;
	const scene = scenes.find((s) => s.id === input.sceneId);
	_stackPushConst((input.rounded)? 1: 0);
	_stackPushConst(toUbyteDir(input.direction));
	_stackPushConst((scene)? `_${scene.symbol}`: 0);
	_stackPushConst((scene)? `___bank_${scene.symbol}`: 0);
	
	_callNative("set_neighbour_scene"); 
	_stackPop(4);    
	
	_addNL();
};

const toUbyteDir = direction => {
  if (direction === "up") {
    return 1;
  } else if (direction === "right") {
    return 2;
  } else if (direction === "down") {
    return 4;
  } else if (direction === "left") {
    return 8;
  }
  return 0;
};