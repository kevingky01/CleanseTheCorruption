#include "tween_system.hpp"
#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"
#include <functional> 
#include <iostream>

void TweenSystem::step(float elapsed_ms) {

	std::vector<Entity> to_be_destroyed;

	for (Entity tween_entity : registry.tweens.entities) {
		if (!registry.tweens.has(tween_entity)) {
			continue;
		}
		Tween& tween = registry.tweens.get(tween_entity);

		float stepSeconds = elapsed_ms / 1000.0f;

		if (!tween.is_active) {
			to_be_destroyed.push_back(tween_entity);
			continue;
		}

		tween.current_time -= stepSeconds;

		float lerp_t = 1 - (tween.current_time / tween.duration);

		switch (tween.type) {
		case TWEEN_TYPE::FLOAT: { 
			*tween.f_value = tween.interp_func(tween.f_from, tween.f_to, lerp_t);
			break;
		}
		case TWEEN_TYPE::VEC2: {
			std::cout << tween.v2_from.x << ", " << tween.v2_from.y << std::endl;
			std::cout << tween.v2_to.x << ", " << tween.v2_to.y << std::endl;
			tween.v2_value->x = tween.interp_func(tween.v2_from.x, tween.v2_to.x, lerp_t); 
			tween.v2_value->y = tween.interp_func(tween.v2_from.y, tween.v2_to.y, lerp_t);
			break;
		}
		}

		if (tween.current_time <= 0) {
			switch (tween.type) {
			case TWEEN_TYPE::FLOAT: {
				*tween.f_value = tween.f_to;
				break;
			}
			case TWEEN_TYPE::VEC2: {
				*tween.v2_value = tween.v2_to;
				break;
			}
			}

			tween.is_active = false;
			std::function<void()> tweenCallback = tween.timeout;
			tweenCallback();
		}
	}

	for (Entity entity : to_be_destroyed) {
		registry.remove_all_components_of(entity);
	}
}