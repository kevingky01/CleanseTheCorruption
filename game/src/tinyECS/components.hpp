#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include "../ext/stb_image/stb_image.h"

#include "map_gen/map_node.hpp" // Munn: Kinda sus including this here
#include "dialogue/dialogue.hpp"

/*
 * Munn: I'm not going to remove any components yet, so we can refer to these for inspiration for what components we'll need. 
 *       Some of these I also don't know what they exactly do, for example ColoredVertex and TexturedVertex, so I'm kinda scared
 *		 to remove them lol
 */

struct Health {
	float currentHealth;
	float maxHealth;
};

// For mesh based collision 
struct CollisionMesh {
	
	std::vector<vec2> local_points;

};

// BOSS NEW
struct Boss {
	int phaseCount = 3;
	int curPhase = 1;
	float hp[3] = { 100.f, 100.f, 100.f };

	// 	TEXTURE_ASSET_ID phaseTexture[3] = {
	//     TEXTURE_ASSET_ID::BOSS_FORM1,
	//     TEXTURE_ASSET_ID::BOSS_FORM2,
	//     TEXTURE_ASSET_ID::BOSS_FORM3
	// };

};

// Player component
struct Player
{
	
};

struct Tile {
	vec2 tilecoord;
	int h_tiles;
	int v_tiles;
};

// TODO: Add different floor type enums in this struct
struct Floor {

};

struct Wall {};

struct Chest {};

struct Room {
	vec2 size;
	Tile** tiles;
};




enum class RELIC_ID {
	STRENGTH = 0,				
	TIME = STRENGTH + 1,		
	SPEED = TIME + 1,			
	NUMBERS = SPEED + 1,
	RELIC_COUNT = NUMBERS + 1,
};
const int relic_count = (int)RELIC_ID::RELIC_COUNT;


enum class SPELL_TYPE {
	PROJECTILE = 0,
	MOVEMENT = PROJECTILE + 1
};

enum class PROJECTILE_SPELL_ID {
	// player usable
	FIREBALL = 0,
	WATERBALL = FIREBALL + 1,
	SHOTGUN = WATERBALL + 1,
	THORN_BOMB = SHOTGUN + 1,
	THORN = THORN_BOMB + 1,
	BOOMERANG = THORN + 1,
	BOOMERANG_RETURN = BOOMERANG + 1,
	MAGNET = BOOMERANG_RETURN + 1,
	LIGHTNING = MAGNET + 1,
	ACID = LIGHTNING + 1,
	ACID_EFFECT = ACID + 1,
	CUTTER = ACID_EFFECT + 1,
	CUTTER_LEFT = CUTTER + 1,
	CUTTER_RIGHT = CUTTER_LEFT + 1,

	// enemy usable
	RED_ORB = CUTTER_RIGHT + 1,
	MEDIUM_SPEED_RED_ORB = RED_ORB + 1,
	FAKE_MELEE = MEDIUM_SPEED_RED_ORB + 1,
	BOSS_ORB = FAKE_MELEE + 1,
	BOSS_BOMB_PROJECTILE = BOSS_ORB + 1,
	BOSS_BOMB_ACTIVATE = BOSS_BOMB_PROJECTILE + 1,

	// MEDIUM_SPEED_FIREBALL = BOSS_BOMB_ACTIVATE + 1,

	PROJECTILE_SPELL_COUNT = BOSS_BOMB_ACTIVATE + 1,
};
const int projectile_spell_count = (int)PROJECTILE_SPELL_ID::PROJECTILE_SPELL_COUNT;

enum class MOVEMENT_SPELL_ID {
	DASH = 0,
	BLINK = DASH + 1,
	MOVEMENT_SPELL_COUNT = BLINK + 1
};
const int movement_spell_count = (int)MOVEMENT_SPELL_ID::MOVEMENT_SPELL_COUNT;

const int spell_count = projectile_spell_count + movement_spell_count;

struct SpellSlot {
	SPELL_TYPE spell_type;
	int spell_id;
	float remainingCooldown;

	std::vector<RELIC_ID> relics;

	float internalCooldown;
	int num_casts;
};

struct SpellSlotContainer {
	std::vector<SpellSlot> spellSlots;
};


