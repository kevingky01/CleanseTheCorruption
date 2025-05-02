#pragma once

#include "world_init.hpp"
#include "render_system.hpp"
#include "map_gen/map_node.hpp"
#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"

// Spawn these enemies
struct EnemyWave { 
	// waves
	std::vector<std::pair<ENEMY_TYPE, vec2>> enemies; // enemies, and their positions
};

struct EnemyRoomManager { 
	EnemyRoomManager() { }

	EnemyRoomManager(std::vector<EnemyWave> enemy_waves) {
		this->enemy_waves = enemy_waves;
	}

	// Munn: scuffed?
	Entity this_entity;

	// States
	bool is_triggered = false; 
	bool is_active = false;

	// Wave management
	int current_wave = 0;
	std::vector<EnemyWave> enemy_waves;

	// Enemy management
	int current_enemies = 0;

	// Random enemy types (if waves are not specified)
	std::vector<ENEMY_TYPE> enemy_types;
	
	// For instantiating enemies
	RenderSystem* renderer; 

	std::vector<int> wall_entities;

	// For random enemies
	void init(std::vector<std::vector<int>>& arr, MapNode* map_node, std::vector<ENEMY_TYPE> enemy_types) {
		 
		int num_waves = (int)(uniform_dist(rng) * (MAX_WAVES - MIN_WAVES)) + MIN_WAVES;

		for (int i = 0; i < num_waves; i++) {
			int num_enemies = (int)(uniform_dist(rng) * (MAX_ENEMIES_PER_WAVE - MIN_ENEMIES_PER_WAVE)) + MIN_ENEMIES_PER_WAVE;

			EnemyWave enemy_wave;

			for (int j = 0; j < num_enemies; j++) {
				int rand_enemy = (int)(uniform_dist(rng) * enemy_types.size());
				ENEMY_TYPE enemy_type = enemy_types[rand_enemy];

				int rand_x;
				int rand_y;
				ivec2 rand_loc;

				do {
					rand_x = (int)(uniform_dist(rng) * map_node->size.x);
					rand_y = (int)(uniform_dist(rng) * map_node->size.y);

					rand_loc = ivec2(
						(map_node->array_pos.x + rand_x),
						(map_node->array_pos.y + rand_y)
					);

				} while (arr[rand_loc.x][rand_loc.y] == 1);

				vec2 rand_pos = { rand_loc.x * TILE_SIZE, rand_loc.y * TILE_SIZE };
				
				enemy_wave.enemies.push_back(std::pair(enemy_type, rand_pos));
			} 

			enemy_waves.push_back(enemy_wave);
		}
	}

	// Called when an enemy is killed
	// Returns true if last enemy has died, false otherwise
	bool enemyDied() {
		current_enemies -= 1;

		//std::cout << "Current enemies: " << current_enemies << std::endl;
		if (current_enemies <= 0) {
			current_wave++;
			if (current_wave >= enemy_waves.size()) {
				//std::cout << "End of room" << std::endl;
				// release walls 
				is_active = false;
				return true;
			}
			else {
				// spawn new wave  
				spawnEnemies(renderer);
				//std::cout << "Spawning new wave!" << std::endl;
				//std::cout << "Wave: " << current_wave << std::endl;
			}
		}

		return false;
	}

	void onPlayerEntered() {
		current_wave = 0;
		is_triggered = true;
		is_active = true;

		spawnEnemies(renderer);
	}

	// Creates enemy indicators
	void spawnEnemies(RenderSystem* renderer) {
		//std::cout << "Spawning enemies!" << std::endl;
		if (current_wave >= enemy_waves.size()) {
			std::cout << "Warning: Attempted to spawn enemies when no enemies remain!" << std::endl;
			return;
		}
		 
		EnemyWave wave = enemy_waves[current_wave];

		current_enemies = wave.enemies.size();

		for (int i = 0; i < wave.enemies.size(); i++) {
			std::pair<ENEMY_TYPE, vec2> enemy_info = wave.enemies[i];

			// TODO: Loop here to find a position that is not in a wall
			// Munn: not sure of a great way to do this? 
			std::cout << "ENEMY TYPE: " << (int)enemy_info.first << std::endl;
			createEnemySpawnIndicator(renderer, enemy_info.second, enemy_info.first); // render system, position, enemy type
		}
		//std::cout << "Current enemies: " << current_enemies << std::endl;
	}
};