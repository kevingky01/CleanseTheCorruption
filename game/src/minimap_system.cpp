#include "common.hpp"
#include "minimap_system.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include <iostream>

void MinimapSystem::step(float elapsed_ms) {
	Entity minimap_entity = registry.minimaps.entities[0];
	Minimap& minimap = registry.minimaps.get(minimap_entity);

	Entity player_entity = registry.players.entities[0];
	Transformation player_transform = registry.transforms.get(player_entity);

	for (Entity wall_entity : registry.walls.entities) {
		Transformation wall_transform = registry.transforms.get(wall_entity);

		float dist_to_wall = distance(wall_transform.position, player_transform.position);

		// If the wall is within range of the player
		if (dist_to_wall < minimap.reveal_range) {
			// And the wall isn't already revealed
			if (find(minimap.walls_revealed.begin(), minimap.walls_revealed.end(), (int)wall_entity) == minimap.walls_revealed.end()) {
				// Add it to revealed walls
				minimap.walls_revealed.push_back(wall_entity);

				vec2 tile_pos = vec2(
					(int)(wall_transform.position.x / TILE_SIZE),
					(int)(wall_transform.position.y / TILE_SIZE)
				);

				minimap.wall_positions.push_back(tile_pos);
				//std::cout << minimap.walls_revealed.size() << std::endl;
			}
		}
	}
}
