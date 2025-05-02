#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"
#include "render_system.hpp"

struct Transformation;
bool collides(Entity entity_i, Entity entity_j);
bool collideAABB(Entity entity_i, Entity entity_j);
std::vector<vec2> getWorldPoints(Entity e);
bool polygonsCollide(const std::vector<vec2>& polyA, const std::vector<vec2>& polyB);
void getAxes(const std::vector<vec2>& poly, std::vector<vec2>& axesOut);
void projectPolygon(const std::vector<vec2>& poly, 
                    const vec2& axis, 
                    float& outMin, float& outMax);




// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:

	void step(float elapsed_ms);

	PhysicsSystem()
	{
	}
};