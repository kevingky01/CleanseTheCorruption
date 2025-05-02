#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "render_system.hpp"

void createMap(RenderSystem* renderer, vec2 position);

void createTutorialMap(RenderSystem* renderer, vec2 position);

void createBossMap(RenderSystem* renderer, vec2 position);

void createHubMap(RenderSystem* renderer, vec2 position);
void createBossMap2(RenderSystem* renderer, vec2 position);


void createInbetweenRoom(RenderSystem* renderer, vec2 position);

Entity createWallCollisionEntity(vec2 position, vec2 scale); // Munn: Using this for enemy room managers
