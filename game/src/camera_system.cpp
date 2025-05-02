#include "common.hpp"
#include "camera_system.hpp"
#include "world_system.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include <iostream>

const float MAX_CAM_DISTANCE = 500;
const float LERP_STRENGTH = 2.0;
void CameraSystem::step(float elapsed_ms) {
	for (Entity cameraEntity : registry.cameras.entities) { 
		
		// Get player position
		Entity playerEntity = registry.players.entities[0];
		vec2 player_pos = registry.transforms.get(playerEntity).position;

		// Get mouse position 
		vec2 mouse_world_pos = vec2(mouse_pos_x - WINDOW_WIDTH_PX / 2.0, mouse_pos_y - WINDOW_HEIGHT_PX / 2.0);

		// Put max distance on mouse position
		if (glm::length(mouse_world_pos) > MAX_CAM_DISTANCE) {
			mouse_world_pos = glm::normalize(mouse_world_pos) * MAX_CAM_DISTANCE; 
		}

		// Get midpoint between mouse and player
		vec2 mid_point = (player_pos + mouse_world_pos / 2.0f); 

		// Lerp camera pos to midpoint
		Transformation& camera_transform = registry.transforms.get(cameraEntity);
		float lerp_time = LERP_STRENGTH * elapsed_ms / 1000.0f;

		// Here, we interpolate the camera position towards the midpoint between the player and the mouse
		float cam_x = lerp(camera_transform.position.x, mid_point.x, lerp_time);
		float cam_y = lerp(camera_transform.position.y, mid_point.y, lerp_time);

		camera_transform.position = vec2(cam_x, cam_y); 
	}
}
