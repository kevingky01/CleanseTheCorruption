#include "world_system.hpp"
#include "world_init.hpp"
#include "physics_system.hpp"

#include "spell_cast_manager.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

#include "physics_system.hpp"
#include <map_gen/map_gen.hpp>

#include "interactables/interactable.hpp"

#include "dialogue/dialogue.hpp"


float mouse_pos_x = 0.0f;
float mouse_pos_y = 0.0f;

// create the world
WorldSystem::WorldSystem() {
	player_dead = false;
	is_player_input_enabled = true;
	
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
	}
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (pipe != nullptr)
		Mix_FreeChunk(pipe);

	Mix_CloseAudio();
	
	// Shutdown SDL
	SDL_Quit();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		std::cerr << error << ": " << desc << std::endl;
	}
}

// call to close the window, wrapper around GLFW commands
void WorldSystem::close_window() {
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {

	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		std::cerr << "ERROR: Failed to initialize GLFW in world_system.cpp" << std::endl;
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Just for macOS
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	// CK: setting GLFW_SCALE_TO_MONITOR to true will rescale window but then you must handle different scalings
	// glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_TRUE);		// GLFW 3.3+
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_FALSE);		// GLFW 3.3+

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(WINDOW_WIDTH_PX, WINDOW_HEIGHT_PX, "Cleanse the Corruption", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "ERROR: Failed to glfwCreateWindow in world_system.cpp" << std::endl;
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_button_pressed_redirect = [](GLFWwindow* wnd, int _button, int _action, int _mods) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button_pressed(_button, _action, _mods); };
	
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_pressed_redirect);

	return window;
}

bool WorldSystem::start_and_load_sounds() {
	
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
			fprintf(stderr, "Failed to initialize SDL Audio: %s\n", SDL_GetError());
			return false;
		}
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return false;
	}

	background_music = Mix_LoadMUS(audio_path("background_music.wav").c_str());
	pipe = Mix_LoadWAV(audio_path("pipe.wav").c_str());

	// (Kevin): FEATURE - adds loads sound effects to fulfil audio requirement for M1
	player_hurt = Mix_LoadWAV(audio_path("player_hit.wav").c_str());
	enemy_spawned = Mix_LoadWAV(audio_path("enemy_spawn.wav").c_str());
	enemy_hurt = Mix_LoadWAV(audio_path("enemy_hit_new.wav").c_str());

	// TODO: we can clean this up using lists
	if (background_music == nullptr || 
		pipe == nullptr || 
		player_hurt == nullptr || 
		enemy_hurt == nullptr ||
		enemy_spawned == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("msm.wav").c_str(),
			audio_path("pipe.wav").c_str(),
			audio_path("pipe.wav").c_str(),
			audio_path("pipe.wav").c_str(),
			audio_path("pipe.wav").c_str());
		return false;
	}
	
	return true;
}


void WorldSystem::transitionToScene(GAME_SCREEN_ID game_screen_id, float fade_to_black_duration = 0.5f, float fade_from_black_duration = 0.5f) {
	if (is_transitioning) {
		return;
	}

	for (auto pair : key_held_map) {
		key_held_map[pair.first] = false;
	}

	is_player_input_enabled = false;

	is_transitioning = true;

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
	screen_state.darken_screen_factor = 0.0;

	// Fade out of black
	std::function<void()> timeout = [this, screen_state_entity, fade_from_black_duration, game_screen_id]() {
		game_screen = game_screen_id;
		is_player_input_enabled = true;
		loadLevel();
		ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
		createTween(fade_from_black_duration, (std::function<void()>)[this](){ is_transitioning = false; }, & screen_state.darken_screen_factor, 1.0f, 0.0f);
		};

	// Fade to black
	createTween(fade_to_black_duration, timeout, &screen_state.darken_screen_factor, 0.0f, 1.0f);
}

void WorldSystem::display_setting_info() {

	createDisplayableText("Back", vec3(1.0f), vec2(1), 0.0,
		vec2(WINDOW_WIDTH_PX / 2.0f, WINDOW_HEIGHT_PX * (0.1f)), true, true);

	//createDisplayableText("Reset Key Bind", vec3(1.0f), vec2(1), 0.0,
	//	vec2(WINDOW_WIDTH_PX / 2.0f, WINDOW_HEIGHT_PX * (0.95f)), true, true);

	// audio
	createDisplayableText("Audio:", vec3(1.0f), vec2(1.0f), 0.0,
		vec2(WINDOW_WIDTH_PX * (0.095f), WINDOW_HEIGHT_PX * (0.225f)), true, false);

	float percentage = setting.get_audio() / 96.0f;
	createSlider(renderer, vec2(WINDOW_WIDTH_PX * (0.4f), WINDOW_HEIGHT_PX * (0.2f)), percentage);

	// key bind
	std::map<std::string, std::vector<int>>& actionKeys = setting.get_action_key();
	float scale = 0.75f;
	float ypos = 0.35f / scale;
	for (auto it = actionKeys.begin(); it != actionKeys.end(); ++it) {
		std::ostringstream textss;
		textss << it->first << " : ";

		//std::string text = it->first + ": ";
		for (auto iit = it->second.begin(); iit != it->second.end(); ++iit) {
			//textss << std::to_string(*iit);
			textss << KEY_TO_STRING[*iit];
			if (iit + 1 != it->second.end()) textss << "  ";
		}
		createDisplayableText(textss.str(), vec3(1.0f), vec2(scale), 0.0,
			vec2(WINDOW_WIDTH_PX / 8.0f, WINDOW_HEIGHT_PX * (ypos)), true, false);
		ypos += 0.08f;
	}

	if (record_element != "") {
		
		createDisplayableText(record_element + ":", vec3(1.0f), vec2(0.75f), 0.0,
			vec2(6.0f * WINDOW_WIDTH_PX / 8.0f, WINDOW_HEIGHT_PX * (0.6f)), true, false);
		std::ostringstream textss;
		for (int i = 0; i < keys.size(); i++) {
			//textss << keys[i];
			textss << KEY_TO_STRING[keys[i]];
			if (i + 1 != keys.size()) textss << "  ";
		}
		createDisplayableText(textss.str(), vec3(1.0f), vec2(0.75f), 0.0,
			vec2(6.0f * WINDOW_WIDTH_PX / 8.0f, WINDOW_HEIGHT_PX * (0.7f)), true, false);
	}
}

void WorldSystem::pause_on(std::string state="") {
	for (Entity text_entity : registry.texts.entities) { registry.remove_all_components_of(text_entity); }
	for (Entity bar_entity : registry.slideBars.entities) { registry.remove_all_components_of(bar_entity); }
	for (Entity block_entity : registry.slideBlocks.entities) { registry.remove_all_components_of(block_entity); }

	if (state == "") {
		//createDisplayableText("Resume Game", vec3(1.0f), vec2(1), 0.0, vec2(573, 430), true);
		//createDisplayableText("Setting", vec3(1.0f), vec2(1), 0.0, vec2(650, 530), true);
		//createDisplayableText("Back To Title", vec3(1.0f), vec2(1), 0.0, vec2(570, 630), true);

		createDisplayableText("Resume Game", vec3(1.0f), vec2(1), 0.0,
			vec2(WINDOW_WIDTH_PX / 2.0f, WINDOW_HEIGHT_PX * (0.5f)), true, true);
		createDisplayableText("Setting", vec3(1.0f), vec2(1), 0.0,
			vec2(WINDOW_WIDTH_PX / 2.0f, WINDOW_HEIGHT_PX * (0.63f)), true, true);
		createDisplayableText("Back To Title", vec3(1.0f), vec2(1), 0.0,
			vec2((WINDOW_WIDTH_PX + 57.0f) / 2.0f, WINDOW_HEIGHT_PX * (0.76f)), true, true);
	}
	else if (state == "setting") {
		display_setting_info();
	}

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
	screen_state.is_paused = true;
	screen_state.pause_state = state;
}

void WorldSystem::pause_off() {
	for (Entity text_entity : registry.texts.entities) { registry.remove_all_components_of(text_entity); }
	for (Entity bar_entity : registry.slideBars.entities) { registry.remove_all_components_of(bar_entity); }
	for (Entity block_entity : registry.slideBlocks.entities) { registry.remove_all_components_of(block_entity); }

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
	screen_state.is_paused = false;
	screen_state.pause_state = "";
}

