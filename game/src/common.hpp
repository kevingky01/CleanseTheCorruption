#pragma once

// stlib
#include <cassert>
#include <sstream>		// string-stream for splitting lines
#include <iostream>
#include <fstream>		// stdout, stderr..
#include <string>
#include <tuple>
#include <vector>
#include <random>
#include <algorithm>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3
using namespace glm;

// json
// library taken from https://github.com/nlohmann/json
#include <json.hpp>
using json = nlohmann::json;
using namespace nlohmann::literals;

#include "tinyECS/tiny_ecs.hpp"

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
// Get defintion of PROJECT_SOURCE_DIR from:
#include "../ext/project_path.hpp"
#define SDL_MAIN_HANDLED
#include <SDL.h>

// Use SDL_GetBasePath() in release mode to get executable directory
// Only use PROJECT_SOURCE_DIR in debug mode
inline std::string get_base_path() {
	char* base_path = SDL_GetBasePath();
	if (base_path) {
		std::string path(base_path);
		SDL_free(base_path);
		return path;
	} else {
		return std::string(PROJECT_SOURCE_DIR);
	}
}

inline std::string data_path() { 
	return get_base_path() + "data"; 
};
inline std::string shader_path(const std::string& name) {
	return get_base_path() + "shaders/" + name;
};
inline std::string textures_path(const std::string& name) {return data_path() + "/textures/" + std::string(name);};
inline std::string audio_path(const std::string& name) {return data_path() + "/audio/" + std::string(name);};
inline std::string mesh_path(const std::string& name) {return data_path() + "/meshes/" + std::string(name);};
inline std::string persistance_path(const std::string& name) { return data_path() + "/persistance/" + std::string(name); };


//
// game constants
//

// 16:9 is a pretty standard screen resolution
// 4:3 is more boxy resolution, and tiles fit more perfectly (320x240 both divide perfectly by 16, but 320x180 does not -- not a huge deal though)
const float SCREEN_RESOLUTION_RATIO = 16.0f / 9.0f;

// Size of the window (of the application)
const int WINDOW_WIDTH_PX = (int)(1280 * 1.1); // 1920
const int WINDOW_HEIGHT_PX = (int)((float) WINDOW_WIDTH_PX / SCREEN_RESOLUTION_RATIO);

// Number of pixels in the in game
const int VIEWPORT_WIDTH_PX = (int)(320 * 1.1); // 384 // Munn: this should be some factor of WINDOW_WIDTH_PX, so we get an integer PIXEL_SCALE_FACTOR
const int VIEWPORT_HEIGHT_PX = (int)((float)VIEWPORT_WIDTH_PX / SCREEN_RESOLUTION_RATIO);

// The amount you need to scale a pixel up to fit the viewport size
const int PIXEL_SCALE_FACTOR = WINDOW_WIDTH_PX / (VIEWPORT_WIDTH_PX); 

const float PLAYER_MOVE_SPEED = 300;
const float PLAYER_ACCELERATION = PLAYER_MOVE_SPEED / 0.1; // takes 0.1 sec to get to full speed

const int ANIMATION_FRAME_RATE = 12;

const int PROJECTILE_DEFAULT_SPEED = 200;

const int NUM_FLOOR_GOALS = 3;


const int NUM_WALL_TILES_H = 5;
const int NUM_WALL_TILES_V = 1;
const int NUM_FLOOR_TILES_H = 5;
const int NUM_FLOOR_TILES_V = 1;
const int WALL_SIZE = 16;
const int TILE_SIZE = WALL_SIZE * PIXEL_SCALE_FACTOR;



// Enemy Room Spawn Numbers
const int MIN_ENEMIES_PER_WAVE = 3;
const int MAX_ENEMIES_PER_WAVE = 4;

const int MIN_WAVES = 2;
const int MAX_WAVES = 3;



// Each initialized enemy defaultly in M_STOP_R status in IDLE status with same start timer
const int IDLE_STATUS_TIMER_INITIALIZATION = 1000;

const int PLAYER_MAX_HEALTH = 50;

// Munn: 2 times window width for now, we can adjust later
const int COLLISION_CULLING_DISTANCE = WINDOW_WIDTH_PX * 2;

const int CHEST_HEALTH = 20;

const int VIGNETTE_EFFECT_LAST_MS = 1000;

// Global Variables for debugging. Basically, allows us to easily conditional in systems
// which helps debugging for various reasons (i.e., you need something to be true iff its the first loop
extern bool debug1;
extern bool debug2;

const float HITFLASH_DURATION = 0.05;

const float FONT_SIZE = 48;
const vec3 DAMAGE_NUMBER_COLOR = vec3(222, 158, 65) / 255.0f;
const vec3 HEALING_NUMBER_COLOR = vec3(117, 167, 67) / 255.0f;


//const int GRID_CELL_WIDTH_PX = 60;
//const int GRID_CELL_HEIGHT_PX = 60;
//const int GRID_LINE_WIDTH_PX = 2;
//
//const int TOWER_TIMER_MS = 1000;	// number of milliseconds between tower shots
//const int MAX_TOWERS_START = 5;
//
//const int INVADER_HEALTH = 50;
//const int INVADER_SPAWN_RATE_MS = 2 * 1000;
//
//const int PROJECTILE_DAMAGE = 10;

// These are hard coded to the dimensions of the entity's texture

//// invaders are 64x64 px, but cells are 60x60
//const float INVADER_BB_WIDTH = (float)GRID_CELL_WIDTH_PX;
//const float INVADER_BB_HEIGHT = (float)GRID_CELL_HEIGHT_PX;
//
//// towers are 64x64 px, but cells are 60x60
//const float TOWER_BB_WIDTH = (float)GRID_CELL_WIDTH_PX;
//const float TOWER_BB_HEIGHT = (float)GRID_CELL_HEIGHT_PX;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
// We recommend making all components non-copyable by derving from ComponentNonCopyable
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

bool gl_has_errors();

float lerp(float start, float end, float t);
float cubic_interp(float start, float end, float t);

// C++ random number generator
extern std::default_random_engine rng;
extern std::uniform_real_distribution<float> uniform_dist; // number between 0..1
extern std::normal_distribution<float> normal_dist;

