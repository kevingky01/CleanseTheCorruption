#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

class InteractableSystem {
public:
	void step(float elapsed_ms);

	InteractableSystem() {}
};