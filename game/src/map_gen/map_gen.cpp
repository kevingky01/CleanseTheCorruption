#include "map_gen.hpp"
#include "tinyECS/registry.hpp"
#include <iostream>
#include "map_node.hpp"
#include "world_init.hpp"
#include "common.hpp" 

#include <fstream>
#include <sstream>

#include <algorithm>
#include <random> 

#include "enemy_types/enemy_components.hpp"
#include "dialogue/dialogue.hpp"

const int offset = TILE_SIZE;


// Constants for map generation (lots of tweaking may need to be done to get the map to look "right")
const int MAP_LENGTH = 100;
const int MAP_HEIGHT = 100;

const int MIN_ROOM_WIDTH = 8;
const int MIN_ROOM_HEIGHT = 8;

const int MAX_ROOM_ADJUST_SIZE = 4;

const int MIN_ROOM_TILES = 500;
const float MIN_SPLIT_PERCENTAGE = 0.45f;
// this ensures that when we split the last time, we'll always have
// at least min_room_tiles in the children
const int MAX_ROOM_TILES = MIN_ROOM_TILES / (1 - MIN_SPLIT_PERCENTAGE);

const int CORRIDOR_THICKNESS = 2;

const int MIN_WALL_THICKNESS = 8;
const int MIN_WALL_COLLISION_SIZE = 3;

// Room generation related constants
// Munn: Just random notes for myself. Let's say you have 16 rooms, and you want to fill them with stuff. 
//		 One room must be for the player spawn
//		 One room must be for the exit
//		 So you have 14 rooms to fill with enemies/chests or whatever
//		 4 chest rooms 
//		 10 rooms to be just filled with enemies
//		 Other ideas for rooms:
//			- "Ritual" rooms, sacrifice your hp for some sort of buff
//			- "Fountain" rooms, which just restore your health to full when you drink the fountain
//			- Hades-esque special rooms, where you are locked in for X seconds, and must just survive until the end. You will be rewarded for surviving

const int MAX_ENEMY_ROOMS = 10;
const int MAX_CHEST_ROOMS = 4;

// Variables, used when generating the map
int currentRoom = 3;

int num_enemy_rooms = 0;
int num_chest_rooms = 0;

// The very "bottom" nodes of the BSP, which are the actual rooms themselves
std::vector<MapNode*> room_nodes;


/* MAP INFO
* 0:		empty space
* 1:		WALL
* 2:		CORRIDOR
* 3 - 127:	ROOM (each room is filled with a different number)
* 128 - 255: CORRIDOR
*
 */


// Munn: This should be used for individual rooms (eg. tutorial room, hub room, boss rooms, etc.), not procedural generation
void loadLevelFromFile(std::vector<std::vector<int>>& arr, std::string file_path) {

	arr.clear();

	// Get file path and open file
	std::string path_to_rooms = "data/";
	std::string full_file_path = get_base_path() + path_to_rooms + file_path;
	std::ifstream file(full_file_path);

	// Initialize curr_line string
	std::string curr_line;

	// Iterate over file lines
	while (std::getline(file, curr_line)) {

		std::stringstream curr_line_stream(curr_line);

		std::string curr_int;
		std::vector<int> line_ints;

		// Iterate over line numbers and add them to line_ints
		while (std::getline(curr_line_stream, curr_int, ' ')) {
			int num = stoi(curr_int);

			line_ints.push_back(num);
		}

		// Add integers to file_arr
		for (int i = 0; i < line_ints.size(); i++) {
			int tile_num = line_ints[i];

			if (arr.size() <= i) {
				std::vector<int> new_arr;
				new_arr.push_back(tile_num);

				arr.push_back(new_arr);
			}
			else {
				std::vector<int>& this_arr = arr[i];
				this_arr.push_back(tile_num);
			}
		}
	}

	file.close();
}

void printMapArray(std::vector<std::vector<int>> arr) {
	// reversed the j and i for x y coordinate purposes
	/*for (int j = 0; j < MAP_HEIGHT; j++) {
		for (int i = 0; i < MAP_LENGTH; i++) {
			std::cout << arr[i][j] << " ";
			if (arr[i][j] < 10) {
				std::cout << " ";
			}
		}
		std::cout << std::endl;
	}*/

	std::ofstream myfile;
	std::string file_path = "data/test_map_gen.txt";
	std::string full_file_path = get_base_path() + file_path;
	myfile.open(full_file_path);
	for (int j = 0; j < arr[0].size(); j++) {
		for (int i = 0; i < arr.size(); i++) {
			myfile << arr[i][j] << " ";
			if (arr[i][j] < 10) {
				myfile << " ";
			}
		}
		myfile << std::endl;
	}
	myfile.close();
}

float getRandomSplitPercent(float max_split_percentage) {
	float random_split_percentage = 0;

	while (random_split_percentage <= max_split_percentage || random_split_percentage >= 1 - max_split_percentage) {
		random_split_percentage = uniform_dist(rng);
	}

	return random_split_percentage;
}

void initializeMap(std::vector<std::vector<int>>& arr, int length, int height) {

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			arr[i][j] = 0; // important to initialize entire array
		}
	}
}

bool is_exit_generated = false;


void adjustRoomSize(std::vector<std::vector<int>>& arr, MapNode* node, vec2 new_size) {
	// Empty all room info
	for (int x = node->array_pos.x; x < node->array_pos.x + node->size.x; x++) {
		for (int y = node->array_pos.y; y < node->array_pos.y + node->size.y; y++) {
			arr[x][y] = 0;
		}
	}

	new_size -= vec2(1);

	// Find center of the room
	vec2 roomCenterPos = vec2(
		((int)node->array_pos.x + (int)(node->size.x) / 2),
		((int)node->array_pos.y + (int)(node->size.y) / 2)
	);

	// Construct smaller room around center
	for (int x = roomCenterPos.x - (new_size.x / 2); x <= roomCenterPos.x + (new_size.x / 2); x++) {
		if (x <= 0 || x >= MAP_LENGTH) {
			continue;
		}
		for (int y = roomCenterPos.y - (new_size.y / 2); y <= roomCenterPos.y + (new_size.y / 2); y++) {
			if (y <= 0 || y >= MAP_HEIGHT) {
				continue;
			}

			arr[x][y] = node->room_number;
		}
	}

	// Adjust node properties
	node->array_pos.x = roomCenterPos.x - (new_size.x / 2);
	node->array_pos.y = roomCenterPos.y - (new_size.y / 2);
	node->size = new_size;
}



const vec2 PLAYER_SPAWN_ROOM_SIZE = vec2(9, 9);
// Move player to center of this room
void createPlayerSpawnRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {

	adjustRoomSize(arr, node, PLAYER_SPAWN_ROOM_SIZE);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	vec2 player_spawn_pos = roomCenter;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;
}

const vec2 EXIT_ROOM_SIZE = vec2(9, 9);

// Create floor exit at center of room
void createFloorExitRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	is_exit_generated = true;

	adjustRoomSize(arr, node, EXIT_ROOM_SIZE);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	// Create random boss
	float rand = uniform_dist(rng);
	if (rand > 0.5) {
		createNextLevelEntry(renderer, roomCenter, GAME_SCREEN_ID::BOSS_1);
	}
	else {
		createNextLevelEntry(renderer, roomCenter, GAME_SCREEN_ID::BOSS_2);
	}
}

