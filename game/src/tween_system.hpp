#pragma once

#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

class TweenSystem {
public:
	void step(float elapsed_ms);

	TweenSystem() {}
};