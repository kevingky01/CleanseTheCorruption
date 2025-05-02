#include "animation_system.hpp"

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include <iostream>

void AnimationSystem::step(float elapsed_ms) {
	for (Entity animated_entity : registry.animation_managers.entities) {
		if (!registry.renderRequests.has(animated_entity)) {
			continue;
		}

		float stepSeconds = elapsed_ms / 1000.0f;

		AnimationManager& animation_manager = registry.animation_managers.get(animated_entity);
		Animation& animation = animation_manager.current_animation;
		RenderRequest& render_request = registry.renderRequests.get(animated_entity);

		render_request.used_texture = animation.asset_id;

		if (animation.is_paused) {
			continue;
		}

		animation.current_time += stepSeconds * animation.frame_rate;

		if (animation.current_time >= animation.num_frames) {
			if (animation.is_looping) {
				animation.current_time -= animation.num_frames;
			}
			else {
				animation.current_time = animation.num_frames - 1;
			}
		}
	}
}