// Mark: Function for load level
void WorldSystem::loadLevel() {

	for (Entity enemy_room_entity : registry.enemyRoomManagers.entities) {
		registry.remove_all_components_of(enemy_room_entity);
	}

	for (Entity environment_object_entity : registry.environmentObjects.entities) {
		registry.remove_all_components_of(environment_object_entity);
	}

	// Munn: Is there a more efficient way to just remove all entities? this feels a bit scuffed lol
	for (Entity chest_entity : registry.chests.entities) {
		registry.remove_all_components_of(chest_entity);
	}

	for (Entity decor_entity : registry.floorDecors.entities) {
		registry.remove_all_components_of(decor_entity);
	}

	for (Entity enemy_entity : registry.enemies.entities) {
		registry.remove_all_components_of(enemy_entity);
	}

	for (Entity projectile_entity : registry.projectiles.entities) {
		registry.remove_all_components_of(projectile_entity);
	}

	for (Entity wallCollision_entity : registry.wallCollisions.entities) {
		registry.remove_all_components_of(wallCollision_entity);
	}

	for (Entity interactable_entity : registry.interactables.entities) {
		registry.remove_all_components_of(interactable_entity);
	}

	for (Entity tile_entity : registry.tiles.entities) {
		if (registry.floors.has(tile_entity) || registry.walls.has(tile_entity)) {
			registry.remove_all_components_of(tile_entity);
		}
	}

	for (Entity entity : registry.backgroundImages.entities) {
		registry.remove_all_components_of(entity);
	}

	Minimap& minimap = registry.minimaps.components[0];
	minimap.clear();

	GoalManager& goal_manager = registry.goalManagers.components[0];

	//if (game_screen == GAME_SCREEN_ID::TUTORIAL)
	for (Entity text_entity : registry.texts.entities) {
		registry.remove_all_components_of(text_entity);
	}

	for (Entity text_entity : registry.textPopups.entities) {
		TextPopup& text_popup = registry.textPopups.get(text_entity);
		if (text_popup.alpha != nullptr) {
			delete text_popup.alpha;
		}
		if (text_popup.translation != nullptr) {
			delete text_popup.translation;
		}
		registry.remove_all_components_of(text_entity);
	}

	for (Entity entity : registry.dialogueBoxes.entities) {
		registry.remove_all_components_of(entity);
	}

	if (game_screen == GAME_SCREEN_ID::INTRO)
	{
		// hard-coded, all the text in the screen should be aligned at the middle of the screen.

		pause_off();

		Entity player_entity = registry.players.entities[0];
		Entity camera_entity = registry.cameras.entities[0];
		Transformation& player_trans = registry.transforms.get(player_entity);
		Transformation& camera_trans = registry.transforms.get(camera_entity);

		camera_trans.position = vec2(player_trans.position.x, player_trans.position.y + 140); 

		//createDisplayableText("Cleanse the Corruption", vec3(1.0f), vec2(1.5), 0.0, vec2(150, 100), true);
		createDisplayableText("Cleanse the Corruption", vec3(1.0f), vec2(1.5), 0.0, 
			vec2((WINDOW_WIDTH_PX - 15.0f) / 2.0f, WINDOW_HEIGHT_PX * (0.15f)), true, true);

		//createDisplayableText("Start Play", vec3(1.0f), vec2(1), 0.0, vec2(605, 430), true);
		//createDisplayableText("Setting", vec3(1.0f), vec2(1), 0.0, vec2(650, 530), true);
		//createDisplayableText("Exit Game", vec3(1.0f), vec2(1), 0.0, vec2(620, 630), true);

		// Mark: Since the width of each letter is different, it's necessary to manually edit the x-axis of the print text
		createDisplayableText("Start Play", vec3(1.0f), vec2(1), 0.0, 
			vec2((WINDOW_WIDTH_PX + 20.0f) / 2.0f, WINDOW_HEIGHT_PX * (0.5f)), true, true);
		createDisplayableText("Setting", vec3(1.0f), vec2(1), 0.0, 
			vec2((WINDOW_WIDTH_PX + 20.0f) / 2.0f, WINDOW_HEIGHT_PX * (0.63f)), true, true);
		createDisplayableText("Exit Game", vec3(1.0f), vec2(1), 0.0, 
			vec2((WINDOW_WIDTH_PX + 20.0f) / 2.0f, WINDOW_HEIGHT_PX * (0.76f)), true, true);
	}
	else if (game_screen == GAME_SCREEN_ID::TUTORIAL)
	{
		createTutorialMap(renderer, vec2(0, 0));
		goal_manager.timer_active = false;
		resetGoalManagerStats();
	}
	else if (game_screen == GAME_SCREEN_ID::LEVEL_1) 
	{
		current_floor++; 
		createMap(renderer, vec2(0, 0));  // Default LEVEL_1
		resetGoalManagerStats(); 
		createFloorGoals();
		goal_manager.timer_active = true;

		createAnnouncement("Floor " + std::to_string((int)current_floor), vec3(1.0), 1.0);
	}
	else if (game_screen == GAME_SCREEN_ID::BOSS_1) { 
		createBossMap(renderer, vec2(0, 0)); 
		createAnnouncement("Bob", vec3(1.0), 1.0);
	}
	else if (game_screen == GAME_SCREEN_ID::HUB) {

		pause_off();

		update_record();

		Entity player_entity = registry.players.entities[0];
		Health& player_health = registry.healths.get(player_entity);
		player_health.currentHealth = player_health.maxHealth;
		current_kills = 0;
		current_floor = 0;
		reset_player_spells();
		createHubMap(renderer, vec2(0, 0));
		resetGoalManagerStats();
		goal_manager.timer_active = false;

		createAnnouncement("The Hub", vec3(1.0), 1.0);
	}
	else if (game_screen == GAME_SCREEN_ID::IN_BETWEEN) {

		// Play outro cutscene
		if (current_floor == 2) {
			game_screen = GAME_SCREEN_ID::CUTSCENE_OUTRO;
			loadLevel();
			return;
		}


		createInbetweenRoom(renderer, vec2(0, 0));
		goal_manager.timer_active = false;
		createAnnouncement("The Shop", vec3(1.0), 1.0);
	}
// !!debug
	else if (game_screen == GAME_SCREEN_ID::BOSS_2) { 
		createBossMap2(renderer, vec2(0, 0));
		createAnnouncement("The Twins", vec3(1.0), 1.0);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_INTRO, vec2(1.2));
		std::vector<std::string> texts = {
			"The wizard world was once a place of harmony and peace.",
			"Wizards lived together in a happy coexistence, with",
			"few conflicts ever occurring."
		};
		createCutsceneDialogue(renderer, texts);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_2) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_INTRO_2, vec2(1.2));
		std::vector<std::string> texts = {
			"But there was one wizard who wanted more than peace.",
			"They wanted power.",
		};
		createCutsceneDialogue(renderer, texts);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_3) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_INTRO_3, vec2(1.2));
		std::vector<std::string> texts = {
			"This wizard cursed the lands, corrupting every being",
			"that moved, and bringing ruin to the peaceful world."
		};
		createCutsceneDialogue(renderer, texts);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_4) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_INTRO_4, vec2(1.2));
		std::vector<std::string> texts = {
			"You are the final wizard who has not been corrupted",
			"by dark magic. You must defeat the Grand Wizard and",
			"free the world from corruption!",
		};
		createCutsceneDialogue(renderer, texts);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_OUTRO, vec2(1.2));
		std::vector<std::string> texts = {
			"You enter the Grand Wizards chamber.",
			"This is what all your work has been for."
		};
		createCutsceneDialogue(renderer, texts);
	}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO_2) {
		createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_OUTRO_2, vec2(1.2));
		std::vector<std::string> texts = {
			"There is nothing in the room but a chair.",
			"As you move to see who is on it, you find",
			"the Grand Wizard lying lifeless in the chair.",
			"In the end, even he had succumbed to the corruption.",
		};
		createCutsceneDialogue(renderer, texts);
		}
	else if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO_3) {
			createBackgroundImage(renderer, vec2(WINDOW_WIDTH_PX / 2, WINDOW_HEIGHT_PX / 2), TEXTURE_ASSET_ID::CUTSCENE_INTRO, vec2(1.2));
			std::vector<std::string> texts = {
				"After casting a spell that you found in his",
				"chamber, you cure the wizards around you.",
				"Finally, you have Cleansed the Corruption.",
				"Thank you for playing!",
			};
			createCutsceneDialogue(renderer, texts);
			}

	/*Entity player_entity = registry.players.entities[0];
	Entity camera_entity = registry.cameras.entities[0];

	if (registry.transforms.has(player_entity)) registry.transforms.get(player_entity).position = vec2(128);
	if (registry.transforms.has(camera_entity)) registry.transforms.get(camera_entity).position = vec2(128);*/
}

void WorldSystem::update_record()
{
	for (int i = 0; i < size(top_10_score); i++)
	{
		if (current_kills > top_10_score[i]) 
		{
			top_10_score[i] = current_kills;
			break;
		}
	}
	// sort the int vector in descending order
	std::sort(top_10_score.begin(), top_10_score.end(), std::greater<int>());

	highest_score = top_10_score[0];
	save_scores(highest_score, top_10_score);
}

void WorldSystem::update_volume() {

	int audio = setting.get_audio();
	//int audio = 0;

	Mix_VolumeMusic(audio);
	Mix_VolumeChunk(pipe, audio);
	Mix_VolumeChunk(player_hurt, audio);
	Mix_VolumeChunk(enemy_spawned, audio);
	Mix_VolumeChunk(enemy_hurt, audio);

}

void WorldSystem::init(RenderSystem* renderer_arg) {

	this->renderer = renderer_arg;

	setting.init();
	setting.load_setting();

	// start playing background music indefinitely
	std::cout << "Starting music..." << std::endl;
	Mix_PlayMusic(background_music, -1);
	Mix_VolumeMusic(35); // Munn: 0 means silent, 128 is max volume https://wiki.libsdl.org/SDL2_mixer/Mix_VolumeMusic
	//Mix_VolumeMusic(setting.audio); // Munn: 0 means silent, 128 is max volume https://wiki.libsdl.org/SDL2_mixer/Mix_VolumeMusic

	update_volume();

	// Set all states to default
	restart_game();

	vec2 player_spawn_pos = vec2(0); // Munn: So we spawn inside the map
	
	createMinimap();
	createPlayer(renderer, player_spawn_pos);
	createCamera(renderer, player_spawn_pos); 
	createGoalManager();

	//transitionToScene(GAME_SCREEN_ID::HUB, 0.0f); // load level 1 in default
	transitionToScene(GAME_SCREEN_ID::INTRO, 0.0f); // Ready for changing when INTRO screen complete

	load_scores(highest_score, top_10_score);
}