std::vector<ENEMY_TYPE> random_enemy_types = {
	ENEMY_TYPE::BASIC_RANGED_ENEMY,
	ENEMY_TYPE::SHOTGUN_RANGED_ENEMY,
	ENEMY_TYPE::BASIC_MELEE_ENEMY,
	// M4
	ENEMY_TYPE::TOWER_ENEMY
};

// // For procedurally generated room managers
Entity createEnemyRoomManager(std::vector<std::vector<int>>& arr, RenderSystem* renderer, vec2 position, vec2 scale, MapNode* map_node) {
	auto entity = Entity();

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = scale;

	EnemyRoomManager& room_manager = registry.enemyRoomManagers.emplace(entity);
	room_manager.init(arr, map_node, random_enemy_types);
	room_manager.renderer = renderer; 

	// For debugging
	//registry.renderRequests.insert(
	//	entity,
	//	{
	//		TEXTURE_ASSET_ID::SPELL_DROP,
	//		EFFECT_ASSET_ID::TEXTURED,
	//		GEOMETRY_BUFFER_ID::SPRITE
	//	}
	//);

	Hitbox hitbox;
	hitbox.layer = (int)COLLISION_LAYER::ENEMY_ROOM_TRIGGER;
	hitbox.mask = (int)COLLISION_LAYER::PLAYER;
	hitbox.hitbox_scale = vec2(WALL_SIZE);
	registry.hitboxes.insert(entity, hitbox);

	return entity;
}



// For custom room managers
Entity createEnemyRoomManager(RenderSystem* renderer, vec2 position, vec2 scale, std::vector<EnemyWave> enemy_waves) {
	auto entity = Entity();

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = scale;

	MapNode* map_node = new MapNode();
	map_node->array_pos.x = position.x - scale.x / 2.0f;
	map_node->array_pos.y = position.y - scale.y / 2.0f;
	map_node->size = scale;
	map_node->hasChildren = false;

	EnemyRoomManager& room_manager = registry.enemyRoomManagers.emplace(entity);  
	room_manager.renderer = renderer;
	room_manager.enemy_waves = enemy_waves;



	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SPELL_DROP,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);
 
	Hitbox hitbox;
	hitbox.layer = (int)COLLISION_LAYER::ENEMY_ROOM_TRIGGER;
	hitbox.mask = (int)COLLISION_LAYER::PLAYER;
	hitbox.hitbox_scale = vec2(WALL_SIZE);
	registry.hitboxes.insert(entity, hitbox);

	return entity;
}


void createEnemyRoomFromFile(RenderSystem* renderer, std::vector<std::vector<int>>& arr, MapNode* node, std::string file_name) {
	
	// Get file path and open file
	std::string path_to_rooms = "data/enemy_rooms/";
	std::string full_file_path = get_base_path() + path_to_rooms + file_name;
	std::ifstream file(full_file_path);

	// Initialize curr_line string
	std::string curr_line;

	// Initialize file array (to eventually copy into arr)
	std::vector<std::vector<int>> file_arr;


	//std::cout << "Generating enemy room!" << std::endl;
	// Iterate over file lines
	while (std::getline(file, curr_line)) {
		//std::cout << curr_line << std::endl;

		std::stringstream curr_line_stream(curr_line);

		std::string curr_int;
		std::vector<int> line_ints;

		// Iterate over line numbers and add them to line_ints
		while (std::getline(curr_line_stream, curr_int, ' ')) {
			int num = stoi(curr_int);

			line_ints.push_back(num);
		}

		// Add integers to file_arr
		for (int i = 0; i < line_ints.size(); i++) {
			int tile_num = line_ints[i];

			if (file_arr.size() <= i) {
				std::vector<int> new_arr;
				new_arr.push_back(tile_num);

				file_arr.push_back(new_arr);
			}
			else {
				std::vector<int>& this_arr = file_arr[i];
				this_arr.push_back(tile_num);
			}
		}
		//std::cout << std::endl;
	}
	//std::cout << "Done generating enemy room!" << std::endl;

	// Adjust room size (Munn: if we want to be super efficient, we could just set the tile info as we adjust the room size, but I'm too lazy atm)
	vec2 new_size = vec2(
		file_arr.size(),
		file_arr[0].size()
	);
	adjustRoomSize(arr, node, new_size);

	// Copy tiles info into room
	for (int x = 0; x < new_size.x; x++) {
		for (int y = 0; y < new_size.y; y++) {
			int tile_info = file_arr[x][y];

			vec2 world_pos = vec2(
				(node->array_pos.x + x) * offset,
				(node->array_pos.y + y) * offset
			);

			// I don't know why... but without this there's a weird placement bug?
			int x_size = new_size.x;
			int y_size = new_size.y;
			if (x_size % 2 == 0) {
				world_pos.x -= 0.5 * offset;
			}
			if (y_size % 2 == 0) {
				world_pos.y -= 0.5 * offset;
			}

			if (tile_info == 0) {
				tile_info = node->room_number;
			}
			if (tile_info == 3) {
				createDestructableBox(renderer, world_pos);
				tile_info = node->room_number;
			}

			arr[node->array_pos.x + x][node->array_pos.y + y] = tile_info;
		}
	}
	
	file.close();
}

const int NUM_ENEMY_ROOMS = 8;

void createEnemyRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {

	// Adjust map according to enemy room file
	int random_room = (int)(uniform_dist(rng) * NUM_ENEMY_ROOMS);
	std::string file_name = "room_" + std::to_string(random_room) + ".txt";
	createEnemyRoomFromFile(renderer, arr, node, file_name);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	/*createRandomEnemy(renderer, roomCenter);
	createRandomEnemy(renderer, roomCenter + vec2(2, 2) * (float)TILE_SIZE);
	createRandomEnemy(renderer, roomCenter - vec2(2, 2) * (float)TILE_SIZE);*/

	createEnemyRoomManager(arr, renderer, roomCenter, node->size, node);
}


const vec2 CHEST_ROOM_SIZE = vec2(7, 7);

void createChestRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {

	adjustRoomSize(arr, node, CHEST_ROOM_SIZE);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	createChestWithRandomLoot(renderer, roomCenter);
}



void fillRoomContent(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	
	// Spawn Player Here
	if (node->room_number == 3) {
		createPlayerSpawnRoom(arr, renderer, node);
		node->isFilled = true;
		return;
	}

	// Munn: Currently it generates the exit at room 15, which is usually near the bottom right of the map
	if (!is_exit_generated && node->room_number == 15) {
		createFloorExitRoom(arr, renderer, node);
		node->isFilled = true;
		return;
	}

	if (num_enemy_rooms < MAX_ENEMY_ROOMS) {
		float chance_to_spawn_enemy_room = (float)(MAX_ENEMY_ROOMS - num_enemy_rooms + 1) / (float)MAX_ENEMY_ROOMS;

		float rand = uniform_dist(rng);

		if (rand < chance_to_spawn_enemy_room) {
			createEnemyRoom(arr, renderer, node);
			num_enemy_rooms++;
			node->isFilled = true;
			return;
		}
	}

	if (num_chest_rooms < MAX_CHEST_ROOMS) {
		float chance_to_spawn_chest_room = (float)(MAX_CHEST_ROOMS - num_chest_rooms + 1) / (float)MAX_CHEST_ROOMS;

		float rand = uniform_dist(rng);

		if (rand < chance_to_spawn_chest_room) {
			createChestRoom(arr, renderer, node);
			num_chest_rooms++;
			node->isFilled = true;
			return;
		}
	}
}