// Projectile
struct Projectile {
	PROJECTILE_SPELL_ID spell_id;
	float lifetime;
	bool is_dead;
	Entity owner; // Munn: The entity who shot it
	float damage;
};

// Munn: Useful components to add to spells
struct Seeking {
	Entity target; 
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2  velocity = { 0, 0 };
	bool is_dashing = false;
	bool is_in_combat = false;
};

// Guo: taking everything except velocity from motion and moving to position
// Munn: Transform class is already taken by GLM, see render_system.cpp for usage. I'll name it Transformations for now 
struct Transformation {
	vec2  position = { 0, 0 };
	float angle = 0;
	vec2  scale = { 1, 1 };
};

struct Camera {};

struct Hitbox {
	int layer;
	int mask;
	vec2 hitbox_scale = vec2(1, 1);
};

// Munn: Collision layer and mask bits, layers are what the entity IS, masks are what the entity DETECTS COLLISIONS WITH
//		 eg. a player projectile would have a layer 0b100, and a mask 0b01010 to detect enemies and walls
enum class COLLISION_LAYER{
	PLAYER = 0b1,
	ENEMY = 0b10,
	P_PROJECTILE = 0b100,
	E_PROJECTILE = 0b1000,
	WALL = 0b10000,
	INTERACTABLE = 0b100000,
	ENEMY_ROOM_TRIGGER = 0b1000000,
};

enum class COLLISION_MASK {
	PLAYER = 0b1,
	ENEMY = 0b10,
	P_PROJECTILE = 0b100,
	E_PROJECTILE = 0b1000,
	WALL = 0b10000,
	INTERACTABLE = 0b100000,
	ENEMY_ROOM_TRIGGER = 0b1000000,
};

// Stucture to store collision information
struct Collision
{
	Entity other; // the second object involved in the collision
	Collision(Entity& other) { this->other = other; };
};

struct WallCollision {};

// Create a timer, and give it a function to call on timeout 
// Munn: This can have other functionality too, such as looping, pausing, slow motion, etc. if we decide we need functionality like that
struct Timer {
	float duration;
	float current_time;
	bool is_active;
	bool is_looping;
	std::function<void()> timeout;

	void start() {
		is_active = true;
		current_time = duration;
	}

	void stop() {
		is_active = false;
	}
};


enum class TWEEN_TYPE {
	FLOAT = 0,
	VEC2 = FLOAT + 1,
};

struct Tween {
	float duration;
	float current_time;
	bool is_active;
	std::function<void()> timeout;

	TWEEN_TYPE type;

	float* f_value; // Pointer to the float that it is adjusting
	float f_to;		// What value to lerp the float towards
	float f_from;   // What value to lerp the float from

	vec2* v2_value;	// Same as above, but for vec2
	vec2 v2_to;
	vec2 v2_from;

	std::function<float(float, float, float)> interp_func = lerp;
};




/*
 * Munn: All components below here I either don't think we need them, or don't know what they do
 */


// Tower
struct Tower {
	float range;	// for vision / detection
	int timer_ms;	// when to shoot - this could also be a separate timer component...
};





// used for Entities that cause damage
struct Deadly
{

};

// used for edible entities
struct Eatable
{

};




// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float darken_screen_factor = 0;
	float vignette_factor = 0;
	float vignette_persist_duration = 0;

	bool is_paused = false; // Mark: pause screen
	std::string pause_state = ""; // default
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// used to hold grid line start and end positions
struct GridLine {
	vec2 start_pos = {  0,  0 };
	vec2 end_pos   = { 10, 10 };	// default to diagonal line
};