// Update our game world
bool WorldSystem::step(float elapsed_ms) {
	update_window_caption();
	// PLAYER MOVEMENT
	// Munn: this definitely should NOT be here, probably move it into some sort of player_controller_system at some point?
	vec2 move_direction = vec2(0, 0);

	/*
	if (key_held_map[GLFW_KEY_W]) {
		move_direction -= vec2(0, 1);
	} 
	if (key_held_map[GLFW_KEY_S]) {
		move_direction += vec2(0, 1);
	}
	if (key_held_map[GLFW_KEY_A]) {
		move_direction -= vec2(1, 0);
	}
	if (key_held_map[GLFW_KEY_D]) {
		move_direction += vec2(1, 0);
	}
	*/
	// Mark: edit for handling key bind
	for (auto it = key_held_map.begin(); it != key_held_map.end(); ++it) {
		if (it->second) {
			if (setting.key_has_action(it->first, "player_move_up")) move_direction -= vec2(0, 1);
			if (setting.key_has_action(it->first, "player_move_down")) move_direction += vec2(0, 1);
			if (setting.key_has_action(it->first, "player_move_left")) move_direction -= vec2(1, 0);
			if (setting.key_has_action(it->first, "player_move_right")) move_direction += vec2(1, 0);
			// Normalize diagonal movement
			if ((setting.key_has_action(it->first, "player_move_up") || setting.key_has_action(it->first, "player_move_down"))
				&& (setting.key_has_action(it->first, "player_move_left") || setting.key_has_action(it->first, "player_move_right")))
			{
				move_direction /= sqrt(2);
			}
		}
	}

	// Normalize diagonal movement
	/*
	if ((key_held_map[GLFW_KEY_W] || key_held_map[GLFW_KEY_S]) && (key_held_map[GLFW_KEY_A] || key_held_map[GLFW_KEY_D])) {
		move_direction /= sqrt(2);
	}
	*/

	if (!is_player_input_enabled) {
		move_direction = vec2(0, 0);
	}

	Entity playerEntity = registry.players.entities[0];
	Motion& motion = registry.motions.get(playerEntity);

	float stepSeconds = elapsed_ms / 1000.0f;

	if (motion.is_dashing) {
		handle_dashing_movement(motion, stepSeconds);
	}
	else {
		handle_walking_movement(motion, move_direction, stepSeconds);
	}

	handle_key_held_update(renderer, move_direction);
	
	Transformation& playerTransform = registry.transforms.get(playerEntity);

	update_player_animations(playerEntity); 

	update_player_particles(playerEntity, motion);

	// Mark: Only available when tutorial enable
	if (game_screen == GAME_SCREEN_ID::TUTORIAL) update_tutorial_stage();

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
	
	Health player_health = registry.healths.get(playerEntity);

	// Pulsate vignette if player is critically low on health
	if (player_health.currentHealth / player_health.maxHealth < 0.25f) {

		float sin_value = sinf(glfwGetTime() * 4);

		float target_vignette_factor = 0.5f + 0.1f * sin_value;

		screen_state.vignette_factor = lerp(screen_state.vignette_factor, target_vignette_factor, stepSeconds * 4);
	} 
	else if (screen_state.vignette_persist_duration > 0) {
		screen_state.vignette_persist_duration -= stepSeconds;

		if (screen_state.vignette_persist_duration < 0) {
			screen_state.vignette_persist_duration = 0;
		}
	}
	else if (screen_state.vignette_factor > 0) {
		screen_state.vignette_factor -= stepSeconds;

		if (screen_state.vignette_factor < 0) {
			screen_state.vignette_factor = 0;
		}
	}

	GoalManager& goal_manager = registry.goalManagers.components[0];
	if (goal_manager.timer_active) {
		goal_manager.current_time += stepSeconds;
	} 

	return true;
}

void WorldSystem::handle_dashing_movement(Motion& playerMotion, float stepSeconds) {
	if (glm::length(playerMotion.velocity) < PLAYER_ACCELERATION * stepSeconds) {
		playerMotion.velocity = vec2(0, 0);
	}
	else {
		playerMotion.velocity -= glm::normalize(playerMotion.velocity) * PLAYER_ACCELERATION * stepSeconds;
	}
}

void WorldSystem::handle_walking_movement(Motion& playerMotion, vec2 move_direction, float stepSeconds) {
	float out_of_combat_accel = 1;
	//float out_of_combat_accel = is_in_combat ? 1.f : 2.f;

	// Decelerate if not holding keys
	if (glm::length(move_direction) == 0.0f) {
		if (glm::length(playerMotion.velocity) < PLAYER_ACCELERATION * stepSeconds * out_of_combat_accel) {
			playerMotion.velocity = vec2(0, 0);
		}
		else {
			playerMotion.velocity -= glm::normalize(playerMotion.velocity) * PLAYER_ACCELERATION * stepSeconds * out_of_combat_accel;
		}
	}
	else {
		playerMotion.velocity += move_direction * PLAYER_ACCELERATION * stepSeconds * out_of_combat_accel;

		if (glm::length(playerMotion.velocity) > PLAYER_MOVE_SPEED * out_of_combat_accel) {
			playerMotion.velocity = glm::normalize(playerMotion.velocity) * PLAYER_MOVE_SPEED * out_of_combat_accel;
		}
	}
}

void WorldSystem::handle_key_held_update(RenderSystem* renderer, vec2 move_direction) {
	if (game_screen >= GAME_SCREEN_ID::CUTSCENE_INTRO) {
		return;
	}

	if (!is_player_input_enabled) {
		return;
	}

	// LMB pressed down (does not detect hold)
	if (key_held_map[GLFW_MOUSE_BUTTON_LEFT]) {
		
		Entity playerEntity = registry.players.entities[0];
		SpellSlotContainer& playerSpellSlotContainer = registry.spellSlotContainers.get(playerEntity);
		assert(playerSpellSlotContainer.spellSlots.size() > 1);
		SpellSlot& damageSpellSlot = playerSpellSlotContainer.spellSlots[0];

		if (damageSpellSlot.spell_type != SPELL_TYPE::PROJECTILE) {
			return;
		} 

		Transformation player_transform = registry.transforms.get(playerEntity);

		Entity camera_entity = registry.cameras.entities[0];
		Transformation camera_transform = registry.transforms.get(camera_entity);

		vec2 mouse_pos = vec2(mouse_pos_x - WINDOW_WIDTH_PX / 2.0, mouse_pos_y - WINDOW_HEIGHT_PX / 2.0);
		vec2 player_to_cam = camera_transform.position - player_transform.position;
		vec2 projectile_direction = player_to_cam + mouse_pos;

		// Static spell cast manager that applies relics and whatnot
		SpellCastManager::castSpell(renderer, damageSpellSlot, player_transform.position, projectile_direction, playerEntity);  

		if (key_held_map[GLFW_KEY_LEFT_CONTROL]) {
			std::cout << "Player Pos: " << player_transform.position.x << ", " << player_transform.position.y << std::endl;
		}
	}

	// Mark: edit for handling key bind
	bool movement_spell_launch = false;
	for (int k : setting.action_key["movement_spell"]) {
		if (key_held_map[k]) {
			movement_spell_launch = true;
			break;
		}
	}

	// LShift held down
	//if (key_held_map[GLFW_KEY_LEFT_SHIFT] && glm::length(move_direction) > 0.0) {
	if (movement_spell_launch && glm::length(move_direction) > 0.0) {


		Entity playerEntity = registry.players.entities[0];
		SpellSlotContainer& playerSpellSlotContainer = registry.spellSlotContainers.get(playerEntity);
		assert(playerSpellSlotContainer.spellSlots.size() > 1);
		SpellSlot& utilitySpellSlot = playerSpellSlotContainer.spellSlots[1];

		if (utilitySpellSlot.spell_type != SPELL_TYPE::MOVEMENT) {
			return;
		}

		if (utilitySpellSlot.remainingCooldown <= 0) {
			Transformation playerTransform = registry.transforms.get(playerEntity);
			Motion& player_motion = registry.motions.get(playerEntity);
			player_motion.is_in_combat = true;
			MovementSpell* spell = movement_spells[utilitySpellSlot.spell_id];

			// Static spell cast manager that applies relics and whatnot

			SpellCastManager::castSpell(renderer, utilitySpellSlot, playerTransform.position, move_direction, playerEntity);
			player_motion.is_in_combat = false;
		}
	}
}

void WorldSystem::update_player_animations(Entity player_entity) {

	Motion& playerMotion = registry.motions.get(player_entity);
	Transformation& playerTransform = registry.transforms.get(player_entity);

	if (playerMotion.velocity.x > 0) {
		playerTransform.scale.x = 1;
	}
	else if (playerMotion.velocity.x < 0) {
		playerTransform.scale.x = -1;
	}

	AnimationManager& player_animation_manager = registry.animation_managers.get(player_entity);

	if (glm::length(playerMotion.velocity) > 0) {
		player_animation_manager.transition_to(TEXTURE_ASSET_ID::PLAYER_RUN);
	}
	else {
		player_animation_manager.transition_to(TEXTURE_ASSET_ID::PLAYER_IDLE);
	}
}

void WorldSystem::update_player_particles(Entity player_entity, Motion& playerMotion) {
	ParticleEmitterContainer& particle_emitter_container = registry.particle_emitter_containers.get(player_entity);
	ParticleEmitter& particle_emitter = particle_emitter_container.particle_emitter_map[PARTICLE_EMITTER_ID::PLAYER_FOOTSTEPS];

	if (glm::length(playerMotion.velocity) > 0) {
		particle_emitter.start_emitting();
	}
	else {
		particle_emitter.stop_emitting();
	}
}

