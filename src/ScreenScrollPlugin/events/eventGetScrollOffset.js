export const id = "EVENT_GET_SCROLL_OFFSET";
export const name = "Assign current scene scroll offset to Variable";
export const groups = ["EVENT_GROUP_SCENE"];

export const autoLabel = (fetchArg) => {
  return `Assign current scene scroll offset to Variable`;
};

export const fields = [
  {
    key: "x_variable",
	label: "X Offset Variable",
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  },
  {
    key: "y_variable",
	label: "Y Offset Variable",
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  },
];


export const compile = (input, helpers) => {
	const { appendRaw, getVariableAlias, _addComment, _rpn } = helpers;

	const x_variable_alias = getVariableAlias(input.x_variable);
	const y_variable_alias = getVariableAlias(input.y_variable);
	_addComment("Store scrolling offset in variable");
	appendRaw(
	`VM_GET_UINT8 ${x_variable_alias}, _bkg_offset_x 
	 VM_GET_UINT8 ${y_variable_alias}, _bkg_offset_y`
	);
	
	_rpn()
          .ref(x_variable_alias)
		  .int8(31)
		  .operator(".B_AND")
          .refSet(x_variable_alias)
		  .ref(y_variable_alias)
		  .int8(31)
		  .operator(".B_AND")
          .refSet(y_variable_alias)
          .stop();
};