enum class UNIQUE_ROOM_ID {
	FOUNTAIN = 0,				// Restores health to full
	SACRIFICE = FOUNTAIN + 1,	// Sacrifice half your HP for 2 relics
	CHOICE = SACRIFICE + 1,		// Choose one of 2 spells
	MIMIC = CHOICE + 1,			// Mimic room
	UNIQUE_ROOM_COUNT = MIMIC + 1,
};
const int unique_room_count = (int)UNIQUE_ROOM_ID::UNIQUE_ROOM_COUNT;

std::vector<UNIQUE_ROOM_ID> remaining_unique_rooms;


const vec2 FOUNTAIN_ROOM_SIZE = vec2(7, 7);

// Create a fountain that heals the player at the center of the room
void createFountainRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	
	adjustRoomSize(arr, node, FOUNTAIN_ROOM_SIZE);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	createHealingFountain(renderer, roomCenter, 500);
}

// Create a bloody fountain that deals half the player's health, but then spawns 2 relics (or some sort of reward)
void createSacrificeRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	adjustRoomSize(arr, node, FOUNTAIN_ROOM_SIZE);

	vec2 roomCenter = vec2(
		(node->array_pos.x + (int)(node->size.x) / 2) * offset,
		(node->array_pos.y + (int)(node->size.y) / 2) * offset
	);

	createSacrificeFountain(renderer, roomCenter);
	std::cout << "Created a sacrifice fountain" << std::endl;
}

// Create 2 spells/some sort of item, then when the player interacts with one, delete the other
void createChoiceRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	
}

// Create a mimic enemy at the center of the room
void createMimicRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	
}






void fillUniqueRoom(std::vector<std::vector<int>>& arr, RenderSystem* renderer, MapNode* node) {
	if (remaining_unique_rooms.size() == 0) {
		return;
	}

	int rand_index = (int)(uniform_dist(rng) * remaining_unique_rooms.size());

	UNIQUE_ROOM_ID room_id = remaining_unique_rooms[rand_index];

	std::cout << "Creating unique room: " << (int)room_id << std::endl;

	switch (room_id) {
	case UNIQUE_ROOM_ID::FOUNTAIN: {
		createFountainRoom(arr, renderer, node);
		break;
	}
	case UNIQUE_ROOM_ID::SACRIFICE: {
		createSacrificeRoom(arr, renderer, node);
		break;
	}
	case UNIQUE_ROOM_ID::CHOICE: {
		createChoiceRoom(arr, renderer, node);
		break;
	}
	case UNIQUE_ROOM_ID::MIMIC: {
		createMimicRoom(arr, renderer, node);
		break;
	}
	}

	remaining_unique_rooms.erase(remaining_unique_rooms.begin() + rand_index);

	node->isFilled = true;
}









// THIS is where we build rooms.
// TODO: Add different sized and shaped rooms here (don't jsut fill the cell)
void fillRoomTiles(RenderSystem* renderer, MapNode* node, std::vector<std::vector<int>>& arr, vec2 array_pos, vec2 current_map_size, int length, int height) {

	int room_size_adjust_x_left = (int)(uniform_dist(rng) * MAX_ROOM_ADJUST_SIZE / 2);
	int room_size_adjust_x_right = (int)(uniform_dist(rng) * MAX_ROOM_ADJUST_SIZE / 2);
	int room_size_adjust_y_top = (int)(uniform_dist(rng) * MAX_ROOM_ADJUST_SIZE / 2);
	int room_size_adjust_y_bottom = (int)(uniform_dist(rng) * MAX_ROOM_ADJUST_SIZE / 2);

	vec2 room_size = vec2(0, 0);
	for (int i = array_pos.x + MIN_WALL_THICKNESS + room_size_adjust_x_left; i < array_pos.x + node->size.x - 1 - room_size_adjust_x_right; i++) {
		room_size.y = 0;
		for (int j = array_pos.y + MIN_WALL_THICKNESS + room_size_adjust_y_top; j < array_pos.y + node->size.y - 1 - room_size_adjust_y_bottom; j++) {
			arr[i][j] = currentRoom;
			room_size.y++;
		}
		room_size.x++;
	}

	node->array_pos.x = (int)(node->array_pos.x + MIN_WALL_THICKNESS + room_size_adjust_x_left);
	node->array_pos.y = (int)(node->array_pos.y + MIN_WALL_THICKNESS + room_size_adjust_y_top);

	node->size = room_size;

	node->room_number = currentRoom;

	room_nodes.push_back(node);
	
	/*std::cout << "CURRENT ROOM: " << currentRoom << std::endl;
	std::cout << "x-range: " << node->array_pos.x << " - " << node->array_pos.x + node->size.x << std::endl;
	std::cout << "y-range: " << node->array_pos.y << " - " << node->array_pos.y + node->size.y << std::endl;*/
	currentRoom++;
}





//void connectMapNodes(std::vector<std::vector<int>>& arr,
//	bool split_vertically,  MapNode& child_one, MapNode& child_two, int length, int height) {
//
//	// these are the tiles we're connecting
//	vec2 child_one_tile;
//	vec2 child_two_tile;
//
//	std::cout << "----------------------------------------" << std::endl;
//
//
//	// the next large chunk of code is to find two tiles that we want to connect
//	int room_one_right = child_one.array_pos.x + child_one.size.x - 1;
//	int room_one_left = child_one.array_pos.x;
//	int room_one_top = child_one.array_pos.y;
//	int room_one_bottom = child_one.array_pos.y + child_one.size.y - 1;
//
//	int room_two_right = child_two.array_pos.x + child_two.size.x - 1;
//	int room_two_left = child_two.array_pos.x;
//	int room_two_top = child_two.array_pos.y;
//	int room_two_bottom = child_two.array_pos.y + child_two.size.y - 1;
//
//
//	// (Shaw) TODO: See if we can use a non-uniform distribution (normal distribution) to keep the cooridoors closer
//	// to the center. This is not important for a room by room basis, but its pretty important for a map-structure
//	// basis because it influences the connections between super-rooms
//	if (split_vertically) {
//		// if Split Vertically, then they're side by side
//		// this just makes sure we don't start on a corner
//		do {
//			float rand = clamp(normal_dist(rng), 0.0f, 1.0f);
//
//			int rooms_y_min = max(room_one_top, room_two_top);
//			int rooms_y_max = min(room_one_bottom, room_two_bottom);
//
//			int rooms_y_overlap = rooms_y_max - rooms_y_min;
//
//			int random_height = clamp(rooms_y_min + rand * rooms_y_overlap, (float)rooms_y_min + 1, (float)rooms_y_max - 1);
//
//			child_one_tile = vec2(room_one_right, random_height);
//			child_two_tile = vec2(room_two_left, random_height);
//		} while (arr[child_one_tile.x][child_one_tile.y] < 3 && arr[child_two_tile.x][child_two_tile.y] < 3);
//	}
//	else {
//		// if not, then they're split top/bottom
//		// this just makes sure we don't start on a corner
//		do {
//			float rand = clamp(normal_dist(rng), 0.0f, 1.0f);
//
//			int rooms_x_min = max(room_one_left, room_two_left);
//			int rooms_x_max = min(room_one_right, room_two_right);
//
//			int rooms_x_overlap = rooms_x_max - rooms_x_min;
//
//			int random_width = clamp(rooms_x_min + rand * rooms_x_overlap, (float)rooms_x_min + 1, (float)rooms_x_max - 1);
//
//			child_one_tile = vec2(random_width, room_one_bottom);
//			child_two_tile = vec2(random_width, room_two_top);
//		} while (arr[child_one_tile.x][child_one_tile.y] < 3 && arr[child_two_tile.x][child_two_tile.y] < 3);
//	}
//
//	// CONNECT The cells 
//	int i = child_one_tile.x;
//	int j = child_one_tile.y;
//	if (split_vertically) {
//		while (arr[i][j] == 1) {
//			arr[i][j] = 2;
//			i++;
//			// Munn: THIS CAN GO OUT OF BOUNDS WITHOUT A BREAK HERE!
//			if (i >= length) {
//				break;
//			}
//		}
//	}
//	else {
//		while (arr[i][j] == 1) {
//			arr[i][j] = 2;
//			j++;
//			// Munn: THIS CAN GO OUT OF BOUNDS WITHOUT A BREAK HERE!
//			if (j >= height) {
//				break;
//			}
//		}
//	}
//}