void WorldSystem::update_tutorial_stage()
{

	if (stage_presented)
	{
		if (tutorial_stage == 2 && registry.tutorialUses.size() == 0) {
			tutorial_stage++;
			stage_presented = false;
		}
		return;
	}

	switch (tutorial_stage) 
	{
		case 0: 
		{
			// "Press 'Space' to continue"
			std::cout << "Press 'Space' to continue" << std::endl;
			break;
		}
		case 1:
		{
			// "Press 'WASD' to control player's movement"
			std::cout << "Press 'WASD' to control player's movement" << std::endl;
			break;
		}
		case 2:
		{
			// "Left click can shoot projectiles to the direction of the cursor, now try to kill the enemy"
			std::cout << "Left click can shoot projectiles to the direction of the cursor" << std::endl;
			std::cout << "Now try to kill the enemy" << std::endl;
			//createRandomEnemy();
			if (registry.enemies.size() < 1) {
				// Entity tutorial_enemy = createEnemy(renderer, vec2(700), BASIC_RANGED_ENEMY);
				Entity tutorial_enemy = createEnemy(renderer, vec2(700), BASIC_MELEE_ENEMY); // GUO TODO: this doesn't affect the tutorial spawn?
				registry.tutorialUses.emplace(tutorial_enemy);
			}
			break;
		}
		case 3:
		{
			// "You made it!"
			std::cout << "You made it!" << std::endl;
			break;
		}
		case 4:
		{
			// "We can switch the projectile and movement spells by interact with items by pressing 'E'" 
			std::cout << "Switching the projectile and movement spells by interact with items by pressing 'E'" << std::endl;
			break;
		}
		case 5:
		{
			// ""
			std::cout << "" << std::endl;
			std::cout << "(Go to the door and press 'E' to leave tutorial)" << std::endl;
			//createNextLevelEntry(renderer, vec2(700));
			break;
		}
	}
	stage_presented = true;
}




// Reset the world state to its initial state
void WorldSystem::restart_game() {

	std::cout << "Restarting..." << std::endl;

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created by iterating over entities that have a position

	while (registry.transforms.entities.size() > 0) {
		registry.remove_all_components_of(registry.transforms.entities.back());
	}

	// debugging for memory/component leaks
	registry.list_all_components();
}

// Compute collisions between entities
// bullets - enemies players walls
// walls - enemies players
// enemies - players -- 2/15 meeting: No collisions for now
void WorldSystem::handle_collisions() {
	/*ComponentContainer<Collision>& collision_container = registry.collisions;*/
	for (Entity collision_entity : registry.collisions.entities) {
		if (!registry.collisions.has(collision_entity))
			continue;
		Entity other_entity = registry.collisions.get(collision_entity).other;

		// If either entity is "destroyed", do not calculate new collisions
		if (std::find(to_be_destroyed.begin(), to_be_destroyed.end(), (int)collision_entity) != to_be_destroyed.end() || 
			std::find(to_be_destroyed.begin(), to_be_destroyed.end(), (int)other_entity) != to_be_destroyed.end() || 
			std::find(already_collided.begin(), already_collided.end(), (int)collision_entity) != already_collided.end() ||
			std::find(already_collided.begin(), already_collided.end(), (int)other_entity) != already_collided.end()) {
			continue;
		}

		// projectile collisions -- GUO: ik very ugly gotta figure out an optimization
		if (registry.projectiles.has(collision_entity) && registry.enemies.has(other_entity))
			handle_projectile_enemy_collision(collision_entity, other_entity);
		else if (registry.enemies.has(collision_entity) && registry.projectiles.has(other_entity))
			handle_projectile_enemy_collision(other_entity, collision_entity);
		else if (registry.projectiles.has(collision_entity) && registry.players.has(other_entity))
			handle_projectile_player_collision(collision_entity, other_entity);
		else if (registry.players.has(collision_entity) && registry.projectiles.has(other_entity))
			handle_projectile_player_collision(other_entity, collision_entity);
		else if (registry.projectiles.has(collision_entity) && registry.wallCollisions.has(other_entity))
			handle_projectile_wall_collision(collision_entity, other_entity);
		else if (registry.wallCollisions.has(collision_entity) && registry.projectiles.has(other_entity))
			handle_projectile_wall_collision(other_entity, collision_entity);
		else if (registry.projectiles.has(collision_entity) && registry.chests.has(other_entity))
			handle_projectile_chest_collision(collision_entity, other_entity);
		else if (registry.chests.has(collision_entity) && registry.projectiles.has(other_entity))
			handle_projectile_chest_collision(other_entity, collision_entity);
		// wall collisions
		else if (registry.wallCollisions.has(collision_entity) && registry.enemies.has(other_entity))
			handle_wall_enemy_collision(collision_entity, other_entity);
		else if (registry.enemies.has(collision_entity) && registry.wallCollisions.has(other_entity))
			handle_wall_enemy_collision(other_entity, collision_entity);
		else if (registry.wallCollisions.has(collision_entity) && registry.players.has(other_entity))
			handle_wall_player_collision(collision_entity, other_entity);
		else if (registry.players.has(collision_entity) && registry.wallCollisions.has(other_entity))
			handle_wall_player_collision(other_entity, collision_entity);
		// enemy - player
		else if (registry.players.has(collision_entity) && registry.enemies.has(other_entity))
			handle_player_enemy_collision(collision_entity, other_entity);
		else if (registry.enemies.has(collision_entity) && registry.players.has(other_entity))
			handle_player_enemy_collision(other_entity, collision_entity);
		else if (registry.players.has(collision_entity) && registry.enemyRoomManagers.has(other_entity))
			handle_player_enemy_room_collision(collision_entity, other_entity);
		else if (registry.players.has(other_entity) && registry.enemyRoomManagers.has(collision_entity))
			handle_player_enemy_room_collision(other_entity, collision_entity);

		else if (registry.projectiles.has(other_entity) && registry.environmentObjects.has(collision_entity))
			handle_projectile_environment_object_collision(other_entity, collision_entity);
		else if (registry.projectiles.has(collision_entity) && registry.environmentObjects.has(other_entity))
			handle_projectile_environment_object_collision(collision_entity, other_entity);

		else if (registry.players.has(collision_entity) && registry.environmentObjects.has(other_entity))
			handle_wall_player_collision(other_entity, collision_entity);
		else if (registry.players.has(other_entity) && registry.environmentObjects.has(collision_entity))
			handle_wall_player_collision(collision_entity, other_entity);

		else if (registry.enemies.has(collision_entity) && registry.environmentObjects.has(other_entity))
			handle_wall_enemy_collision(other_entity, collision_entity);
		else if (registry.enemies.has(other_entity) && registry.environmentObjects.has(collision_entity))
			handle_wall_enemy_collision(collision_entity, other_entity);
	}

	for (std::vector<Entity>::iterator it = to_be_destroyed.begin(); it != to_be_destroyed.end();)
	{
		if (registry.lootables.has(*it)) {
			Lootable& l = registry.lootables.get(*it);
			if (registry.transforms.has(*it)) {
				Transformation t = registry.transforms.get(*it);
				createInteractableDrop(renderer, t.position, l.drop);

				std::cout << "LOOT DROPPED" << std::endl;
			}
		}
		registry.remove_all_components_of(*it);
		it++;
	}

	already_collided.clear();
	to_be_destroyed.clear();

	registry.collisions.clear();
}

void WorldSystem::handle_projectile_enemy_collision(Entity projectile_entity, Entity enemy_entity)
{
	Mix_PlayChannel(-1, enemy_hurt, 0);
	auto& projectile = registry.projectiles.get(projectile_entity);

	auto& enemyHP = registry.healths.get(enemy_entity).currentHealth;
	enemyHP -= projectile.damage; 
	 
	Transformation enemy_transform = registry.transforms.get(enemy_entity); 
	if (projectile.damage != 0)
		createTextPopup(std::to_string((int)projectile.damage), DAMAGE_NUMBER_COLOR, 1.0, vec2(0.5), 0, enemy_transform.position, false);
	if (enemyHP <= 0) {
		to_be_destroyed.push_back(enemy_entity);
		current_kills++;
		GoalManager& goal_manager = registry.goalManagers.components[0];
		goal_manager.current_kills++;
		notify_room_manager();
	} else if (registry.renderRequests.has(enemy_entity)) { // Only hitflash if enemy is not dead

		RenderRequest& rr = registry.renderRequests.get(enemy_entity);
		rr.is_hitflash = true;

		std::function<void()> timeout = [enemy_entity]() {
			if (!registry.renderRequests.has(enemy_entity)) {
				return;
			}

			RenderRequest& rr = registry.renderRequests.get(enemy_entity);
			rr.is_hitflash = false;
			};
		createTimer(HITFLASH_DURATION, timeout);
	}

	ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];

	spell->onDeath(renderer, projectile_entity);

	already_collided.push_back(projectile_entity);
}

void WorldSystem::notify_room_manager() {
	for (Entity entity : registry.enemyRoomManagers.entities) {
		EnemyRoomManager& room_manager = registry.enemyRoomManagers.get(entity);
		//std::cout << "Looking for room manager.." << std::endl;
		if (room_manager.is_active) {
			bool last_enemy_killed = room_manager.enemyDied();
			//std::cout << "Enemy died!" << std::endl;

			if (last_enemy_killed) {
				is_in_combat = false;
				removeWallsFromRoom(entity);
			}
			return;
		}
	}
	std::cout << "Warning: enemy died but could not find active room manager!" << std::endl;

	
}

