#pragma once

#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

class TimerSystem {
public:
	void step(float elapsed_ms);

	TimerSystem(){}
};