vec2 getTopLeftRecursive(MapNode* node) {
	if (!node->hasChildren) {
		return vec2(
			node->array_pos.x,	// Left
			node->array_pos.y	// Top
		); 
	}

	vec2 child_one_top_left = getTopLeftRecursive(node->child_one);
	vec2 child_two_top_left = getTopLeftRecursive(node->child_two);

	vec2 top_leftest = vec2(
		min(child_one_top_left.x, child_two_top_left.x),
		min(child_one_top_left.y, child_two_top_left.y)
	);

	return top_leftest;
}

vec2 getBottomRightRecursive(MapNode* node) {
	if (!node->hasChildren) {
		return vec2(
			node->array_pos.x + node->size.x,	// Right
			node->array_pos.y + node->size.y	// Bottom
		);
	}

	vec2 child_one_bottom_right = getBottomRightRecursive(node->child_one);
	vec2 child_two_bottom_right = getBottomRightRecursive(node->child_two);

	vec2 bottom_rightest = vec2(
		max(child_one_bottom_right.x, child_two_bottom_right.x),
		max(child_one_bottom_right.y, child_two_bottom_right.y)
	);

	return bottom_rightest;
}

void connectMapNodesRecursive(std::vector<std::vector<int>>& arr, MapNode* node, int length, int height) {

	if (!node->hasChildren) {
		return;
	}

	connectMapNodesRecursive(arr, node->child_one, length, height);
	connectMapNodesRecursive(arr, node->child_two, length, height);

	// Connect child 1 and 2
	// NOTE: if split vertically, child 1 is on TOP. if split horizontally, child 1 is on LEFT
	
	MapNode child_one = *node->child_one;
	MapNode child_two = *node->child_two;

	vec2 child_one_top_left = getTopLeftRecursive(node->child_one);
	vec2 child_one_bottom_right = getBottomRightRecursive(node->child_one);

	vec2 child_two_top_left = getTopLeftRecursive(node->child_two);
	vec2 child_two_bottom_right = getBottomRightRecursive(node->child_two);

	// the next large chunk of code is to find two tiles that we want to connect
	int room_one_right = child_one_bottom_right.x;
	int room_one_left = child_one_top_left.x;
	int room_one_top = child_one_top_left.y;
	int room_one_bottom = child_one_bottom_right.y;

	int room_two_right = child_two_bottom_right.x;
	int room_two_left = child_two_top_left.y;
	int room_two_top = child_two_top_left.y;
	int room_two_bottom = child_two_bottom_right.y;


	vec2 child_one_tile;
	vec2 child_two_tile;
	if (node->split_vertically) {
		float rand = clamp(normal_dist(rng), 0.0f, 1.0f);

		int rooms_y_min = max(room_one_top, room_two_top);
		int rooms_y_max = min(room_one_bottom, room_two_bottom);

		int rooms_y_overlap = rooms_y_max - rooms_y_min;

		int random_height = clamp(rooms_y_min + rand * rooms_y_overlap, (float)rooms_y_min + 1, (float)rooms_y_max - CORRIDOR_THICKNESS);

		child_one_tile = vec2(room_one_right, random_height);
		child_two_tile = vec2(room_two_left, random_height);
	}
	else {
		float rand = clamp(normal_dist(rng), 0.0f, 1.0f);

		int rooms_x_min = max(room_one_left, room_two_left);
		int rooms_x_max = min(room_one_right, room_two_right);

		int rooms_x_overlap = rooms_x_max - rooms_x_min;

		int random_width = clamp(rooms_x_min + rand * rooms_x_overlap, (float)rooms_x_min + 1, (float)rooms_x_max - CORRIDOR_THICKNESS);

		child_one_tile = vec2(random_width, room_one_bottom);
		child_two_tile = vec2(random_width, room_two_top);
	}

	// CONNECT The cells 
	int i = child_one_tile.x;
	int j = child_one_tile.y;
	if (node->split_vertically) {
		for (int corridor_j = 0; corridor_j < CORRIDOR_THICKNESS; corridor_j++) {
			i = room_one_right + 1;

			// Step back until you are out of the wall
			while (arr[i][j + corridor_j] == 0) {
				i--;
				if (i <= room_one_left) {
					break;
				}
			}

			// Step back into the wall
			i++;

			// Turn into corridors
			while (arr[i][j + corridor_j] == 0) {

				arr[i][j + corridor_j] = 2;
				i++;
				
				if (i >= room_two_right) {
					break;
				}
			}
		}
	}
	else {
		for (int corridor_i = 0; corridor_i < CORRIDOR_THICKNESS; corridor_i++) {
			j = room_one_bottom + 1;

			while (arr[i + corridor_i][j] == 0) {
				j--;
				if (j <= room_one_top) {
					break;
				}
			}

			j++;

			while (arr[i + corridor_i][j] == 0) {
			
				arr[i + corridor_i][j] = 2;
				j++;
				
				if (j >= room_two_bottom) {
					break;
				}
			}
		}
	}
}

bool shouldSplitVertically(int x, int y) { 

	bool shouldSplitRandomly = abs(x - y) <= 2 && x > 3 && y > 3 && x * y > 50;
	if (shouldSplitRandomly) {
		return uniform_dist(rng) > 0.5f;
	}

	return x >= y;
}