void WorldSystem::handle_projectile_chest_collision(Entity projectile_entity, Entity chest_entity)
{
	//Mix_PlayChannel(-1, enemy_hurt, 0);
	auto& projectile = registry.projectiles.get(projectile_entity);
	auto& chest = registry.chests.get(chest_entity);
	//std::cout << "Projectile-Enemy Collision" << std::endl;

	ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];

	auto& chestHP = registry.healths.get(chest_entity).currentHealth;
	chestHP -= spell->getDamage(); //change to projec damage
	Transformation transform = registry.transforms.get(chest_entity);
	createTextPopup(std::to_string((int)projectile.damage), DAMAGE_NUMBER_COLOR, 1.0, vec2(0.5), 0, transform.position, false);
	if (chestHP <= 0) {
		to_be_destroyed.push_back(chest_entity);
		//std::cout << "Enemy killed" << std::endl;
	} else if (registry.renderRequests.has(chest_entity)) { // Only hitflash if enemy is not dead

		RenderRequest& rr = registry.renderRequests.get(chest_entity);
		rr.is_hitflash = true;

		std::function<void()> timeout = [chest_entity]() {
			if (!registry.renderRequests.has(chest_entity)) {
				return;
			}

			RenderRequest& rr = registry.renderRequests.get(chest_entity);
			rr.is_hitflash = false;
			};
		createTimer(HITFLASH_DURATION, timeout);
	}

	spell->onDeath(renderer, projectile_entity);

	already_collided.push_back(projectile_entity);
}

void WorldSystem::handle_projectile_player_collision(Entity projectile_entity, Entity player_entity)
{
	Mix_PlayChannel(-1, player_hurt, 0);
	auto& projectile = registry.projectiles.get(projectile_entity);
	auto& player = registry.players.get(player_entity);
	
	if (projectile.owner == player_entity) {
		// already_collided.push_back(projectile_entity);
		return;
	}

	Mix_PlayChannel(-1, player_hurt, 0);
	std::cout << "You were hit!" << std::endl;

	ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];

	auto& playerHP = registry.healths.get(player_entity).currentHealth;
	playerHP -= projectile.damage; 
	Transformation transform = registry.transforms.get(player_entity);
	createTextPopup(std::to_string((int)projectile.damage), DAMAGE_NUMBER_COLOR, 1.0, vec2(0.5), 0, transform.position, false);
	if (playerHP <= 0) {
		startPlayerDeathSequence();
	} 

	RenderRequest& rr = registry.renderRequests.get(player_entity);
	rr.is_hitflash = true;

	std::function<void()> timeout = [player_entity]() {
		if (!registry.renderRequests.has(player_entity)) {
			return;
		}

		RenderRequest& rr = registry.renderRequests.get(player_entity);
		rr.is_hitflash = false;
		};
	createTimer(HITFLASH_DURATION, timeout);

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);

	screen_state.vignette_factor = 1.0;
	screen_state.vignette_persist_duration = 0.5;

	spell->onDeath(renderer, projectile_entity);

	already_collided.push_back(projectile_entity);

	GoalManager& goal_manager = registry.goalManagers.components[0];
	goal_manager.current_times_hit++;
}

void WorldSystem::handle_projectile_wall_collision(Entity projectile_entity, Entity wall_entity)
{
	Projectile projectile = registry.projectiles.get(projectile_entity);
	ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];

	if (projectile.spell_id == PROJECTILE_SPELL_ID::LIGHTNING)
	{
		// Get Transforms and hitboxes
		Transformation& projectile_transform = registry.transforms.get(projectile_entity);
		Transformation& wall_transform = registry.transforms.get(wall_entity);
		Hitbox projectile_hitbox = registry.hitboxes.get(projectile_entity);
		Hitbox wall_hitbox = registry.hitboxes.get(wall_entity);

		Motion& projectile_motion = registry.motions.get(projectile_entity);

		float player_half_width = abs(projectile_transform.scale.x) * projectile_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR / 2.0f;
		float player_half_height = abs(projectile_transform.scale.y) * projectile_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR / 2.0f;
		float wall_half_width = abs(wall_transform.scale.x) * wall_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR / 2.0f;
		float wall_half_height = abs(wall_transform.scale.y) * wall_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR / 2.0f;

		float left_overlap = (wall_transform.position.x + wall_half_width) - (projectile_transform.position.x - player_half_width);
		float right_overlap = (projectile_transform.position.x + player_half_width) - (wall_transform.position.x - wall_half_width);
		float top_overlap = (wall_transform.position.y + wall_half_height) - (projectile_transform.position.y - player_half_height);
		float bottom_overlap = (projectile_transform.position.y + player_half_height) - (wall_transform.position.y - wall_half_height);

		if (right_overlap <= 0 || left_overlap <= 0 || bottom_overlap <= 0 || top_overlap <= 0) {
			return;
		}

		float min_overlap = std::min({ right_overlap, left_overlap, bottom_overlap, top_overlap });

		vec2 collision_normal;
		if (min_overlap == right_overlap) {
			collision_normal = vec2(-1, 0);
		}
		else if (min_overlap == left_overlap) {
			collision_normal = vec2(1, 0);
		}
		else if (min_overlap == bottom_overlap) {
			collision_normal = vec2(0, -1);
		}
		else {
			collision_normal = vec2(0, 1);
		}

		float dot_product = glm::dot(projectile_motion.velocity, collision_normal);
		if (dot_product < 0) {
			projectile_motion.velocity = projectile_motion.velocity - 2.0f * dot_product * collision_normal;

			// Guo: Should projectiles bounce slower?
			// projectile_motion.velocity *= 0.8f;
		}
	}
	else 
	{
		spell->onDeath(renderer, projectile_entity);
		already_collided.push_back(projectile_entity);
	}
	
}

void WorldSystem::handle_wall_player_collision(Entity wall_entity, Entity player_entity)
{
	//auto& wall = registry.walls.get(wall_entity);
	auto& player = registry.players.get(player_entity);
 
	// Get Transforms and hitboxes
	Transformation& player_transform = registry.transforms.get(player_entity);
	Transformation& wall_transform = registry.transforms.get(wall_entity);
	Hitbox player_hitbox = registry.hitboxes.get(player_entity);
	Hitbox wall_hitbox = registry.hitboxes.get(wall_entity);
	
	Motion& player_motion = registry.motions.get(player_entity);
	
	// Divide hitboxes into 4
	float player_half_width = abs(player_transform.scale.x) * player_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR / 2.0f;
	float player_half_height = abs(player_transform.scale.y) * player_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR / 2.0f;
	float wall_half_width = abs(wall_transform.scale.x) * wall_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR / 2.0f;
	float wall_half_height = abs(wall_transform.scale.y) * wall_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR / 2.0f;
	
	// Calculate overlap (in terms of player)
	float left_overlap = (wall_transform.position.x + wall_half_width) - (player_transform.position.x - player_half_width);
	float right_overlap = (player_transform.position.x + player_half_width) - (wall_transform.position.x - wall_half_width);
	float top_overlap = (wall_transform.position.y + wall_half_height) - (player_transform.position.y - player_half_height);
	float bottom_overlap = (player_transform.position.y + player_half_height) - (wall_transform.position.y - wall_half_height);
	
	// Ensure collisions are actually happening
	if (right_overlap <= 0 || left_overlap <= 0 || bottom_overlap <= 0 || top_overlap <= 0) {
		return;
	}
	
	// Find the smallest overlap to determine which direction to resolve
	float min_overlap = std::min({right_overlap, left_overlap, bottom_overlap, top_overlap});
	
	vec2 collision_normal;
	if (min_overlap == right_overlap) {
		collision_normal = vec2(-1, 0);
	}
	else if (min_overlap == left_overlap) {
		collision_normal = vec2(1, 0);
	}
	else if (min_overlap == bottom_overlap) {
		collision_normal = vec2(0, -1);
	}
	else {
		collision_normal = vec2(0, 1);
	}
	
	if (collision_normal.x != 0) {
		// Horizontal collisions
		player_transform.position.x += collision_normal.x * min_overlap;
	} else {
		// vertical collisions
		player_transform.position.y += collision_normal.y * min_overlap;
	}
	
	// Calculate dot product between velocity and normal
	float dot_product = glm::dot(player_motion.velocity, collision_normal);
	if (dot_product < 0) {
		player_motion.velocity -= collision_normal * dot_product;
	}

	//std::cout << "Wall collision!" << std::endl;
}

