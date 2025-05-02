
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stdlib
#include <chrono>
#include <iostream>

// internal
#include "physics_system.hpp"
#include "ai_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "spell_slot_system.hpp"
#include "camera_system.hpp"
#include "projectile_spell_system.hpp"
#include "timer_system.hpp"
#include "tween_system.hpp"
#include "animation_system.hpp"
#include "particle_system.hpp"
#include "minimap_system.hpp"
#include "interactables/interactable_system.hpp"

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// global systems
	AISystem	  ai_system;
	WorldSystem   world_system;
	RenderSystem  renderer_system;
	PhysicsSystem physics_system;
	SpellSlotSystem spell_slot_system;
	CameraSystem camera_system;
	ProjectileSpellSystem projectile_spell_system;
	TimerSystem timer_system;
	TweenSystem tween_system;
	AnimationSystem animation_system;
	ParticleSystem particle_system;
	InteractableSystem interactable_system;
	MinimapSystem minimap_system;

	int seed = std::chrono::system_clock::now().time_since_epoch().count();
	//int seed = -1070238144;
	std::cout << seed << std::endl;
	rng.seed(seed);

	// initialize window
	GLFWwindow* window = world_system.create_window();
	if (!window) {
		// Time to read the error message
		std::cerr << "ERROR: Failed to create window.  Press any key to exit" << std::endl;
		getchar(); // Munn: what is this for? 
		return EXIT_FAILURE;
	}

	if (!world_system.start_and_load_sounds()) {
		std::cerr << "ERROR: Failed to start or load sounds." << std::endl;
	}

	std::cout << "Initializing renderer" << std::endl;
	// initialize the main systems
	renderer_system.init(window);
	world_system.init(&renderer_system);
	ai_system.init(&renderer_system);
	projectile_spell_system.renderer = &renderer_system;

	// variable timestep loop
	auto t = Clock::now();
	while (!world_system.is_over()) {

		// Mark: Check game screen and pause status
		GAME_SCREEN_ID game_screen = world_system.get_game_screen();
		Entity screen_state_entity = registry.screenStates.entities[0];
		ScreenState& screen_state = registry.screenStates.get(screen_state_entity);
		
		// processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// calculate elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		/*if (world_system.player_dead) {
			world_system.player_dead = false;
			world_system.init(&renderer_system);
			ai_system.init(&renderer_system);
			projectile_spell_system.renderer = &renderer_system;
			continue;
		}*/

		//std::cout << "Frames per second: " << 1 / (elapsed_ms / 1000.0) << std::endl; // Munn: we can use this for FPS counter requirement

		// CK: be mindful of the order of your systems and rearrange this list only if necessary
		world_system.step(elapsed_ms);

		if (game_screen != GAME_SCREEN_ID::INTRO && !screen_state.is_paused)
		{
			interactable_system.step(elapsed_ms);

			ai_system.step(elapsed_ms);

			projectile_spell_system.step(elapsed_ms);

			physics_system.step(elapsed_ms);

			spell_slot_system.step(elapsed_ms);

			world_system.handle_collisions();

			camera_system.step(elapsed_ms);

			timer_system.step(elapsed_ms);

			minimap_system.step(elapsed_ms);
		}

		tween_system.step(elapsed_ms);
    
		animation_system.step(elapsed_ms);

		if (game_screen != GAME_SCREEN_ID::INTRO && !screen_state.is_paused)
			particle_system.step(elapsed_ms);

		renderer_system.draw(game_screen);
	}

	return EXIT_SUCCESS;
}