MapNode* populateMapWithArray(RenderSystem* renderer, std::vector<std::vector<int>>& arr, vec2 array_pos,
	vec2 current_map_size, int max_room_tiles, float max_split_percentage, int length, int height) {

	MapNode* cur_node = new MapNode();
	cur_node->array_pos = array_pos;
	cur_node->size = current_map_size;
	cur_node->room_number = -1; // no room, just children
	
	if (current_map_size.x * current_map_size.y <= max_room_tiles) {
		cur_node->hasChildren = false;
		cur_node->room_number = currentRoom;
		fillRoomTiles(renderer, cur_node, arr, array_pos, current_map_size, length, height);
		return cur_node;
	}
	cur_node->hasChildren = true;
	bool split_vertically = shouldSplitVertically(current_map_size.x, current_map_size.y);
	cur_node->split_vertically = split_vertically;

	float random_split_percentage = getRandomSplitPercent(max_split_percentage);

	vec2 child_one_size;
	vec2 child_two_size;
	vec2 new_pos = array_pos;

	if (split_vertically) {
		child_one_size = vec2(current_map_size.x * random_split_percentage, current_map_size.y);
		child_two_size = vec2(current_map_size.x - child_one_size.x, current_map_size.y);
		new_pos.x = array_pos.x + child_one_size.x;
	}
	else {
		child_one_size = vec2(current_map_size.x, current_map_size.y * random_split_percentage);
		child_two_size = vec2(current_map_size.x, current_map_size.y - child_one_size.y);
		new_pos.y = array_pos.y + child_one_size.y;
	}

	cur_node->child_one = populateMapWithArray(renderer, arr, array_pos, child_one_size, max_room_tiles, max_split_percentage, length, height);
	cur_node->child_two = populateMapWithArray(renderer, arr, new_pos, child_two_size, max_room_tiles, max_split_percentage, length, height);

	// inefficient but to avoid a really weird bug
	MapNode first = *cur_node->child_one;
	MapNode second = *cur_node->child_two;

	//connectMapNodes(arr, split_vertically, first, second, length, height);

	return cur_node;
}

// Shaw: Yes, it's really ugly, but please overlook
void addWallsToArray(std::vector<std::vector<int>>& arr, int length, int height) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < length; j++) {

			// If this is blank space
			if (arr[i][j] == 0) {

				// Check if near a room
				for (int a = -1; a <= 1; a++) {
					if (i + a >= length || i + a <= 0) {
						continue;
					}

					for (int b = -1; b <= 1; b++) {
						if (j + b >= height || j + b <= 0) {
							continue;
						}

						// Check if it is a room or a corridor
						if (arr[i + a][j + b] >= 2) { 
							arr[i][j] = 1;
						}
					}
				}
			}
		}
	}

	// fill walls on the bottom and top edges
	//for (int j = 0; j < length; j++) {
	//	arr[0][j] = 1;
	//	arr[length - 1][j] = 1;
	//}

	//// fill walls on the left and right edges
	//for (int i = 0; i < height; i++) {
	//	arr[i][0] = 1;
	//	arr[i][length - 1] = 1;
	//}
}

void createWall(RenderSystem* renderer, int wallType, vec2 position) {
	auto entity = Entity();

	Wall& wall = registry.walls.emplace(entity);

	Tile& tile = registry.tiles.emplace(entity);
	vec2 rand_vec = vec2((int)(uniform_dist(rng) * NUM_WALL_TILES_H), (int)(uniform_dist(rng) * NUM_WALL_TILES_V));
	tile.tilecoord = rand_vec;

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	auto& transform_comp = registry.transforms.emplace(entity);
	transform_comp.angle = 0.f;
	transform_comp.position = position;

	//transform_comp.scale = vec2(2, 2); // our current wall sprite is 16x16, not 32x32

}

void createFloor(RenderSystem* renderer, vec2 position) {
	auto entity = Entity();

	Floor& floor = registry.floors.emplace(entity);

	Tile& tile = registry.tiles.emplace(entity);
	vec2 rand_vec = vec2((int)(uniform_dist(rng) * NUM_FLOOR_TILES_H), (int)(uniform_dist(rng) * NUM_FLOOR_TILES_V));
	tile.tilecoord = rand_vec;


	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	auto& transform_comp = registry.transforms.emplace(entity);

	transform_comp.angle = 0.f;
	transform_comp.position = position;
	//transform_comp.scale = vec2(2, 2);
}


Entity createWallCollisionEntity(vec2 position, vec2 scale) {
	auto entity = Entity();

	Transformation& transform_comp = registry.transforms.emplace(entity);
	transform_comp.position = position;
	transform_comp.scale = scale;

	registry.wallCollisions.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::DOOR,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	Hitbox wall_hitbox;
	wall_hitbox.layer = (int)COLLISION_LAYER::WALL;
	wall_hitbox.mask = (int)COLLISION_LAYER::PLAYER | (int)COLLISION_LAYER::ENEMY | (int)COLLISION_LAYER::E_PROJECTILE | (int)COLLISION_LAYER::P_PROJECTILE;
	wall_hitbox.hitbox_scale = vec2(WALL_SIZE); 
	registry.hitboxes.insert(entity, wall_hitbox);

	//std::cout << "Created a wall collision at: " << transform_comp.position.x / offset << ", " << transform_comp.position.y / offset << " with scale " << transform_comp.scale.x << ", " << transform_comp.scale.y << std::endl;

	return entity;
}


// Munn: I know this loops 3 times MAP_LENGTH * MAP_HEIGHT, but it only runs at the start of runtime, so it won't affect gameplay hopefully
void addWallCollisionEntities(vec2 map_gen_pos, std::vector<std::vector<int>>& arr, int length, int height) {

	// Initialize all values to 0, eg. does not have collision yet
	std::vector<std::vector<int>> has_collision_v(
		length,
		std::vector<int>(
			height,
			0
		)
	); 

	std::vector<std::vector<int>> has_collision_h(
		length,
		std::vector<int>(
			height,
			0
		)
	);

	// Create long vertical walls
	for (int x = 0; x < length; x++) {

		int wall_start = -1;

		for (int y = 0; y < height; y++) {
			// If the current tile is a wall
			if (arr[x][y] == 1) {

				// Initialize new wall
				if (wall_start == -1) {
					wall_start = y;
				}

				has_collision_v[x][y] = 1;
			}
			else if (wall_start != -1) {
				// If current tile isn't a wall, and we were in the process of creating a wall, make a wall now
				int wall_end = y - 1;
				int wall_scale_y = wall_end - wall_start + 1;

				// Don't create walls for small walls, as those are usually better as horizontal walls
				if (wall_scale_y < MIN_WALL_COLLISION_SIZE) {
					for (int i = wall_start; i <= wall_end; i++) {
						has_collision_v[x][i] = 0;
					}
					wall_start = -1;
					continue;
				}

				vec2 currPos;
				currPos.x = map_gen_pos.x + x * offset;
				currPos.y = map_gen_pos.y + y * offset - ((float)wall_scale_y / 2.0 + 0.5) * offset; // current y pos - half the size of the wall

				createWallCollisionEntity(currPos, vec2(1, wall_scale_y));
				// Reset 
				wall_start = -1;
			}
		}

		// For walls that end at the bottom
		if (wall_start != -1 && arr[x][height - 1] == 1) {
			int wall_end = height - 1;
			int wall_scale_y = wall_end - wall_start + 1;

			// Don't create walls for small walls, as those are usually better as horizontal walls
			if (wall_scale_y < MIN_WALL_COLLISION_SIZE) {
				for (int i = wall_start; i <= wall_end; i++) {
					has_collision_v[x][i] = 0;
				}
				wall_start = -1;
				continue;
			}

			vec2 currPos;
			currPos.x = map_gen_pos.x + x * offset;
			currPos.y = map_gen_pos.y + height * offset - ((float)wall_scale_y / 2.0 + 0.5) * offset;

			createWallCollisionEntity(currPos, vec2(1, wall_scale_y));
		}
	}

	// Create long horizontal walls
	for (int y = 0; y < height; y++) {

		int wall_start = -1;

		for (int x = 0; x < length; x++) {
			if (arr[x][y] == 1) { 

				// Initialize new wall
				if (wall_start == -1) {
					wall_start = x;
				}

				has_collision_h[x][y] = 1;
			}
			else if (wall_start != -1) {
				// If current tile isn't a wall, and we were in the process of creating a wall, make a wall now
				int wall_end = x - 1;
				int wall_scale_x = wall_end - wall_start + 1;

				// Don't create walls for small walls, as those are usually better as horizontal walls
				if (wall_scale_x < MIN_WALL_COLLISION_SIZE) {
					for (int i = wall_start; i <= wall_end; i++) {
						has_collision_h[i][y] = 0;
					}
					wall_start = -1;
					continue;
				}

				vec2 currPos;
				currPos.x = map_gen_pos.x + x * offset - ((float)wall_scale_x / 2.0 + 0.5) * offset;
				currPos.y = map_gen_pos.y + y * offset; // current x pos - half the size of the wall

				createWallCollisionEntity(currPos, vec2(wall_scale_x, 1));

				// Reset 
				wall_start = -1;
			}
		}

		if (wall_start != -1 && arr[length - 1][y] == 1) {
			int wall_end = length - 1;
			int wall_scale_x = wall_end - wall_start + 1;

			if (wall_scale_x < MIN_WALL_COLLISION_SIZE) {
				for (int i = wall_start; i <= wall_end; i++) {
					has_collision_h[i][y] = 0;
				}
				wall_start = -1;
				continue;
			}

			vec2 currPos;
			currPos.x = map_gen_pos.x + length * offset - ((float)wall_scale_x / 2.0 + 0.5) * offset;
			currPos.y = map_gen_pos.y + y * offset; // current y pos - half the size of the wall

			createWallCollisionEntity(currPos, vec2(wall_scale_x, 1));
		}
	}

	// Fill in the rest
	for (int x = 0; x < length; x++) {
		for (int y = 0; y < height; y++) {
			if (arr[x][y] == 1 && has_collision_h[x][y] == 0 && has_collision_v[x][y] == 0) {
				vec2 currPos;
				currPos.x = map_gen_pos.x + x * offset;
				currPos.y = map_gen_pos.y + y * offset; // current y pos - half the size of the wall

				createWallCollisionEntity(currPos, vec2(1, 1));
			}
		}
	}
}