void WorldSystem::handle_wall_enemy_collision(Entity wall_entity, Entity enemy_entity)
{
	//auto& wall = registry.walls.get(wall_entity);
	Enemy* enemy = registry.enemies.get(enemy_entity);

	// Get Transforms and hitboxes
	Transformation& enemy_transform = registry.transforms.get(enemy_entity);
	Transformation& wall_transform = registry.transforms.get(wall_entity);
	Hitbox enemy_hitbox = registry.hitboxes.get(enemy_entity);
	Hitbox wall_hitbox = registry.hitboxes.get(wall_entity);

	Motion& enemy_motion = registry.motions.get(enemy_entity);

	// Divide hitboxes into 4
	const float ENEMY_WALL_PADDING = 1.05f;
	float enemy_half_width = abs(enemy_transform.scale.x) * enemy_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR * ENEMY_WALL_PADDING / 2.0f;
	float enemy_half_height = abs(enemy_transform.scale.y) * enemy_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR * ENEMY_WALL_PADDING / 2.0f;
	float wall_half_width = abs(wall_transform.scale.x) * wall_hitbox.hitbox_scale.x * PIXEL_SCALE_FACTOR / 2.0f;
	float wall_half_height = abs(wall_transform.scale.y) * wall_hitbox.hitbox_scale.y * PIXEL_SCALE_FACTOR / 2.0f;

	// Calculate overlap (in terms of enemy)
	float left_overlap = (wall_transform.position.x + wall_half_width) - (enemy_transform.position.x - enemy_half_width);
	float right_overlap = (enemy_transform.position.x + enemy_half_width) - (wall_transform.position.x - wall_half_width);
	float top_overlap = (wall_transform.position.y + wall_half_height) - (enemy_transform.position.y - enemy_half_height);
	float bottom_overlap = (enemy_transform.position.y + enemy_half_height) - (wall_transform.position.y - wall_half_height);

	// Ensure collisions are actually happening
	if (right_overlap <= 0 || left_overlap <= 0 || bottom_overlap <= 0 || top_overlap <= 0) {
		return;
	}

	// Find the smallest overlap to determine which direction to resolve
	float min_overlap = std::min({ right_overlap, left_overlap, bottom_overlap, top_overlap });

	vec2 collision_normal;
	if (min_overlap == right_overlap) {
		collision_normal = vec2(-1, 0);
	}
	else if (min_overlap == left_overlap) {
		collision_normal = vec2(1, 0);
	}
	else if (min_overlap == bottom_overlap) {
		collision_normal = vec2(0, -1);
	}
	else {
		collision_normal = vec2(0, 1);
	}

	const float PUSHBACK_PADDING = 1.05;
	if (collision_normal.x != 0) {
		// Horizontal collisions
		enemy_transform.position.x += collision_normal.x * min_overlap * PUSHBACK_PADDING;
	}
	else {
		// vertical collisions
		enemy_transform.position.y += collision_normal.y * min_overlap * PUSHBACK_PADDING;
	}

	// Calculate dot product between velocity and normal
	float dot_product = glm::dot(enemy_motion.velocity, collision_normal);
	if (dot_product < 0) {
		vec2 perpendicular = vec2(-collision_normal.y, collision_normal.x);
		enemy_motion.velocity -= collision_normal * dot_product * 1.05f;
		
		// If enemy is moving diagonally into a wall, adjust the velocity more aggressively
		if (enemy_motion.velocity.x != 0 && enemy_motion.velocity.y != 0) {
			enemy_motion.velocity += perpendicular * 0.05f * glm::length(enemy_motion.velocity);
		}
	}
}

void WorldSystem::handle_player_enemy_collision(Entity player_entity, Entity enemy_entity)
{
	//std::cout << "Player-Enemy Collision" << std::endl;
}

void WorldSystem::handle_player_enemy_room_collision(Entity player_entity, Entity enemy_room_entity)
{
	EnemyRoomManager& enemy_room_manager = registry.enemyRoomManagers.get(enemy_room_entity);
	if (enemy_room_manager.is_triggered) {
		return;
	}

	is_in_combat = true;

	std::cout << "Player-Enemy-Room Collision!" << std::endl;

	enemy_room_manager.onPlayerEntered();
	addWallsToRoom(enemy_room_entity);
}

void WorldSystem::addWallsToRoom(Entity room_manager_entity) {
	EnemyRoomManager& room_manager = registry.enemyRoomManagers.get(room_manager_entity);
	Transformation transform = registry.transforms.get(room_manager_entity);

	Entity top_wall = createWallCollisionEntity(transform.position - vec2(0, (transform.scale.y / 2 + 1) * TILE_SIZE), vec2(transform.scale.x + 2, 1));
	Entity bottom_wall = createWallCollisionEntity(transform.position + vec2(0, (transform.scale.y / 2 + 1) * TILE_SIZE), vec2(transform.scale.x + 2, 1));

	Entity left_wall = createWallCollisionEntity(transform.position - vec2((transform.scale.x / 2 + 1) * TILE_SIZE, 0), vec2(1, transform.scale.y + 2));
	Entity right_wall = createWallCollisionEntity(transform.position + vec2((transform.scale.x / 2 + 1) * TILE_SIZE, 0), vec2(1, transform.scale.y + 2));

	room_manager.wall_entities.push_back(top_wall);
	room_manager.wall_entities.push_back(bottom_wall);
	room_manager.wall_entities.push_back(left_wall);
	room_manager.wall_entities.push_back(right_wall);
}

void WorldSystem::removeWallsFromRoom(Entity room_manager_entity) {
	EnemyRoomManager& room_manager = registry.enemyRoomManagers.get(room_manager_entity);

	for (int entity : room_manager.wall_entities) {
		registry.remove_all_components_of(entity);
	}
}

void WorldSystem::handle_projectile_environment_object_collision(Entity projectile_entity, Entity environment_object_entity) {
	Projectile projectile = registry.projectiles.get(projectile_entity);
	ProjectileSpell* spell = projectile_spells[(int)projectile.spell_id];
	spell->onDeath(renderer, projectile_entity);

	already_collided.push_back(projectile_entity);

	if (!registry.healths.has(environment_object_entity)) {
		return;
	}

	Health& health = registry.healths.get(environment_object_entity);
	health.currentHealth -= projectile.damage;

	if (health.currentHealth <= 0) {
		to_be_destroyed.push_back(environment_object_entity);
	}
	else if (registry.renderRequests.has(environment_object_entity)) { // Only hitflash if enemy is not dead

		RenderRequest& rr = registry.renderRequests.get(environment_object_entity);
		rr.is_hitflash = true;

		std::function<void()> timeout = [environment_object_entity]() {
			if (!registry.renderRequests.has(environment_object_entity)) {
				return;
			}

			RenderRequest& rr = registry.renderRequests.get(environment_object_entity);
			rr.is_hitflash = false;
			};
		createTimer(HITFLASH_DURATION, timeout);
	}
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

void WorldSystem::update_key_held_map() {
	key_held_map.clear();
	auto up = setting.action_key["player_move_up"];
	for (auto it = up.begin(); it != up.end(); ++it) key_held_map[*it] = false;

	auto left = setting.action_key["player_move_left"];
	for (auto it = left.begin(); it != left.end(); ++it) key_held_map[*it] = false;

	auto down = setting.action_key["player_move_down"];
	for (auto it = down.begin(); it != down.end(); ++it) key_held_map[*it] = false;

	auto right = setting.action_key["player_move_right"];
	for (auto it = right.begin(); it != right.end(); ++it) key_held_map[*it] = false;

	auto movement = setting.action_key["movement_spell"];
	for (auto it = movement.begin(); it != movement.end(); ++it) key_held_map[*it] = false;

}

// on key callback
void WorldSystem::on_key(int key, int, int action, int mod) {

	// If you are in a cutscene, go to the next cutscene
	if ((int)game_screen >= (int)GAME_SCREEN_ID::CUTSCENE_INTRO && action == GLFW_PRESS) {

		// Escape to skip
		if (key == GLFW_KEY_ESCAPE) {
			if (!setting.get_tutorial_completed()) {
				transitionToScene(GAME_SCREEN_ID::TUTORIAL);
				tutorial_stage = 0;
				stage_presented = false;
			} else {
				transitionToScene(GAME_SCREEN_ID::HUB);
			}
			return;
		}
		
		// Press any key to continue to the next cutscene
		goto_next_cutscene();

		return;
	}

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);

	if (screen_state.is_paused && screen_state.pause_state == "setting") {
		if (key == GLFW_KEY_ENTER) {
			if (action == GLFW_PRESS && record_element != "") {
				setting.modify_key_binding(record_element, keys, setting.find_key(record_element));
				update_key_held_map();
				record_element = "";
				keys.clear();
				pause_on("setting");
				setting.save_setting();
			}
		}
		else {
			if (action == GLFW_PRESS) {
				bool has_key = false;
				for (int k : keys) {
					if (k == key) { 
						has_key = true; 
						break; 
					}
				}
				if (!has_key) keys.push_back(key);
				pause_on("setting");
			}
		}
	}

	//test
	//if (action == GLFW_PRESS && key == GLFW_KEY_L) update_volume();

	// exit game w/ ESC
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {

		audio_slider_is_held = false;
		// Mark: Save the Highest Score here 
		//update_record();
		//close_window();
		
		if (game_screen >= GAME_SCREEN_ID::CUTSCENE_INTRO) {
			// Don't pause on cutscene 
		}
		else if (game_screen != GAME_SCREEN_ID::INTRO)
		{
			Entity screen_state_entity = registry.screenStates.entities[0];
			ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
			screen_state.is_paused ? pause_off() : pause_on();
		}
		else {
			close_window();
		}
		
	}

	if (screen_state.is_paused) return; // Mark: if game is paused, every trigger after this line will be disabled.

	// if action is pressed or held, handle WASD
	// Munn: Unfortunately, GLFW does not support a simple "when held" action, so i made a map of
	//       key -> is_held to manually check it each frame
	if (action == GLFW_PRESS) {
		// if key is in key_held_map
		if (key_held_map.find(key) != key_held_map.end()) {
			key_held_map[key] = true;
		}

		/*
		if (key == GLFW_KEY_E || key == GLFW_KEY_Q) {
			on_interact_pressed(key, mod);
		}
		*/
		if (setting.key_has_action(key, "interact_relic")
			|| setting.key_has_action(key, "interact_spell")
			|| setting.key_has_action(key, "interact_other")
			|| setting.key_has_action(key, "interact_health_pack"))
		{
			on_interact_pressed(key, mod);
		}

		// Press T to switch to Tutorial, only availble in LEVEL_1
		if (key == GLFW_KEY_T) 
		{	
			// testing
			//if (game_screen != GAME_SCREEN_ID::INTRO) {
			//	transitionToScene(GAME_SCREEN_ID::INTRO, 0.0f);
			//}
			//else {
			//	transitionToScene(GAME_SCREEN_ID::HUB);
			//}

			//if (game_screen == GAME_SCREEN_ID::LEVEL_1) 
			//{
			//	game_screen = GAME_SCREEN_ID::TUTORIAL;
			//	//restart_game();
			//	Entity player_entity = registry.players.entities[0];
			//	Health& player_health = registry.healths.get(player_entity);
			//	player_health.currentHealth = player_health.maxHealth;

			//	tutorial_stage = 0;
			//	loadLevel();
			//	registry.screenStates.components[0].vignette_effect_time = VIGNETTE_EFFECT_LAST_MS;
			//}
			//else if (game_screen == GAME_SCREEN_ID::TUTORIAL)
			//{
			//	game_screen = GAME_SCREEN_ID::LEVEL_1;
			//	//restart_game();
			//	loadLevel();
			//	registry.screenStates.components[0].vignette_effect_time = VIGNETTE_EFFECT_LAST_MS;
			//}
		}

		if (key == GLFW_KEY_R) {
			//game_screen = GAME_SCREEN_ID::BOSS_1;
			//loadLevel();
			//startPlayerDeathSequence();
			/*Entity player_entity = registry.players.entities[0];
			Transformation player_transform = registry.transforms.get(player_entity);

			createTextPopup("Text", vec3(1.0), 1.0, vec2(0.5), 0, player_transform.position, false);*/
		}

		/*if (key == GLFW_KEY_SPACE)
		{
			if (game_screen == GAME_SCREEN_ID::TUTORIAL)
			{
				if (tutorial_stage != 2) {
					tutorial_stage++;
					stage_presented = false;
				}
			}
		}*/

	}

	if (action == GLFW_RELEASE) {
		if (key_held_map.find(key) != key_held_map.end()) {
			key_held_map[key] = false;
		}
	}
}