// A timer that will be associated to dying chicken
struct DeathTimer
{
	float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & chicken.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

struct TileInfo
{
	mat3 transform_matrix;
	vec2 tilecoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = {1,1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID {
	PLAYER_IDLE = 0,
	PLAYER_RUN = PLAYER_IDLE + 1,
	PLAYER_DEATH = PLAYER_RUN + 1,
	PLAYER_RESPAWN = PLAYER_DEATH + 1,

	PLAYER_DASH_PARTICLES = PLAYER_RESPAWN + 1,

	FLOOR = PLAYER_DASH_PARTICLES + 1,
	WALL = FLOOR + 1, 
	CHEST = WALL + 1,
	NEXT_LEVEL_ENTRY = CHEST + 1, 
	BOXES = NEXT_LEVEL_ENTRY + 1,
	DOOR = BOXES + 1,

	RANGED_ENEMY_IDLE = DOOR + 1,
	RANGED_ENEMY_RUN = RANGED_ENEMY_IDLE + 1,
	SHOTGUN_ENEMY_IDLE = RANGED_ENEMY_RUN + 1,
	SHOTGUN_ENEMY_RUN = SHOTGUN_ENEMY_IDLE + 1,
	MELEE_ENEMY_IDLE = SHOTGUN_ENEMY_RUN + 1, // New enemy sprite
	MELEE_ENEMY_RUN = MELEE_ENEMY_IDLE + 1,
	DUMMY_ENEMY = MELEE_ENEMY_RUN + 1,
	TURRET_ENEMY = DUMMY_ENEMY + 1,

	BOSS_1_IDLE = TURRET_ENEMY + 1,
	BOSS_1_RUN = BOSS_1_IDLE + 1,

	// Projectiles
	FIREBALL = BOSS_1_RUN + 1,
	WATERBALL = FIREBALL + 1,
	SHOTGUN_PELLET = WATERBALL + 1,
	THORN_BOMB = SHOTGUN_PELLET + 1,
	THORN = THORN_BOMB + 1,
	BOOMERANG = THORN + 1,
	BOOMERANG_RETURN = BOOMERANG + 1,
	MAGNET = BOOMERANG + 1,
	LIGHTNING = MAGNET + 1,
	ACID = LIGHTNING + 1,
	ACID_EFFECT = ACID + 1,
	CUTTER = ACID_EFFECT + 1,

	// Enemy Projectiles
	RED_ORB = CUTTER + 1,
  
	// Movement spell icons
	DASH_ICON = RED_ORB + 1,
	BLINK_ICON = DASH_ICON + 1,

	// Relic Icons
	STRENGTH_RELIC = BLINK_ICON + 1,
	TIME_RELIC = STRENGTH_RELIC + 1,
	SPEED_RELIC = TIME_RELIC + 1,
	NUMBER_RELIC = SPEED_RELIC + 1,

	// Interactable icons
	SPELL_DROP = NUMBER_RELIC + 1,
	RELIC_DROP = SPELL_DROP + 1,
	HEALTH_RESTORE = RELIC_DROP + 1,
	FOUNTAIN = HEALTH_RESTORE + 1,
	FOUNTAIN_EMPTY = FOUNTAIN + 1,
	SACRIFICE_FOUNTAIN = FOUNTAIN_EMPTY + 1,
	SACRIFICE_FOUNTAIN_EMPTY = SACRIFICE_FOUNTAIN + 1,

	SHADOW = SACRIFICE_FOUNTAIN_EMPTY + 1,
	ENEMY_SPAWN_INDICATOR = SHADOW + 1,

	// Particles
	PARTICLE = ENEMY_SPAWN_INDICATOR + 1,

	// Player HUD
	HEART_CONTAINER = PARTICLE + 1,
	HEALTH_BAR_BOTTOM = HEART_CONTAINER + 1,
	HEALTH_BAR_MID = HEALTH_BAR_BOTTOM + 1,
	HEALTH_BAR_FILL = HEALTH_BAR_MID + 1,
	SPELL_CONTAINER_UI = HEALTH_BAR_FILL + 1,


	TUT_TEXT_E = SPELL_CONTAINER_UI + 1,
	TUT_TEXT_EQ = TUT_TEXT_E + 1,
	TUT_TEXT_LMB = TUT_TEXT_EQ + 1,
	TUT_TEXT_LSHIFT = TUT_TEXT_LMB + 1,

	HUB_TUTORIAL = TUT_TEXT_LSHIFT + 1,
	HUB_START = HUB_TUTORIAL + 1,

	OLD_MAN_IDLE = HUB_START + 1,
	SHOP_KEEPER_IDLE = OLD_MAN_IDLE + 1,

	MINIMAP = SHOP_KEEPER_IDLE + 1,
	PLAYER_ICON = MINIMAP + 1,

	BLACK_PIXEL = PLAYER_ICON + 1,

	CUTSCENE_INTRO = BLACK_PIXEL + 1,
	CUTSCENE_INTRO_2 = CUTSCENE_INTRO + 1,
	CUTSCENE_INTRO_3 = CUTSCENE_INTRO + 2,
	CUTSCENE_INTRO_4 = CUTSCENE_INTRO + 3,
	CUTSCENE_OUTRO = CUTSCENE_INTRO_4 + 1,
	CUTSCENE_OUTRO_2 = CUTSCENE_OUTRO + 1,

	// Mark: texture for slider
	SLIDE_BAR = CUTSCENE_OUTRO_2 + 1,
	SLIDE_BLOCK = SLIDE_BAR + 1,

	TEXTURE_COUNT = SLIDE_BLOCK + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	EGG = COLOURED + 1,
	CHICKEN = EGG + 1,
	TEXTURED = CHICKEN + 1,
	ENVIRONMENT = TEXTURED + 1,
	VIGNETTE = ENVIRONMENT + 1,
	PARTICLE = VIGNETTE + 1,
	ANIMATED = PARTICLE + 1,
	OUTLINE = ANIMATED + 1,
  	ANIMATED_OUTLINE = OUTLINE + 1,
	HEALTH_BAR = ANIMATED_OUTLINE + 1,
	TILE = HEALTH_BAR + 1,
	FONT = TILE + 1,
	MINIMAP = FONT + 1,
	EFFECT_COUNT = MINIMAP + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
	CHICKEN = 0,
	SPRITE = CHICKEN + 1,
	ANIMATED_SPRITE = SPRITE + 1,
	BACKGROUND = ANIMATED_SPRITE + 1,
	EGG = BACKGROUND + 1,
	DEBUG_LINE = EGG + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	HEALTH_BAR = SCREEN_TRIANGLE + 1, // NEW
	PARTICLE = HEALTH_BAR + 1,
	FONT = PARTICLE + 1,
	GEOMETRY_COUNT = FONT + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
	TEXTURE_ASSET_ID   used_texture  = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID    used_effect   = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
	bool is_hitflash = false;
	// new 
	bool colour_edge = false;
};




struct Animation {
	TEXTURE_ASSET_ID asset_id; // Use this as base for the texture_asset_id
	int num_frames;
	int h_frames; // number of horizontal frames
	int v_frames; // number of vertical frames
	float current_time;
	bool is_looping;
	int frame_rate = ANIMATION_FRAME_RATE;

	bool is_interruptable = true;
	bool is_paused = false;

	// Default constructor
	Animation() {}

	Animation(TEXTURE_ASSET_ID asset_id, int num_frames, bool is_looping) {
		this->asset_id = asset_id;
		this->num_frames = num_frames;
		this->is_looping = is_looping;
	}

	bool operator==(const Animation& other) {
		return (asset_id == other.asset_id) &&
			(num_frames == other.num_frames) && 
			(h_frames == other.h_frames) &&
			(v_frames == other.v_frames) &&
			(current_time == other.current_time) &&
			(is_looping == other.is_looping) &&
			(frame_rate == other.frame_rate);
	}

	void play() {
		current_time = 0;
		is_paused = false;
	}

	void pause() {
		is_paused = true;
	}
};

struct AnimationManager {
	Animation current_animation;
	std::unordered_map<TEXTURE_ASSET_ID, Animation> animations;

	void transition_to(Animation animation, bool force_interrupt = false) {
		if (!current_animation.is_interruptable && current_animation.current_time < current_animation.num_frames && !force_interrupt) {
			return;
		}
		if (animation == current_animation) {
			return;
		}
		current_animation = animation;
		current_animation.play();
	}

	void transition_to(TEXTURE_ASSET_ID asset_id, bool force_interrupt = false) {
		if (animations.find(asset_id) == animations.end()) {
			std::cout << "Could not transition to animation!" << std::endl;
			return;
		}

		if (!current_animation.is_interruptable && current_animation.current_time < current_animation.num_frames && !force_interrupt) {
			return;
		}

		if (asset_id == current_animation.asset_id) {
			return;
		}

		current_animation = animations[asset_id];
		current_animation.play();
	}
};


// Particle system
struct Particle {
	// Runtime values
	vec2 position = vec2(0, 0);
	vec2 velocity = vec2(0, 0);
	vec2 scale = vec2(1, 1);
	vec4 color = vec4(1, 0, 0, 1);
	float lifetime = 0;

	Particle() {}

	Particle(vec2 position, vec2 velocity, vec4 color, float lifetime) {
		this->position = position;
		this->velocity = velocity;
		this->color = color;
		this->lifetime = lifetime;
	}
};

struct ParticleEmitter {
	int max_particles = 50;
	std::vector<Particle> particles;

	bool is_emitting = false;
	float loop_duration = 1.0;
	float current_respawn_time = 0.0;

	// reference to parent transformation
	int parent_entity = -1;

	// Initial values
	vec2 position_i = vec2(0, 0);
	vec2 velocity_i = vec2(0, 0);
	vec2 scale_i = vec2(1, 1);
	vec4 color_i = vec4(1, 1, 1, 1); // RED DEFAULT PARTICLE COLOR


	// Random values
	vec2 random_position_min = vec2(0, 0);
	vec2 random_position_max = vec2(0, 0);
	vec2 random_velocity_min = vec2(0, 0);
	vec2 random_velocity_max = vec2(0, 0);
	vec2 random_scale_min = vec2(1, 1);
	vec2 random_scale_max = vec2(1, 1);

	TEXTURE_ASSET_ID sprite_id = TEXTURE_ASSET_ID::PARTICLE;

	ParticleEmitter() {
		particles.resize(max_particles);
		for (int i = 0; i < max_particles; i++) {
			particles.push_back(Particle());
		}
	}

	// Parameters: num_particles, color, random_pos_range_min, random_pos_range_max, random_velocity_range_min, random_velocity_range_max
	ParticleEmitter(int num_particles, vec4 color, vec2 random_pos_range_min, vec2 random_pos_range_max, vec2 random_velocity_range_min, vec2 random_velocity_range_max) {
		particles.resize(max_particles);
		for (int i = 0; i < max_particles; i++) {
			particles.push_back(Particle());
		}

		this->max_particles = num_particles;
		this->color_i = color;
		this->random_position_min = random_pos_range_min;
		this->random_position_max = random_pos_range_max;
		this->random_velocity_min = random_velocity_range_min;
		this->random_velocity_max = random_velocity_range_max;
	}

	void start_emitting() {
		is_emitting = true;
	}

	void stop_emitting() {
		is_emitting = false;
	}

	void setNumParticles(int particles) {
		this->max_particles = particles;
	}

	void setLoopDuration(float duration) {
		this->loop_duration = duration;
	}

	void setParentEntity(int entity_id) {
		this->parent_entity = entity_id;
	}

	void setRandomPositionRange(vec2 random_position_min, vec2 random_position_max) {
		this->random_position_min = random_position_min;
		this->random_position_max = random_position_max;
	}

	void setInitialPosition(vec2 position) {
		this->position_i = position;
	}

	void setInitialColor(vec4 color) {
		this->color_i = color;
	}

	void setTextureAssetId(TEXTURE_ASSET_ID sprite_id) {
		this->sprite_id = sprite_id;
	}
};


enum class PARTICLE_EMITTER_ID {
	PLAYER_FOOTSTEPS = 0,
	DASH_TRAIL = PLAYER_FOOTSTEPS + 1,
	PROJECTILE_TRAIL = DASH_TRAIL + 1
};

struct ParticleEmitterContainer {
	std::unordered_map<PARTICLE_EMITTER_ID, ParticleEmitter> particle_emitter_map;
};


struct ParticleInfo {
	mat3 transform_matrix;
	vec4 color;
};

// Munn: the only one implemented is spells, the rest are just examples for later
enum class INTERACTABLE_ID {
	PROJECTILE_SPELL_DROP = 0,
	MOVEMENT_SPELL_DROP = PROJECTILE_SPELL_DROP + 1,

	HEALTH_RESTORE = MOVEMENT_SPELL_DROP + 1,

	RELIC_DROP = HEALTH_RESTORE + 1,
	// To be implemented:
	// 
	// HEALTH_UPGRADE = HEALTH_RESTORE + 1,
	// SPEED_UPGRADE = HEALTH_UPGRADE + 1,

	NEXT_LEVEL_ENTRY = RELIC_DROP + 1, // Mark: Go to the next level

	FOUNTAIN = NEXT_LEVEL_ENTRY + 1,
	SACRIFICE_FOUNTAIN = FOUNTAIN + 1,

	NPC = SACRIFICE_FOUNTAIN + 1,

	INTERACTABLE_COUNT = NPC + 1,
}; 
const int interactable_count = (int)INTERACTABLE_ID::INTERACTABLE_COUNT;



// Interactable Items
struct Interactable {

	INTERACTABLE_ID interactable_id;
	bool can_interact = false;
	bool disabled = false;

	// Munn: You can't put inherited components into the registry, so I'm just... doing this
	// For Spells
	int spell_id;

	// For health restores
	int heal_amount;

	// For relics
	int relic_id; 

	// For game screen changes
	int game_screen_id;
};


// Game Screen, based on A2
enum class GAME_SCREEN_ID {
	INTRO  = 0,
	TUTORIAL = INTRO + 1,
	LEVEL_1 = TUTORIAL + 1,	/* default */
	BOSS_1 = LEVEL_1 + 1,
	BOSS_2 = BOSS_1 + 1, //!!DEBUG
	HUB = BOSS_2 + 1,
	IN_BETWEEN = HUB + 1,

	CUTSCENE_INTRO = IN_BETWEEN + 1,
	CUTSCENE_INTRO_2 = CUTSCENE_INTRO + 1,
	CUTSCENE_INTRO_3 = CUTSCENE_INTRO + 2,
	CUTSCENE_INTRO_4 = CUTSCENE_INTRO + 3,
	CUTSCENE_OUTRO = CUTSCENE_INTRO_4 + 1,
	CUTSCENE_OUTRO_2 = CUTSCENE_OUTRO + 1,
	CUTSCENE_OUTRO_3 = CUTSCENE_OUTRO_2 + 1,

	GAME_SCREEN_COUNT = CUTSCENE_OUTRO_3 + 1
};
const int game_screen_count = (int)GAME_SCREEN_ID::GAME_SCREEN_COUNT;

struct TutorialUse {};

struct Exit {};

struct FloorDecor {}; 

struct Lootable {
	Interactable drop;
	// Todo : Gold if we want
};

struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	glm::ivec2   Size;       // Size of glyph
	glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
	char character;
};

struct Text {
	std::string text = "";
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); // white in default

