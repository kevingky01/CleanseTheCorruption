#pragma once
#include "enemy_components.hpp"
#include <tinyECS/components.hpp>
#include <tinyECS/registry.hpp>
#include <spell_cast_manager.hpp>
#include <world_init.hpp>
#include <glm/gtx/rotate_vector.hpp>

const float ATTACK_RANGE_FRACTION = 0.7;
const int BURST_COUNT = 4;
const float BURST_DELAY = 0.11;

ShotgunRangedEnemy::ShotgunRangedEnemy(Entity& entity) {
	max_health = 50;
	speed = 175;
	detection_range = 600;
	shooting_range = detection_range * ATTACK_RANGE_FRACTION;
	base_recharge_time = 1500;
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
	idle_state_time_ms = IDLE_STATUS_TIMER_INITIALIZATION;
	std::cout << "MADE NORMAL" << std::endl;

	addComponents(entity);
}



const int shotgun_enemy_spell_count = 1;
std::array<PROJECTILE_SPELL_ID, shotgun_enemy_spell_count> shotgun_enemy_projectile_spells = {
	PROJECTILE_SPELL_ID::RED_ORB,
};



void ShotgunRangedEnemy::addComponents(Entity& entity) {

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::SHOTGUN_ENEMY_IDLE,
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
	motion.velocity = { 0, 0 };  // need function to let enemy search for player


	// Create spell slots
	SpellSlotContainer& spellSlotContainer = registry.spellSlotContainers.emplace(entity);

	SpellSlot damageSpellSlot = SpellSlot();
	damageSpellSlot.spell_type = SPELL_TYPE::PROJECTILE;

	float random_n = uniform_dist(rng);
	int n = ((int)random_n == 1) ? 0 : (int)(shotgun_enemy_spell_count * random_n);
	int spell_id = (int)shotgun_enemy_projectile_spells[n];

	damageSpellSlot.spell_id = spell_id;

	spellSlotContainer.spellSlots.push_back(damageSpellSlot);

	// Create animation manager
	AnimationManager& animation_manager = registry.animation_managers.emplace(entity);

	// Create Idle and Run animations
	Animation idle_animation = Animation(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_IDLE, 4, true); // Asset id, num_frames, is_looping
	idle_animation.h_frames = 4;
	idle_animation.v_frames = 1;
	idle_animation.frame_rate = ANIMATION_FRAME_RATE / 2;
	Animation run_animation = Animation(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_RUN, 4, true);
	run_animation.h_frames = 4;
	run_animation.v_frames = 1;
	run_animation.frame_rate = ANIMATION_FRAME_RATE / 2;

	// Insert animations into animation_manager
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_IDLE, idle_animation));
	animation_manager.animations.insert(std::pair(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_RUN, run_animation));

	// Play idle animation
	animation_manager.transition_to(animation_manager.animations[TEXTURE_ASSET_ID::SHOTGUN_ENEMY_IDLE]);

	if (random_n > 0.7) {
		random_n = uniform_dist(rng);
		int type = ((int)random_n == 1) ? 0 : (int)(10 * random_n);

		Interactable interactable;
		switch (type) {
			case 0: {
			// only spawn numbers
				int numbers_id = relic_count - 1;
				interactable = buildInteractableComponent(INTERACTABLE_ID::RELIC_DROP, 0, 0, numbers_id);
				//std::cout << "Relic" << std::endl;
				break;
			}
			case 1: {
				interactable = buildInteractableComponent(INTERACTABLE_ID::HEALTH_RESTORE, 0, 25, 0);
				break;
			}
			default: {
				// all relics except numbers
				random_n = uniform_dist(rng);
				int relic_id = ((int)random_n == 1) ? 0 : (int)((relic_count - 1) * random_n);
				interactable = buildInteractableComponent(INTERACTABLE_ID::RELIC_DROP, 0, 0, relic_id);
				//std::cout << "Relic" << std::endl;
				break;
			}
		}
		//std::cout << "Enemy Drop" << std::endl;
		Lootable& lootable = registry.lootables.emplace(entity);
		lootable.drop = interactable;
	}
}



