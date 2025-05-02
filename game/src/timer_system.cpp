#include "timer_system.hpp"
#include "tinyECS/registry.hpp"
#include "tinyECS/components.hpp"
#include <functional> 


void TimerSystem::step(float elapsed_ms) {


	for (Entity timerEntity : registry.timers.entities) {
		Timer& timer = registry.timers.get(timerEntity);

		float stepSeconds = elapsed_ms / 1000.0f;

		if (!timer.is_active) {
			registry.remove_all_components_of(timerEntity);
			continue;
		}

		timer.current_time -= stepSeconds;

		if (timer.current_time <= 0) {
			timer.is_active = false;
			std::function<void()> timerCallback = timer.timeout;
			timerCallback();

			if (timer.is_looping) {
				timer.current_time = timer.duration;
				timer.is_active = true;
			}
		}
	}
}