// Mark: edit for handling key bind
void WorldSystem::on_interact_pressed(int key, int mods) {
	
	for (Entity entity : registry.interactables.entities) {
		Interactable& interactable = registry.interactables.get(entity);

		if (interactable.can_interact && !interactable.disabled) {

			switch (interactable.interactable_id) {
			case INTERACTABLE_ID::PROJECTILE_SPELL_DROP: {

				if (!setting.key_has_action(key, "interact_spell")) break;

				projectile_spell_interactable.interact(entity);
				std::cout << "Interacted: Picked up projectile spell: " << interactable.spell_id << std::endl;
				break;
			}
			case INTERACTABLE_ID::MOVEMENT_SPELL_DROP: {

				if (!setting.key_has_action(key, "interact_spell")) break;

				movement_spell_interactable.interact(entity);
				std::cout << "Interacted: Picked up movement spell: " << interactable.spell_id << std::endl;
				break;
			}
			case INTERACTABLE_ID::HEALTH_RESTORE: {

				if (!setting.key_has_action(key, "interact_health_pack")) break;

				health_restore_interactable.interact(entity);
				std::cout << "Interacted: Picked up health restore! Healed for: " << interactable.heal_amount << std::endl;
				break;
			}
			case INTERACTABLE_ID::NEXT_LEVEL_ENTRY: {
				if (!setting.key_has_action(key, "interact_other")) break;

				entry_interactable.interact(entity);

				// Save tutorial completed
				if (game_screen == GAME_SCREEN_ID::TUTORIAL) {
					setting.set_tutorial_completed(true);
					setting.save_setting();
				}

				transitionToScene((GAME_SCREEN_ID)interactable.game_screen_id, 0.5f);
				break;
			}
			case INTERACTABLE_ID::RELIC_DROP: {
				// Munn: LShift + E to apply to movement spell, maybe change in the future?
				/*
				if (key == GLFW_KEY_Q) {
					relic_interactable.interact(entity, true); 
					std::cout << "Interacted: Applied relic " << interactable.relic_id << " to movement spell" << std::endl;
				}
				else {
					relic_interactable.interact(entity, false); 
					std::cout << "Interacted: Applied relic " << interactable.relic_id << " to projectile spell" << std::endl;
				}
				*/
				if (!setting.key_has_action(key, "interact_relic")) break;

				relic_interactable.interact(entity, false);
				std::cout << "Interacted: Applied relic " << interactable.relic_id << " to projectile spell" << std::endl;
				break;
			}
			case INTERACTABLE_ID::FOUNTAIN: { 
				if (!setting.key_has_action(key, "interact_other")) break;

				fountain_interactable.interact(entity);
				break;
			}
			case INTERACTABLE_ID::SACRIFICE_FOUNTAIN: { 
				if (!setting.key_has_action(key, "interact_other")) break;

				sacrifice_fountain_interactable.interact(renderer, entity);
				break;
			}
			case INTERACTABLE_ID::NPC: {
				NPC& npc = registry.npcs.get(entity);
				TextPopup& text = registry.textPopups.get(npc.text_entity);
				*text.alpha = 1.0;

				std::string dialogue = getDialogue(npc.npc_name, npc.npc_conversation, npc.current_dialogue_index);
				npc.current_dialogue_index++;

				if (dialogue == "END") {
					*text.alpha = 0.0;
					text.text = "";
					interactable.can_interact = false;
					interactable.disabled = true;
				}
				else {
					text.text = dialogue;
				}
				break;
			}
			};

			break; // Only interact with one item (if there are multiple items you can interact with... somehow
		} 
	}
}


void WorldSystem::on_mouse_move(vec2 mouse_position) {

	// record the current mouse position
	mouse_pos_x = mouse_position.x;
	mouse_pos_y = mouse_position.y;

	/* (mouse_pos_x >= transform_comp.position.x - 10.0f
					&& mouse_pos_x <= transform_comp.position.x + 10.0f
					&& mouse_pos_y >= transform_comp.position.y - 50.0f
					&& mouse_pos_y <= transform_comp.position.y + 50.0f)*/

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);


	if ((game_screen == GAME_SCREEN_ID::INTRO || screen_state.is_paused) && audio_slider_is_held) {
		Entity& slideBarEntity = registry.slideBars.entities[0];
		Slide_Bar slideBar = registry.slideBars.get(slideBarEntity);
		Transformation& slideBarTransform = registry.transforms.get(slideBarEntity);

		float audio_intensity = setting.get_audio();
		float bar_ratio = 0;
		for (Entity block_entity : registry.slideBlocks.entities) {
			Transformation& transform_comp = registry.transforms.get(block_entity);
			if (mouse_pos_x <= slideBarTransform.position.x - 200) { // hardcoded cause rushed but stored in textures
				transform_comp.position.x = slideBarTransform.position.x - 200;
			}
			else if (mouse_pos_x >= slideBarTransform.position.x + 200) { // hardcoded cause rushed but stored in textures
				transform_comp.position.x = slideBarTransform.position.x + 200;
			}
			else {
				transform_comp.position.x = mouse_position.x;
			}

			bar_ratio = (transform_comp.position.x - (slideBarTransform.position.x - 200)) / (slideBarTransform.position.x - 200);
		}

		// get the relevant audio value according to the slider pos


		std::cout << bar_ratio << std::endl;
		audio_intensity = 0 + bar_ratio * 96; // so we range from 16 to 80
		setting.modify_audio_intensity(audio_intensity);
		update_volume();
		setting.save_setting();
	}
}