void ShotgunRangedEnemy::step(float elapsed_ms, Entity& self) {
	Transformation& player_transform = registry.transforms.get(registry.players.entities[0]);
	Transformation& enemy_transform = registry.transforms.get(self);
	Motion& enemy_motion = registry.motions.get(self);

	// Sense
	vec2 distance_vector = player_transform.position - enemy_transform.position;
	float distance = glm::distance(player_transform.position, enemy_transform.position);
	
	float fleeing_range = shooting_range * 0.5;
	float out_of_flee_range = detection_range * 0.75;
	
	bool player_in_detect_range = distance <= detection_range;
	bool player_in_shoot_range = distance <= shooting_range;
	bool player_in_flee_range = distance <= fleeing_range;
	bool player_out_of_flee_range = distance > out_of_flee_range;

	// Think
	switch (status) {
	case ENEMY_STATUS::IDLE:
		if (player_in_detect_range) {
			status = ENEMY_STATUS::TARGETING;
		}
		break;
	case ENEMY_STATUS::TARGETING:
		if (player_in_flee_range) {
			status = ENEMY_STATUS::FLEEING;
		}
		else if (player_in_shoot_range) {
			status = ENEMY_STATUS::SHOOTING;
		}
		break;
	case ENEMY_STATUS::SHOOTING:
		if (player_in_flee_range) {
			status = ENEMY_STATUS::FLEEING;
		}
		else if (!player_in_shoot_range) {
			status = ENEMY_STATUS::TARGETING;
		}
		break;
	case ENEMY_STATUS::FLEEING:
		if (player_out_of_flee_range) {
			status = ENEMY_STATUS::TARGETING;
		}
		break;
	}

	// Act
	switch (status) {
	case ENEMY_STATUS::IDLE:
		if (idle_state_time_ms <= 0) {
			// Random movements
			random_direction = glm::normalize(vec2((uniform_dist(rng) * 2.0 - 1.0), (uniform_dist(rng) * 2.0 - 1.0)));
			
			if (uniform_dist(rng) > 0.2) {
				enemy_motion.velocity = speed * 0.5f * random_direction;
			} else {
				enemy_motion.velocity = vec2(0.0);
			}
			
			idle_state_time_ms = 1000 + uniform_dist(rng) * 1500;
		}
		
		idle_state_time_ms -= idle_state_time_ms <= 0 ? 0 : elapsed_ms;
		break;

	case ENEMY_STATUS::TARGETING:
		enemy_motion.velocity = speed * glm::normalize(distance_vector);
		break;

	case ENEMY_STATUS::SHOOTING:
		// Move randomly while shooting
		if (idle_state_time_ms <= 0) {
			random_direction = glm::normalize(vec2((uniform_dist(rng) * 2.0 - 1.0), (uniform_dist(rng) * 2.0 - 1.0)));
			enemy_motion.velocity = speed * random_direction;
			idle_state_time_ms = 1000 + uniform_dist(rng) * 1500;
		}
		
		idle_state_time_ms -= idle_state_time_ms <= 0 ? 0 : elapsed_ms;
		
		if (attack_countdown <= 0) {
			attack_countdown = base_recharge_time;
			shotgunAttack(self);
		}
		
		attack_countdown -= attack_countdown <= 0 ? 0 : elapsed_ms;
		break;

	case ENEMY_STATUS::FLEEING:
		// Flee away from player
		vec2 flee_direction = glm::normalize(enemy_transform.position - player_transform.position);
		enemy_motion.velocity = speed * flee_direction * 1.5f;
		
		// Attack while fleeing
		if (attack_countdown <= 0) {
			attack_countdown = base_recharge_time;
			shotgunAttack(self);
		}
		attack_countdown -= attack_countdown <= 0 ? 0 : elapsed_ms;
		break;
	}

	updateEnemyAnimation(self);
}



void ShotgunRangedEnemy::shotgunAttack(Entity& self) {
	Transformation& player_transform = registry.transforms.get(registry.players.entities[0]);
	Transformation& enemy_transform = registry.transforms.get(self);
	SpellSlotContainer& spell_slot_container = registry.spellSlotContainers.get(self);
	SpellSlot& spell_slot = spell_slot_container.spellSlots[0];

	vec2 direction = player_transform.position - enemy_transform.position;

	int bullet_count = 3;
	float radianInc = 2 * M_PI / 11;

	direction = glm::rotate(direction, -radianInc*2);
	for (int i = 0; i < bullet_count; i++) {
		direction = glm::rotate(direction, radianInc);
		SpellCastManager::castBossSpell(renderer, spell_slot, enemy_transform.position, direction, self);
	}
}



void ShotgunRangedEnemy::updateEnemyAnimation(Entity& self) {
	Motion& enemy_motion = registry.motions.get(self);
	Transformation& enemy_transform = registry.transforms.get(self);

	if (enemy_motion.velocity.x > 0) {
		enemy_transform.scale.x = 1;
	}
	else if (enemy_motion.velocity.x < 0) {
		enemy_transform.scale.x = -1;
	}

	AnimationManager& animation_manager = registry.animation_managers.get(self);

	if (glm::length(enemy_motion.velocity) > 0.0) {
		animation_manager.transition_to(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_RUN);
	}
	else {
		animation_manager.transition_to(TEXTURE_ASSET_ID::SHOTGUN_ENEMY_IDLE);
	}
}
