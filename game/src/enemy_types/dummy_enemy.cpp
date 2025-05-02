#include "enemy_components.hpp"
#include <tinyECS/components.hpp>
#include <tinyECS/registry.hpp>
#include <spell_cast_manager.hpp>
#include <world_init.hpp>

DummyEnemy::DummyEnemy(Entity& entity) {
	max_health = 20;
	hitbox_size = vec2(10, 20);

	col_mesh.local_points = {
		 vec2(0, -10),
		 vec2(+5,  -5),
		 vec2(+5,  +5),
		 vec2(0, +10),
		 vec2(-5,  +5),
		 vec2(-5,  -5)
	};

	status = ENEMY_STATUS::IDLE;
	std::cout << "MADE DUMMY " << std::endl;

	addComponents(entity);
}



void DummyEnemy::addComponents(Entity& entity) {

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::DUMMY_ENEMY,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::ANIMATED_SPRITE
		}
	);

	// health
	float enemy_health = max_health;
	Health& health = registry.healths.emplace(entity);
	health.maxHealth = enemy_health;
	health.currentHealth = enemy_health;

	// Enemy hitbox
	Hitbox enemy_hitbox;
	enemy_hitbox.layer = (int)collision_layer;
	enemy_hitbox.mask = collision_mask;
	enemy_hitbox.hitbox_scale = getHitboxSize();
	registry.hitboxes.emplace(entity, enemy_hitbox);

	CollisionMesh colMesh = col_mesh;
	registry.collisionMeshes.insert(entity, colMesh);

    // transform
	Transformation t = registry.transforms.emplace(entity);
	t.angle = 0.f;
	t.position = vec2(0, 0);

	// motion
	auto& motion = registry.motions.emplace(entity);
	motion.velocity = { 0, 0 };

	float random_n = uniform_dist(rng);

	// Create animation manager
	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::DUMMY_ENEMY, 4, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::DUMMY_ENEMY, idle_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::DUMMY_ENEMY]);
}



void DummyEnemy::step(float elapsed_ms, Entity& self) {
	Transformation& player_transform = registry.transforms.get(registry.players.entities[0]);
	Transformation& enemy_transform = registry.transforms.get(self);
	Motion& enemy_motion = registry.motions.get(self);

	// Act
	if (status == ENEMY_STATUS::IDLE)
		enemy_motion.velocity = vec2(0.0);

	updateEnemyAnimation(self);
}



void DummyEnemy::updateEnemyAnimation(Entity& self) {
	AnimationManager& animation_manager = registry.animation_managers.get(self);
	animation_manager.transition_to(TEXTURE_ASSET_ID::DUMMY_ENEMY);
}