void createEnvironment(RenderSystem* renderer, vec2 starting_position, 
	std::vector<std::vector<int>>& arr, int height, int length, bool spawn_entities = true) {

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < length; j++) {
			if (arr[i][j] > 1) {
				vec2 curPos;
				curPos.x = starting_position.x + i * offset;
				curPos.y = starting_position.y + j * offset;

				createFloor(renderer, curPos); 
			}

			if (arr[i][j] == 1) {
				vec2 curPos;
				curPos.x = starting_position.x + i * offset;
				curPos.y = starting_position.y + j * offset;

				createWall(renderer, 0, curPos);
			}
		}
	}
}



void resetMapVariables() {
	is_exit_generated = false;
	currentRoom = 3; // RESET ROOM
	num_enemy_rooms = 0;
	num_chest_rooms = 0;

	for (MapNode* node : room_nodes) {
		delete node;
	}

	room_nodes.clear();

	remaining_unique_rooms = {
		UNIQUE_ROOM_ID::FOUNTAIN,
		UNIQUE_ROOM_ID::SACRIFICE,
		UNIQUE_ROOM_ID::CHOICE,
		UNIQUE_ROOM_ID::MIMIC,
	};
}


void createMap(RenderSystem* renderer, vec2 position) {
	vec2 startingPos = position;

	// do map generation here by populating array with 1s or 0s
	// can use different numbers to distinguish rooms-- will think later
	// can also automatically put walls down by looking at array

	// Reset map generation variables

	std::cout << std::endl;
	std::cout << "START MAP GEN..." << std::endl;
	
	resetMapVariables();

	vec2 size = vec2(MAP_LENGTH, MAP_HEIGHT);
	//int arr[MAP_LENGTH][MAP_LENGTH];
	std::vector<std::vector<int>> arr(
		MAP_LENGTH,
		std::vector<int>(MAP_LENGTH)
	);

 
	// Initialize map and create rooms with random size
 
	initializeMap(arr, MAP_LENGTH, MAP_HEIGHT);
	MapNode* rootNode = populateMapWithArray(renderer, arr, vec2(0, 0), size, MAX_ROOM_TILES, MIN_SPLIT_PERCENTAGE, MAP_LENGTH, MAP_HEIGHT);

	// Then, fill rooms with content
	// Randomize order that we fill rooms
	std::shuffle(room_nodes.begin(), room_nodes.end(), rng);

	// Fill the map with essential rooms (player spawn room, floor exit, enemy rooms, chest rooms
	for (MapNode* map_node : room_nodes) {
		fillRoomContent(arr, renderer, map_node);
	}

	int num_unique_rooms = 0; // For logging
	// Fill any remaining empty rooms with "unique" rooms
	for (MapNode* map_node : room_nodes) {
		if (!map_node->isFilled) {
			fillUniqueRoom(arr, renderer, map_node);
			num_unique_rooms++;
		}
	}

	// Connect map nodes after generating map nodes
	connectMapNodesRecursive(arr, rootNode, MAP_LENGTH, MAP_HEIGHT);
	
	// Add walls to the array
	addWallsToArray(arr, MAP_LENGTH, MAP_HEIGHT);

	// Create floor tiles and wall tiles based on array information
	createEnvironment(renderer, position, arr, MAP_LENGTH, MAP_HEIGHT);

	// Add wall collision entities
	addWallCollisionEntities(position, arr, MAP_LENGTH, MAP_HEIGHT);

	// Logging information
	int true_num_rooms = currentRoom - 3; // -3 because currentRoom starts at 3
	std::cout << "-----------------------------------------------------------" << std::endl;
	std::cout << "Map generated! Here are the stats" << std::endl;
	std::cout << "Total number of rooms: " << true_num_rooms << std::endl;
	std::cout << "Number of enemy rooms: " << num_enemy_rooms << std::endl;
	std::cout << "Number of chest rooms: " << num_chest_rooms << std::endl;
	std::cout << "Number of unique rooms: " << num_unique_rooms << std::endl;
	std::cout << "Number of empty rooms: " << true_num_rooms - num_enemy_rooms - num_chest_rooms - num_unique_rooms - 2 << std::endl; // -2 for the spawn and exit room
	std::cout << "-----------------------------------------------------------" << std::endl;
	std::cout << "Here are some numbers that might be important" << std::endl;

	int num_long_collisions = 0;
	int num_small_collisions = 0;
	for (Entity entity : registry.wallCollisions.entities) {
		Transformation& transform = registry.transforms.get(entity);

		if (transform.scale == vec2(1, 1)) {
			num_small_collisions++;
		}
		else {
			num_long_collisions++;
		}
	}
	std::cout << "Number of wall collision entities (long): " << num_long_collisions << std::endl;
	std::cout << "Number of wall collision entities (1x1): " << num_small_collisions << " (less of these is generally better)" << std::endl;
	std::cout << "Total number of wall collision entities: " << registry.wallCollisions.size() << std::endl;

	std::cout << "Number of tiles: " << registry.tiles.size() << std::endl;
	std::cout << "Number of walls: " << registry.walls.size() << std::endl;
	std::cout << "-----------------------------------------------------------" << std::endl;
	std::cout << "END MAP GEN!" << std::endl;
	std::cout << std::endl;

	// This sends the array info to the file: data/test_map_gen.txt
	//printMapArray(arr);
}



