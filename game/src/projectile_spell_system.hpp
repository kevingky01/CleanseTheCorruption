#pragma once 

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include "render_system.hpp"

class ProjectileSpellSystem {
public:
	void step(float elapsed_ms);

	RenderSystem* renderer;
};
