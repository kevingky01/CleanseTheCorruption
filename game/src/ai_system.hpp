#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tinyECS/registry.hpp"
#include "world_init.hpp"

#include <functional> // for function callbacks
#include <iostream>
#include <cassert>

class AISystem
{
public:

	// Initialize AI System
	void init(RenderSystem* renderer);

	// go through every enemies to update their behavior
	void step(float elapsed_ms);

protected:

	RenderSystem* renderer;

	// Manage the behavior of enemies
	// No behaviour in default
	void update(float elapsed_ms, Entity& player_entity, Entity& enemy_entity, Enemy& enemy);

	// C++ random number generator (Taken from world_system.hpp)
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};