const int TUTORIAL_LENGTH = 55;
const int TUTORIAL_HEIGHT = 13;

void createTutorialMap(RenderSystem* renderer, vec2 position)
{
	vec2 startingPos = position;

	std::vector<std::vector<int>> arr;
	std::string file_path = "unique_rooms/tutorial_room.txt";

	loadLevelFromFile(arr, file_path);

	createEnvironment(renderer, vec2(0, 0), arr, arr.size(), arr[0].size(), false);

	addWallCollisionEntities(position, arr, arr.size(), arr[0].size()); 

	// ADD INTERACTABLES AND TEXT AND STUFF 

	// Create player
	vec2 player_spawn_pos = vec2(3.5, 8) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;


	// Create LMB and LSHIFT text
	//createFloorDecor(renderer, vec2(3.5, 12) * (float)TILE_SIZE, TEXTURE_ASSET_ID::TUT_TEXT_LSHIFT);
	createDialogue("Use WASD to move", vec3(1), vec2(3.5, 6.45) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);

	createDialogue("Press LShift while", vec3(1), vec2(3.5, 18) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);
	createDialogue("moving to dash", vec3(1), vec2(3.5, 18.6) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);

	std::vector<EnemyWave> dummy_waves;
	EnemyWave dummy_wave;
	dummy_wave.enemies.push_back(std::pair(
		ENEMY_TYPE::DUMMY_ENEMY,
		vec2(3.5, 29) * (float)TILE_SIZE
	));
	dummy_waves.push_back(dummy_wave);

	createFloorDecor(renderer, vec2(3.5, 31) * (float)TILE_SIZE, TEXTURE_ASSET_ID::TUT_TEXT_LMB);

	// pos, scale, waves
	createEnemyRoomManager(renderer, vec2(3.5, 29.5) * (float)TILE_SIZE, vec2(5, 5), dummy_waves);

	Interactable relic;
	relic.interactable_id = INTERACTABLE_ID::RELIC_DROP;
	relic.relic_id = (int)RELIC_ID::NUMBERS;
	
	createInteractableDrop(renderer, vec2(17.5, 32.5) * (float)TILE_SIZE, relic);
	createDialogue("Hit the chest", vec3(1), vec2(17.5, 25.55) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);
	createDialogue("E to interact", vec3(1), vec2(17.5, 33.75) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);

	Interactable spell;
	spell.interactable_id = INTERACTABLE_ID::PROJECTILE_SPELL_DROP;
	spell.spell_id = (int)PROJECTILE_SPELL_ID::WATERBALL;

	createChest(renderer, vec2(17.5, 26.5) * (float)TILE_SIZE, spell);

	std::vector<EnemyWave> first_waves;
	EnemyWave first_wave;
	first_wave.enemies.push_back(std::pair(
		ENEMY_TYPE::BASIC_RANGED_ENEMY,
		vec2(34, 34) * (float)TILE_SIZE
	));
	first_waves.push_back(first_wave);

	// pos, scale, waves
	createEnemyRoomManager(renderer, vec2(32, 31) * (float)TILE_SIZE, vec2(8, 8), first_waves);

	std::vector<EnemyWave> second_waves;
	EnemyWave second_wave;
	second_wave.enemies.push_back(std::pair(
		ENEMY_TYPE::BASIC_MELEE_ENEMY,
		vec2(30, 17) * (float)TILE_SIZE
	));
	second_wave.enemies.push_back(std::pair(
		ENEMY_TYPE::BASIC_RANGED_ENEMY,
		vec2(36, 15) * (float)TILE_SIZE
	));
	second_waves.push_back(second_wave);

	// pos, scale, waves
	createEnemyRoomManager(renderer, vec2(29.5, 17) * (float)TILE_SIZE, vec2(15, 6), second_waves);

	
	// Create exit door  
 
	createNextLevelEntry(renderer, vec2(29.5 , 2.5) * (float)TILE_SIZE, GAME_SCREEN_ID::HUB);
	createDialogue("E to go through doors", vec3(1), vec2(29.5, 1.2) * (float)TILE_SIZE, vec2(0.7), false, 1.0f);
}
  


void createBossMap(RenderSystem* renderer, vec2 position) {
	vec2 startingPos = position;

	int length = 24;
	int height = 16;

	// do map generation here by populating array with 1s or 0s
	// can use different numbers to distinguish rooms-- will think later
	// can also automatically put walls down by looking at array

	vec2 size = vec2(length / 2, height / 2);

	// Initialize to zero
	std::vector<std::vector<int>> arr(
		length,
		std::vector<int>(
			height,
			0
		)
	);

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			arr[i][j] = 0; // important to initialize entire array
		}
	}
	// fill with room tile
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			arr[i][j] = 3; // important to initialize entire array
		}
	}

	// fill walls on the left and right edges
	for (int j = 0; j < height; j++) {
		arr[0][j] = 1;
		arr[length - 1][j] = 1;
	}

	// fill walls on the bottom and top edges
	for (int i = 0; i < length; i++) {
		arr[i][0] = 1;
		arr[i][height - 1] = 1;
	}

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			if (arr[i][j] > 1) {
				vec2 curPos;
				curPos.x = position.x + i * offset;
				curPos.y = position.y + j * offset;

				createFloor(renderer, curPos);
			}

			if (arr[i][j] == 1) {
				vec2 curPos;
				curPos.x = position.x + i * offset;
				curPos.y = position.y + j * offset;

				createWall(renderer, 0, curPos);
			}
		}
	}

	addWallCollisionEntities(position, arr, length, height);



	// ADD INTERACTABLES AND TEXT AND STUFF 

	// Create player
	vec2 player_spawn_pos = vec2(5, 10) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;

	createEnemy(renderer, vec2(20, 10) * (float)TILE_SIZE, ENEMY_TYPE::BOSS_1);
}
void createBossMap2(RenderSystem* renderer, vec2 position) {
	vec2 startingPos = position;

	int length = 24;
	int height = 16;

	// do map generation here by populating array with 1s or 0s
	// can use different numbers to distinguish rooms-- will think later
	// can also automatically put walls down by looking at array

	vec2 size = vec2(length / 2, height / 2);

	// Initialize to zero
	std::vector<std::vector<int>> arr(
		length,
		std::vector<int>(
			height,
			0
		)
	);

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			arr[i][j] = 0; // important to initialize entire array
		}
	}
	// fill with room tile
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			arr[i][j] = 3; // important to initialize entire array
		}
	}

	// fill walls on the left and right edges
	for (int j = 0; j < height; j++) {
		arr[0][j] = 1;
		arr[length - 1][j] = 1;
	}

	// fill walls on the bottom and top edges
	for (int i = 0; i < length; i++) {
		arr[i][0] = 1;
		arr[i][height - 1] = 1;
	}

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < height; j++) {
			if (arr[i][j] > 1) {
				vec2 curPos;
				curPos.x = position.x + i * offset;
				curPos.y = position.y + j * offset;

				createFloor(renderer, curPos);
			}

			if (arr[i][j] == 1) {
				vec2 curPos;
				curPos.x = position.x + i * offset;
				curPos.y = position.y + j * offset;

				createWall(renderer, 0, curPos);
			}
		}
	}

	addWallCollisionEntities(position, arr, length, height);



	// ADD INTERACTABLES AND TEXT AND STUFF 

	// Create player
	vec2 player_spawn_pos = vec2(5, 10) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;

	createEnemy(renderer, vec2(20, 10) * (float)TILE_SIZE, ENEMY_TYPE::BOSS_2);
}
/*
void createBossMap(RenderSystem* renderer, vec2 position) {
	// read text file into array

	std::vector<std::vector<int>> arr;

	// TODO: change this to more normal relative path maybe?
	std::string filepath = PROJECT_SOURCE_DIR + std::string("data//boss_rooms//boss_1.txt");
	std::ifstream infile(filepath);

	if (infile.is_open())
	{
		std::cout << "OPENING" << std::endl;
		std::string line;

		// throwaway the "tile" part
		std::string name;

		std::string tile_x;
		std::string tile_y;
		std::string tile_texture_id;

		while (!infile.eof()) {
			std::getline(infile, line);
			std::vector<int> local_vector;

			// deletes all spaces
			line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

			for (int i = 0; i < line.size(); i++) {
				if (line[i] == '0') {
					//std::cout << "0  0  ";
					local_vector.push_back(0);
				}
				else if (line[i] == '1') {
					//std::cout << "1  1  ";
					local_vector.push_back(1);
				}
				else if (line[i] == '2') {
					//std::cout << "2  2  ";
					local_vector.push_back(2);
				}
			}

			//std::cout << std::endl;
			// begin comment here for printing out array double the size 
			for (int i = 0; i < line.size(); i++) {
				if (line[i] == '0') {
					std::cout << "0  0  ";
				}
				else if (line[i] == '1') {
					std::cout << "1  1  ";
				}
				else if (line[i] == '2') {
					std::cout << "2  2  ";
				}
			}

			std::cout << std::endl; // end commment here

			arr.push_back(local_vector);
		}
	}

	// set player and camera position to start of boss room
	vec2 player_spawn_pos = vec2(22, 38) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;


	// the length and height of the TXT FILE
	createEnvironment(renderer, position, arr, arr.size(), arr[0].size(), false);
	addWallCollisionEntities(position, arr, arr.size(), arr[0].size());

	
	
	createEnemy(renderer, player_spawn_pos + vec2(0, -24) * (float)TILE_SIZE, ENEMY_TYPE::BOSS_1);

	createNextLevelEntry(renderer, player_spawn_pos, GAME_SCREEN_ID::IN_BETWEEN);
}
*/













