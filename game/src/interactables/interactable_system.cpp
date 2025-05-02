#include "interactable_system.hpp"

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include "physics_system.hpp"
#include "interactable.hpp"

void InteractableSystem::step(float elapsed_ms) {
	
	Entity player_entity = registry.players.entities[0];

	Entity nearest_entity = -1; // "null" entity
	float nearest_distance = 99999999;
	for (Entity entity : registry.interactables.entities) {
		
		Interactable& interactable = registry.interactables.get(entity);
		interactable.can_interact = false;

		if (interactable.disabled) {
			continue;
		}

		// Check for collision
		if (collides(player_entity, entity)) {
			Transformation player_transform = registry.transforms.get(player_entity);
			Transformation interactable_transform = registry.transforms.get(entity);

			// Check if it is the nearest interactable
			float interactable_distance = glm::distance(player_transform.position, interactable_transform.position);
			if (interactable_distance < nearest_distance) {
				nearest_distance = interactable_distance;
				nearest_entity = entity;
			}
		}
	}

	// Allow nearest entity to be interacted with
	if (nearest_entity != -1) {
		Interactable& interactable = registry.interactables.get(nearest_entity);
		interactable.can_interact = true; 
	}
}