void WorldSystem::on_mouse_button_pressed(int button, int action, int mods) {

	Entity screen_state_entity = registry.screenStates.entities[0];
	ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
 
	// LMB pressed down (does not detect hold)
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT) { 

		// Mark: Just for hard coding
		std::cout << "Mouse position: <" << mouse_pos_x << ", " << mouse_pos_y << ">" << std::endl;

		//std::cout << "Test" << std::endl;

		// Written just for testing enemy AI
		//createRandomEnemy(renderer, vec2(mouse_pos_x, mouse_pos_y));
		//Mix_PlayChannel(-1, enemy_spawned, 0);
	}

	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		// Mark: Press left button on the text
		if (game_screen == GAME_SCREEN_ID::INTRO || screen_state.is_paused)
		{
			for (Entity text_entity : registry.texts.entities)
			{
				Text& text_info = registry.texts.get(text_entity);

				// Mark: Text here is not press-able
				if (strcmp(text_info.text.c_str(), "Cleanse the Corruption") == 0) continue;
				//if (strcmp(text_info.text.c_str(), "Audio:") == 0) continue;
				if (!text_info.text.empty() && text_info.text.back() == ':') continue;

				if (mouse_pos_x >= text_info.mouse_detection_box_start.x
					&& mouse_pos_x <= text_info.mouse_detection_box_end.x
					&& mouse_pos_y >= text_info.mouse_detection_box_start.y
					&& mouse_pos_y <= text_info.mouse_detection_box_end.y)
				{
					text_info.mouse_pressed = true;
					if (strcmp(screen_state.pause_state.c_str(), "setting") == 0) {

						if (strcmp(text_info.text.c_str(), "Back") == 0) continue;
						//if (strcmp(text_info.text.c_str(), "Reset Key Bind") == 0) continue;

						std::string pending_record, useless;
						std::istringstream textss(text_info.text);
						textss >> pending_record >> useless;

						//if (pending_record.back() == ':') continue;

						record_element = pending_record;
						keys.clear();
						pause_on("setting");
					}
				} 
			}

			// need to fix
			for (Entity block_entity : registry.slideBlocks.entities)
			{
				Transformation& transform_comp = registry.transforms.get(block_entity);
				if (mouse_pos_x >= transform_comp.position.x - 10.0f
					&& mouse_pos_x <= transform_comp.position.x + 10.0f
					&& mouse_pos_y >= transform_comp.position.y - 50.0f
					&& mouse_pos_y <= transform_comp.position.y + 50.0f)
				{
					audio_slider_is_held = !audio_slider_is_held;
				}
			}
		}
	}


	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
		// Mark: Release left mouse on text to activate function
		if (game_screen == GAME_SCREEN_ID::INTRO || screen_state.is_paused)
		{
			audio_slider_is_held = false;
			bool switch_to_setting = false;
			bool from_setting_to_pause = false;
			bool ready_leave_pause = false;
			bool ready_back_title = false;
			//bool key_binding_reset = false;

			for (Entity text_entity : registry.texts.entities)
			{
				Text& text_info = registry.texts.get(text_entity);
				text_info.mouse_pressed = false;

				if (mouse_pos_x >= text_info.mouse_detection_box_start.x
					&& mouse_pos_x <= text_info.mouse_detection_box_end.x
					&& mouse_pos_y >= text_info.mouse_detection_box_start.y
					&& mouse_pos_y <= text_info.mouse_detection_box_end.y)
				{
					if (strcmp(text_info.text.c_str(), "Start Play") == 0)
					{
						transitionToScene(GAME_SCREEN_ID::CUTSCENE_INTRO);
						if (!setting.get_tutorial_completed()) {
							tutorial_stage = 0;
							stage_presented = false;
						}
						/*ScreenState& screen_state = registry.screenStates.components[0];
						screen_state.is_paused = false;*/
					}
					else if (strcmp(text_info.text.c_str(), "Setting") == 0)
					{
						// stub
						switch_to_setting = true;
						break;
					}
					else if (strcmp(text_info.text.c_str(), "Exit Game") == 0)
					{
						close_window();
					}
					else if (strcmp(text_info.text.c_str(), "Resume Game") == 0)
					{
						ready_leave_pause = true;
						break;
					}
					else if (strcmp(text_info.text.c_str(), "Back To Title") == 0)
					{
						ready_leave_pause = true;
						ready_back_title = true;
						break;
					}
					else if (strcmp(text_info.text.c_str(), "Back") == 0) {
						record_element = "";
						keys.clear();
						if (game_screen == GAME_SCREEN_ID::INTRO) {
							ready_leave_pause = true;
							ready_back_title = true;
						}
						else {
							from_setting_to_pause = true;
						}
						break;
					}
					//else if (strcmp(text_info.text.c_str(), "Reset Key Bind") == 0) {
					//	//key_binding_reset = true;
					//	//switch_to_setting = true;
					//	break;
					//}
				}
			}

			//if (key_binding_reset) setting.reset_key_bind();
			if (switch_to_setting) pause_on("setting");
			if (from_setting_to_pause) pause_on();
			if (ready_leave_pause) pause_off();
			if (ready_back_title) {
				update_record();
				transitionToScene(GAME_SCREEN_ID::INTRO);
			}
		}
	}
	
	if (game_screen != GAME_SCREEN_ID::INTRO && !screen_state.is_paused) {
		if (action == GLFW_PRESS) {
			// if key is in key_held_map
			if (key_held_map.find(button) != key_held_map.end()) {
				key_held_map[button] = true;
			}
		}

		if (action == GLFW_RELEASE) {
			if (key_held_map.find(button) != key_held_map.end()) {
				key_held_map[button] = false;
			}
		}
	}

	if (game_screen >= GAME_SCREEN_ID::CUTSCENE_INTRO && action == GLFW_PRESS) {
		goto_next_cutscene();
	}
}


void WorldSystem::goto_next_cutscene() {
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO) {
		transitionToScene(GAME_SCREEN_ID::CUTSCENE_INTRO_2);
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_2) {
		transitionToScene(GAME_SCREEN_ID::CUTSCENE_INTRO_3);
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_3) {
		transitionToScene(GAME_SCREEN_ID::CUTSCENE_INTRO_4);
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_INTRO_4) {
		if (!setting.get_tutorial_completed()) {
			transitionToScene(GAME_SCREEN_ID::TUTORIAL);
			tutorial_stage = 0;
			stage_presented = false;
		} else {
			transitionToScene(GAME_SCREEN_ID::HUB);
		}
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO) {
		transitionToScene(GAME_SCREEN_ID::CUTSCENE_OUTRO_2);
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO_2) {
		transitionToScene(GAME_SCREEN_ID::CUTSCENE_OUTRO_3);
	}
	if (game_screen == GAME_SCREEN_ID::CUTSCENE_OUTRO_3) {
		transitionToScene(GAME_SCREEN_ID::HUB);
	}
}


void WorldSystem::update_window_caption() {

	int fps = getFPS();
	std::stringstream title_ss;
	title_ss << "Cleanse the Corruption" << " / ";

	title_ss << "Highest score: " << highest_score << " / ";

	title_ss << "Enemies Killed: " << current_kills << " / ";

	if (game_screen == GAME_SCREEN_ID::INTRO)
	{
		title_ss << "Floor: Intro / ";
	}
	else 
	{
		title_ss << "Floor " << current_floor << " / ";
	}

	title_ss << "FPS: " << fps << " / ";

	glfwSetWindowTitle(window, title_ss.str().c_str());
	
}


int WorldSystem::getFPS() {
    static Uint32 lastTime = SDL_GetTicks();
    static int frameCount = 0;
    static int fps = 0;

    frameCount++;

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime > lastTime + 1000) { 
        fps = frameCount;
        frameCount = 0;
        lastTime = currentTime;
    }

    return fps;
}
void WorldSystem::reset_player_spells() {
	Entity player_entity = registry.players.entities[0];

	registry.spellSlotContainers.remove(player_entity);

	SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(player_entity);

	SpellSlot damageSpellSlot = SpellSlot();
	damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;
	damageSpellSlot.spell_id = (int)PROJECTILE_SPELL_ID::FIREBALL;

	spellSlotContainer.spellSlots.push_back(damageSpellSlot);

	SpellSlot movementSpellSlot = SpellSlot();
	movementSpellSlot.spell_type = SPELL_TYPE::MOVEMENT;
	movementSpellSlot.spell_id = (int)MOVEMENT_SPELL_ID::DASH;

	spellSlotContainer.spellSlots.push_back(movementSpellSlot);
}

// Munn: this is pretty much just a chain of timers
void WorldSystem::startPlayerDeathSequence() {
	Entity player_entity = registry.players.entities[0];
	
	player_dead = true;
	is_player_input_enabled = false;

	// Player death animation
	AnimationManager& animation_manager = registry.animation_managers.get(player_entity);
	animation_manager.transition_to(TEXTURE_ASSET_ID::PLAYER_DEATH);

	// Remove hitbox
	Hitbox& hitbox = registry.hitboxes.get(player_entity);
	hitbox.layer = 0;
	hitbox.mask = 0;

	// Fade screen to black
	std::function<void()> timeout = [this, player_entity]() {
		Entity screen_state_entity = registry.screenStates.entities[0];
		ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
		screen_state.darken_screen_factor = 0.0;

		// Fade out of black
		std::function<void()> timeout = [this, screen_state_entity, player_entity]() {
			// Load hub level
			game_screen = GAME_SCREEN_ID::HUB;
			loadLevel();

			// Enable hitbox
			Hitbox& hitbox = registry.hitboxes.get(player_entity);
			hitbox.layer = (int)COLLISION_LAYER::PLAYER;
			hitbox.mask = (int)COLLISION_MASK::WALL | (int)COLLISION_MASK::E_PROJECTILE | (int)COLLISION_MASK::ENEMY;

			// Pause player animation
			AnimationManager& animation_manager = registry.animation_managers.get(player_entity);
			animation_manager.transition_to(TEXTURE_ASSET_ID::PLAYER_RESPAWN, true);
			animation_manager.current_animation.pause();

			// After fading back in, play respawn animation
			std::function<void()> timeout = [this, player_entity]() {
				AnimationManager& animation_manager = registry.animation_managers.get(player_entity);
				animation_manager.current_animation.play();

				std::function<void()> timeout = [this, player_entity]() {
					AnimationManager& animation_manager = registry.animation_managers.get(player_entity);
					animation_manager.transition_to(TEXTURE_ASSET_ID::PLAYER_IDLE, true);
					is_player_input_enabled = true;
					player_dead = false;
					};

				createTimer(3.0f, timeout);
				};

			ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
			createTween(1.0f, timeout, &screen_state.darken_screen_factor, 1.0f, 0.0f);
			};

		// Fade to black
		createTween(1.0f, timeout, &screen_state.darken_screen_factor, 0.0f, 1.0f);
		};

	createTimer(2.0f, timeout);
}












void createFloorGoals() {
	GoalManager& goal_manager = registry.goalManagers.components[0];
	goal_manager.goals.clear();

	// Easy goal
	FloorGoal easy = FloorGoal();
	easy.goal_type = GoalType::KILLS;
	easy.num_kills_needed = 20;

	// Medium goal
	FloorGoal medium = FloorGoal();
	medium.goal_type = GoalType::TIME;
	medium.time_to_beat = 4 * 60; // 5 mins

	// Hard goal
	FloorGoal hard = FloorGoal();
	hard.goal_type = GoalType::TIMES_HIT;
	hard.num_times_hit = 5;

	goal_manager.goals.push_back(easy);
	goal_manager.goals.push_back(medium);
	goal_manager.goals.push_back(hard);
}


void resetGoalManagerStats() {
	GoalManager& goal_manager = registry.goalManagers.components[0];

	goal_manager.current_kills = 0;
	goal_manager.current_time = 0;
	goal_manager.current_times_hit = 0;
	goal_manager.timer_active = false;
	goal_manager.goals.clear();
}