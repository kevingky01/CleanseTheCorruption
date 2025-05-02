#pragma once

#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"


class CameraSystem
{
public:
	void step(float elapsed_ms);

	CameraSystem()
	{
	}
};