	glm::vec2 scale = glm::vec2(1.0f); // normal scale in default
	float rotation = 0.0f;
	glm::vec2 translation = glm::vec2(0, 0); // Warning: scale can also effect translation here

	glm::vec2 mouse_detection_box_start = vec2(0);
	glm::vec2 mouse_detection_box_end = vec2(0);

	glm::vec3 mouse_pressing_color;
	glm::vec2 mouse_pressing_translation;

	bool mouse_pressed = false;

	bool in_screen = true; // true in screen, false in world
};

enum class TEXT_PIVOT {
	LEFT = 0,
	CENTER = LEFT + 1,
	RIGHT = CENTER + 1,
};


struct TextPopup {
	std::string text = "";
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); // white in default
	float* alpha;

	glm::vec2 scale = glm::vec2(1.0f); // normal scale in default
	float rotation = 0.0f;
	glm::vec2* translation; // Warning: scale can also effect translation here 

	TEXT_PIVOT pivot = TEXT_PIVOT::LEFT;

	bool in_screen = true; // true in screen, false in world
};

struct EnvironmentObject {};




enum class GoalType {
	TIME = 0,
	KILLS = TIME + 1,
	TIMES_HIT = KILLS + 1,
}; 


struct FloorGoal {
	GoalType goal_type; 


	// Munn: Similar to interactables, you can't put inherited structs into the registry, so i'm just putting stuff here
	float time_to_beat;

	int num_kills_needed;

	int num_times_hit;
};

struct GoalManager { 
	std::vector<FloorGoal> goals;

	float current_time;
	int current_kills;
	int current_times_hit;

	bool timer_active = false;

	std::vector<Entity> wall_entities;
};

struct NPC {
	Entity text_entity;
	NPC_NAME npc_name;
	NPC_CONVERSATION npc_conversation;
	int current_dialogue_index = 0;
};

struct Minimap {
	std::vector<int> walls_revealed;
	std::vector<vec2> wall_positions;
	float reveal_range = TILE_SIZE * 15;

	void clear() {
		walls_revealed.clear();
		wall_positions.clear();
	}
};

struct BackgroundImage {};

struct DialogueBox {};
// Mark: texture for slider
struct Slide_Bar {};
struct Slide_Block {};
