#include "ai_system.hpp"
#include "physics_system.hpp"

void AISystem::init(RenderSystem* renderer) {
	this->renderer = renderer;
	// seeding rng with random device (Take from world_system.cpp)
	rng = std::default_random_engine(std::random_device()());
}

void AISystem::update(float elapsed_ms, Entity& player_entity, Entity& enemy_entity, Enemy& enemy)
{
	// No behaviour, just use for placeholder, specified behaviour implemented in "enemy_types" folder
	(void)elapsed_ms;
	(void)player_entity;
	(void)enemy_entity;
	(void)enemy;
}

void AISystem::step(float elapsed_ms)
{
	for (Entity& enemy_entity : registry.enemies.entities)
	{
		Enemy* enemy = registry.enemies.get(enemy_entity);
		enemy->step(elapsed_ms, enemy_entity);
	}
}
