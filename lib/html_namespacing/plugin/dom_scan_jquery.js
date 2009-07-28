jQuery(function($) {
	function NS_from_node(root_node) {
		var SPLIT_REGEX = /[ \t\n]+/,
		    ret = {},
				node_stack = [root_node],
				cur_node,
				class_name_list,
				class_names,
				class_name,
				i,
				j,
				children,
				subnode;

		while (node_stack.length) {
			cur_node = node_stack.pop();

			// Populate entry in $.NS
			class_name_list = cur_node.className;
			if (class_name_list) {
				class_names = class_name_list.split(SPLIT_REGEX);
				for (i = 0; i < class_names.length; i++) {
					class_name = class_names[i];
					if (!ret[class_name]) {
						ret[class_name] = [cur_node];
					} else {
						ret[class_name].push(cur_node);
					}
				}
			}

			// "recurse" to the children, so they are handled in document order
			children = cur_node.childNodes;
			for (var j = children.length - 1; j >= 0; j--) {
				subnode = children[j];
				node_stack.push(subnode);
			}
		}

		return ret;
	}

	$.NS = NS_from_node(document.getElementsByTagName('body')[0]);
});