void createHubMap(RenderSystem* renderer, vec2 position)
{
	vec2 startingPos = position;

	std::vector<std::vector<int>> arr;
	std::string file_path = "unique_rooms/hub_room.txt";
	
	loadLevelFromFile(arr, file_path);

	createEnvironment(renderer, vec2(0, 0), arr, arr.size(), arr[0].size(), false);

	addWallCollisionEntities(position, arr, arr.size(), arr[0].size());



	// ADD INTERACTABLES AND TEXT AND STUFF 

	// Create player
	vec2 player_spawn_pos = vec2(9.5, 12) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;

	

	// Create exit doors
	createNextLevelEntry(renderer, vec2(9.5, 2.5) * (float)TILE_SIZE, GAME_SCREEN_ID::LEVEL_1);
	createNextLevelEntry(renderer, vec2(16, 6) * (float)TILE_SIZE, GAME_SCREEN_ID::TUTORIAL);

	createNPC(renderer, vec2(11.5, 4) * (float)TILE_SIZE, NPC_NAME::OLD_MAN);

	// TODO: createNextLevelEntry(renderer, vec2(3, 7) * (float)TILE_SIZE, GAME_SCREEN_ID::IN_BETWEEN);
	//createNextLevelEntry(renderer, vec2(9.5, 11) * (float)TILE_SIZE, GAME_SCREEN_ID::BOSS_1);
	//createNextLevelEntry(renderer, vec2(4, 7) * (float)TILE_SIZE, GAME_SCREEN_ID::BOSS_2);


	// create text for tutorial and start 
	createFloorDecor(renderer, vec2(9.5, 4) * (float)TILE_SIZE, TEXTURE_ASSET_ID::HUB_START);
	createFloorDecor(renderer, vec2(16, 7.5) * (float)TILE_SIZE, TEXTURE_ASSET_ID::HUB_TUTORIAL);
}


bool isGoalCompleted(FloorGoal floor_goal) {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	switch (floor_goal.goal_type) {
	case GoalType::TIME:
		return goal_manager.current_time <= floor_goal.time_to_beat;
	case GoalType::KILLS:
		return goal_manager.current_kills >= floor_goal.num_kills_needed;
	case GoalType::TIMES_HIT:
		return goal_manager.current_times_hit < floor_goal.num_times_hit;
	}
}

void checkFloorGoals() {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	// Goal manager should have 3 goals
	assert(goal_manager.goals.size() == NUM_FLOOR_GOALS);

	int mins_to_clear = int(goal_manager.current_time) / 60;
	float secs_to_clear = int(goal_manager.current_time - mins_to_clear * 60);

	std::cout << "Time to beat floor: " << mins_to_clear << ":" << secs_to_clear << std::endl;
	std::cout << "Number of kills on floor: " << goal_manager.current_kills << std::endl;

	// If goals aren't completed, create doors

	// Easy
	if (!isGoalCompleted(goal_manager.goals[0])) {
		Entity entity = createWallCollisionEntity(vec2(9, 11) * (float)TILE_SIZE, vec2(1, 5));
		goal_manager.wall_entities.push_back(entity);
		std::cout << "Easy goal failed!" << std::endl;
	}

	// Medium
	if (!isGoalCompleted(goal_manager.goals[1])) {
		Entity entity = createWallCollisionEntity(vec2(9, 18) * (float)TILE_SIZE, vec2(1, 5));
		goal_manager.wall_entities.push_back(entity);
		std::cout << "Medium goal failed!" << std::endl;
	}

	// Hard
	if (!isGoalCompleted(goal_manager.goals[2])) {
		Entity entity = createWallCollisionEntity(vec2(9, 25) * (float)TILE_SIZE, vec2(1, 5));
		goal_manager.wall_entities.push_back(entity);
		std::cout << "Hard goal failed!" << std::endl;
	}
} 





 
void createInbetweenRoom(RenderSystem* renderer, vec2 position) {
	vec2 startingPos = position;

	std::vector<std::vector<int>> arr;
	std::string file_path = "unique_rooms/inbetween_room.txt";

	loadLevelFromFile(arr, file_path);

	createEnvironment(renderer, vec2(0, 0), arr, arr.size(), arr[0].size(), false);

	addWallCollisionEntities(position, arr, arr.size(), arr[0].size());

	checkFloorGoals();

	// Create player
	vec2 player_spawn_pos = vec2(14, 32) * (float)TILE_SIZE;

	Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = player_spawn_pos;
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = player_spawn_pos;
	 

	createRandomRelic(renderer, vec2(6, 25) * (float)TILE_SIZE);
	createRandomRelic(renderer, vec2(6, 18) * (float)TILE_SIZE);
	createRandomRelic(renderer, vec2(6, 11) * (float)TILE_SIZE);

	createNPC(renderer, vec2(20, 18) * (float)TILE_SIZE, NPC_NAME::SHOP_KEEPER);

	// Create exit doors
	createNextLevelEntry(renderer, vec2(14, 3) * (float)TILE_SIZE, GAME_SCREEN_ID::LEVEL_1);
}


