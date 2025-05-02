#pragma once

struct MapNode {
	vec2 array_pos;
	vec2 size;
	bool hasChildren;
	bool split_vertically;
	int room_number;
	MapNode* child_one;
	MapNode* child_two;

	bool isFilled = false;

	MapNode() {};
};